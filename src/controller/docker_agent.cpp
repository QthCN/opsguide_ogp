//
// Created by thuanqin on 16/5/24.
//

#include "controller/docker_agent.h"

#include <thread>

#include "common/config.h"
#include "common/log.h"


void DockerAgent::init() {
    LOG_INFO("DockerAgent begin initialization now.")
}

void DockerAgent::associate_sess(sess_ptr sess) {
    agent_lock.lock();
    if (controller_sess) {
        agent_lock.unlock();
        throw std::runtime_error("controller_sess is not nullptr");
    } else {
        controller_sess = sess;
        agent_lock.unlock();
    }
}

void DockerAgent::invalid_sess(sess_ptr sess) {
    agent_lock.lock();
    controller_sess = nullptr;
    agent_lock.unlock();
}

void DockerAgent::send_heartbeat() {
    auto period = static_cast<unsigned int>(config_mgr.get_item("agent_heartbeat_period")->get_int());
    while (true) {
        agent_lock.lock();
        if (controller_sess != nullptr) {
            controller_sess->send_msg(
                    std::make_shared<Message>(
                            MsgType::DA_DOCKER_HEARTBEAT_REQ,
                            new char[0],
                            0
                    )
            );
        } else {
            LOG_WARN("controller_sess is nullptr, no heartbeat msg send.")
        }
        agent_lock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(period));
    }
}

void DockerAgent::handle_msg(sess_ptr sess, msg_ptr msg) {
    switch(msg->get_msg_type()) {
        // controller对心跳请求的回复
        case MsgType::CT_DOCKER_HEARTBEAT_RES:
            break;
        default:
            LOG_ERROR("Unknown msg type: " << static_cast<unsigned int>(msg->get_msg_type()));
    }
}