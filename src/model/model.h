//
// Created by thuanqin on 16/6/1.
//

#ifndef OGP_MODEL_MODEL_H
#define OGP_MODEL_MODEL_H

#include <string>
#include <vector>

#include "mysql_connection.h"
#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"

#include "common/config.h"

class PublishAppHintsModel {
public:
    PublishAppHintsModel() {}

    int get_uniq_id() const {
        return uniq_id;
    }

    void set_uniq_id(int uniq_id) {
        PublishAppHintsModel::uniq_id = uniq_id;
    }

    const std::string &get_item() const {
        return item;
    }

    void set_item(const std::string &item) {
        PublishAppHintsModel::item = item;
    }

    const std::string &get_value() const {
        return value;
    }

    void set_value(const std::string &value) {
        PublishAppHintsModel::value = value;
    }

private:
    int uniq_id;
    std::string item;
    std::string value;
};
typedef std::shared_ptr<PublishAppHintsModel> publish_app_hints_model_ptr;

class PublishAppCfgPortsModel {
public:
    PublishAppCfgPortsModel() {}

    int get_uniq_id() const {
        return uniq_id;
    }

    void set_uniq_id(int uniq_id) {
        PublishAppCfgPortsModel::uniq_id = uniq_id;
    }

    int get_private_port() const {
        return private_port;
    }

    void set_private_port(int private_port) {
        PublishAppCfgPortsModel::private_port = private_port;
    }

    int get_public_port() const {
        return public_port;
    }

    void set_public_port(int public_port) {
        PublishAppCfgPortsModel::public_port = public_port;
    }

    const std::string &get_type() const {
        return type;
    }

    void set_type(const std::string &type) {
        PublishAppCfgPortsModel::type = type;
    }

private:
    int uniq_id;
    int private_port;
    int public_port;
    std::string type;
};
typedef std::shared_ptr<PublishAppCfgPortsModel> publish_app_cfg_ports_model_ptr;

class PublishAppCfgVolumesModel {
public:
    PublishAppCfgVolumesModel() {}

    int get_uniq_id() const {
        return uniq_id;
    }

    void set_uniq_id(int uniq_id) {
        PublishAppCfgVolumesModel::uniq_id = uniq_id;
    }

    const std::string &get_docker_volume() const {
        return docker_volume;
    }

    void set_docker_volume(const std::string &docker_volume) {
        PublishAppCfgVolumesModel::docker_volume = docker_volume;
    }

    const std::string &get_host_volume() const {
        return host_volume;
    }

    void set_host_volume(const std::string &host_volume) {
        PublishAppCfgVolumesModel::host_volume = host_volume;
    }

private:
    int uniq_id;
    std::string docker_volume;
    std::string host_volume;
};
typedef std::shared_ptr<PublishAppCfgVolumesModel> publish_app_cfg_volumes_model_ptr;

class PublishAppCfgDnsModel {
public:
    PublishAppCfgDnsModel() {}

    int get_uniq_id() const {
        return uniq_id;
    }

    void set_uniq_id(int uniq_id) {
        PublishAppCfgDnsModel::uniq_id = uniq_id;
    }

    const std::string &get_dns() const {
        return dns;
    }

    void set_dns(const std::string &dns) {
        PublishAppCfgDnsModel::dns = dns;
    }

    const std::string &get_address() const {
        return address;
    }

    void set_address(const std::string &address) {
        PublishAppCfgDnsModel::address = address;
    }

private:
    int uniq_id;
    std::string dns;
    std::string address;
};
typedef std::shared_ptr<PublishAppCfgDnsModel> publish_app_cfg_dns_model_ptr;

class PublishAppCfgExtraCmdModel {
public:
    PublishAppCfgExtraCmdModel() {}

    int get_uniq_id() const {
        return uniq_id;
    }

    void set_uniq_id(int uniq_id) {
        PublishAppCfgExtraCmdModel::uniq_id = uniq_id;
    }

    const std::string &get_extra_cmd() const {
        return extra_cmd;
    }

    void set_extra_cmd(const std::string &extra_cmd) {
        PublishAppCfgExtraCmdModel::extra_cmd = extra_cmd;
    }

private:
    int uniq_id;
    std::string extra_cmd;
};
typedef std::shared_ptr<PublishAppCfgExtraCmdModel> publish_app_cfg_extra_cmd_model_ptr;

class MachineAppsInfoModel {
public:
    MachineAppsInfoModel() {}

    int get_machine_app_list_id() const {
        return machine_app_list_id;
    }

    void set_machine_app_list_id(int machine_app_list_id) {
        MachineAppsInfoModel::machine_app_list_id = machine_app_list_id;
    }

    const std::string &get_ip_address() const {
        return ip_address;
    }

    void set_ip_address(const std::string &ip_address) {
        MachineAppsInfoModel::ip_address = ip_address;
    }

    int get_app_id() const {
        return app_id;
    }

    void set_app_id(int app_id) {
        MachineAppsInfoModel::app_id = app_id;
    }

    int get_version_id() const {
        return version_id;
    }

    void set_version_id(int version_id) {
        MachineAppsInfoModel::version_id = version_id;
    }

    const std::string &get_version() const {
        return version;
    }

    void set_version(const std::string &version) {
        MachineAppsInfoModel::version = version;
    }

    const std::string &get_name() const {
        return name;
    }

    void set_name(const std::string &name) {
        MachineAppsInfoModel::name = name;
    }

    std::vector<publish_app_cfg_ports_model_ptr> &get_cfg_ports() {
        return cfg_ports;
    }

    void set_cfg_ports(const std::vector<publish_app_cfg_ports_model_ptr> &cfg_ports) {
        MachineAppsInfoModel::cfg_ports = cfg_ports;
    }

    std::vector<publish_app_cfg_volumes_model_ptr> &get_cfg_volumes() {
        return cfg_volumes;
    }

    void set_cfg_volumes(const std::vector<publish_app_cfg_volumes_model_ptr> &cfg_volumes) {
        MachineAppsInfoModel::cfg_volumes = cfg_volumes;
    }

    std::vector<publish_app_cfg_dns_model_ptr> &get_cfg_dns() {
        return cfg_dns;
    }

    void set_cfg_dns(const std::vector<publish_app_cfg_dns_model_ptr> &cfg_dns) {
        MachineAppsInfoModel::cfg_dns = cfg_dns;
    }

    const publish_app_cfg_extra_cmd_model_ptr &get_cfg_extra_cmd() const {
        return cfg_extra_cmd;
    }

    void set_cfg_extra_cmd(const publish_app_cfg_extra_cmd_model_ptr &cfg_extra_cmd) {
        MachineAppsInfoModel::cfg_extra_cmd = cfg_extra_cmd;
    }

    std::vector<publish_app_hints_model_ptr> &get_hints() {
        return hints;
    }

    void set_hints(const std::vector<publish_app_hints_model_ptr> &hints) {
        MachineAppsInfoModel::hints = hints;
    }

    const std::string &get_runtime_name() const {
        return runtime_name;
    }

    void set_runtime_name(const std::string &runtime_name) {
        MachineAppsInfoModel::runtime_name = runtime_name;
    }

private:
    int machine_app_list_id;
    std::string ip_address;
    int app_id;
    int version_id;
    std::string version;
    std::string name;
    std::string runtime_name;
    std::vector<publish_app_hints_model_ptr> hints;
    std::vector<publish_app_cfg_ports_model_ptr> cfg_ports;
    std::vector<publish_app_cfg_volumes_model_ptr> cfg_volumes;
    std::vector<publish_app_cfg_dns_model_ptr> cfg_dns;
    publish_app_cfg_extra_cmd_model_ptr cfg_extra_cmd;
};
typedef std::shared_ptr<MachineAppsInfoModel> machine_apps_info_model_ptr;

class ApplicationModel {
public:
    ApplicationModel() {}

    int get_id() const {
        return id;
    }

    void set_id(int id) {
        ApplicationModel::id = id;
    }

    const std::string &get_source() const {
        return source;
    }

    void set_source(const std::string &source) {
        ApplicationModel::source = source;
    }

    const std::string &get_name() const {
        return name;
    }

    void set_name(const std::string &name) {
        ApplicationModel::name = name;
    }

    const std::string &get_description() const {
        return description;
    }

    void set_description(const std::string &description) {
        ApplicationModel::description = description;
    }

private:
    int id;
    std::string source;
    std::string name;
    std::string description;
};
typedef std::shared_ptr<ApplicationModel> application_model_ptr;

class AppVersionsModel {
public:
    AppVersionsModel() {}

    int get_id() const {
        return id;
    }

    void set_id(int id) {
        AppVersionsModel::id = id;
    }

    int get_app_id() const {
        return app_id;
    }

    void set_app_id(int app_id) {
        AppVersionsModel::app_id = app_id;
    }

    const std::string &get_version() const {
        return version;
    }

    void set_version(const std::string &version) {
        AppVersionsModel::version = version;
    }

    const std::string &get_registe_time() const {
        return registe_time;
    }

    void set_registe_time(const std::string &registe_time) {
        AppVersionsModel::registe_time = registe_time;
    }

    const std::string &get_description() const {
        return description;
    }

    void set_description(const std::string &description) {
        AppVersionsModel::description = description;
    }

private:
    int id;
    int app_id;
    std::string version;
    std::string registe_time;
    std::string description;
};
typedef std::shared_ptr<AppVersionsModel> app_versions_model_ptr;

class ModelMgrBase {
public:
    virtual ~ModelMgrBase() {}
    virtual std::vector<machine_apps_info_model_ptr> get_machine_apps_info() = 0;
    virtual std::vector<application_model_ptr> get_applications() = 0;
    virtual std::vector<app_versions_model_ptr> get_app_versions_by_app_id(int app_id) = 0;
    virtual void add_app(std::string app_name, std::string app_source, std::string app_desc,
                         std::string app_version, std::string app_version_desc,
                         int *app_id, int *version_id, std::string *registe_time) = 0;
    virtual void bind_app(std::string ip_address, int app_id, int version_id, std::string runtime_name,
                          std::vector<publish_app_cfg_ports_model_ptr> cfg_ports,
                          std::vector<publish_app_cfg_volumes_model_ptr> cfg_volumes,
                          std::vector<publish_app_cfg_dns_model_ptr> cfg_dns,
                          publish_app_cfg_extra_cmd_model_ptr cfg_extra_cmd,
                          std::vector<publish_app_hints_model_ptr> hints,
                          int*uniq_id) = 0;
    virtual void remove_version(int uniq_id) = 0;
    virtual void update_version(int uniq_id, int new_version_id, std::string new_runtime_name) = 0;
};

// todo(tianhuan)目前使用短连接的形式,后续考虑使用连接池
class ModelMgr:public ModelMgrBase {
public:
    ModelMgr() {
        mysql_address = config_mgr.get_item("controller_mysql_address")->get_str();
        mysql_user = config_mgr.get_item("controller_mysql_user")->get_str();
        mysql_password = config_mgr.get_item("controller_mysql_password")->get_str();
        mysql_schema = config_mgr.get_item("controller_mysql_schema")->get_str();
    }

    ~ModelMgr() {}

    ModelMgr(std::string mysql_address_,
             std::string mysql_user_,
             std::string mysql_password_,
             std::string mysql_schema_):
                mysql_address(mysql_address_),
                mysql_user(mysql_user_),
                mysql_password(mysql_password_),
                mysql_schema(mysql_schema_) {}

    std::vector<machine_apps_info_model_ptr> get_machine_apps_info();
    std::vector<application_model_ptr> get_applications();
    std::vector<app_versions_model_ptr> get_app_versions_by_app_id(int app_id);
    void add_app(std::string app_name, std::string app_source, std::string app_desc,
                 std::string app_version, std::string app_version_desc,
                 int *app_id, int *version_id, std::string *registe_time);
    void bind_app(std::string ip_address, int app_id, int version_id, std::string runtime_name,
                  std::vector<publish_app_cfg_ports_model_ptr> cfg_ports,
                  std::vector<publish_app_cfg_volumes_model_ptr> cfg_volumes,
                  std::vector<publish_app_cfg_dns_model_ptr> cfg_dns,
                  publish_app_cfg_extra_cmd_model_ptr cfg_extra_cmd,
                  std::vector<publish_app_hints_model_ptr> hints,
                  int*uniq_id);
    void remove_version(int uniq_id);
    void update_version(int uniq_id, int new_version_id, std::string new_runtime_name);

private:
    std::string mysql_address;
    std::string mysql_user;
    std::string mysql_password;
    std::string mysql_schema;

    sql::Connection *get_conn();
    void close_conn(sql::Connection *conn);
};

#endif //OGP_MODEL_MODEL_H
