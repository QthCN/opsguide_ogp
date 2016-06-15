//
// Created by thuanqin on 16/6/14.
//

#ifndef OGP_CONTROLLER_UTILS_H
#define OGP_CONTROLLER_UTILS_H

#include "common/log.h"
#include "service/message.h"
#include "service/session.h"

template <typename AGENT, typename PROTOBUF_MSG>
void send_msg(std::mutex &lock, AGENT agent, PROTOBUF_MSG &protobuf_msg, MsgType msg_type) {
    lock.lock();
    if (agent->get_sess() != nullptr) {
        auto msg_size = protobuf_msg.ByteSize();
        char *msg_data = new char[msg_size];
        protobuf_msg.SerializeToArray(msg_data, msg_size);
        agent->get_sess()->send_msg(
                std::make_shared<Message>(
                        msg_type,
                        msg_data,
                        msg_size
                )
        );
    } else {
        LOG_ERROR("sess is null, no msg send.")
    }
    lock.unlock();
};

template <typename PROTOBUF_MSG>
void send_msg(std::mutex &lock, sess_ptr sess, PROTOBUF_MSG &protobuf_msg, MsgType msg_type) {
    lock.lock();
    if (sess != nullptr) {
        auto msg_size = protobuf_msg.ByteSize();
        char *msg_data = new char[msg_size];
        protobuf_msg.SerializeToArray(msg_data, msg_size);
        sess->send_msg(
                std::make_shared<Message>(
                        msg_type,
                        msg_data,
                        msg_size
                )
        );
    } else {
        LOG_ERROR("sess is null, no msg send.")
    }
    lock.unlock();
}

template <typename PROTOBUF_MSG>
void send_msg(sess_ptr sess, PROTOBUF_MSG &protobuf_msg, MsgType msg_type) {
    if (sess != nullptr) {
        auto msg_size = protobuf_msg.ByteSize();
        char *msg_data = new char[msg_size];
        protobuf_msg.SerializeToArray(msg_data, msg_size);
        sess->send_msg(
                std::make_shared<Message>(
                        msg_type,
                        msg_data,
                        msg_size
                )
        );
    } else {
        LOG_ERROR("sess is null, no msg send.")
    }
}

void send_simple_msg(sess_ptr sess, MsgType msg_type);

#endif //OGP_CONTROLLER_UTILS_H
