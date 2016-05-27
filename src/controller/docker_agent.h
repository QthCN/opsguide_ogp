//
// Created by thuanqin on 16/5/24.
//

#ifndef OGP_CONTROLLER_DOCKER_AGENT_H
#define OGP_CONTROLLER_DOCKER_AGENT_H

#include "controller/base.h"
#include "service/message.h"
#include "service/session.h"

class DockerAgent: public BaseController {
public:
    DockerAgent() = default;
    void init();
    void associate_sess(sess_ptr sess);
    void handle_msg(sess_ptr sess, msg_ptr msg);
    void send_heartbeat();
    void sync();
    void invalid_sess(sess_ptr sess);

private:
    sess_ptr controller_sess = nullptr;
    std::mutex agent_lock;

};

#endif //OGP_CONTROLLER_DOCKER_AGENT_H
