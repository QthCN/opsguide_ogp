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
                            conn->setAutoCommit(0);\
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
    if (*p != nullptr) {
        delete *p;
        *p = nullptr;
    }
}

sql::Connection *ModelMgr::get_conn() {
    try {
        auto driver = get_driver_instance();
        auto conn = driver->connect(mysql_address, mysql_user, mysql_password);
        conn->setSchema(mysql_schema);
        return conn;
    } catch (sql::SQLException &e) {
        LOG_ERROR("" << e.what())
        throw e;
    }

}

void ModelMgr::close_conn(sql::Connection *conn) {
    if (conn == nullptr) {
        return;
    }
    conn->commit();
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
                                         "MACHINE_APP_LIST.runtime_name AS runtime_name,"
                                         "APP_VERSIONS.version,"
                                         "APP_LIST.name AS app_name "
                                         "FROM MACHINE_APP_LIST, APP_VERSIONS, APP_LIST "
                                         "WHERE MACHINE_APP_LIST.version_id=APP_VERSIONS.id "
                                         "AND MACHINE_APP_LIST.app_id=APP_LIST.id "
                                         "ORDER BY MACHINE_APP_LIST.id DESC");
        std::vector<machine_apps_info_model_ptr> result;
        while (res->next()) {
            auto record = std::make_shared<MachineAppsInfoModel>();
            record->set_machine_app_list_id(res->getInt("machine_app_list_id"));
            record->set_ip_address(res->getString("ip_address"));
            record->set_app_id(res->getInt("app_id"));
            record->set_version_id(res->getInt("version_id"));
            record->set_version(res->getString("version"));
            record->set_name(res->getString("app_name"));
            record->set_runtime_name(res->getString("runtime_name"));
            result.push_back(record);
        }

        delete_ptr(&res);

        for (auto &record: result) {
            auto uniq_id = record->get_machine_app_list_id();

            // cfg_ports
            pstmt = conn->prepareStatement("SELECT private_port, public_port, type "
                                                   "FROM PUBLISH_APP_CFG_PORTS "
                                                   "WHERE uniq_id=?");
            pstmt->setInt(1, uniq_id);
            res = pstmt->executeQuery();
            while (res->next()) {
                auto cfg_port = std::make_shared<PublishAppCfgPortsModel>();
                cfg_port->set_uniq_id(uniq_id);
                cfg_port->set_private_port(res->getInt("private_port"));
                cfg_port->set_public_port(res->getInt("public_port"));
                cfg_port->set_type(res->getString("type"));
                record->get_cfg_ports().push_back(cfg_port);
            }
            delete_ptr(&res);
            delete_ptr(&pstmt);

            // cfg_volumes
            pstmt = conn->prepareStatement("SELECT docker_volume, host_volume "
                                                   "FROM PUBLISH_APP_CFG_VOLUMES "
                                                   "WHERE uniq_id=?");
            pstmt->setInt(1, uniq_id);
            res = pstmt->executeQuery();
            while (res->next()) {
                auto cfg_volume = std::make_shared<PublishAppCfgVolumesModel>();
                cfg_volume->set_uniq_id(uniq_id);
                cfg_volume->set_docker_volume(res->getString("docker_volume"));
                cfg_volume->set_host_volume(res->getString("host_volume"));
                record->get_cfg_volumes().push_back(cfg_volume);
            }
            delete_ptr(&res);
            delete_ptr(&pstmt);

            // cfg_dns
            pstmt = conn->prepareStatement("SELECT dns, address "
                                                   "FROM PUBLISH_APP_CFG_DNS "
                                                   "WHERE uniq_id=?");
            pstmt->setInt(1, uniq_id);
            res = pstmt->executeQuery();
            while (res->next()) {
                auto cfg_dns = std::make_shared<PublishAppCfgDnsModel>();
                cfg_dns->set_uniq_id(uniq_id);
                cfg_dns->set_dns(res->getString("dns"));
                cfg_dns->set_address(res->getString("address"));
                record->get_cfg_dns().push_back(cfg_dns);
            }
            delete_ptr(&res);
            delete_ptr(&pstmt);

            // cfg_extra_cmd
            pstmt = conn->prepareStatement("SELECT extra_cmd "
                                                   "FROM PUBLISH_APP_CFG_EXTRA_CMD "
                                                   "WHERE uniq_id=?");
            pstmt->setInt(1, uniq_id);
            res = pstmt->executeQuery();
            while (res->next()) {
                auto cfg_extra_cmd = std::make_shared<PublishAppCfgExtraCmdModel>();
                cfg_extra_cmd->set_uniq_id(uniq_id);
                cfg_extra_cmd->set_extra_cmd(res->getString("extra_cmd"));
                record->set_cfg_extra_cmd(cfg_extra_cmd);
            }
            delete_ptr(&res);
            delete_ptr(&pstmt);

            // hints
            pstmt = conn->prepareStatement("SELECT item, value "
                                                   "FROM PUBLISH_APP_HINTS "
                                                   "WHERE uniq_id=?");
            pstmt->setInt(1, uniq_id);
            res = pstmt->executeQuery();
            while (res->next()) {
                auto hint = std::make_shared<PublishAppHintsModel>();
                hint->set_item(res->getString("item"));
                hint->set_value(res->getString("value"));
                record->get_hints().push_back(hint);
            }
        }

        SQLQUERY_CLEAR_RESOURCE

        return result;

    SQLQUERY_END
}

std::vector<application_model_ptr> ModelMgr::get_applications() {
    SQLQUERY_BEGIN

        stmt = conn->createStatement();
        res = stmt->executeQuery("SELECT id, source, name, description "
                                         "FROM APP_LIST "
                                         "ORDER BY id DESC");
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

std::vector<app_versions_model_ptr> ModelMgr::get_app_versions_by_app_id(int app_id) {
    SQLQUERY_BEGIN

        pstmt = conn->prepareStatement("SELECT id, app_id, version, registe_time, description "
                                               "FROM APP_VERSIONS WHERE app_id=? "
                                               "ORDER BY id DESC");
        pstmt->setInt(1, app_id);
        res = pstmt->executeQuery();
        std::vector<app_versions_model_ptr> result;
        while (res->next()) {
            auto record = std::make_shared<AppVersionsModel>();
            record->set_id(res->getInt("id"));
            record->set_app_id(res->getInt("app_id"));
            record->set_version(res->getString("version"));
            record->set_registe_time(res->getString("registe_time"));
            record->set_description(res->getString("description"));
            result.push_back(record);
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
            delete_ptr(&pstmt);
            delete_ptr(&res);
            pstmt = conn->prepareStatement("SELECT id FROM APP_LIST WHERE name=? ");
            pstmt->setString(1, app_name);
            res = pstmt->executeQuery();

            if (res->next() == false) {
                delete_ptr(&pstmt);
                delete_ptr(&res);
                pstmt = conn->prepareStatement("INSERT INTO APP_LIST(source, name, description) VALUES (?, ?, ?)");
                pstmt->setString(1, app_source);
                pstmt->setString(2, app_name);
                pstmt->setString(3, app_desc);
                pstmt->execute();
                delete_ptr(&pstmt);
            }
            pstmt = conn->prepareStatement("SELECT id FROM APP_LIST WHERE name=? ");
            pstmt->setString(1, app_name);
            res = pstmt->executeQuery();
            res->next();
            *app_id = res->getInt("id");
            delete_ptr(&pstmt);
            delete_ptr(&res);

            pstmt = conn->prepareStatement("INSERT INTO APP_VERSIONS(app_id, version, registe_time, description) "
                                                   "VALUES (?, ?, NOW(), ?)");
            pstmt->setInt(1, *app_id);
            pstmt->setString(2, app_version);
            pstmt->setString(3, app_version_desc);
            pstmt->execute();
            delete_ptr(&pstmt);

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

void ModelMgr::bind_app(std::string ip_address, int app_id, int version_id, std::string runtime_name,
                        std::vector<publish_app_cfg_ports_model_ptr> cfg_ports,
                        std::vector<publish_app_cfg_volumes_model_ptr> cfg_volumes,
                        std::vector<publish_app_cfg_dns_model_ptr> cfg_dns,
                        publish_app_cfg_extra_cmd_model_ptr cfg_extra_cmd,
                        std::vector<publish_app_hints_model_ptr> hints, int *uniq_id) {
    SQLQUERY_BEGIN

        stmt = conn->createStatement();
        stmt->execute("LOCK TABLES MACHINE_APP_LIST WRITE, PUBLISH_APP_HINTS WRITE, "
                              "PUBLISH_APP_CFG_PORTS WRITE, "
                              "PUBLISH_APP_CFG_VOLUMES WRITE, "
                              "PUBLISH_APP_CFG_DNS WRITE, "
                              "PUBLISH_APP_CFG_EXTRA_CMD WRITE");

        pstmt = conn->prepareStatement("INSERT INTO MACHINE_APP_LIST(ip_address, app_id, version_id, runtime_name) "
                                               "VALUES (?, ?, ?, ?)");
        pstmt->setString(1, ip_address);
        pstmt->setInt(2, app_id);
        pstmt->setInt(3, version_id);
        pstmt->setString(4, runtime_name);
        pstmt->execute();
        delete_ptr(&pstmt);
        pstmt = conn->prepareStatement("SELECT LAST_INSERT_ID() AS id");
        res = pstmt->executeQuery();
        res->next();
        *uniq_id = res->getInt("id");
        delete_ptr(&pstmt);
        delete_ptr(&res);

        // cfg_ports
        for (auto &cfg_port: cfg_ports) {
            pstmt = conn->prepareStatement("INSERT INTO PUBLISH_APP_CFG_PORTS(uniq_id, private_port, public_port, "
                                                   "type) VALUES (?, ?, ?, ?)");
            pstmt->setInt(1, *uniq_id);
            pstmt->setInt(2, cfg_port->get_private_port());
            pstmt->setInt(3, cfg_port->get_public_port());
            pstmt->setString(4, cfg_port->get_type());
            pstmt->execute();
            delete_ptr(&pstmt);
        }

        // cfg_volumes
        for (auto &cfg_volume: cfg_volumes) {
            pstmt = conn->prepareStatement("INSERT INTO PUBLISH_APP_CFG_VOLUMES(uniq_id, docker_volume, host_volume) "
                                                   "VALUES (?, ?, ?)");
            pstmt->setInt(1, *uniq_id);
            pstmt->setString(2, cfg_volume->get_docker_volume());
            pstmt->setString(3, cfg_volume->get_host_volume());
            pstmt->execute();
            delete_ptr(&pstmt);
        }

        // cfg_dns
        for (auto &cfg_dns_: cfg_dns) {
            pstmt = conn->prepareStatement("INSERT INTO PUBLISH_APP_CFG_DNS(uniq_id, dns, address) "
                                                   "VALUES (?, ?, ?)");
            pstmt->setInt(1, *uniq_id);
            pstmt->setString(2, cfg_dns_->get_dns());
            pstmt->setString(3, cfg_dns_->get_address());
            pstmt->execute();
            delete_ptr(&pstmt);
        }

        // cfg_extra_cmd
        if (cfg_extra_cmd != nullptr) {
            pstmt = conn->prepareStatement("INSERT INTO PUBLISH_APP_CFG_EXTRA_CMD(uniq_id, extra_cmd) "
                                                   "VALUES(?, ?)");
            pstmt->setInt(1, *uniq_id);
            pstmt->setString(2, cfg_extra_cmd->get_extra_cmd());
            pstmt->execute();
            delete_ptr(&pstmt);
        }

        // hints
        for (auto &hint: hints) {
            pstmt = conn->prepareStatement("INSERT INTO PUBLISH_APP_HINTS(uniq_id, item, value) "
                                                   "VALUES(?, ?, ?)");
            pstmt->setInt(1, *uniq_id);
            pstmt->setString(2, hint->get_item());
            pstmt->setString(3, hint->get_value());
            pstmt->execute();
            delete_ptr(&pstmt);
        }

        stmt->execute("UNLOCK TABLES");
        SQLQUERY_CLEAR_RESOURCE

    SQLQUERY_END
}

void ModelMgr::remove_version(int uniq_id) {
    SQLQUERY_BEGIN

        pstmt = conn->prepareStatement("DELETE FROM MACHINE_APP_LIST WHERE id=?");
        pstmt->setInt(1, uniq_id);
        pstmt->execute();
        delete_ptr(&pstmt);

        pstmt = conn->prepareStatement("DELETE FROM PUBLISH_APP_CFG_DNS WHERE uniq_id=?");
        pstmt->setInt(1, uniq_id);
        pstmt->execute();
        delete_ptr(&pstmt);

        pstmt = conn->prepareStatement("DELETE FROM PUBLISH_APP_CFG_EXTRA_CMD WHERE uniq_id=?");
        pstmt->setInt(1, uniq_id);
        pstmt->execute();
        delete_ptr(&pstmt);

        pstmt = conn->prepareStatement("DELETE FROM PUBLISH_APP_CFG_PORTS WHERE uniq_id=?");
        pstmt->setInt(1, uniq_id);
        pstmt->execute();
        delete_ptr(&pstmt);

        pstmt = conn->prepareStatement("DELETE FROM PUBLISH_APP_CFG_VOLUMES WHERE uniq_id=?");
        pstmt->setInt(1, uniq_id);
        pstmt->execute();
        delete_ptr(&pstmt);

        pstmt = conn->prepareStatement("DELETE FROM PUBLISH_APP_HINTS WHERE uniq_id=?");
        pstmt->setInt(1, uniq_id);
        pstmt->execute();
        SQLQUERY_CLEAR_RESOURCE

    SQLQUERY_END
}

void ModelMgr::update_version(int uniq_id, int new_version_id, std::string new_runtime_name) {
    SQLQUERY_BEGIN

        pstmt = conn->prepareStatement("UPDATE MACHINE_APP_LIST SET version_id=?, "
                                               "runtime_name=? WHERE id=?");
        pstmt->setInt(1, new_version_id);
        pstmt->setString(2, new_runtime_name);
        pstmt->setInt(3, uniq_id);
        pstmt->execute();
        SQLQUERY_CLEAR_RESOURCE

    SQLQUERY_END
}

std::vector<service_model_ptr> ModelMgr::list_services() {
    SQLQUERY_BEGIN

        stmt = conn->createStatement();
        res = stmt->executeQuery("SELECT id, service_type, app_id FROM SERVICES");
        std::vector<service_model_ptr> services;
        while (res->next()) {
            auto service_model = std::make_shared<ServiceModel>();
            service_model->set_service_type(res->getString("service_type"));
            service_model->set_id(res->getInt("id"));
            service_model->set_app_id(res->getInt("app_id"));
            services.push_back(service_model);
        }
        delete_ptr(&res);

        for(auto s: services) {
            auto service_id = s->get_id();
            pstmt = conn->prepareStatement("SELECT service_port, private_port FROM SERVICE_PORTSERVICE "
                                                   "WHERE service_id=?");
            pstmt->setInt(1, service_id);
            res = pstmt->executeQuery();
            while (res->next()) {
                if (res->getInt("service_port") == -1) {
                    s->set_private_port(-1);
                } else {
                    s->set_private_port(res->getInt("private_port"));
                }
                s->set_service_port(res->getInt("service_port"));
            }
        }
        SQLQUERY_CLEAR_RESOURCE
        return services;
    SQLQUERY_END
}

void ModelMgr::remove_service(int service_id) {
    SQLQUERY_BEGIN

        pstmt = conn->prepareStatement("DELETE FROM SERVICE_PORTSERVICE WHERE service_id=?");
        pstmt->setInt(1, service_id);
        pstmt->execute();
        delete_ptr(&pstmt);
        pstmt = conn->prepareStatement("DELETE FROM SERVICES WHERE id=?");
        pstmt->setInt(1, service_id);
        pstmt->execute();
        SQLQUERY_CLEAR_RESOURCE

    SQLQUERY_END
}

void ModelMgr::add_port_service(std::string service_type, int app_id, int service_port, int private_port,
                                int *service_id) {
    SQLQUERY_BEGIN

        stmt = conn->createStatement();
        stmt->execute("LOCK TABLES SERVICES WRITE, "
                              "SERVICE_PORTSERVICE WRITE");

        pstmt = conn->prepareStatement("INSERT INTO SERVICES(service_type, app_id) VALUES(?, ?)");
        pstmt->setString(1, service_type);
        pstmt->setInt(2, app_id);
        pstmt->execute();
        delete_ptr(&pstmt);

        pstmt = conn->prepareStatement("SELECT LAST_INSERT_ID() AS id");
        res = pstmt->executeQuery();
        res->next();
        *service_id = res->getInt("id");
        delete_ptr(&pstmt);
        delete_ptr(&res);

        pstmt = conn->prepareStatement("INSERT INTO SERVICE_PORTSERVICE(service_id, service_port, private_port) "
                                               "VALUES(?, ?, ?)");
        pstmt->setInt(1, *service_id);
        pstmt->setInt(2, service_port);
        pstmt->setInt(3, private_port);
        pstmt->execute();

        stmt->execute("UNLOCK TABLES");
        SQLQUERY_CLEAR_RESOURCE
    SQLQUERY_END
}
