//
// Created by thuanqin on 16/6/1.
//
#include "model/model.h"

#include "mysql_connection.h"
#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"

#include "common/log.h"

#define SQLQUERY_BEGIN try {\
                                auto conn = get_conn();\
                                sql::Statement *stmt = nullptr;\
                                sql::ResultSet *res = nullptr;

#define SQLQUERY_CLEAR_RESOURCE delete stmt;\
                                delete res;\
                                close_conn(conn);

#define SQLQUERY_END } catch (sql::SQLException &e) {\
                     LOG_ERROR("SQLException: " << e.what() << " MySQL error code: " << e.getErrorCode() << " SQLState: " << e.getSQLState())\
                     throw std::runtime_error("SQLException");\
                }

sql::Connection *ModelMgr::get_conn() {
    auto driver = get_driver_instance();
    auto conn = driver->connect(mysql_address, mysql_user, mysql_password);
    conn->setSchema(mysql_schema);
    return conn;
}

void ModelMgr::close_conn(sql::Connection *conn) {
    if (conn == nullptr) {
        return;
    }
    delete conn;
}

std::vector<machine_apps_info_model_ptr> ModelMgr::get_machine_apps_info() {
    SQLQUERY_BEGIN

        stmt = conn->createStatement();
        res = stmt->executeQuery("SELECT MACHINE_APP_LIST.id AS machine_app_list_id,"
                                         "MACHINE_APP_LIST.ip_address,"
                                         "MACHINE_APP_LIST.app_id AS app_id,"
                                         "MACHINE_APP_LIST.version_id AS version_id,"
                                         "APP_VERSIONS.version,"
                                         "APP_LIST.name AS app_name "
                                         "FROM MACHINE_APP_LIST, APP_VERSIONS, APP_LIST "
                                         "WHERE MACHINE_APP_LIST.version_id=APP_VERSIONS.id "
                                         "AND MACHINE_APP_LIST.app_id=APP_LIST.id");
        std::vector<machine_apps_info_model_ptr> result;
        while (res->next()) {
            auto record = std::make_shared<MachineAppsInfoModel>();
            record->set_machine_app_list_id(res->getInt("machine_app_list_id"));
            record->set_ip_address(res->getString("ip_address"));
            record->set_app_id(res->getInt("app_id"));
            record->set_version_id(res->getInt("version_id"));
            record->set_version(res->getString("version"));
            record->set_name(res->getString("app_name"));
            result.push_back(record);
        }

        SQLQUERY_CLEAR_RESOURCE

        return result;

    SQLQUERY_END
}

