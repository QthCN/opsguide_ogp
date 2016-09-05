//
// Created by thuanqin on 16/6/17.
//

#ifndef OGP_TESTS_MODEL_TEST_H
#define OGP_TESTS_MODEL_TEST_H

#include "gmock/gmock.h"

#include "model/model.h"

class MockModelMgr: public ModelMgrBase {
public:
    MOCK_METHOD0(get_machine_apps_info, std::vector<machine_apps_info_model_ptr>());
    MOCK_METHOD0(get_applications, std::vector<application_model_ptr>());
    MOCK_METHOD1(get_app_versions_by_app_id, std::vector<app_versions_model_ptr>(int app_id));
    MOCK_METHOD8(add_app, void(std::string app_name,
                                std::string app_source,
                                std::string app_desc,
                                std::string app_version,
                                std::string app_version_desc,
                                int *app_id,
                                int *version_id,
                                std::string *registe_time));
    MOCK_METHOD10(bind_app, void(std::string ip_address,
                                int app_id,
                                int version_id,
                                std::string runtime_name,
                                std::vector<publish_app_cfg_ports_model_ptr> cfg_ports,
                                std::vector<publish_app_cfg_volumes_model_ptr> cfg_volumes,
                                std::vector<publish_app_cfg_dns_model_ptr> cfg_dns,
                                publish_app_cfg_extra_cmd_model_ptr cfg_extra_cmd,
                                std::vector<publish_app_hints_model_ptr> hints,
                                int*uniq_id));
    MOCK_METHOD1(remove_version, void(int uniq_id));
    MOCK_METHOD3(update_version, void((int uniq_id, int new_version_id, std::string new_runtime_name)));
    MOCK_METHOD5(add_port_service, void(std::string service_type, int app_id, int service_port, int private_port, int *service_id));
    MOCK_METHOD1(remove_service, void(int service_id));
    MOCK_METHOD0(list_services, std::vector<service_model_ptr>());
    MOCK_METHOD0(get_app_cfgs, std::vector<appcfg_model_ptr>());
    MOCK_METHOD3(update_appcfg, void(int app_id, std::string path, std::string content));
};

#endif //OGP_TESTS_MODEL_TEST_H
