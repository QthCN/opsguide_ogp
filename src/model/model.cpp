//
// Created by thuanqin on 16/6/1.
//
#include "model/model.h"

#include "mysql_connection.h"
#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/resultset.h"
#include "cppconn/prepared_statement.h"
#include "cppconn/statement.h"

#include "common/log.h"

#define SQLQUERY_BEGIN      sql::Connection *conn = nullptr;\
                            conn = get_conn();\
                            sql::Statement *stmt = nullptr;\
                            sql::ResultSet *res = nullptr;\
                            sql::PreparedStatement *pstmt = nullptr;\
                            try {

#define SQLQUERY_CLEAR_RESOURCE if (stmt != nullptr) {delete stmt;}\
                                if (res != nullptr) {delete res;}\
                                if (pstmt != nullptr) {delete pstmt;}\
                                if (conn != nullptr) {close_conn(conn);}

#define SQLQUERY_END } catch (sql::SQLException &e) {\
                     LOG_ERROR("SQLException: " << e.what() << " MySQL error code: " << e.getErrorCode() << " SQLState: " << e.getSQLState())\
                     if (stmt != nullptr) {delete stmt;}\
                     if (res != nullptr) {delete res;}\
                     if (pstmt != nullptr) {delete pstmt;}\
                     if (conn != nullptr) {close_conn(conn);}\
                     throw e;}

template<typename PTR>
void delete_ptr(PTR p) {
    if (p != nullptr) {
        delete p;
        p = nullptr;
    }
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
    conn->close();
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

std::vector<application_model_ptr> ModelMgr::get_applications() {
    SQLQUERY_BEGIN

        stmt = conn->createStatement();
        res = stmt->executeQuery("SELECT id, source, name, description "
                                         "FROM APP_LIST");
        std::vector<application_model_ptr> result;
        while (res->next()) {
            auto record = std::make_shared<ApplicationModel>();
            record->set_id(res->getInt("id"));
            record->set_source(res->getString("source"));
            record->set_name(res->getString("name"));
            record->set_description(res->getString("description"));
            result.push_back(record);
        }

        SQLQUERY_CLEAR_RESOURCE

        return result;

    SQLQUERY_END
}

app_versions_model_ptr ModelMgr::get_app_versions_by_app_id(int app_id) {
    SQLQUERY_BEGIN

        pstmt = conn->prepareStatement("SELECT id, app_id, version, registe_time, description "
                                               "FROM APP_VERSIONS WHERE app_id=?");
        pstmt->setInt(1, app_id);
        res = pstmt->executeQuery();
        app_versions_model_ptr result = std::make_shared<AppVersionsModel>();
        while (res->next()) {
            result->set_id(res->getInt("id"));
            result->set_id(res->getInt("app_id"));
            result->set_version(res->getString("version"));
            result->set_registe_time(res->getString("registe_time"));
            result->set_description(res->getString("description"));
            break;
        }

        SQLQUERY_CLEAR_RESOURCE

        return result;

    SQLQUERY_END
}

void ModelMgr::add_app(std::string app_name, std::string app_source, std::string app_desc, std::string app_version,
                       std::string app_version_desc, int *app_id, int *version_id, std::string *registe_time) {
    SQLQUERY_BEGIN
        stmt = conn->createStatement();
        stmt->execute("LOCK TABLES APP_LIST WRITE, APP_VERSIONS WRITE");
        pstmt = conn->prepareStatement("SELECT APP_VERSIONS.id FROM APP_LIST, APP_VERSIONS "
                                               "WHERE APP_LIST.id=APP_VERSIONS.app_id "
                                               "AND APP_LIST.name=? "
                                               "AND APP_VERSIONS.version=?");
        pstmt->setString(1, app_name);
        pstmt->setString(2, app_version);
        res = pstmt->executeQuery();
        if (res->next()) {
            LOG_ERROR("application with name: " << app_name << " and version: " << app_version << " already exist.")
            stmt->execute("UNLOCK TABLES");
            SQLQUERY_CLEAR_RESOURCE
            throw std::runtime_error("application with name: " + app_name
                                     + " and version: " + app_version
                                     + " already exist.");
        } else {
            delete_ptr(pstmt);
            delete_ptr(res);
            pstmt = conn->prepareStatement("SELECT id FROM APP_LIST WHERE name=? ");
            pstmt->setString(1, app_name);
            res = pstmt->executeQuery();

            if (res->next() == false) {
                delete_ptr(pstmt);
                delete_ptr(res);
                pstmt = conn->prepareStatement("INSERT INTO APP_LIST(source, name, description) VALUES (?, ?, ?)");
                pstmt->setString(1, app_source);
                pstmt->setString(2, app_name);
                pstmt->setString(3, app_desc);
                pstmt->execute();
                delete_ptr(pstmt);
            }
            pstmt = conn->prepareStatement("SELECT id FROM APP_LIST WHERE name=? ");
            pstmt->setString(1, app_name);
            res = pstmt->executeQuery();
            res->next();
            *app_id = res->getInt("id");
            delete_ptr(pstmt);
            delete_ptr(res);

            pstmt = conn->prepareStatement("INSERT INTO APP_VERSIONS(app_id, version, registe_time, description) "
                                                   "VALUES (?, ?, NOW(), ?)");
            pstmt->setInt(1, *app_id);
            pstmt->setString(2, app_version);
            pstmt->setString(3, app_version_desc);
            pstmt->execute();
            delete_ptr(pstmt);

            pstmt = conn->prepareStatement("SELECT id, registe_time FROM APP_VERSIONS "
                                                   "WHERE app_id=? AND version=?");
            pstmt->setInt(1, *app_id);
            pstmt->setString(2, app_version);
            res = pstmt->executeQuery();
            res->next();
            *version_id = res->getInt("id");
            *registe_time = res->getString("registe_time");

            stmt->execute("UNLOCK TABLES");
            SQLQUERY_CLEAR_RESOURCE
        }

    SQLQUERY_END
}

