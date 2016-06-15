//
// Created by thuanqin on 16/5/27.
//

#ifndef OGP_COMMON_DOCKER_CLIENT_H
#define OGP_COMMON_DOCKER_CLIENT_H

#include <string>
#include <map>
#include <vector>

#include "restclient-cpp/restclient.h"

#include "common/log.h"
#include "third/json/json.hpp"

class DockerClient {
public:
    DockerClient(const std::string &host): host(host) { }
    nlohmann::json list_containers(int all=0);
    nlohmann::json create_container(nlohmann::json parameters, std::string name);
    void start_container(std::string container_id);
    void stop_container(std::string container_id);
    void kill_container(std::string container_id);
    void remove_container(std::string container_id);
    std::string get_container_id_by_name(std::string name);
    bool ping_docker();

private:
    std::string host;
};

#endif //OGP_COMMON_DOCKER_CLIENT_H
