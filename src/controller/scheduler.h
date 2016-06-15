//
// Created by thuanqin on 16/6/13.
//

#ifndef OGP_CONTROLLER_SCHEDULER_H
#define OGP_CONTROLLER_SCHEDULER_H

#include <string>

class Scheduler {
public:
    void init();
    std::string da_schedule(std::string da_ip="");
};

#endif //OGP_CONTROLLER_SCHEDULER_H
