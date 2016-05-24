//
// Created by thuanqin on 16/5/24.
//

#ifndef OGP_CONTROLLER_BASE_H
#define OGP_CONTROLLER_BASE_H

#include "service/message.h"
#include "service/session.h"

class BaseController {
public:
    virtual void init() = 0;
    virtual void associate_sess(sess_ptr sess) = 0;
    virtual void handle_msg(sess_ptr sess, msg_ptr msg) = 0;
    virtual void send_heartbeat() = 0;
};

#endif //OGP_CONTROLLER_BASE_H
