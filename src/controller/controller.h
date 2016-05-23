//
// Created by thuanqin on 16/5/13.
//

#ifndef OG_CONTROLLER_CONTROLLER_H
#define OG_CONTROLLER_CONTROLLER_H

#include "service/message.h"
#include "service/session.h"

class Controller {
public:
    Controller() = default;
    void init();
    void associate_sess(sess_ptr sess);
    void handle_msg(sess_ptr sess, msg_ptr msg);

private:

};

#endif //OG_CONTROLLER_CONTROLLER_H
