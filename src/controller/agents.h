//
// Created by thuanqin on 16/6/17.
//

#ifndef OGP_CONTROLLER_AGENTS_H
#define OGP_CONTROLLER_AGENTS_H

#include <ctime>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "common/log.h"
#include "controller/applications.h"
#include "controller/ma.h"
#include "controller/utils.h"
#include "model/model.h"

// agent type
#define DA_NAME "DOCKER_AGENT"
#define SDP_NAME "SDPROXY_AGENT"
#define NX_NAME "NGINX_AGENT"

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

class SDProxyAgent: public Agent {
public:
    SDProxyAgent(): Agent(SDP_NAME) {}
    ~SDProxyAgent() {}
};


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

    void remove_application(int uniq_id) {
        applications_lock.lock();
        for (auto it=applications.begin(); it!=applications.end(); it++) {
            if ((*it)->get_uniq_id() == uniq_id) {
                applications.erase(it);
                break;
            }
        }
        applications_lock.unlock();
    }

    std::mutex &get_applications_lock() {return applications_lock;}

    std::vector<machine_application_ptr> get_applications() {return applications;}

private:
    std::vector<machine_application_ptr> applications;
    std::mutex applications_lock;
};

class NginxAgent: public Agent {
public:
    NginxAgent(): Agent(NX_NAME) {}
};

class AgentsBase {
public:
    virtual ~AgentsBase() {}
    virtual std::string get_key(agent_ptr agent) = 0;
    virtual std::string get_key(std::string address, std::string agent_type) = 0;
    virtual agent_ptr get_agent_by_key(std::string key) = 0;
    virtual agent_ptr get_agent_by_sess(sess_ptr sess) = 0;
    virtual agent_ptr get_agent_by_sess(sess_ptr sess, std::string agent_type) = 0;
    virtual void add_agent(std::string key, agent_ptr agent) = 0;
    virtual std::vector<agent_ptr> get_agents() = 0;
    virtual std::vector<agent_ptr> get_agents_by_type(std::string agent_type) = 0;
    virtual void dump_status() = 0;
    virtual void init(ModelMgrBase *model_mgr_, ApplicationsBase *applications_) = 0;
};

class Agents: public AgentsBase {
public:
    Agents() {
    }

    ~Agents() {

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


    std::vector<agent_ptr> get_agents() {
        std::vector<agent_ptr> agents;
        agents_map_lock.lock();
        for (auto it=agents_map.begin(); it!=agents_map.end(); it++) {
            agents.push_back(it->second);
        }
        agents_map_lock.unlock();
        return agents;
    };

    std::vector<agent_ptr> get_agents_by_type(std::string agent_type) {
        std::vector<agent_ptr> agents;
        agents_map_lock.lock();
        for (auto it=agents_map.begin(); it!=agents_map.end(); it++) {
            if (it->second->get_agent_type() == agent_type) {
                agents.push_back(it->second);
            }
        }
        agents_map_lock.unlock();
        return agents;
    }

    void dump_status() {
        auto agents = get_agents();
        LOG_INFO("Agent size: " << std::to_string(agents.size()))
        LOG_INFO("")
        for (auto &agent: agents) {
            auto agent_type = agent->get_agent_type();
            if (agent_type == DA_NAME) {
                LOG_INFO("DA")
                LOG_INFO("IP: " << agent->get_machine_ip())
                LOG_INFO("Has SESS: " << std::to_string(agent->has_sess()))
                auto da = std::static_pointer_cast<DockerAgent>(agent);
                da->get_applications_lock().lock();
                auto apps = da->get_applications();
                for (auto app: apps) {
                    std::string app_status = "";
                    switch(app->get_status()) {
                        case MAStatus::STOP:
                            app_status = "stop";
                            break;
                        case MAStatus::RUNNING:
                            app_status = "running";
                            break;
                        case MAStatus::UNKNOWN:
                            app_status = "unknown";
                            break;
                        case MAStatus::NOT_RUNNING:
                            app_status = "not running";
                            break;
                    }
                    LOG_INFO("APP, NAME: " << app->get_app_name() << " VERSION: " << app->get_version()
                             << " RUNTIME_NAME: " << app->get_runtime_name() << " STATUS: " << app_status)
                }
                da->get_applications_lock().unlock();
            } else if (agent_type == SDP_NAME) {
                LOG_INFO("SDP")
                LOG_INFO("IP: " << agent->get_machine_ip())
                LOG_INFO("Has SESS: " << std::to_string(agent->has_sess()))
            }
            LOG_INFO("---------")
        }
    }

    void init(ModelMgrBase *model_mgr_, ApplicationsBase *applications_) {
        LOG_INFO("Initialize applications from DB.")
        model_mgr = model_mgr_;
        applications = applications_;
        auto applications_info = model_mgr->get_applications();
        for (auto application: applications_info) {
            auto a = std::make_shared<Application>(
                    application->get_id(),
                    application->get_source(),
                    application->get_name(),
                    application->get_description()
            );
            auto vs = model_mgr->get_app_versions_by_app_id(a->get_id());
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
            applications->add_application(a);
        }

        LOG_INFO("Initialize agents from DB.")
        auto machine_apps_info = model_mgr->get_machine_apps_info();
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
            app->set_runtime_name(machine_app_info->get_runtime_name());

            // cfg_ports
            for (auto &dcp: machine_app_info->get_cfg_ports()) {
                auto cfg_port = std::make_shared<MACfgPort>();
                cfg_port->private_port = dcp->get_private_port();
                cfg_port->public_port = dcp->get_public_port();
                cfg_port->type = dcp->get_type();
                app->add_cfg_port(cfg_port);
            }

            // cfg_volumes
            for (auto &dcv: machine_app_info->get_cfg_volumes()) {
                auto cfg_volume = std::make_shared<MACfgVolume>();
                cfg_volume->docker_volume = dcv->get_docker_volume();
                cfg_volume->host_volume = dcv->get_host_volume();
                app->add_cfg_volume(cfg_volume);
            }

            // cfg_dns
            for (auto &dcd: machine_app_info->get_cfg_dns()) {
                auto cfg_dns = std::make_shared<MACfgDns>();
                cfg_dns->address = dcd->get_address();
                cfg_dns->dns = dcd->get_dns();
                app->add_cfg_dns(cfg_dns);
            }

            // cfg_extra_cmd
            auto cfg_extra_cmd = std::make_shared<MACfgExtraCmd>();
            if (machine_app_info->get_cfg_extra_cmd() != nullptr) {
                cfg_extra_cmd->extra_cmd = machine_app_info->get_cfg_extra_cmd()->get_extra_cmd();
            }
            app->set_cfg_extra_cmd(cfg_extra_cmd);

            // hints
            for (auto &dh: machine_app_info->get_hints()) {
                auto hint = std::make_shared<MAHint>();
                hint->item = dh->get_item();
                hint->value = dh->get_value();
                app->add_hint(hint);
            }

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
    ModelMgrBase *model_mgr;
    ApplicationsBase *applications;
};

#endif //OGP_CONTROLLER_AGENTS_H
