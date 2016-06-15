//
// Created by thuanqin on 16/6/13.
//
#include "controller/scheduler.h"

#include "common/log.h"

void Scheduler::init() {
    LOG_INFO("Scheduler initialization finished.")
}

std::string Scheduler::da_schedule(std::string da_ip) {
    if (da_ip != "") {
        return da_ip;
    }
    LOG_ERROR("Only support specific da_ip now.")
    return "";
}
