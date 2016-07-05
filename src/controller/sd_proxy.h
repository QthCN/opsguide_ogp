//
// Created by thuanqin on 16/7/5.
//

#ifndef OGP_CONTROLLER_SD_PROXY_H
#define OGP_CONTROLLER_SD_PROXY_H

#include <mutex>
#include <queue>

#include "controller/base.h"
#include "ogp_msg.pb.h"
#include "service/message.h"
#include "service/session.h"

class SDProxy: public BaseController {
public:
    SDProxy() {};
    ~SDProxy() { }
    void init();
    void associate_sess(sess_ptr sess);
    void handle_msg(sess_ptr sess, msg_ptr msg);
    void invalid_sess(sess_ptr sess);

private:
    std::mutex g_lock;

};

#endif //OGP_CONTROLLER_SD_PROXY_H
