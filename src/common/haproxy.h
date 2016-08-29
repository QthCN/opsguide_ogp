//
// Created by thuanqin on 16/7/21.
//

#ifndef OGP_COMMON_HAPROXY_H
#define OGP_COMMON_HAPROXY_H

#include "string"

#include <fstream>

#include "ogp_msg.pb.h"

class HAProxyHelper {
public:
    HAProxyHelper(std::string config_file, std::string haproxy_bin, std::string pid_file):
            config_file(config_file),
            haproxy_bin(haproxy_bin),
            pid_file(pid_file){
        // 确保pid文件存在
        std::ofstream pidf(pid_file, std::fstream::app);
        pidf << "";
        pidf.close();
    }
    // 根据service_info生成config_file的内容
    std::string generate_config_file(ogp_msg::ServiceSyncData &services_info);
    // 获取config_file的内容
    std::string get_config_file_content();
    // 更新config_file的内容
    void update_config_file_content(const std::string &new_content);
    // 重启haproxy服务,如果haproxy是dead的则直接启动
    void restart_haproxy_service();
private:
    std::string config_file;
    std::string haproxy_bin;
    std::string pid_file;

};

#endif //OGP_COMMON_HAPROXY_H
