//
// Created by thuanqin on 16/5/27.
//
#include <restclient-cpp/restclient.h>
#include "common/docker_client.h"

#include "restclient-cpp/restclient.h"

#include "common/log.h"
#include "third/json/json.hpp"

// based on https://docs.docker.com/engine/reference/api/docker_remote_api_v1.19

using json = nlohmann::json;

json DockerClient::list_containers(int all) {
    auto subpath = "/containers/json";
    auto path = "http://" + host + subpath;
    if (all==1) {
        path = path + "?all=1";
    }

    RestClient::Response r = RestClient::get(path);
    if (r.code != 200) {
        LOG_ERROR("err code: " << r.code << " err body: " << r.body)
        throw std::runtime_error("list_containers error: " + r.body);
    }
    json result = json::parse(r.body);
    return result;
}

json DockerClient::create_container(json parameters, std::string name) {
    std::string subpath = "/containers/create?name=/" + name;
    auto path = "http://" + host + subpath;
    std::string request_body = parameters.dump(4);
    RestClient::Response r = RestClient::post(path, "application/json", request_body);
    if (r.code != 201) {
        LOG_ERROR("err code: " << r.code << " err body: " << r.body)
        throw std::runtime_error("create_container error: " + r.body);
    }
    json result = json::parse(r.body);
    return result;
}

std::string DockerClient::get_container_id_by_name(std::string name) {
    name = "/" + name;
    auto containers_info = list_containers(1);
    for (auto &c: containers_info) {
        for (auto it=c["Names"].begin(); it!=c["Names"].end(); it++) {
            if (it.value() == name) {
                return c["Id"];
            }
        }
    }
    return "";
}

void DockerClient::start_container(std::string container_id) {
    auto subpath01 = "/containers/";
    auto subpath02 = "/start";
    auto path = "http://" + host + subpath01 + container_id + subpath02;
    RestClient::Response r = RestClient::post(path, "application/json", "");
    if (r.code != 204) {
        LOG_ERROR("err code: " << r.code << " err body: " << r.body)
        throw std::runtime_error("start_container error: " + r.body);
    }
}

void DockerClient::stop_container(std::string container_id) {
    auto subpath01 = "/containers/";
    auto subpath02 = "/stop";
    auto path = "http://" + host + subpath01 + container_id + subpath02;
    RestClient::Response r = RestClient::post(path, "application/json", "");
    if (r.code != 204) {
        LOG_ERROR("err code: " << r.code << " err body: " << r.body)
        throw std::runtime_error("stop_container error: " + r.body);
    }
}

void DockerClient::kill_container(std::string container_id) {
    auto subpath01 = "/containers/";
    auto subpath02 = "/kill";
    auto path = "http://" + host + subpath01 + container_id + subpath02;
    RestClient::Response r = RestClient::post(path, "application/json", "");
    if (r.code != 204) {
        LOG_ERROR("err code: " << r.code << " err body: " << r.body)
        throw std::runtime_error("kill_container error: " + r.body);
    }
}

void DockerClient::remove_container(std::string container_id) {
    auto subpath = "/containers/";
    auto path = "http://" + host + subpath + container_id;
    RestClient::Response r = RestClient::del(path);
    if (r.code != 204) {
        LOG_ERROR("err code: " << r.code << " err body: " << r.body)
        throw std::runtime_error("remove_container error: " + r.body);
    }
}

bool DockerClient::ping_docker() {
    auto subpath = "/_ping";
    auto path = "http://" + host + subpath;
    RestClient::Response r = RestClient::get(path);
    if (r.code != 200) {
        LOG_ERROR("err code: " << r.code << " err body: " << r.body)
        throw std::runtime_error("list_containers error: " + r.body);
    }
    return true;
}

