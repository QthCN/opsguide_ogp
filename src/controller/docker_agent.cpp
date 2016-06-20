//
// Created by thuanqin on 16/5/24.
//

#include "controller/docker_agent.h"

#include <map>
#include <sstream>
#include <thread>
#include <vector>

#include "common/config.h"
#include "common/docker_client.h"
#include "common/log.h"
#include "controller/utils.h"
#include "ogp_msg.pb.h"
#include "third/json/json.hpp"

using json = nlohmann::json;


void DockerAgent::init() {
    LOG_INFO("DockerAgent begin initialization now.")
    // 心跳线程
    std::thread t([this](){send_heartbeat();});
    add_thread(std::move(t));
    // controller状态同步线程
    std::thread t2([this](){sync();});
    add_thread(std::move(t2));
    // docker状态同步线程
    std::thread t3([this](){docker_worker();});
    add_thread(std::move(t3));
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

void DockerAgent::docker_worker() {
    unsigned int period = 1;
    while (true) {
        bool do_sync = false;
        try {
            while (!actions_queue_empty()) {
                do_sync = true;
                daaction_ptr action = nullptr;
                action = get_action();
                if (action == nullptr) {
                    // 没有要处理的动作
                    break;
                }
                switch(action->get_action_type()) {
                    // 启动容器
                    case DAActionType::START_CONTAINER:
                        start_container(action->get_container());
                        break;
                    // 停止并删除容器
                    case DAActionType::STOP_AND_REMOVE_CONTAINER:
                        stop_and_remove_container(action->get_container());
                        break;
                }
                // 每次操作之间都休息一下,防止docker进程出问题,同时也能减少controller的压力
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (do_sync) {
                collect_docker_rt_and_sync();
            }
        } catch (...) {
            LOG_ERROR("docker_worker work error.")
        }
        std::this_thread::sleep_for(std::chrono::seconds(period));
    }
}

void DockerAgent::start_container(container_ptr container) {
    LOG_INFO("start container: " << container->get_name())
    try {
        // 首先判断容器是否存在,如果不存在需要先创建容器
        auto container_id = docker_client->get_container_id_by_name(container->get_name());
        if (container_id == "") {
            // 容器不存在,先创建容器
            // 构造请求报文
            json j;
            j["Image"] = container->get_image();
            j["Volumes"] = std::map<std::string, std::map<std::string, std::string>>();
            for (auto &v: container->get_cfg_volumes()) {
                j["Volumes"][v->docker_volume] = std::map<std::string, std::string>();
            }
            for (auto &p: container->get_cfg_ports()) {
                std::string key = std::to_string(p->public_port) + "/" + p->type;
                j["ExposedPorts"][key] = std::map<std::string, std::string>();
            }
            std::vector<std::string> hostconfig_binds;
            for (auto &v: container->get_cfg_volumes()) {
                hostconfig_binds.push_back(
                        v->host_volume + ":" + v->docker_volume
                );
            }
            j["HostConfig"]["Binds"] = hostconfig_binds;
            std::map<std::string, std::vector<std::map<std::string, std::string>>> hostconfig_portbindings;
            for (auto &p: container->get_cfg_ports()) {
                std::map<std::string, std::string> hp;
                hp["HostPort"] = std::to_string(p->private_port);
                std::vector<std::map<std::string, std::string>> ps;
                ps.push_back(hp);
                std::string key = std::to_string(p->public_port) + "/" + p->type;
                hostconfig_portbindings[key] = ps;
            }
            j["HostConfig"]["PortBindings"] = hostconfig_portbindings;
            std::vector<std::string> hostconfig_extrahosts;
            for (auto &d: container->get_cfg_dns()) {
                std::string hd = d->dns + ":" + d->address;
                hostconfig_extrahosts.push_back(hd);
            }
            j["HostConfig"]["ExtraHosts"] = hostconfig_extrahosts;

            LOG_DEBUG("Create container, request data:")
            LOG_DEBUG(j.dump())

            docker_client->create_container(j, container->get_name());
            container_id = docker_client->get_container_id_by_name(container->get_name());
        }

        //启动容器
        docker_client->start_container(container_id);

    } catch (...) {
        LOG_ERROR("start container: " << container->get_name() << " error.")
    }
}

void DockerAgent::stop_and_remove_container(container_ptr container) {
    LOG_INFO("stop and remove container: " << container->get_name())
    try {
        auto container_id = docker_client->get_container_id_by_name(container->get_name());
        if (container_id == "") {
            LOG_WARN("container " << container->get_name() << " not exist.")
            return;
        }
        try {
            docker_client->kill_container(container_id);
        } catch (...) {
            // 忽略这个异常,因为我们马上就会执行remove操作,如果有问题的话remove会暴露问题
        }

        docker_client->remove_container(container_id);

    } catch (...) {
        LOG_ERROR("stop and remove container: " << container->get_name() << " error.")
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
            handle_ct_sync_req_msg(sess, msg);
            break;
        default:
            LOG_ERROR("Unknown msg type: " << static_cast<unsigned int>(msg->get_msg_type()));
    }
}

void DockerAgent::handle_ct_sync_req_msg(sess_ptr sess, msg_ptr msg) {
    // controller发来同步请求,此时da需要收集本地信息交给controller,并从controller中获取target信息
    try {
        ogp_msg::DockerRuntimeInfo docker_runtime_info;
        docker_runtime_info = get_docker_runtime_info_msg();
        send_msg(agent_lock, controller_sess, docker_runtime_info, MsgType::DA_DOCKER_RUNTIME_INFO_SYNC_REQ);
    } catch (std::runtime_error &e) {
        LOG_ERROR("" << e.what())
    }
}

ogp_msg::DockerRuntimeInfo DockerAgent::get_docker_runtime_info_msg() {
    // 获取docker的运行时信息,包括系统资源情况及容器信息
    ogp_msg::DockerRuntimeInfo docker_runtime_info;
    try {
        // 容器信息
        auto containers_info = docker_client->list_containers(1);

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
    } catch (std::runtime_error &e) {
        LOG_ERROR("" << e.what())
    }
    return docker_runtime_info;
}

void DockerAgent::collect_docker_rt_and_sync() {
    try {
        // 获取docker的运行时信息,包括系统资源情况及容器信息
        ogp_msg::DockerRuntimeInfo docker_runtime_info;
        docker_runtime_info = get_docker_runtime_info_msg();

        // 发送同步信息给controller
        send_msg(agent_lock, controller_sess, docker_runtime_info, MsgType::DA_DOCKER_RUNTIME_INFO_SYNC_REQ);

    } catch (std::runtime_error &e) {
        LOG_ERROR("" << e.what())
    }
}

void DockerAgent::sync() {
    auto period = static_cast<unsigned int>(config_mgr.get_item("agent_sync_period")->get_int());

    while (true) {
        collect_docker_rt_and_sync();
        std::this_thread::sleep_for(std::chrono::seconds(period));
    }
}

void DockerAgent::handle_ct_sync_msg(sess_ptr sess, msg_ptr msg) {
    ogp_msg::DockerTargetRuntimeInfo docker_target_runtime_info;
    docker_target_runtime_info.ParseFromArray(msg->get_msg_body(), msg->get_msg_body_size());
    ogp_msg::DockerRuntimeInfo current_docker_rt_infos = get_docker_runtime_info_msg();

    std::vector<container_ptr> containers_to_remove;
    std::vector<container_ptr> containers_to_start;
    // 目前在本地,但实际上controller上没有的容器,这些容器需要被删除
    for (auto &lc: current_docker_rt_infos.containers()) {
        bool need_delete = true;
        std::string c_name = "";
        for (auto &cc: docker_target_runtime_info.containers()) {
            for (auto &name: lc.names()) {
                if (name == ("/" + cc.name())) {
                    need_delete = false;
                    break;
                }
            }
            if (!need_delete) break;
        }
        if (need_delete) {
            container_ptr container = std::make_shared<Container>();
            for (auto &t: lc.names()) {
                auto lc_n = t;
                // 容器名的开头会带有'/',需要去除该符号
                lc_n = lc_n.substr(1, lc_n.length()-1);
                container->set_name(lc_n);
                containers_to_remove.push_back(container);
                break;
            }
        }
    }

    // 目前本地不在运行,但实际上controller上需要本地运行的容器,这些容器需要被创建
    for (auto &cc: docker_target_runtime_info.containers()) {
        bool need_create = true;
        container_ptr container = std::make_shared<Container>();
        for (auto &lc: current_docker_rt_infos.containers()) {
            for (auto &name: lc.names()) {
                const std::string container_status_ok_kw = "Up";
                if (name == ("/" + cc.name()) && lc.status().find(container_status_ok_kw) != std::string::npos) {
                    need_create = false;
                    break;
                }
            }
            if (!need_create) break;
        }
        if (need_create) {
            container->set_name(cc.name());
            container->set_image(cc.image());
            // cfg ports
            for (auto &cfg_port: cc.ports()) {
                auto cfg_port_ = std::make_shared<MACfgPort>();
                cfg_port_->private_port = cfg_port.private_port();
                cfg_port_->public_port = cfg_port.public_port();
                cfg_port_->type = cfg_port.type();
                container->get_cfg_ports().push_back(cfg_port_);
            }
            // cfg volumes
            for (auto &cfg_volume: cc.volumes()) {
                auto cfg_volume_ = std::make_shared<MACfgVolume>();
                cfg_volume_->docker_volume = cfg_volume.docker_volume();
                cfg_volume_->host_volume = cfg_volume.host_volume();
                container->get_cfg_volumes().push_back(cfg_volume_);
            }
            // cfg dns
            for (auto &cfg_dns: cc.dns()) {
                auto cfg_dns_ = std::make_shared<MACfgDns>();
                cfg_dns_->dns = cfg_dns.dns();
                cfg_dns_->address = cfg_dns.address();
                container->get_cfg_dns().push_back(cfg_dns_);
            }
            // cfg extra_cmd
            if (cc.has_extra_cmd()) {
                auto cfg_extra_cmd_ = std::make_shared<MACfgExtraCmd>();
                cfg_extra_cmd_->extra_cmd = cc.extra_cmd();
                container->set_cfg_extra_cmd(cfg_extra_cmd_);
            }

            containers_to_start.push_back(container);
        }
    }

    // 将动作放到动作队列中,等待工作线程处理
    for (auto &c: containers_to_remove) {
        auto action = std::make_shared<DAAction>(c, DAActionType::STOP_AND_REMOVE_CONTAINER);
        add_action(action);
    }
    for (auto &c: containers_to_start) {
        auto action = std::make_shared<DAAction>(c, DAActionType::START_CONTAINER);
        add_action(action);
    }
}