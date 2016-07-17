//
// Created by thuanqin on 16/7/12.
//

#ifndef OGP_CONTROLLER_SERVICE_H
#define OGP_CONTROLLER_SERVICE_H

#include <mutex>
#include <string>
#include <vector>

#include "controller/agents.h"
#include "controller/applications.h"
#include "controller/utils.h"
#include "model/model.h"
#include "ogp_msg.pb.h"

#define ST_PORT_SERVICE "PORT_SERVICE"

class Service {
public:
    Service(int id, std::string service_type,
            int app_id, std::string app_name, int service_port,
            int private_port):id(id), service_type(service_type),
                              app_id(app_id), app_name(app_name),
                              service_port(service_port),
                              private_port(private_port){}
    int get_id() {return id;}
    std::string get_service_type() {return service_type;}
    int get_app_id() {return app_id;}
    std::string get_app_name() {return app_name;}
    int get_service_port() {return service_port;}
    int get_private_port() {return private_port;}
private:
    int id;
    std::string service_type;
    int app_id;
    std::string app_name;
    int service_port;
    int private_port;
};
typedef std::shared_ptr<Service> service_ptr;

class ServicesBase {
public:
    virtual ~ServicesBase() {}
    virtual void add_service(int service_id, std::string service_type,
                             int app_id,
                             int service_port, int private_port) = 0;
    virtual void remove_service(int service_id) = 0;
    virtual std::vector<service_ptr> list_services() = 0;
    virtual void sync_services() = 0;
    virtual void init(AgentsBase *agents, ApplicationsBase *applications, ModelMgrBase *model_mgr) = 0;
    virtual void get_sync_services_data(ogp_msg::ServiceSyncData &service_sync_data) = 0;
};

class Services: public ServicesBase {
public:
    Services() {
        current_uniq_id_lock.lock();
        current_uniq_id = 1;
        current_uniq_id_lock.unlock();
    }
    ~Services() {}
    void init(AgentsBase *agents_, ApplicationsBase *applications_, ModelMgrBase *model_mgr_) {
        agents = agents_;
        applications = applications_;
        model_mgr = model_mgr_;

        LOG_INFO("Initialize services from DB.")
        auto db_services = model_mgr_->list_services();
        for (auto db_service: db_services) {
            add_service(db_service->get_id(), db_service->get_service_type(),
                        db_service->get_app_id(), db_service->get_service_port(),
                        db_service->get_private_port());
        }
    }

    void add_service(int service_id, std::string service_type,
                     int app_id,
                     int service_port, int private_port) {
        auto app_info = applications->get_application(app_id);
        auto s = std::make_shared<Service>(service_id, service_type, app_id,
                                           app_info->get_name(), service_port,
                                           private_port);
        services_lock.lock();
        services.push_back(s);
        services_lock.unlock();
    }

    void remove_service(int service_id) {
        services_lock.lock();
        for (auto it=services.begin(); it!=services.end(); it++) {
            if ((*it)->get_id() == service_id) {
                services.erase(it);
                break;
            }
        }
        services_lock.unlock();
    }

    std::vector<service_ptr> list_services() {
        std::vector<service_ptr> result;
        services_lock.lock();
        for (auto s: services) {
            result.push_back(s);
        }
        services_lock.unlock();
        return result;
    }

    void get_sync_services_data(ogp_msg::ServiceSyncData &service_sync_data) {
        int uniq_id = -1;
        std::vector<agent_ptr> das;
        std::vector<agent_ptr> sdps;
        std::vector<service_ptr> current_services;

        // 保证uniq_id的递增可以反映时间的递增, SDP会依赖这个uniq_id来判断应用什么更新
        current_uniq_id_lock.lock();
        das = agents->get_agents_by_type(DA_NAME);
        sdps = agents->get_agents_by_type(SDP_NAME);
        current_services = list_services();
        uniq_id = current_uniq_id;
        current_uniq_id += 1;

        auto header = service_sync_data.mutable_header();
        int rc = 0;
        std::string ret_msg = "";
        service_sync_data.set_uniq_id(uniq_id);

        for (auto s: current_services) {
            auto service_sync_info = service_sync_data.add_infos();
            service_sync_info->set_service_id(s->get_id());
            service_sync_info->set_service_type(s->get_service_type());
            service_sync_info->set_app_id(s->get_app_id());
            service_sync_info->set_app_name(s->get_app_name());
            service_sync_info->set_service_port(s->get_service_port());

            // 获取ma信息
            for (auto da_: das) {
                auto da = std::static_pointer_cast<DockerAgent>(da_);
                std::string ma_ip;
                int mapping_port = -1;
                // 遍历这个da上的ma
                da->get_applications_lock().lock();
                for (auto ma: da->get_applications()) {
                    if (ma->get_app_id() == s->get_app_id()) {
                        // 获取这个ma的主机信息及端口信息
                        ma_ip = da->get_machine_ip();
                        if (s->get_private_port() != -1) {
                            mapping_port = s->get_private_port();
                        } else {
                            auto ma_ports_info = ma->get_cfg_ports();
                            if (ma_ports_info.size() == 0) {
                                LOG_ERROR("Service " << std::to_string(s->get_id()) << "'s ma has no ports.")
                            } else {
                                mapping_port = ma_ports_info[0]->public_port;
                            }
                        }
                    }
                    if (mapping_port != -1) {
                        auto service_sync_item = service_sync_info->add_items();
                        service_sync_item->set_ma_ip(ma_ip);
                        service_sync_item->set_public_port(mapping_port);
                    }
                }
                da->get_applications_lock().unlock();
            }
        }
        current_uniq_id_lock.unlock();

        header->set_rc(rc);
        header->set_message(ret_msg);
    }

    void sync_services() {

        ogp_msg::ServiceSyncData service_sync_data;
        get_sync_services_data(service_sync_data);

        // 广播给所有的SDProxy
        auto msg_type = MsgType::CT_SDPROXY_SERVICE_DATA_SYNC_REQ;
        first_sync_lock.unlock();
        auto sdps = agents->get_agents_by_type(SDP_NAME);
        for (auto sdp: sdps) {
            send_msg(sdp->get_sess(), service_sync_data, msg_type);
        }

    }
private:
    AgentsBase *agents;
    ApplicationsBase *applications;
    ModelMgrBase *model_mgr;
    std::vector<service_ptr> services;
    std::mutex services_lock;
    std::mutex current_uniq_id_lock;
    int current_uniq_id;
    bool first_sync = true;
    std::mutex first_sync_lock;
};



#endif //OGP_CONTROLLER_SERVICE_H
