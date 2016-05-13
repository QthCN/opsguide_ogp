//
// Created by thuanqin on 16/5/13.
//
#include "gtest/gtest.h"

#include <fstream>
#include <iostream>
#include <string>

#include "common/config.h"

const std::string config_file_name = "controller.cfg";
void generate_test_config_file() {
    std::ofstream out;
    out.open(config_file_name, std::ios::out);
    if (out.is_open()) {
        out << "# test config file" << std::endl;
        out << " " << std::endl;
        out << "item_int=100" << std::endl;
        out << "item_str=http://test.com" << std::endl;
        out << "item_true=true" << std::endl;
        out << "item_falue=false" << std::endl;
        out << " item_int_with_space  =  99" << std::endl;
        out.close();
    }
}

TEST(ConfigMgr, GetItem) {
    generate_test_config_file();
    ConfigMgr config_mgr("./", "controller.cfg");
    EXPECT_EQ(100, config_mgr.get_item("item_int")->get_int());
    EXPECT_EQ("http://test.com", config_mgr.get_item("item_str")->get_str());
    EXPECT_EQ(true, config_mgr.get_item("item_true")->get_bool());
    EXPECT_EQ(false, config_mgr.get_item("item_falue")->get_bool());
    EXPECT_EQ(99, config_mgr.get_item("item_int_with_space")->get_int());
}