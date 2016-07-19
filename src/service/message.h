//
// Created by thuanqin on 16/5/20.
//

#ifndef OGP_SERVICE_MESSAGE_H
#define OGP_SERVICE_MESSAGE_H

#include <iostream>
#include <memory>

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
    CT_PORTAL_ADD_SERVICE_RES = 9,
    CT_PORTAL_DEL_SERVICE_RES = 10,
    CT_PORTAL_LIST_SERVICES_RES = 11,
    CT_SDPROXY_SERVICE_DATA_SYNC_REQ = 12,
    CT_SDPROXY_HEARTBEAT_RES = 13,
    CT_SDPROXY_SERVICE_FIRST_DATA_SYNC_REQ = 14,
    CT_PORTAL_LIST_SERVICES_DETAIL_RES = 15,
    CT_SDAGENT_LIST_SDP_RES = 16,
    CT_SDPROXY_LISTEN_INFO_SYNC_RES = 17,

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
    PO_PORTAL_ADD_SERVICE_REQ = 9005,
    PO_PORTAL_DEL_SERVICE_REQ = 9006,
    PO_PORTAL_LIST_SERVICES_REQ = 9007,
    PO_PORTAL_LIST_SERVICES_DETAIL_REQ = 9008,

    // CLI
    CI_CLI_ADD_APP_REQ = 12000,

    // sd proxy
    SP_SDPROXY_SAY_HI = 15000,
    SP_SDPROXY_HEARTBEAT_REQ = 15001,
    SP_SDPPROXY_SERVICE_SYNC_REQ = 15002,
    SP_SDAGENT_HEARTBEAT_RES = 15003,
    SP_SDPROXY_LISTEN_INFO_SYNC_REQ = 15004,

    // sd agent
    SA_SDAGENT_HEARTBEAT_REQ = 18000,
    SA_SDAGENT_SAY_HI = 18001,
    SA_SDAGENT_LIST_SDP_REQ= 18002,

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
