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
    // SDA同步线程
    std::thread t3([this](){sync_sda();});
    add_thread(std::move(t3));
}

void SDProxy::associate_sess(sess_ptr sess) {
    // 判断sess是否是controler的
    auto address = sess->get_address();
    auto port = sess->get_port();
    auto controller_address = config_mgr.get_item("proxy_controller_address")->get_str();
    auto controller_port = static_cast<unsigned int>(config_mgr.get_item("proxy_controller_port")->get_int());
    // 如果是controller sess
    if (address == controller_address && port == controller_port) {
        LOG_INFO("associate controller_sess")
        g_lock.lock();
        controller_sess = sess;
        g_lock.unlock();
    } else {
        // 如果controller_sess为nullptr,则不能接收这个sess
        g_lock.lock();
        if (controller_sess == nullptr) {
            LOG_WARN("controller_sess is nullptr, so this sda sess can not be accepted. sess info: "
                     << sess->get_address() << ":" << std::to_string(sess->get_port()))
            sess->invalid_sess();
        }
        g_lock.unlock();
    }
}

void SDProxy::invalid_sess(sess_ptr sess) {
    g_lock.lock();
    if (controller_sess != nullptr
        && controller_sess->get_address() == sess->get_address()
        && controller_sess->get_port() == sess->get_port()) {
        controller_sess = nullptr;
        LOG_INFO("invalid controller_sess now")
        cs_lock.lock();
        ssda_id_lock.lock();
        current_services.set_uniq_id(-1);
        sync_sda_current_uniq_id = -1;
        // 断开所有的sda,因为此时可能是这个sdp出现了问题无法和controller建立联系,如果不断开sda则sda可能会一直获取到错误的信息
        disconnect_sdas();
        ssda_id_lock.unlock();
        cs_lock.unlock();
    }
    g_lock.unlock();
}

void SDProxy::disconnect_sdas() {
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

void SDProxy::sync_sda() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        cs_lock.lock();
        ssda_id_lock.lock();
        if (sync_sda_current_uniq_id >= current_services.uniq_id()) {
            ssda_id_lock.unlock();
            cs_lock.unlock();
            continue;
        }
        sync_sda_current_uniq_id = current_services.uniq_id();
        // TODO(tianhuan): 拷贝一份current_services
        ssda_id_lock.unlock();
        cs_lock.unlock();

        // TODO(tianhuan): 向SDA广播信息
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
        send_listen_info();
        g_lock.unlock();

        int sleep_t = 0;
        while (sleep_t <= 300) {
            g_lock.lock();
            if (controller_sess == nullptr) {
                g_lock.unlock();
                break;
            }
            g_lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(5));
            sleep_t += 5;
        }
    }
}

void SDProxy::send_listen_info() {
    ogp_msg::SDProxyListenInfoSyncReq listen_info_req;
    auto controller_address = config_mgr.get_item("proxy_listen_address")->get_str();
    auto controller_port = static_cast<unsigned int>(config_mgr.get_item("proxy_listen_port")->get_int());
    listen_info_req.set_ip(controller_address);
    listen_info_req.set_port(controller_port);
    send_msg(controller_sess, listen_info_req, MsgType::SP_SDPROXY_LISTEN_INFO_SYNC_REQ);
}

void SDProxy::handle_ct_sync_service_data_msg(sess_ptr sess, msg_ptr msg) {
    LOG_INFO("sync request received.")
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

