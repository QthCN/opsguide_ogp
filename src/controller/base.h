//
// Created by thuanqin on 16/5/24.
//

#ifndef OGP_CONTROLLER_BASE_H
#define OGP_CONTROLLER_BASE_H

#include <mutex>
#include <vector>
#include <thread>

#include "service/message.h"
#include "service/session.h"

struct IpEndpoint {
    std::string ip;
    int port;
};

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

    // todo(tianhuan), 基于BaseController需要有一个BaseAgentController包含下面两个方法
    // 获取新的controller信息。如果这里返回的信息的端口号为-1则重连的时候使用配置文件中的配置进行连接,否则使用这里给出的新的地址
    IpEndpoint get_new_controller_and_reset() {
        new_controller_lock.lock();
        IpEndpoint ip_endpoint;
        ip_endpoint.ip = new_controller_ip;
        ip_endpoint.port = new_controller_port;
        // 这个地址只使用一次。例如sda像controller获取到sdp的地址,但sdp有问题则下一次重连的时候sda依旧需要去连接controller
        new_controller_ip = "";
        new_controller_port = -1;
        new_controller_lock.unlock();
        return ip_endpoint;
    }

    void set_new_controller(std::string ip, int port) {
        new_controller_lock.lock();
        new_controller_ip = ip;
        new_controller_port = port;
        new_controller_lock.unlock();
    }


protected:
    // 内部工作线程
    std::vector<std::thread> cs_threads;
    // 新controller的信息
    std::string new_controller_ip = "";
    int new_controller_port = -1;
    std::mutex new_controller_lock;
};

#endif //OGP_CONTROLLER_BASE_H
