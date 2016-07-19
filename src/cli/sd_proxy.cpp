//
// Created by thuanqin on 16/7/5.
//
#include <iostream>
#include <string>

#include "boost/program_options.hpp"

#include "common/config.h"
#include "controller/agents.h"
#include "controller/sd_proxy.h"
#include "service/proxy.h"

using namespace boost::program_options;

int main(int argc, char *argv[]) {
    options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            ("dir,d", value<std::string>()->default_value("./etc"), "config file path")
            ("file,f", value<std::string>()->default_value("sd_proxy.cfg"), "config file name");
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

    auto c_thread_num = static_cast<unsigned int>(config_mgr.get_item("client_thread_num")->get_int());
    auto s_thread_num = static_cast<unsigned int>(config_mgr.get_item("server_thread_num")->get_int());
    auto controller_address = config_mgr.get_item("proxy_controller_address")->get_str();
    auto controller_port = static_cast<unsigned int>(config_mgr.get_item("proxy_controller_port")->get_int());
    auto listen_address = config_mgr.get_item("proxy_listen_address")->get_str();
    auto listen_port = static_cast<unsigned int>(config_mgr.get_item("proxy_listen_port")->get_int());
    SDProxy sd_proxy(new Agents());
    ProxyService proxy_service(s_thread_num, c_thread_num, controller_address, controller_port,
                                listen_address, listen_port, &sd_proxy, MsgType::SP_SDPROXY_SAY_HI);
    proxy_service.run();
    return 0;
}
