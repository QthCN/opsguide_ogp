//
// Created by thuanqin on 16/7/21.
//

#ifndef OGP_COMMON_HAPROXY_H
#define OGP_COMMON_HAPROXY_H

#include "string"

#include "ogp_msg.pb.h"

class HAProxyHelper {
public:
    HAProxyHelper(std::string config_file): config_file(config_file) {}
    // 将当前的config_file中的内容和service_info做比较
    bool compare_config_file_with_services_info(ogp_msg::ServiceSyncData &services_info);
    // 根据service_info生成config_file的内容
    std::string generate_config_file(ogp_msg::ServiceSyncData &services_info);
    // 获取config_file的内容
    std::string get_config_file_content();
    // 重启haproxy服务,如果haproxy是dead的则直接启动
    void restart_haproxy_service();
private:
    std::string config_file;

};

#endif //OGP_COMMON_HAPROXY_H
