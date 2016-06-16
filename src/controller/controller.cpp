//
// Created by thuanqin on 16/5/13.
//
#include "controller/controller.h"

#include<ctime>
#include <map>
#include <string>
#include <vector>

#include "common/log.h"
#include "controller/ma.h"
#include "controller/utils.h"
#include "model/model.h"
#include "ogp_msg.pb.h"

// agent type
#define DA_NAME "DOCKER_AGENT"
#define NX_NAME "NGINX_AGENT"

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
    app_version_ptr get_version(std::string version) {
        app_version_ptr ver = nullptr;
        versions_lock.lock();
        for (auto v: versions) {
            if (v->get_version() == version) {
                ver = v;
                break;
            }
        }
        versions_lock.unlock();
        return ver;
    }

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
        for (auto app: applications) {
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
        for (auto app: applications) {
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

    void remove_application(int uniq_id) {
        LOG_INFO("remove_application")
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


    std::vector<agent_ptr> get_agents() {
        std::vector<agent_ptr> agents;
        agents_map_lock.lock();
        for (auto it=agents_map.begin(); it!=agents_map.end(); it++) {
            agents.push_back(it->second);
        }
        agents_map_lock.unlock();
        return agents;
    };

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
            }
            LOG_INFO("---------")
        }
    }

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

};

Agents agents;

void Controller::init() {
    LOG_INFO("Initialize agents.")
    agents.init();
    LOG_INFO("Initialize scheduler.")
    scheduler.init();
    LOG_INFO("Controller initialization finished.")
}

void Controller::associate_sess(sess_ptr sess) {

}

void Controller::invalid_sess(sess_ptr sess) {
    g_lock.lock();
    auto agent = agents.get_agent_by_sess(sess);
    if (agent == nullptr) {
        g_lock.unlock();
        return;
    }
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
            // portal发来的发布app的请求
        case MsgType::PO_PORTAL_PUBLISH_APP_REQ:
            handle_po_publish_app_msg(sess, msg);
            break;
            // portal发来的下线version的请求
        case MsgType::PO_PORTAL_REMOVE_APPVER_REQ:
            handle_po_remove_appver_msg(sess, msg);
            break;
            // portal发来的升级version的请求
        case MsgType::PO_PORTAL_UPGRADE_APPVER_REQ:
            handle_po_upgrade_appver_msg(sess, msg);
            break;

            // cli发来的创建app请求
        case MsgType::CI_CLI_ADD_APP_REQ:
            handle_ci_add_app(sess, msg);
            break;
        default:
            LOG_ERROR("Unknown msg type: " << static_cast<unsigned int>(msg->get_msg_type()));
    }
}

void Controller::handle_po_upgrade_appver_msg(sess_ptr sess, msg_ptr msg) {
    auto model_mgr = ModelMgr();
    ogp_msg::UpgradeAppVerReq upgrade_appver_req;
    upgrade_appver_req.ParseFromArray(msg->get_msg_body(), msg->get_msg_body_size());
    ogp_msg::UpgradeAppVerRes upgrade_appver_res;
    auto header = upgrade_appver_res.mutable_header();
    int rc = 0;
    std::string ret_msg = "";

    auto new_version = upgrade_appver_req.new_version();
    auto old_uniq_id = upgrade_appver_req.old_ver_uniq_id();
    auto new_runtime_name = upgrade_appver_req.runtime_name();

    // 获取相关的内存对象
    auto agents_ = agents.get_agents();
    std::shared_ptr<DockerAgent> target_agent = nullptr;
    machine_application_ptr old_app = nullptr;
    for (auto &agent: agents_) {
        if (agent->get_agent_type() == DA_NAME) {
            auto da = std::static_pointer_cast<DockerAgent>(agent);
            auto app = da->get_application(old_uniq_id);
            if (app != nullptr) {
                old_app = app;
                target_agent = da;
                break;
            }
        }
    }

    // 检查新的版本是否存在,另外判断新的版本和老的版本是否一致
    if (old_app == nullptr || target_agent == nullptr) {
        LOG_WARN("no record exist with uniq_id: " << std::to_string(old_uniq_id))
        rc = 1;
        ret_msg = ("no record exist with uniq_id: " + std::to_string(old_uniq_id));
    } else if (new_version == old_app->get_version()) {
        LOG_WARN("same version, no action taken.")
        rc = 2;
        ret_msg = "same version, no action taken.";
    } else if (new_runtime_name == "") {
        LOG_ERROR("runtime_name is empty.")
        rc = 3;
        ret_msg = "runtime_name is empty.";
    } else {
        // 进行版本更新,首先更新DB,然后更新内存,接着向da发送消息
        try {
            model_mgr.update_version(old_uniq_id, old_app->get_version_id(), new_runtime_name);
        } catch (sql::SQLException &e) {
            rc = 4;
            ret_msg = e.what();
        }

        // DB持久化成功,更新内存结构
        if (rc == 0) {
            old_app->set_version(new_version);
            old_app->set_version_id(old_app->get_version_id());
            // todo(tianhuan): runtime_name是我们判断一个应用的运行时信息是否改变的唯一要素,因此需要最后再更新,
            // 但status可能会由于竞争存在一段时间的错误状态,不过这种概率非常小
            old_app->set_runtime_name(new_runtime_name);
            old_app->set_status(MAStatus::NOT_RUNNING);
            // 通知da
            send_simple_msg(target_agent->get_sess(), MsgType::CT_DOCKER_RUNTIME_INFO_SYNC_REQ);
        }
    }


    header->set_rc(rc);
    header->set_message(ret_msg);
    send_msg(sess, upgrade_appver_res, MsgType::CT_PORTAL_UPGRADE_APPVER_RES);
}

void Controller::handle_po_remove_appver_msg(sess_ptr sess, msg_ptr msg) {
    auto model_mgr = ModelMgr();
    ogp_msg::RemoveAppVerReq remove_appver_req;
    remove_appver_req.ParseFromArray(msg->get_msg_body(), msg->get_msg_body_size());
    ogp_msg::RemoveAppVerRes remove_appver_res;
    auto header = remove_appver_res.mutable_header();
    int rc = 0;
    std::string ret_msg = "";

    try {
        model_mgr.remove_version(remove_appver_req.uniq_id());
    } catch (sql::SQLException &e) {
        rc = 1;
        ret_msg = e.what();
    }

    // DB层面已经版本下线完成
    if (rc == 0) {
        auto agents_ = agents.get_agents();
        for (auto &agent: agents_) {
            if (agent->get_agent_type() == DA_NAME) {
                auto da = std::static_pointer_cast<DockerAgent>(agent);
                da->remove_application(remove_appver_req.uniq_id());
                // 通知da
                send_simple_msg(da->get_sess(), MsgType::CT_DOCKER_RUNTIME_INFO_SYNC_REQ);
                break;
            }
        }
    }

    header->set_rc(rc);
    header->set_message(ret_msg);
    send_msg(sess, remove_appver_res, MsgType::CT_PORTAL_REMOVE_APPVER_RES);
}

void Controller::handle_po_publish_app_msg(sess_ptr sess, msg_ptr msg) {
    auto model_mgr = ModelMgr();
    ogp_msg::PublishAppReq publish_app_req;
    publish_app_req.ParseFromArray(msg->get_msg_body(), msg->get_msg_body_size());
    ogp_msg::PublishAppRes publish_app_res;
    auto header = publish_app_res.mutable_header();
    int rc = 0;
    std::string ret_msg = "";

    auto application = applications.get_application(publish_app_req.app_id());
    if (application == nullptr) {
        rc = -1;
        ret_msg = "Application not exist";
    } else {
        auto version = application->get_version(publish_app_req.app_version());
        if (version == nullptr) {
            rc = -2;
            ret_msg = "Version not exist";
        }
    }

    // 应用版本存在,可以进行部署
    std::string target_agent_ip = "";
    int uniq_id = -1;
    auto app_id = -1;
    std::string app_name = "";
    std::string runtime_name = "";
    auto version_id = -1;
    if (rc == 0) {
        // 从scheduler中获取目标主机
        if (publish_app_req.has_hints() && publish_app_req.hints().has_da_ip()) {
            target_agent_ip = scheduler.da_schedule(publish_app_req.hints().da_ip());
        } else {
            target_agent_ip = scheduler.da_schedule();
        }

        // 现在我们已经知道这个应用要部署在哪个主机上了,因此我们可以将数据写入数据库,然后同步内存信息,并通知da
        app_id = publish_app_req.app_id();
        app_name = applications.get_application(app_id)->get_name();
        runtime_name = publish_app_req.runtime_name();
        version_id = application->get_version(publish_app_req.app_version())->get_id();
        std::vector<publish_app_cfg_ports_model_ptr> cfg_ports;
        std::vector<publish_app_cfg_volumes_model_ptr> cfg_volumes;
        std::vector<publish_app_cfg_dns_model_ptr> cfg_dns;
        publish_app_cfg_extra_cmd_model_ptr cfg_extra_cmd = nullptr;
        std::vector<publish_app_hints_model_ptr> hints;

        if (publish_app_req.has_app_cfg()) {
            // cfg_ports
            for (auto cp: publish_app_req.app_cfg().ports()) {
                auto cp_ = std::make_shared<PublishAppCfgPortsModel>();
                cp_->set_uniq_id(uniq_id);
                cp_->set_private_port(cp.private_port());
                cp_->set_public_port(cp.public_port());
                cp_->set_type(cp.type());
                cfg_ports.push_back(cp_);
            }

            // cfg_volumes
            for (auto cv: publish_app_req.app_cfg().volumes()) {
                auto cv_ = std::make_shared<PublishAppCfgVolumesModel>();
                cv_->set_uniq_id(uniq_id);
                cv_->set_docker_volume(cv.docker_volume());
                cv_->set_host_volume(cv.host_volume());
                cfg_volumes.push_back(cv_);
            }

            // cfg_dns
            for (auto cd: publish_app_req.app_cfg().dns()) {
                auto cd_ = std::make_shared<PublishAppCfgDnsModel>();
                cd_->set_uniq_id(uniq_id);
                cd_->set_address(cd.address());
                cd_->set_dns(cd.dns());
                cfg_dns.push_back(cd_);
            }

            // cfg_extra_cmd
            if (publish_app_req.app_cfg().has_extra_cmd()) {
                cfg_extra_cmd = std::make_shared<PublishAppCfgExtraCmdModel>();
                cfg_extra_cmd->set_uniq_id(uniq_id);
                cfg_extra_cmd->set_extra_cmd(publish_app_req.app_cfg().extra_cmd());
            }
        }

        // hints
        if (publish_app_req.has_hints()) {
            if (publish_app_req.hints().has_da_ip()) {
                auto hint = std::make_shared<PublishAppHintsModel>();
                hint->set_uniq_id(uniq_id);
                hint->set_item("da_ip");
                hint->set_value(publish_app_req.hints().da_ip());
                hints.push_back(hint);
            }
        }

        // DB持久化
        try {
            model_mgr.bind_app(target_agent_ip, app_id, version_id, runtime_name,
                               cfg_ports, cfg_volumes, cfg_dns, cfg_extra_cmd,
                               hints, &uniq_id);
        } catch (sql::SQLException &e) {
            rc = 2;
            ret_msg = e.what();
        }
    }

    // DB持久化成功
    if (rc == 0) {
        // 数据结构同步
        auto app = std::make_shared<MachineApplication>();

        app->set_uniq_id(uniq_id);
        app->set_version_id(version_id);
        app->set_version(publish_app_req.app_version());
        app->set_app_id(app_id);
        app->set_app_name(app_name);
        app->set_machine_ip_address(target_agent_ip);
        app->set_runtime_name(runtime_name);

        if (publish_app_req.has_app_cfg()) {
            // cfg_ports
            for (auto &dcp: publish_app_req.app_cfg().ports()) {
                auto cfg_port = std::make_shared<MACfgPort>();
                cfg_port->private_port = dcp.private_port();
                cfg_port->public_port = dcp.public_port();
                cfg_port->type = dcp.type();
                app->add_cfg_port(cfg_port);
            }

            // cfg_volumes
            for (auto &dcv: publish_app_req.app_cfg().volumes()) {
                auto cfg_volume = std::make_shared<MACfgVolume>();
                cfg_volume->docker_volume = dcv.docker_volume();
                cfg_volume->host_volume = dcv.host_volume();
                app->add_cfg_volume(cfg_volume);
            }

            // cfg_dns
            for (auto &dcd: publish_app_req.app_cfg().dns()) {
                auto cfg_dns = std::make_shared<MACfgDns>();
                cfg_dns->address = dcd.address();
                cfg_dns->dns = dcd.dns();
                app->add_cfg_dns(cfg_dns);
            }

            // cfg_extra_cmd
            auto cfg_extra_cmd_ = std::make_shared<MACfgExtraCmd>();
            if (publish_app_req.app_cfg().has_extra_cmd()) {
                cfg_extra_cmd_->extra_cmd = publish_app_req.app_cfg().extra_cmd();
            }
            app->set_cfg_extra_cmd(cfg_extra_cmd_);
        }

        // hints
        if (publish_app_req.has_hints()) {
            if (publish_app_req.hints().has_da_ip()) {
                auto hint = std::make_shared<MAHint>();
                hint->item = "da_ip";
                hint->value = publish_app_req.hints().da_ip();
                app->add_hint(hint);
            }
        }

        auto target_agent = agents.get_agent_by_key(agents.get_key(target_agent_ip, DA_NAME));
        auto da_agent = std::static_pointer_cast<DockerAgent>(target_agent);
        da_agent->add_application(app);
    }

    // 状态同步都成功了,通知docker agent(da)同步最新状态
    if (rc == 0) {
        auto target_agent = agents.get_agent_by_key(agents.get_key(target_agent_ip, DA_NAME));
        send_simple_msg(target_agent->get_sess(), MsgType::CT_DOCKER_RUNTIME_INFO_SYNC_REQ);
    }

    header->set_rc(rc);
    header->set_message(ret_msg);
    send_msg(sess, publish_app_res, MsgType::CT_PORTAL_PUBLISH_APP_RES);
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
            agent->set_has_sess(1);
        } else {
            agent->set_has_sess(0);
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
                agent_app->set_runtime_name(app->get_runtime_name());
                switch(app->get_status()) {
                    case MAStatus::UNKNOWN:
                        agent_app->set_status("UNKNOWN");
                        break;
                    case MAStatus::RUNNING:
                        agent_app->set_status("RUNNING");
                        break;
                    case MAStatus::STOP:
                        agent_app->set_status("STOP");
                        break;
                    case MAStatus::NOT_RUNNING:
                        agent_app->set_status("NOT_RUNNING");
                        break;
                }
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
    auto docker_agent = std::static_pointer_cast<DockerAgent>(agents.get_agent_by_sess(sess));
    if (docker_agent == nullptr) {
        LOG_ERROR("docker_agent is nullptr")
        return;
    }
    // 更新运行时信息同步时间
    docker_agent->set_last_sync_time(std::time(0));

    // 判断状态是否正常
    docker_agent->get_applications_lock().lock();
    auto apps = docker_agent->get_applications();
    // 是否有应该运行在da上的,但da没有运行的应用
    std::vector<machine_application_ptr> apps_not_on_da;
    // 应该运行在da上,且da确实运行了的,状态不正常的应用
    std::vector<machine_application_ptr> apps_on_da_not_ok;
    for (auto app: apps) {
        bool has_this_app = false;
        std::string cn_status = "";
        for (auto &container: docker_runtime_info.containers()) {
            for (auto container_name: container.names()) {
                container_name = container_name.substr(1, container_name.length()-1);
                if (app->get_runtime_name() == container_name) {
                    has_this_app = true;
                    cn_status = container.status();
                    break;
                }
            }
            if (has_this_app) break;
        }

        if (!has_this_app) {
            apps_not_on_da.push_back(app);
            app->set_status(MAStatus::NOT_RUNNING);
        } else {
            const std::string container_status_ok_kw = "Up";
            const std::string container_status_may_not_ok_kw = "Up Less than a second";
            if (cn_status.find(container_status_ok_kw) == std::string::npos
                || cn_status.find(container_status_may_not_ok_kw) != std::string::npos ) {
                apps_on_da_not_ok.push_back(app);
                app->set_status(MAStatus::STOP);
            } else {
                app->set_status(MAStatus::RUNNING);
            }
        }
    }
    docker_agent->get_applications_lock().unlock();

    // 发送回复
    ogp_msg::DockerTargetRuntimeInfo docker_target_runtime_info;
    docker_agent->get_applications_lock().lock();
    auto target_applications = docker_agent->get_applications();
    for (auto &app: target_applications) {
        auto target_container = docker_target_runtime_info.add_containers();
        target_container->set_name(app->get_runtime_name());
        target_container->set_image(app->get_image());
        // cfg extra_cmd
        if (app->get_cfg_extra_cmd() != nullptr) {
            target_container->set_extra_cmd(app->get_cfg_extra_cmd()->extra_cmd);
        }
        // cfg ports
        for (auto cfg_port: app->get_cfg_ports()) {
            auto target_port = target_container->add_ports();
            target_port->set_private_port(cfg_port->private_port);
            target_port->set_public_port(cfg_port->public_port);
            target_port->set_type(cfg_port->type);
        }
        // cfg volumes
        for (auto cfg_volume: app->get_cfg_volumes()) {
            auto target_volume = target_container->add_volumes();
            target_volume->set_docker_volume(cfg_volume->docker_volume);
            target_volume->set_host_volume(cfg_volume->host_volume);
        }
        // cfg dns
        for (auto cfg_dns: app->get_cfg_dns()) {
            auto target_dns = target_container->add_dns();
            target_dns->set_address(cfg_dns->address);
            target_dns->set_dns(cfg_dns->dns);
        }
    }
    docker_agent->get_applications_lock().unlock();

    send_msg(docker_agent->get_agent_lock(), docker_agent,
             docker_target_runtime_info, MsgType::CT_DOCKER_RUNTIME_INFO_SYNC_RES);

    agents.dump_status();
}