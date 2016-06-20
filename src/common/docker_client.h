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

class DockerClientBase {
public:
    virtual ~DockerClientBase() {};
    virtual void set_host(std::string host) = 0;
    virtual nlohmann::json list_containers(int all=0) = 0;
    virtual nlohmann::json create_container(nlohmann::json parameters, std::string name) = 0;
    virtual void start_container(std::string container_id) = 0;
    virtual void stop_container(std::string container_id) = 0;
    virtual void kill_container(std::string container_id) = 0;
    virtual void remove_container(std::string container_id) = 0;
    virtual std::string get_container_id_by_name(std::string name) = 0;
    virtual bool ping_docker() = 0;

};

class DockerClient: public DockerClientBase {
public:
    DockerClient() {};
    ~DockerClient() {};
    void set_host(std::string host_) {host = host_;}
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
