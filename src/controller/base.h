//
// Created by thuanqin on 16/5/24.
//

#ifndef OGP_CONTROLLER_BASE_H
#define OGP_CONTROLLER_BASE_H

#include "service/message.h"
#include "service/session.h"

class BaseController {
public:
    // 执行初始化
    virtual void init() = 0;
    // 关联session和controller内部数据结构,如果有问题需要抛出异常
    virtual void associate_sess(sess_ptr sess) = 0;
    // 处理消息
    virtual void handle_msg(sess_ptr sess, msg_ptr msg) = 0;
    // 向对端发送心跳包
    virtual void send_heartbeat() = 0;
    // 获取同步信息
    virtual void sync() = 0;
    // service无效化一个session的时候会回调此方法,如果此session在controller中已经无效则不用执行任何操作
    virtual void invalid_sess(sess_ptr sess) = 0;
};

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
    }
    lock.unlock();
}

#endif //OGP_CONTROLLER_BASE_H
