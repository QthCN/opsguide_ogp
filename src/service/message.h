//
// Created by thuanqin on 16/5/20.
//

#ifndef OGP_SERVICE_MESSAGE_H
#define OGP_SERVICE_MESSAGE_H

#include "common/log.h"

enum class MsgType: unsigned int {
    // controller
    CT_DOCKER_HEARTBEAT_RES = 0,

    // docker agent
    DA_DOCKER_HEARTBEAT_REQ = 3000,
    DA_DOCKER_MACHINE_INFO_SYNC_REQ = 3001,

};

class Message {
public:
    Message(MsgType msg_type, char*msg_body, unsigned int msg_body_size):
            msg_type(msg_type), msg_body(msg_body), msg_body_size(msg_body_size) {}
    ~Message() {
        delete[] msg_body;
    }
    MsgType get_msg_type() {return msg_type;}
    unsigned int get_msg_body_size() {
        return msg_body_size;
    }
    char *get_msg_body() {return msg_body;}

private:
    MsgType msg_type;
    char *msg_body;
    unsigned int msg_body_size;
};

typedef std::shared_ptr<Message> msg_ptr;

// 报文中,报文体长度信息字段长度
const size_t msg_length_hdr_size = 4;
const size_t msg_type_hdr_size = 2;

#endif //OGP_SERVICE_MESSAGE_H
