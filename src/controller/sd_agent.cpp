//
// Created by thuanqin on 16/7/18.
//
#include "controller/sd_agent.h"

#include <map>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

#include "common/config.h"
#include "common/log.h"
#include "controller/utils.h"
#include "ogp_msg.pb.h"

void SDAgent::init() {
    LOG_INFO("SDAgent begin initialization now.")
    // 心跳线程
    std::thread t([this](){send_heartbeat();});
    add_thread(std::move(t));
    // sdp状态同步线程
    std::thread t2([this](){sync();});
    add_thread(std::move(t2));

    std::random_device rd;
    rng = std::mt19937(rd());
}

void SDAgent::associate_sess(sess_ptr sess) {
    agent_lock.lock();
    auto controller_address = config_mgr.get_item("agent_controller_address")->get_str();
    auto controller_port = static_cast<unsigned int>(config_mgr.get_item("agent_controller_port")->get_int());
    // 判断这个sess是不是controller的sess,如果不是则说明这个sess是sdp
    if (sess->get_address() == controller_address
            && sess->get_port() == controller_port) {
        if (controller_sess) {
            agent_lock.unlock();
            throw std::runtime_error("controller_sess is not nullptr");
        } else {
            controller_sess = sess;
            // 发送请求获取sdp
            send_simple_msg(sess, MsgType::SA_SDAGENT_LIST_SDP_REQ);
            agent_lock.unlock();
        }
    } else {
        if (sdp_sess) {
            agent_lock.unlock();
            throw std::runtime_error("sdp_sess is not nullptr");
        } else {
            sdp_sess = sess;
            agent_lock.unlock();
        }
    }

}

void SDAgent::invalid_sess(sess_ptr sess) {
    agent_lock.lock();
    current_sync_id_lock.lock();
    controller_sess = nullptr;
    sdp_sess = nullptr;
    current_sync_id = -1;
    current_sync_id_lock.unlock();
    agent_lock.unlock();
}

void SDAgent::handle_msg(sess_ptr sess, msg_ptr msg) {
    switch(msg->get_msg_type()) {
        // controller对sdp列表请求的回复
        case MsgType::CT_SDAGENT_LIST_SDP_RES:
            handle_ct_list_sdp_msg(sess, msg);
            break;

            // sdp对心跳请求的回复
        case MsgType::SP_SDAGENT_HEARTBEAT_RES:
            break;
            // sdp发来的最新的service信息
        case MsgType::SP_SDAGENT_SYNC_SERVICE:
            handle_sp_sync_service_msg(sess, msg);
            break;

        default:
            LOG_ERROR("Unknown msg type: " << static_cast<unsigned int>(msg->get_msg_type()));
    }
}

void SDAgent::handle_sp_sync_service_msg(sess_ptr sess, msg_ptr msg) {
    ogp_msg::ServiceSyncData service_sync_data;
    service_sync_data.ParseFromArray(msg->get_msg_body(), msg->get_msg_body_size());
    LOG_INFO("get service data from sdp, uniq_id in msg is: " << std::to_string(service_sync_data.uniq_id()));
}

void SDAgent::send_heartbeat() {
    auto period = static_cast<unsigned int>(config_mgr.get_item("agent_heartbeat_period")->get_int());
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(period));
        agent_lock.lock();
        if (sdp_sess != nullptr) {
            sdp_sess->send_msg(
                    std::make_shared<Message>(
                            MsgType::SA_SDAGENT_HEARTBEAT_REQ,
                            new char[0],
                            0
                    )
            );
        } else {
            LOG_WARN("sdp_sess is nullptr, no heartbeat msg send.")
        }
        agent_lock.unlock();
    }
}

void SDAgent::sync() {
    while (true) {
        agent_lock.lock();
        if (sdp_sess == nullptr) {
            agent_lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        } else {
            send_simple_msg(sdp_sess, MsgType::SA_SDAGENT_SYNC_SERVICE_REQ);
            agent_lock.unlock();
        }

        int sleep_t = 0;
        while (sleep_t <= 300) {
            agent_lock.lock();
            if (sdp_sess == nullptr) {
                agent_lock.unlock();
                std::this_thread::sleep_for(std::chrono::seconds(1));
                break;
            }
            agent_lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(5));
            sleep_t += 5;
        }
    }
}

void SDAgent::handle_ct_list_sdp_msg(sess_ptr sess, msg_ptr msg) {
    // 解析msg获取到sdp列表,选择一个设置为新的controller,然后中断当前controller的连接并进行重连
    ogp_msg::SDEndpointInfoRes sd_endpoint_info_res;
    sd_endpoint_info_res.ParseFromArray(msg->get_msg_body(), msg->get_msg_body_size());
    auto sd_size = sd_endpoint_info_res.sds().size();
    if (sd_size == 0) {
        LOG_WARN("no sdproxy available now.")
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // 发送请求获取sdp
        send_simple_msg(sess, MsgType::SA_SDAGENT_LIST_SDP_REQ);
    } else {
        // 重置连接,重连时会尝试连接sdp
        std::uniform_int_distribution<int> uni(0,sd_size-1);
        auto target_index = uni(rng);
        auto sdp = sd_endpoint_info_res.sds(target_index);
        LOG_INFO("connect to sdp: " << sdp.ip() << ":" << std::to_string(sdp.port()))
        set_new_controller(sdp.ip(), sdp.port());
        agent_lock.lock();
        if (controller_sess) {
            auto controller_sess_ = controller_sess;
            agent_lock.unlock();

            controller_sess_->destory_and_reconnect();
        } else {
            agent_lock.unlock();
        }

    }
}