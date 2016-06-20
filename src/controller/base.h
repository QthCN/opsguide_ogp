//
// Created by thuanqin on 16/5/24.
//

#ifndef OGP_CONTROLLER_BASE_H
#define OGP_CONTROLLER_BASE_H

#include <vector>
#include <thread>

#include "service/message.h"
#include "service/session.h"

class BaseController {
public:
    virtual~BaseController() {};
    // 执行初始化
    virtual void init() = 0;
    // 关联session和controller内部数据结构,如果有问题需要抛出异常
    virtual void associate_sess(sess_ptr sess) = 0;
    // 处理消息
    virtual void handle_msg(sess_ptr sess, msg_ptr msg) = 0;
    // service无效化一个session的时候会回调此方法,如果此session在controller中已经无效则不用执行任何操作
    virtual void invalid_sess(sess_ptr sess) = 0;

    void wait() {
        for (auto t = cs_threads.begin(); t!=cs_threads.end();t++) {
            t->join();
        }
    }

    void add_thread(std::thread t) {
        cs_threads.push_back(std::move(t));
    }

protected:
    // 内部工作线程
    std::vector<std::thread> cs_threads;
};

#endif //OGP_CONTROLLER_BASE_H
