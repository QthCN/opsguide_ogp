//
// Created by thuanqin on 16/5/19.
//
#include <iostream>
#include <string>

#include "boost/program_options.hpp"

#include "common/config.h"
#include "controller/agents.h"
#include "controller/applications.h"
#include "controller/controller.h"
#include "model/model.h"
#include "service/controller.h"

using namespace boost::program_options;

int main(int argc, char *argv[]) {
    options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            ("dir,d", value<std::string>()->default_value("./etc"), "config file path")
            ("file,f", value<std::string>()->default_value("controller.cfg"), "config file name");
    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }
    auto config_file_dir = vm["dir"].as<std::string>();
    auto config_file_name = vm["file"].as<std::string>();

    config_mgr.set_config_file_path(config_file_dir);
    config_mgr.set_config_file_name(config_file_name);
    config_mgr.init();
    auto thread_num = static_cast<unsigned int>(config_mgr.get_item("controller_thread_num")->get_int());
    auto listen_address = config_mgr.get_item("controller_listen_address")->get_str();
    auto listen_port = static_cast<unsigned int>(config_mgr.get_item("controller_listen_port")->get_int());
    Controller controller(new ModelMgr(), new Agents(), new Applications());
    ControllerService controller_service(thread_num, listen_address, listen_port, &controller);
    controller_service.run();
    return 0;
}

