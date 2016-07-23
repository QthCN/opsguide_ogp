//
// Created by thuanqin on 16/7/21.
//
#include "common/haproxy.h"

bool HAProxyHelper::compare_config_file_with_services_info(ogp_msg::ServiceSyncData &services_info) {
    return true;
}

std::string HAProxyHelper::generate_config_file(ogp_msg::ServiceSyncData &services_info) {
    std::string generated_config_file;
    // 写入全局信息
    generated_config_file += "global\n";
    generated_config_file += "  daemon\n";
    generated_config_file += "  nbproc 2\n";
    generated_config_file += "  pidfile /var/run/haproxy.pid\n";
    generated_config_file += "\n";
    // 写入默认信息
    generated_config_file += "defaults\n";
    generated_config_file += "  mode http\n";
    generated_config_file += "  retries 2\n";
    generated_config_file += "  option redispatch\n";
    generated_config_file += "  option abortonclose\n";
    generated_config_file += "  maxconn 4096\n";
    generated_config_file += "  timeout connect 5000ms\n";
    generated_config_file += "  timeout client 30000ms\n";
    generated_config_file += "  timeout server 30000ms\n";
    generated_config_file += "  log 127.0.0.1 local0 err\n";
    generated_config_file += "\n";

    // 写入代理服务信息
    for (auto info: services_info.infos()) {
        auto app_name = info.app_name();
        generated_config_file += "listen ";
        generated_config_file += app_name;
        generated_config_file += "\n";

        auto service_port = info.service_port();
        generated_config_file += "  bind 0.0.0.0:";
        generated_config_file += std::to_string(service_port);
        generated_config_file += "\n";

        generated_config_file += "  mode tcp\n";

        int s_index = 0;
        for (auto item: info.items()) {
            auto ma_ip = item.ma_ip();
            auto public_port = item.public_port();
            s_index++;

            generated_config_file += "  server s";
            generated_config_file += std::to_string(s_index);
            generated_config_file += " ";
            generated_config_file += ma_ip;
            generated_config_file += ":";
            generated_config_file += std::to_string(public_port);
            generated_config_file += "  weight 5\n";
        }
        generated_config_file += "\n";

    }

    // 返回结果
    return generated_config_file;

}

void HAProxyHelper::restart_haproxy_service() {

}

std::string HAProxyHelper::get_config_file_content() {
    return "";
}
