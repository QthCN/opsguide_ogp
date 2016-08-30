//
// Created by thuanqin on 16/7/23.
//

#include "string"

#include "gmock/gmock.h"

#include "common/haproxy.h"
#include "controller/service.h"
#include "ogp_msg.pb.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::SaveArg;
using ::testing::Return;

TEST(HAProxyHelperTest, test_generate_config_files) {
    auto haproxy = HAProxyHelper("test.cfg", "", "/var/run/haproxy.pid");
    ogp_msg::ServiceSyncData services_info;

    services_info.set_uniq_id(100);
    for (int i = 0; i<4; i++) {
        auto service_id = i;
        auto app_id = 100+i;
        auto service_port = 2000 + i;
        std::string service_type = ST_PORT_SERVICE;
        auto app_name = "app_name_" + std::to_string(app_id);
        auto info = services_info.add_infos();
        info->set_service_id(service_id);
        info->set_app_id(app_id);
        info->set_app_name(app_name);
        info->set_service_port(service_port);
        info->set_service_type(service_type);

        for (int j = 0; j<4; j++) {
            std::string ma_ip = "172.0.0." + std::to_string(j);
            int public_port = 3000 + j;
            auto item = info->add_items();
            item->set_ma_ip(ma_ip);
            item->set_public_port(public_port);
        }
    }

    auto generated_config_file = haproxy.generate_config_file(services_info);

    std::string target_config_file_content;
    target_config_file_content += "global\n";
    target_config_file_content += "  daemon\n";
    target_config_file_content += "  nbproc 2\n";
    target_config_file_content += "  pidfile /var/run/haproxy.pid\n";
    target_config_file_content += "\n";
    target_config_file_content += "defaults\n";
    target_config_file_content += "  mode http\n";
    target_config_file_content += "  retries 2\n";
    target_config_file_content += "  option redispatch\n";
    target_config_file_content += "  option abortonclose\n";
    target_config_file_content += "  maxconn 4096\n";
    target_config_file_content += "  timeout connect 5000000ms\n";
    target_config_file_content += "  timeout client 30000000ms\n";
    target_config_file_content += "  timeout server 30000000ms\n";
    target_config_file_content += "  log 127.0.0.1 local0 err\n";
    target_config_file_content += "\n";
    target_config_file_content += "listen app_name_100\n";
    target_config_file_content += "  bind 0.0.0.0:2000\n";
    target_config_file_content += "  mode tcp\n";
    target_config_file_content += "  server s1 172.0.0.0:3000  weight 5\n";
    target_config_file_content += "  server s2 172.0.0.1:3001  weight 5\n";
    target_config_file_content += "  server s3 172.0.0.2:3002  weight 5\n";
    target_config_file_content += "  server s4 172.0.0.3:3003  weight 5\n";
    target_config_file_content += "\n";
    target_config_file_content += "listen app_name_101\n";
    target_config_file_content += "  bind 0.0.0.0:2001\n";
    target_config_file_content += "  mode tcp\n";
    target_config_file_content += "  server s1 172.0.0.0:3000  weight 5\n";
    target_config_file_content += "  server s2 172.0.0.1:3001  weight 5\n";
    target_config_file_content += "  server s3 172.0.0.2:3002  weight 5\n";
    target_config_file_content += "  server s4 172.0.0.3:3003  weight 5\n";
    target_config_file_content += "\n";
    target_config_file_content += "listen app_name_102\n";
    target_config_file_content += "  bind 0.0.0.0:2002\n";
    target_config_file_content += "  mode tcp\n";
    target_config_file_content += "  server s1 172.0.0.0:3000  weight 5\n";
    target_config_file_content += "  server s2 172.0.0.1:3001  weight 5\n";
    target_config_file_content += "  server s3 172.0.0.2:3002  weight 5\n";
    target_config_file_content += "  server s4 172.0.0.3:3003  weight 5\n";
    target_config_file_content += "\n";
    target_config_file_content += "listen app_name_103\n";
    target_config_file_content += "  bind 0.0.0.0:2003\n";
    target_config_file_content += "  mode tcp\n";
    target_config_file_content += "  server s1 172.0.0.0:3000  weight 5\n";
    target_config_file_content += "  server s2 172.0.0.1:3001  weight 5\n";
    target_config_file_content += "  server s3 172.0.0.2:3002  weight 5\n";
    target_config_file_content += "  server s4 172.0.0.3:3003  weight 5\n";
    target_config_file_content += "\n";

    ASSERT_EQ(target_config_file_content, generated_config_file);
}


