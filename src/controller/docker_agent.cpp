//
// Created by thuanqin on 16/5/24.
//

#include "controller/docker_agent.h"

#include "common/log.h"


void DockerAgent::init() {
    LOG_INFO("DockerAgent begin initialization now.")
}

void DockerAgent::associate_sess(sess_ptr sess) {
}

void DockerAgent::handle_msg(sess_ptr sess, msg_ptr msg) {
    LOG_INFO("" << static_cast<unsigned int>(msg->get_msg_type()))
    LOG_INFO("" << msg->get_msg_body_size())
    sess->send_msg(
            std::make_shared<Message>(
                    MsgType::DOCKER_HEARTBEAT,
                    new char[1],
                    1
            )
    );
}

void DockerAgent::send_heartbeat() {

}