//
// Created by thuanqin on 16/5/20.
//

#ifndef OGP_SERVICE_MESSAGE_H
#define OGP_SERVICE_MESSAGE_H

#include "common/log.h"

enum class MsgType: unsigned int {
    OGP_NULL_MSG = 99999,

    // controller
    CT_DOCKER_HEARTBEAT_RES = 0,
    CT_DOCKER_RUNTIME_INFO_SYNC_RES = 1,
    CT_DOCKER_RUNTIME_INFO_SYNC_REQ = 2,
    CT_PORTAL_GET_APPS_RES = 3,
    CT_CLI_ADD_APP_RES = 4,
    CT_PORTAL_GET_AGENTS_RES = 5,
    CT_PORTAL_PUBLISH_APP_RES = 6,
    CT_PORTAL_REMOVE_APPVER_RES = 7,
    CT_PORTAL_UPGRADE_APPVER_RES = 8,

    // docker agent
    DA_DOCKER_HEARTBEAT_REQ = 3000,
    DA_DOCKER_RUNTIME_INFO_SYNC_REQ = 3001,
    DA_DOCKER_SAY_HI = 3002,

    // nginx agent
    // ...

    // portal, 注意这里的消息ID被portal的消息ID依赖
    PO_PORTAL_GET_APPS_REQ = 9000,
    PO_PORTAL_GET_AGENTS_REQ = 9001,
    PO_PORTAL_PUBLISH_APP_REQ = 9002,
    PO_PORTAL_REMOVE_APPVER_REQ = 9003,
    PO_PORTAL_UPGRADE_APPVER_REQ = 9004,

    // CLI
    CI_CLI_ADD_APP_REQ = 12000,

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
