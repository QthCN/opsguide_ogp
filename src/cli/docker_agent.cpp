//
// Created by thuanqin on 16/5/19.
//
#include <iostream>
#include <string>

#include "boost/program_options.hpp"

#include "common/config.h"
#include "common/docker_client.h"
#include "controller/docker_agent.h"
#include "service/agent.h"

using namespace boost::program_options;

int main(int argc, char *argv[]) {
    options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            ("dir,d", value<std::string>()->default_value("./etc"), "config file path")
            ("file,f", value<std::string>()->default_value("docker_agent.cfg"), "config file name");
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
    auto thread_num = static_cast<unsigned int>(config_mgr.get_item("agent_thread_num")->get_int());
    auto controller_address = config_mgr.get_item("agent_controller_address")->get_str();
    auto controller_port = static_cast<unsigned int>(config_mgr.get_item("agent_controller_port")->get_int());
    DockerAgent docker_agent(new DockerClient());
    AgentService agent_service(thread_num, controller_address, controller_port, &docker_agent);
    agent_service.run();
    return 0;
}

