//
// Created by thuanqin on 16/5/13.
//
#include "common/config.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "boost/algorithm/string.hpp"
#include "boost/filesystem.hpp"

ConfigItem::ConfigItem(ConfigMgr *config_mgr, std::string &value): config_mgr(config_mgr), value(value) { }

int ConfigItem::get_int() {
    try {
        int i = std::stoi(value);
        return i;
    } catch (std::invalid_argument e) {
        throw e;
    }
}

std::string ConfigItem::get_str() {
    return value;
}

bool ConfigItem::get_bool() {
    auto s(value);
    boost::algorithm::to_upper(s);
    if (s == "TRUE") {
        return true;
    } else if (s == "FALSE") {
        return false;
    } else {
        throw std::invalid_argument("Invalid boolean string");
    }
}

ConfigMgr::ConfigMgr(std::string config_file_path, std::string config_file_name):
        config_file_path(config_file_path), config_file_name(config_file_name) {
    init();
}

void ConfigMgr::init() {
    config_items.clear();
    auto dir = boost::filesystem::path(config_file_path);
    auto file = boost::filesystem::path(config_file_name);
    auto full_path = dir / file;

    std::fstream cfg_file;
    cfg_file.open(full_path.string());
    if (!cfg_file.is_open()) {
        std::cout << "Can not open config file: " << full_path << std::endl;
        throw std::runtime_error("Can not open config file");
    } else {
        char line[1024];
        while (!cfg_file.eof()) {
            cfg_file.getline(line, 1024);
            std::string line_(line);

            // ignore comments
            if (line_.length() > 0 && line_[0] == '#') continue;

            auto pos = line_.find('=');
            if (pos == std::string::npos) continue;
            std::string item_name = line_.substr(0, pos);
            std::string item_value = line_.substr(pos+1);
            boost::trim(item_name);
            boost::trim(item_value);

            auto config_item = std::make_shared<ConfigItem>(this, item_value);
            config_items[item_name] = config_item;
        }
    }
}

void ConfigMgr::set_config_file_path(std::string config_file_path) {
    config_file_path = config_file_path;
}

void ConfigMgr::set_config_file_name(std::string config_file_name) {
    config_file_name = config_file_name;
}

std::shared_ptr<ConfigItem> &ConfigMgr::get_item(std::string name) {
    return config_items[name];
}

ConfigMgr config_mgr;