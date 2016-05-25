//
// Created by thuanqin on 16/5/19.
//

#ifndef OGP_SERVICE_SESSION_H
#define OGP_SERVICE_SESSION_H

#include "service/message.h"


class Session {
public:
    // 设置对端的IP地址
    virtual Session &set_address(std::string address) = 0;
    // 获取对端的IP地址
    virtual std::string get_address() const = 0;
    // 获取对端的端口
    virtual unsigned short get_port() const = 0;
    // 设置对端的端口
    virtual Session &set_port(unsigned short port) = 0;
    // 向对端发送消息,该调用不阻塞
    virtual void send_msg(msg_ptr msg) = 0;
    // Session是否有效
    virtual bool valid() const = 0;
    // 使Session无效
    virtual void invalid_sess() = 0;
};

typedef std::shared_ptr<Session> sess_ptr;

#endif //OGP_SERVICE_SESSION_H
