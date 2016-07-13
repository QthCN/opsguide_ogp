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
    }
    g_lock.unlock();
}

void SDProxy::handle_msg(sess_ptr sess, msg_ptr msg) {
    switch(msg->get_msg_type()) {
        // controller对心跳请求的回复
        case MsgType::CT_SDPROXY_HEARTBEAT_RES:
            break;
        default:
            LOG_ERROR("Unknown msg type: " << static_cast<unsigned int>(msg->get_msg_type()));
    }
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
