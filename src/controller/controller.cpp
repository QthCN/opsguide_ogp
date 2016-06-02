//
// Created by thuanqin on 16/5/13.
//
#include "controller/controller.h"

#include<ctime>
#include <map>
#include <string>
#include <vector>

#include "common/log.h"
#include "model/model.h"
#include "ogp_msg.pb.h"

// agent type
#define DA_NAME "DOCKER_AGENT"
#define NX_NAME "NGINX_AGENT"
#define MG_NAME "MANAGE_AGENT"

class MachineApplication {
public:
    enum class Status {
        UNKNOWN = 1,
        RUNNING = 2,
        STOP = 3,
    };
    MachineApplication(): status(Status::UNKNOWN) {}
    int get_uniq_id() {return uniq_id;}
    void set_uniq_id(int uniq_id_) {uniq_id = uniq_id_;}
    std::string get_app_name() {return app_name;}
    void set_app_name(const std::string &app_name_) {app_name = app_name_;}
    int get_app_id() {return app_id;}
    void set_app_id(int app_id_) {app_id = app_id_;}
    std::string get_machine_ip_address() {return machine_ip_address;}
    void set_machine_ip_address(std::string machine_ip_address_) {machine_ip_address = machine_ip_address_;}
    int get_version_id() {return version_id;}
    void set_version_id(int version_id_) {version_id = version_id_;}
    std::string get_version() {return version;}
    void set_version(std::string version_) {version = version_;}
    Status get_status() {return status;}
    void set_status(Status status_) {status = status_;}
private:
    int uniq_id; // uniq_id表示某台主机上运行的某个app,这个id是唯一的
    std::string app_name;
    int app_id;
    std::string machine_ip_address;
    int version_id;
    std::string version;
    Status status;
};
typedef std::shared_ptr<MachineApplication> application_ptr;

class Agent {
public:
    Agent(std::string agent_type_): sess(nullptr), agent_type(agent_type_),
                                    last_sync_db_time(std::time(0)),
                                    last_sync_time(std::time(0)),
                                    last_heartbeat_time(std::time(0)) { }
    void set_sess(sess_ptr sess_) {
        agent_lock.lock();
        sess = sess_;
        agent_lock.unlock();
    }
    sess_ptr get_sess() {return sess;}
    std::string get_agent_type() {return agent_type;}
    bool has_sess() {return sess != nullptr;}
    std::time_t get_last_sync_db_time() {return last_sync_db_time;}
    void set_last_sync_db_time(std::time_t t) {last_sync_db_time = t;}
    std::time_t get_last_sync_time() {return last_sync_time;}
    void set_last_sync_time(std::time_t t) {last_sync_time = t;}
    std::time_t get_last_heartbeat_time() {return last_heartbeat_time;}
    void set_last_heartbeat_time(std::time_t t) {last_heartbeat_time = t;}
    std::string get_machine_ip() {return machine_ip;}
    void set_machine_ip(std::string machine_ip_) {machine_ip = machine_ip_;}
    std::mutex &get_agent_lock() {return agent_lock;}

private:
    // Agent关联的session
    sess_ptr sess;
    // Agent类型
    std::string agent_type;
    // 最近一次从DB同步该Agent的时刻
    std::time_t last_sync_db_time;
    // 最近一次收到心跳包的时刻
    std::time_t last_heartbeat_time;
    // 最近一次运行时信息同步的时刻
    std::time_t last_sync_time;
    // Agent所在主机IP
    std::string machine_ip;
    // Agent操作的并发锁
    std::mutex agent_lock;
};
typedef std::shared_ptr<Agent> agent_ptr;

class DockerAgent: public Agent {
public:
    DockerAgent(): Agent(DA_NAME) {

    }

    application_ptr get_application(int id) {
        applications_lock.lock();
        for (auto a :applications) {
            if (a->get_uniq_id() == id) {
                applications_lock.unlock();
                return a;
            }
        }
        applications_lock.unlock();
        return nullptr;
    }

    void add_application(application_ptr app) {
        applications_lock.lock();
        for (auto a :applications) {
            if (a->get_uniq_id() == app->get_uniq_id()) {
                applications_lock.unlock();
                throw std::runtime_error("app already exist.");
            }
        }
        applications.push_back(app);
        applications_lock.unlock();
    }

    std::vector<application_ptr> get_applications() {return applications;}

private:
    std::vector<application_ptr> applications;
    std::mutex applications_lock;
};

class NginxAgent: public Agent {
public:
    NginxAgent(): Agent(NX_NAME) {}
};

class Agents {
public:
    Agents() {
    }

    std::string get_key(agent_ptr agent) {
        return agent->get_machine_ip() + ":" + agent->get_agent_type();
    }

    std::string get_key(std::string address, std::string agent_type) {
        return address + ":" + agent_type;
    }

    agent_ptr get_agent_by_key(std::string key) {
        agents_map_lock.lock();
        if (agents_map.find(key) != agents_map.end()) {
            auto agent = agents_map[key];
            agents_map_lock.unlock();
            return agent;
        } else {
            agents_map_lock.unlock();
            return nullptr;
        }
    }

    agent_ptr get_agent_by_sess(sess_ptr sess) {
        agents_map_lock.lock();
        for (auto it=agents_map.begin(); it!=agents_map.end(); it++) {
            auto agent_sess = it->second->get_sess();
            if (agent_sess == nullptr) continue;
            if (agent_sess->get_address() == sess->get_address() && agent_sess->get_port() == sess->get_port()) {
                agents_map_lock.unlock();
                return it->second;
            }
        }
        agents_map_lock.unlock();
        return nullptr;
    }

    agent_ptr get_agent_by_sess(sess_ptr sess, std::string agent_type) {
        agents_map_lock.lock();
        for (auto it=agents_map.begin(); it!=agents_map.end(); it++) {
            if (it->second->get_machine_ip() == sess->get_address() && it->second->get_agent_type() == agent_type) {
                agents_map_lock.unlock();
                return it->second;
            }
        }
        agents_map_lock.unlock();
        return nullptr;
    }

    void add_agent(std::string key, agent_ptr agent) {
        agents_map_lock.lock();
        if (agents_map.find(key) != agents_map.end()) {
            agents_map_lock.unlock();
            throw std::runtime_error("agent already exist.");
        } else {
            agents_map[key] = agent;
            agents_map_lock.unlock();
        }
        LOG_INFO("Current agents size after add_agent: " << agents_map.size())
    }

    void remove_agent(std::string key) {
        agents_map_lock.lock();
        if (agents_map.find(key) == agents_map.end()) {
            agents_map_lock.unlock();
            LOG_WARN("Agent key: " << key << " not exist.")
            return;
        } else {
            agents_map.erase(key);
            agents_map_lock.unlock();
        }
        LOG_INFO("Current agents size after remove_agent: " << agents_map.size())
    }

    void init() {
        LOG_INFO("Initialize agents from DB.")
        // 从数据库中获取agent信息并建通过这些信息初始化agents信息
        auto model_mgr = ModelMgr();
        auto machine_apps_info = model_mgr.get_machine_apps_info();
        for (auto machine_app_info: machine_apps_info) {
            auto key = get_key(machine_app_info->get_ip_address(), DA_NAME);
            auto agent = get_agent_by_key(key);
            if (agent == nullptr) {
                LOG_INFO("Add docker agent from DB, agent key: " << key)
                agent = std::make_shared<DockerAgent>();
                agent->set_machine_ip(machine_app_info->get_ip_address());
                add_agent(key, agent);
            }
            auto docker_agent = std::static_pointer_cast<DockerAgent>(agent);
            auto app = std::make_shared<MachineApplication>();
            app->set_uniq_id(machine_app_info->get_machine_app_list_id());
            app->set_version_id(machine_app_info->get_version_id());
            app->set_version(machine_app_info->get_version());
            app->set_app_id(machine_app_info->get_app_id());
            app->set_app_name(machine_app_info->get_name());
            app->set_machine_ip_address(machine_app_info->get_ip_address());
            try {
                LOG_INFO("Add app, app unique id: " << std::to_string(app->get_uniq_id())
                         << " name: " << app->get_app_name() << " version: " << app->get_version())
                docker_agent->add_application(app);
            } catch (std::runtime_error &e) {
                LOG_ERROR("" << e.what())
                throw std::runtime_error("BUG!!!");
            }
        }
    }

private:
    std::map<std::string, agent_ptr> agents_map;
    std::mutex agents_map_lock;

};

Agents agents;

void Controller::init() {
    agents.init();
    LOG_INFO("Controller begin initialization now.")
}

void Controller::associate_sess(sess_ptr sess) {

}

void Controller::send_heartbeat() {
    // Controller可以不实现该方法
}

void Controller::sync() {
    // Controller可以不实现该方法
}

void Controller::invalid_sess(sess_ptr sess) {
    g_lock.lock();
    LOG_INFO("invalid_sess: " << sess->get_address() << ":" << sess->get_port())
    auto agent = agents.get_agent_by_sess(sess);
    if (agent == nullptr) {
        LOG_WARN("Invalid a no-agent-associated sess.")
        g_lock.unlock();
        return;
    }
    LOG_INFO("Set sess = nullptr, agent key: " << agents.get_key(agent))
    agent->set_sess(nullptr);
    g_lock.unlock();
}

void Controller::handle_msg(sess_ptr sess, msg_ptr msg) {
    agent_ptr agent = nullptr;
    switch(msg->get_msg_type()) {
        // docker agent发来的say hi
        case MsgType::DA_DOCKER_SAY_HI:
            handle_da_say_hi_msg(sess, msg);
            break;
        // docker agent发来的心跳请求
        case MsgType::DA_DOCKER_HEARTBEAT_REQ:
            // 更新心跳同步时间
            agent = agents.get_agent_by_sess(sess, DA_NAME);
            agent->set_last_heartbeat_time(std::time(0));
            sess->send_msg(
                    std::make_shared<Message>(
                            MsgType::CT_DOCKER_HEARTBEAT_RES,
                            new char[0],
                            0
                    )
            );
            break;
        // docker agent发来的其当前运行时信息同步请求
        case MsgType::DA_DOCKER_RUNTIME_INFO_SYNC_REQ:
            handle_da_sync_msg(sess, msg);
            break;
        default:
            LOG_ERROR("Unknown msg type: " << static_cast<unsigned int>(msg->get_msg_type()));
    }
}

void Controller::handle_da_say_hi_msg(sess_ptr sess, msg_ptr msg) {
    g_lock.lock();
    auto agent = agents.get_agent_by_sess(sess, DA_NAME);
    if (agent != nullptr && agent->get_sess() != nullptr) {
        LOG_ERROR("docker agent say hi, but it already exist. agent key: " << agents.get_key(agent))
        sess->invalid_sess();
        g_lock.unlock();
        return;
    } else if (agent != nullptr && agent->get_sess() == nullptr) {
        LOG_INFO("Set session, agent key: " << agents.get_key(agent))
        agent->set_sess(sess);
        g_lock.unlock();
        return;
    } else if (agent == nullptr) {
        auto docker_agent = std::make_shared<DockerAgent>();
        docker_agent->set_machine_ip(sess->get_address());
        LOG_INFO("Add docker agent from sess, agent key: " << agents.get_key(docker_agent));
        agents.add_agent(agents.get_key(docker_agent), docker_agent);
        docker_agent->set_sess(sess);
        g_lock.unlock();
    }
}

void Controller::handle_da_sync_msg(sess_ptr sess, msg_ptr msg) {
    ogp_msg::DockerRuntimeInfo docker_runtime_info;
    docker_runtime_info.ParseFromArray(msg->get_msg_body(), msg->get_msg_body_size());
    // todo(tianhuan) 判断agent的状态是否正常

    // 发送回复
    auto docker_agent = std::static_pointer_cast<DockerAgent>(agents.get_agent_by_sess(sess));
    if (docker_agent == nullptr) return;
    // 更新运行时信息同步时间
    docker_agent->set_last_sync_time(std::time(0));
    ogp_msg::DockerTargetRuntimeInfo docker_target_runtime_info;
    auto target_applications = docker_agent->get_applications();
    for (auto &app: target_applications) {
        auto target_container = docker_target_runtime_info.add_containers();
        target_container->set_image(app->get_app_name() + ":" + app->get_version());
        target_container->set_name(app->get_app_name() + "-" + app->get_version() + "-" + std::to_string(app->get_uniq_id()));
        // todo(tianhuan) 添加ports、volumes、dns的映射信息
    }

    send_msg(docker_agent->get_agent_lock(), docker_agent,
             docker_target_runtime_info, MsgType::CT_DOCKER_RUNTIME_INFO_SYNC_RES);
}