#include <iostream>
#include <thread>

#include "common/config.h"
#include "common/log.h"
#include "service/restful.h"
#include "service/rpc.h"

void init_config() {
    config_mgr.set_config_file_path("./etc");
    config_mgr.set_config_file_name("controller.cfg");
    config_mgr.init();
}

int main() {

    LOG_ERROR("hello " << "world" << 123)
    LOG_ERROR("hello " << "wo\nrld" << 123)
    LOG_INFO("hi")
    return 0;
}