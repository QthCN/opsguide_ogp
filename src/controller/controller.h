//
// Created by thuanqin on 16/5/13.
//

#ifndef OG_CONTROLLER_CONTROLLER_H
#define OG_CONTROLLER_CONTROLLER_H

#include "controller/base.h"
#include "service/message.h"
#include "service/session.h"

class Controller: public BaseController {
public:
    Controller() = default;
    void init();
    void associate_sess(sess_ptr sess);
    void handle_msg(sess_ptr sess, msg_ptr msg);
    void send_heartbeat();
    void sync();
    void invalid_sess(sess_ptr sess);

private:
    void handle_da_sync_msg(sess_ptr sess, msg_ptr msg);
    void handle_da_say_hi_msg(sess_ptr sess, msg_ptr msg);
    std::mutex g_lock;

};

#endif //OG_CONTROLLER_CONTROLLER_H
