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

class AppVersion {
public:
    AppVersion(int id, int app_id, const std::string &version, const std::string &registe_time,
               const std::string &description) : id(id), app_id(app_id), version(version), registe_time(registe_time),
                                                 description(description) { }

    int get_id() { return id;}
    int get_app_id() {return app_id;}
    std::string get_version() {return version;}
    std::string get_registe_time() {return registe_time;}
    std::string get_description() {return description;}

private:
    int id;
    int app_id;
    std::string version;
    std::string registe_time;
    std::string description;
};
typedef std::shared_ptr<AppVersion> app_version_ptr;

class Application {
public:

    Application(int id, const std::string &source,
                const std::string &name, const std::string &description) : id(id),
                                                                           source(source),
                                                                           name(name),
                                                                           description(description) { }
    int get_id() {return id;}
    std::string get_source() {return source;}
    std::string get_name() {return name;}
    std::string get_description() {return description;}
    void add_version(app_version_ptr v) {
        versions_lock.lock();
        versions.push_back(v);
        versions_lock.unlock();
    }
    std::vector<app_version_ptr> get_versions() {return versions;}

private:
    int id;
    std::string source;
    std::string name;
    std::string description;
    std::vector<app_version_ptr> versions;
    std::mutex versions_lock;
};
typedef std::shared_ptr<Application> application_ptr;

class Applications {
public:
    void add_application(application_ptr a) {
        applications_lock.lock();
        applications.push_back(a);
        applications_lock.unlock();
    }

    application_ptr get_application(int app_id) {
        application_ptr a = nullptr;
        applications_lock.lock();
        for (auto &app: applications) {
            if (app->get_id() == app_id) {
                a = app;
                break;
            }
        }
        applications_lock.unlock();
        return a;
    }

    application_ptr get_application(std::string app_name) {
        application_ptr a = nullptr;
        applications_lock.lock();
        for (auto &app: applications) {
            if (app->get_name() == app_name) {
                a = app;
                break;
            }
        }
        applications_lock.unlock();
        return a;
    }
    std::vector<application_ptr> get_applications() {return applications;}
private:
    std::vector<application_ptr> applications;
    std::mutex applications_lock;
};

Applications applications;

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
typedef std::shared_ptr<MachineApplication> machine_application_ptr;

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

    machine_application_ptr get_application(int id) {
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

    void add_application(machine_application_ptr app) {
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

    std::vector<machine_application_ptr> get_applications() {return applications;}

private:
    std::vector<machine_application_ptr> applications;
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

    std::vector<agent_ptr> get_agents() {
        std::vector<agent_ptr> agents;
        agents_map_lock.lock();
        for (auto it=agents_map.begin(); it!=agents_map.end(); it++) {
            agents.push_back(it->second);
        }
        agents_map_lock.unlock();
        return agents;
    };

    void init() {
        auto model_mgr = ModelMgr();

        LOG_INFO("Initialize applications from DB.")
        auto applications_info = model_mgr.get_applications();
        for (auto application: applications_info) {
            auto a = std::make_shared<Application>(
                    application->get_id(),
                    application->get_source(),
                    application->get_name(),
                    application->get_description()
            );
            auto vs = model_mgr.get_app_versions_by_app_id(a->get_id());
            for (auto &v: vs) {
                auto ver = std::make_shared<AppVersion>(
                        v->get_id(),
                        v->get_app_id(),
                        v->get_version(),
                        v->get_registe_time(),
                        v->get_description()
                );
                a->add_version(ver);
            }
            applications.add_application(a);
        }

        LOG_INFO("Initialize agents from DB.")
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
    LOG_INFO("Controller initialization finished.")
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

            // portal发来的获取app列表的请求
        case MsgType::PO_PORTAL_GET_APPS_REQ:
            handle_po_get_apps_msg(sess, msg);
            break;
            // portal发来的获取agent列表的请求
        case MsgType::PO_PORTAL_GET_AGENTS_REQ:
            handle_po_get_agents_msg(sess, msg);
            break;

            // cli发来的创建app请求
        case MsgType::CI_CLI_ADD_APP_REQ:
            handle_ci_add_app(sess, msg);
            break;
        default:
            LOG_ERROR("Unknown msg type: " << static_cast<unsigned int>(msg->get_msg_type()));
    }
}

void Controller::handle_po_get_agents_msg(sess_ptr sess, msg_ptr msg) {
    ogp_msg::ControllerAgentList agent_list;
    auto header = agent_list.mutable_header();
    int rc = 0;
    std::string ret_msg = "";

    auto agents_ = agents.get_agents();
    for (auto &a: agents_) {
        auto agent = agent_list.add_agents();
        agent->set_type(a->get_agent_type());
        agent->set_ip(a->get_machine_ip());
        agent->set_last_heartbeat_time(a->get_last_heartbeat_time());
        agent->set_last_sync_db_time(a->get_last_sync_db_time());
        agent->set_last_sync_time(a->get_last_sync_time());
        if (a->get_sess() == nullptr) {
            agent->set_has_sess(0);
        } else {
            agent->set_has_sess(1);
        }

        if (a->get_agent_type() == DA_NAME) {
            auto da = std::static_pointer_cast<DockerAgent>(a);
            auto apps = da->get_applications();
            for (auto &app: apps) {
                auto agent_app = agent->add_applications();
                agent_app->set_app_name(app->get_app_name());
                agent_app->set_app_version(app->get_version());
                agent_app->set_app_id(app->get_app_id());
                agent_app->set_uniq_id(app->get_uniq_id());
            }
        }

    }

    header->set_rc(rc);
    header->set_message(ret_msg);
    send_msg(sess, agent_list, MsgType::CT_PORTAL_GET_AGENTS_RES);
}

void Controller::handle_ci_add_app(sess_ptr sess, msg_ptr msg) {
    auto model_mgr = ModelMgr();
    ogp_msg::AddApplicationReq add_app_req;
    ogp_msg::AddApplicationRes add_app_res;
    int app_id;
    int version_id;
    std::string registe_time;
    int rc = 0;
    std::string ret_msg = "";
    try {
        add_app_req.ParseFromArray(msg->get_msg_body(), msg->get_msg_body_size());
        model_mgr.add_app(
                add_app_req.app_name(),
                add_app_req.app_source(),
                add_app_req.app_desc(),
                add_app_req.app_version(),
                add_app_req.app_version_desc(),
                &app_id,
                &version_id,
                &registe_time
        );
        g_lock.lock();
        auto app = applications.get_application(add_app_req.app_name());
        if (app == nullptr) {
            app = std::make_shared<Application>(
                    app_id,
                    add_app_req.app_source(),
                    add_app_req.app_name(),
                    add_app_req.app_desc()
            );
            applications.add_application(app);
        }
        g_lock.unlock();
        app->add_version(
                std::make_shared<AppVersion>(
                    version_id,
                    app_id,
                    add_app_req.app_version(),
                    registe_time,
                    add_app_req.app_version_desc()
                )
        );
    } catch (std::runtime_error &e) {
        rc = 1;
        ret_msg = e.what();
    } catch (sql::SQLException &e) {
        rc = 2;
        ret_msg = e.what();
    }

    auto header = add_app_res.mutable_header();
    header->set_rc(rc);
    header->set_message(ret_msg);
    send_msg(sess, add_app_res, MsgType::CT_CLI_ADD_APP_RES);

}

void Controller::handle_po_get_apps_msg(sess_ptr sess, msg_ptr msg) {
    ogp_msg::ControllerApplicationsList applications_list;
    auto header = applications_list.mutable_header();
    int rc = 0;
    std::string ret_msg = "";
    g_lock.lock();
    auto apps = applications.get_applications();
    for (auto app: apps) {
        auto a = applications_list.add_applications();
        a->set_id(app->get_id());
        a->set_name(app->get_name());
        a->set_description(app->get_description());
        a->set_source(app->get_source());

        for (auto ver: app->get_versions()) {
            auto v = a->add_versions();
            v->set_id(ver->get_id());
            v->set_description(ver->get_description());
            v->set_registe_time(ver->get_registe_time());
            v->set_version(ver->get_version());
        }
    }
    g_lock.unlock();
    header->set_rc(rc);
    header->set_message(ret_msg);
    send_msg(sess, applications_list, MsgType::CT_PORTAL_GET_APPS_RES);
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