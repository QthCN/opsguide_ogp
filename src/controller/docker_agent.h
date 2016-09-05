//
// Created by thuanqin on 16/5/24.
//

#ifndef OGP_CONTROLLER_DOCKER_AGENT_H
#define OGP_CONTROLLER_DOCKER_AGENT_H

#include <mutex>
#include <queue>

#include "common/docker_client.h"
#include "controller/base.h"
#include "controller/ma.h"
#include "ogp_msg.pb.h"
#include "service/message.h"
#include "service/session.h"

enum class DAActionType {
    START_CONTAINER = 1,
    STOP_AND_REMOVE_CONTAINER = 2,
};

class DAAction {
public:
    DAAction(container_ptr container, DAActionType action_type): action_type(action_type), container(container) {};
    DAActionType get_action_type() {return action_type;}
    container_ptr get_container() {return container;}
private:
    DAActionType action_type;
    container_ptr container;
};
typedef std::shared_ptr<DAAction> daaction_ptr;

class DockerAgent: public BaseController {
public:
    DockerAgent(DockerClientBase *docker_client_){
        auto docker_host = static_cast<std::string>(config_mgr.get_item("agent_docker_host")->get_str());
        docker_client = docker_client_;
        docker_client->set_host(docker_host);
    };
    ~DockerAgent() {
        delete docker_client;
    }
    void init();
    void associate_sess(sess_ptr sess);
    void handle_msg(sess_ptr sess, msg_ptr msg);
    // 心跳同步线程
    void send_heartbeat();
    // 与controller之间的状态同步线程
    void sync();
    // 同步本地docker状态的线程
    void docker_worker();
    void invalid_sess(sess_ptr sess);

    // 新增动作
    void add_action(daaction_ptr action) {
        actions_queue_lock.lock();
        actions_queue.push(action);
        actions_queue_lock.unlock();
    }

    // 判断动作队列是否为空
    bool actions_queue_empty() {
        actions_queue_lock.lock();
        bool empty = actions_queue.empty();
        actions_queue_lock.unlock();
        return empty;
    }

    // 获取动作
    daaction_ptr get_action() {
        daaction_ptr action = nullptr;
        actions_queue_lock.lock();
        if (!actions_queue.empty()) {
            action = actions_queue.front();
            actions_queue.pop();
        }
        actions_queue_lock.unlock();
        return action;
    }

private:
    sess_ptr controller_sess = nullptr;
    std::mutex agent_lock;
    void sync_appcfgs();
    void handle_ct_sync_msg(sess_ptr sess, msg_ptr msg);
    void handle_ct_sync_req_msg(sess_ptr sess, msg_ptr msg);
    void handle_ct_sync_app_cfg_msg(sess_ptr sess, msg_ptr msg);
    ogp_msg::DockerRuntimeInfo get_docker_runtime_info_msg();
    std::queue<daaction_ptr> actions_queue;
    std::mutex actions_queue_lock;
    void start_container(container_ptr container);
    void stop_and_remove_container(container_ptr container);
    void collect_docker_rt_and_sync();
    DockerClientBase* docker_client;
};

#endif //OGP_CONTROLLER_DOCKER_AGENT_H
