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

private:
    int machine_app_list_id;
    std::string ip_address;
    int app_id;
    int version_id;
    std::string version;
    std::string name;
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

// todo(tianhuan)目前使用短连接的形式,后续考虑使用连接池
class ModelMgr {
public:
    ModelMgr() {
        mysql_address = config_mgr.get_item("controller_mysql_address")->get_str();
        mysql_user = config_mgr.get_item("controller_mysql_user")->get_str();
        mysql_password = config_mgr.get_item("controller_mysql_password")->get_str();
        mysql_schema = config_mgr.get_item("controller_mysql_schema")->get_str();
    }

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
    app_versions_model_ptr get_app_versions_by_app_id(int app_id);
    void add_app(std::string app_name, std::string app_source, std::string app_desc,
                 std::string app_version, std::string app_version_desc,
                 int *app_id, int *version_id, std::string *registe_time);

private:
    std::string mysql_address;
    std::string mysql_user;
    std::string mysql_password;
    std::string mysql_schema;

    sql::Connection *get_conn();
    void close_conn(sql::Connection *conn);
};

#endif //OGP_MODEL_MODEL_H
