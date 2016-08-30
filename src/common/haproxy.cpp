//
// Created by thuanqin on 16/7/21.
//
#include "common/haproxy.h"

#include <cstdlib>
#include <fstream>

#include "common/config.h"
#include "common/log.h"

std::string HAProxyHelper::generate_config_file(ogp_msg::ServiceSyncData &services_info) {
    std::string generated_config_file;
    // 写入全局信息
    generated_config_file += "global\n";
    generated_config_file += "  daemon\n";
    generated_config_file += "  nbproc 2\n";
    generated_config_file += "  pidfile ";
    generated_config_file += pid_file;
    generated_config_file += "\n";
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
    /*
     * todo(tianhuan), 参照yelp的文档,做到切换时不丢包
     * http://engineeringblog.yelp.com/2015/04/true-zero-downtime-haproxy-reloads.html
     * */

    std::string haproxy_bin = config_mgr.get_item("haproxy_bin")->get_str();
    std::string haproxy_pid_file = config_mgr.get_item("haproxy_pid_file")->get_str();
    std::string haproxy_cfg_file = config_mgr.get_item("haproxy_cfg_file")->get_str();
    std::string haproxy_restart_cmd = haproxy_bin + " -f " + haproxy_cfg_file + " -p " + haproxy_pid_file \
                                      + " -sf $(cat " + haproxy_pid_file + ")";
    LOG_INFO("execute cmd: " + haproxy_restart_cmd)
    auto rc = std::system(haproxy_restart_cmd.c_str());
    LOG_INFO("execute cmd rc: " + std::to_string(rc))
}

std::string HAProxyHelper::get_config_file_content() {
    std::ifstream cfg_file(config_file);
    std::string str;
    std::string cfg_file_content = "";
    while (std::getline(cfg_file, str)) {
        cfg_file_content += str;
        cfg_file_content += "\n";
    }
    return cfg_file_content;
}

void HAProxyHelper::update_config_file_content(const std::string &new_content) {
    LOG_INFO("writing content into: " + config_file)
    std::ofstream cf(config_file);
    if (!cf.good()) {
        LOG_ERROR("can not write content into: " + config_file)
        cf.close();
        return;
    }
    cf << new_content;
    cf.close();
}