//
// Created by thuanqin on 16/5/13.
//

#ifndef OG_COMMON_CONFIG_H
#define OG_COMMON_CONFIG_H

#include <map>
#include <memory>
#include <string>
#include <utility>

class ConfigMgr;

extern ConfigMgr config_mgr;

class ConfigItem {
public:
    ConfigItem(ConfigMgr *config_mgr, std::string &value);
    int get_int();
    std::string get_str();
    bool get_bool();

private:
    ConfigMgr *config_mgr;
    std::string value;
};

class ConfigMgr {
public:
    ConfigMgr() = default;
    ConfigMgr(std::string config_file_path, std::string config_file_name);
    std::shared_ptr<ConfigItem> &get_item(std::string name);
    void init();
    void set_config_file_path(std::string config_file_path);
    void set_config_file_name(std::string config_file_name);

private:
    std::string config_file_path;
    std::string config_file_name;
    std::map<std::string, std::shared_ptr<ConfigItem>> config_items;
};

#endif //OG_COMMON_CONFIG_H
