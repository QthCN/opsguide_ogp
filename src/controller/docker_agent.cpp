//
// Created by thuanqin on 16/5/24.
//

#include "controller/docker_agent.h"

#include <sstream>
#include <thread>

#include "common/config.h"
#include "common/docker_client.h"
#include "common/log.h"
#include "ogp_msg.pb.h"
#include "third/json/json.hpp"

using json = nlohmann::json;


void DockerAgent::init() {
    LOG_INFO("DockerAgent begin initialization now.")
}

void DockerAgent::associate_sess(sess_ptr sess) {
    agent_lock.lock();
    if (controller_sess) {
        agent_lock.unlock();
        throw std::runtime_error("controller_sess is not nullptr");
    } else {
        controller_sess = sess;
        agent_lock.unlock();
    }
}

void DockerAgent::invalid_sess(sess_ptr sess) {
    agent_lock.lock();
    controller_sess = nullptr;
    agent_lock.unlock();
}

void DockerAgent::send_heartbeat() {
    auto period = static_cast<unsigned int>(config_mgr.get_item("agent_heartbeat_period")->get_int());
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(period));
        agent_lock.lock();
        if (controller_sess != nullptr) {
            controller_sess->send_msg(
                    std::make_shared<Message>(
                            MsgType::DA_DOCKER_HEARTBEAT_REQ,
                            new char[0],
                            0
                    )
            );
        } else {
            LOG_WARN("controller_sess is nullptr, no heartbeat msg send.")
        }
        agent_lock.unlock();
    }
}

void DockerAgent::handle_msg(sess_ptr sess, msg_ptr msg) {
    switch(msg->get_msg_type()) {
        // controller对心跳请求的回复
        case MsgType::CT_DOCKER_HEARTBEAT_RES:
            break;
        // controller对于状态信息同步的响应
        case MsgType::CT_DOCKER_RUNTIME_INFO_SYNC_RES:
            handle_ct_sync_msg(sess, msg);
            break;
        // controller发来的状态同步信息请求
        case MsgType::CT_DOCKER_RUNTIME_INFO_SYNC_REQ:
            handle_ct_sync_msg(sess, msg);
            break;
        default:
            LOG_ERROR("Unknown msg type: " << static_cast<unsigned int>(msg->get_msg_type()));
    }
}

void DockerAgent::sync() {
    auto period = static_cast<unsigned int>(config_mgr.get_item("agent_sync_period")->get_int());
    auto docker_host = static_cast<std::string>(config_mgr.get_item("agent_docker_host")->get_str());
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    DockerClient docker_client(docker_host);

    while (true) {
        try {
            // 获取docker的运行时信息,包括系统资源情况及容器信息
            ogp_msg::DockerRuntimeInfo docker_runtime_info;

            // 容器信息
            auto containers_info = docker_client.list_containers(1);

            for (auto &c: containers_info) {
                auto msg_container = docker_runtime_info.add_containers();
                msg_container->set_command(c["Command"]);
                msg_container->set_created(c["Created"]);
                msg_container->set_id(c["Id"]);
                msg_container->set_image(c["Image"]);
                msg_container->set_status(c["Status"]);

                for (auto it=c["Labels"].begin(); it!=c["Labels"].end(); it++) {
                    auto msg_label = msg_container->add_labels();
                    msg_label->set_name(it.key());
                    msg_label->set_value(it.value());
                }

                for (auto it=c["Names"].begin(); it!=c["Names"].end(); it++) {
                    auto msg_name = msg_container->add_names();
                    *msg_name = it.value();
                }

                for (auto it=c["Ports"].begin(); it!=c["Ports"].end(); it++) {
                    auto msg_port = msg_container->add_ports();
                    msg_port->set_private_port(it.value()["PrivatePort"]);
                    msg_port->set_public_port(it.value()["PublicPort"]);
                    msg_port->set_type(it.value()["Type"]);
                }
            }

            // 系统资源信息


            // 发送同步信息给controller
            send_msg(agent_lock, controller_sess, docker_runtime_info, MsgType::DA_DOCKER_RUNTIME_INFO_SYNC_REQ);

        } catch (std::runtime_error &e) {
            LOG_ERROR("" << e.what())
        }

        std::this_thread::sleep_for(std::chrono::seconds(period));
    }
}

void DockerAgent::handle_ct_sync_msg(sess_ptr sess, msg_ptr msg) {
    ogp_msg::DockerTargetRuntimeInfo docker_target_runtime_info;
    docker_target_runtime_info.ParseFromArray(msg->get_msg_body(), msg->get_msg_body_size());
    LOG_INFO("GG: " << docker_target_runtime_info.containers().size());
}