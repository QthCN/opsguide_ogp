//
// Created by thuanqin on 16/5/13.
//
#include "controller/controller.h"

#include "common/log.h"

void Controller::init() {
    LOG_INFO("Controller begin initialization now.")
}

void Controller::associate_sess(sess_ptr sess) {

}

void Controller::send_heartbeat() {

}

void Controller::invalid_sess(sess_ptr sess) {

}

void Controller::handle_msg(sess_ptr sess, msg_ptr msg) {
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