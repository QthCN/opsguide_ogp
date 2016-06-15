//
// Created by thuanqin on 16/6/14.
//
#include "controller/utils.h"

#include "common/log.h"

void send_simple_msg(sess_ptr sess, MsgType msg_type) {
    if (sess != nullptr) {
        auto msg_size = 0;
        char *msg_data = new char[0];
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