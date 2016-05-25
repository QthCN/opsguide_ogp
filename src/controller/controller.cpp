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
    switch(msg->get_msg_type()) {
        // docker agent发来的心跳请求
        case MsgType::DA_DOCKER_HEARTBEAT_REQ:
            sess->send_msg(
                    std::make_shared<Message>(
                            MsgType::CT_DOCKER_HEARTBEAT_RES,
                            new char[0],
                            0
                    )
            );
            break;
        default:
            LOG_ERROR("Unknown msg type: " << static_cast<unsigned int>(msg->get_msg_type()));
    }
}