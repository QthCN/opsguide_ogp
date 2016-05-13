//
// Created by thuanqin on 16/5/13.
//

#ifndef OG_CONTROLLER_CONFIG_H
#define OG_CONTROLLER_CONFIG_H

#include <map>
#include <memory>
#include <string>
#include <utility>

class ConfigMgr;

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
    ConfigMgr(std::string config_file_path="./etc", std::string config_file_name="controller.cfg");
    std::shared_ptr<ConfigItem> &get_item(std::string name);

private:
    std::string config_file_path;
    std::string config_file_name;
    std::map<std::string, std::shared_ptr<ConfigItem>> config_items;
};

#endif //OG_CONTROLLER_CONFIG_H
