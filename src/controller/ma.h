//
// Created by thuanqin on 16/6/14.
//

#ifndef OGP_CONTROLLER_MA_H
#define OGP_CONTROLLER_MA_H

#include <vector>

#include "common/config.h"
#include "common/log.h"

struct MACfgPort {
public:
    int private_port;
    int public_port;
    std::string type;
};
typedef std::shared_ptr<MACfgPort> ma_cfg_port_ptr;

struct MACfgVolume {
public:
    std::string docker_volume;
    std::string host_volume;
};
typedef std::shared_ptr<MACfgVolume> ma_cfg_volume_ptr;

struct MACfgDns {
public:
    std::string dns;
    std::string address;
};
typedef std::shared_ptr<MACfgDns> ma_cfg_dns_ptr;

struct MACfgExtraCmd {
public:
    std::string extra_cmd;
};
typedef std::shared_ptr<MACfgExtraCmd> ma_cfg_extra_cmd_ptr;

struct MAHint {
public:
    std::string item;
    std::string value;
};
typedef std::shared_ptr<MAHint> ma_hint_ptr;

enum class MAStatus {
    // 未知状态。一般是da还没有同步消息造成的
    UNKNOWN = 1,
    // 运行中,且运行正常
    RUNNING = 2,
    // 应该运行在某个da上,但da没有任何关于此容器的消息
    NOT_RUNNING = 3,
    // da已经感知,但是容器运行不正常
    STOP = 4,
};

class Container {
public:
    std::vector<ma_cfg_port_ptr> &get_cfg_ports() {
        return cfg_ports;
    }

    void set_cfg_ports(const std::vector<ma_cfg_port_ptr> &cfg_ports) {
        Container::cfg_ports = cfg_ports;
    }

    std::vector<ma_cfg_volume_ptr> &get_cfg_volumes() {
        return cfg_volumes;
    }

    void set_cfg_volumes(const std::vector<ma_cfg_volume_ptr> &cfg_volumes) {
        Container::cfg_volumes = cfg_volumes;
    }

    std::vector<ma_cfg_dns_ptr> &get_cfg_dns() {
        return cfg_dns;
    }

    void set_cfg_dns(const std::vector<ma_cfg_dns_ptr> &cfg_dns) {
        Container::cfg_dns = cfg_dns;
    }

    ma_cfg_extra_cmd_ptr &get_cfg_extra_cmd() {
        return cfg_extra_cmd;
    }

    void set_cfg_extra_cmd(const ma_cfg_extra_cmd_ptr &cfg_extra_cmd) {
        Container::cfg_extra_cmd = cfg_extra_cmd;
    }

    const std::string &get_name() const {
        return name;
    }

    void set_name(const std::string &name) {
        Container::name = name;
    }

    const std::string &get_image() const {
        return image;
    }

    void set_image(const std::string &image) {
        Container::image = image;
    }

private:
    std::vector<ma_cfg_port_ptr> cfg_ports;
    std::vector<ma_cfg_volume_ptr> cfg_volumes;
    std::vector<ma_cfg_dns_ptr> cfg_dns;
    ma_cfg_extra_cmd_ptr cfg_extra_cmd;
    std::string name;
    std::string image;
};
typedef std::shared_ptr<Container> container_ptr;

class MachineApplication {
public:
    MachineApplication(): status(MAStatus::UNKNOWN) {}
    int get_uniq_id() {return uniq_id;}
    void set_uniq_id(int uniq_id_) {uniq_id = uniq_id_;}
    std::string get_app_name() {return app_name;}
    void set_app_name(const std::string &app_name_) {app_name = app_name_;}
    int get_app_id() {return app_id;}
    void set_app_id(int app_id_) {app_id = app_id_;}
    std::string get_machine_ip_address() {return machine_ip_address;}
    void set_machine_ip_address(std::string machine_ip_address_) {machine_ip_address = machine_ip_address_;}
    int get_version_id() {return version_id;}
    void set_version_id(int version_id_) {version_id = version_id_;}
    std::string get_version() {return version;}
    void set_version(std::string version_) {version = version_;}
    MAStatus get_status() {return status;}
    void set_status(MAStatus status_) {status = status_;}
    std::vector<ma_cfg_port_ptr> get_cfg_ports() {return cfg_ports;}
    std::vector<ma_cfg_volume_ptr> get_cfg_volumes() {return cfg_volumes;}
    std::vector<ma_cfg_dns_ptr> get_cfg_dns() {return cfg_dns;}
    ma_cfg_extra_cmd_ptr get_cfg_extra_cmd() {return cfg_extra_cmd;}
    std::vector<ma_hint_ptr> get_hints() {return hints;}
    void add_cfg_port(ma_cfg_port_ptr cfg_port) {cfg_ports.push_back(cfg_port);}
    void add_cfg_volume(ma_cfg_volume_ptr cfg_volume) {cfg_volumes.push_back(cfg_volume);}
    void add_cfg_dns(ma_cfg_dns_ptr cfg_dns_) {cfg_dns.push_back(cfg_dns_);}
    void set_cfg_extra_cmd(ma_cfg_extra_cmd_ptr extra_cmd) {cfg_extra_cmd = extra_cmd;}
    void add_hint(ma_hint_ptr hint) {hints.push_back(hint);}
    void set_runtime_name(std::string runtime_name_) {runtime_name = runtime_name_;}
    std::string get_runtime_name() {return runtime_name;}
    std::string get_image() {
        auto docker_hub = static_cast<std::string>(config_mgr.get_item("controller_docker_hub_address")->get_str());
        if (docker_hub == "") {
            return app_name + ":" + version;
        } else {
            return docker_hub + "/" + app_name + ":" + version;
        }
    }
private:
    int uniq_id; // uniq_id表示某台主机上运行的某个app,这个id是唯一的
    std::string app_name;
    std::string runtime_name;
    int app_id;
    std::string machine_ip_address;
    int version_id;
    std::string version;
    MAStatus status;

    std::vector<ma_cfg_port_ptr> cfg_ports;
    std::vector<ma_cfg_volume_ptr> cfg_volumes;
    std::vector<ma_cfg_dns_ptr> cfg_dns;
    ma_cfg_extra_cmd_ptr cfg_extra_cmd;
    std::vector<ma_hint_ptr> hints;
};
typedef std::shared_ptr<MachineApplication> machine_application_ptr;

#endif //OGP_CONTROLLER_MA_H
