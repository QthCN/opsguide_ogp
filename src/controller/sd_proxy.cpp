//
// Created by thuanqin on 16/7/5.
//
#include "controller/sd_proxy.h"

#include <ctime>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "common/config.h"
#include "common/log.h"
#include "controller/utils.h"
#include "ogp_msg.pb.h"

void SDProxy::init() {
    LOG_INFO("SDProxy begin initialization now.")
    // 心跳线程
    std::thread t([this](){send_heartbeat();});
    add_thread(std::move(t));
    // 主动同步线程
    std::thread t1([this](){sync_controller();});
    add_thread(std::move(t1));
}

void SDProxy::associate_sess(sess_ptr sess) {
    // 判断sess是否是controler的
    auto address = sess->get_address();
    auto port = sess->get_port();
    auto controller_address = config_mgr.get_item("proxy_controller_address")->get_str();
    auto controller_port = static_cast<unsigned int>(config_mgr.get_item("proxy_controller_port")->get_int());
    if (address == controller_address && port == controller_port) {
        g_lock.lock();
        controller_sess = sess;
        g_lock.unlock();
    }
}

void SDProxy::invalid_sess(sess_ptr sess) {
    g_lock.lock();
    if (controller_sess != nullptr
        && controller_sess->get_address() == sess->get_address()
        && controller_sess->get_port() == sess->get_port()) {
        controller_sess = nullptr;
        cs_lock.lock();
        current_services.set_uniq_id(-1);
        cs_lock.unlock();
    }
    g_lock.unlock();
}

void SDProxy::handle_msg(sess_ptr sess, msg_ptr msg) {
    switch(msg->get_msg_type()) {
        // controller对心跳请求的回复
        case MsgType::CT_SDPROXY_HEARTBEAT_RES:
            break;
        // controller发来的service同步请求
        case MsgType::CT_SDPROXY_SERVICE_DATA_SYNC_REQ:
            handle_ct_sync_service_data_msg(sess, msg);
            break;
        default:
            LOG_ERROR("Unknown msg type: " << static_cast<unsigned int>(msg->get_msg_type()));
    }
}

void SDProxy::sync_controller() {
    // 等待say hi操作完成。由于sda是从controller中获取sdp列表的,因此这里的延迟不会让sda获取到没有数据的sdp
    std::this_thread::sleep_for(std::chrono::seconds(5));
    while (true) {
        g_lock.lock();
        if (controller_sess == nullptr) {
            g_lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        LOG_INFO("sync with controller now.")
        send_simple_msg(controller_sess, MsgType::SP_SDPPROXY_SERVICE_SYNC_REQ);
        g_lock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(300));
    }
}

void SDProxy::handle_ct_sync_service_data_msg(sess_ptr sess, msg_ptr msg) {
    LOG_INFO("received sync request.")
    ogp_msg::ServiceSyncData service_sync_data;
    service_sync_data.ParseFromArray(msg->get_msg_body(), msg->get_msg_body_size());
    cs_lock.lock();
    if (current_services.uniq_id() >= service_sync_data.uniq_id()) {
        LOG_INFO("expired sync msg, ignore it.")
    } else {
        // 比较controller同步来的信息是否和目前的信息存在差异,如果存在差异则进行同步,否则不采取操作
        if (sd_utils.check_diff(current_services, service_sync_data)) {
            // controller同步来的数据和本地的缓存不同,因此需要通知SDA
            current_services = service_sync_data;
        } else {
            // controller同步来的数据和本地相同,只需要更新id
            current_services.set_uniq_id(service_sync_data.uniq_id());
        }
    }
    cs_lock.unlock();
}

void SDProxy::send_heartbeat() {
    auto period = static_cast<unsigned int>(config_mgr.get_item("proxy_heartbeat_period")->get_int());
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(period));
        g_lock.lock();
        if (controller_sess != nullptr) {
            controller_sess->send_msg(
                    std::make_shared<Message>(
                            MsgType::SP_SDPROXY_HEARTBEAT_REQ,
                            new char[0],
                            0
                    )
            );
        } else {
            LOG_WARN("controller_sess is nullptr, no heartbeat msg send.")
        }
        g_lock.unlock();
    }
}

