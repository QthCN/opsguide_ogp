//
// Created by thuanqin on 16/7/18.
//

#ifndef OGP_CONTROLLER_SD_AGENT_H
#define OGP_CONTROLLER_SD_AGENT_H

#include <mutex>
#include <random>
#include <queue>

#include "controller/base.h"
#include "ogp_msg.pb.h"
#include "service/message.h"
#include "service/session.h"

class SDAgent: public BaseController {
public:
    SDAgent(){ };
    ~SDAgent() { }
    void init();
    void associate_sess(sess_ptr sess);
    void handle_msg(sess_ptr sess, msg_ptr msg);
    // 心跳同步线程
    void send_heartbeat();
    // 与sdp之间的状态同步线程
    void sync();
    void invalid_sess(sess_ptr sess);

private:
    void handle_ct_list_sdp_msg(sess_ptr sess, msg_ptr msg);
    void handle_sp_sync_service_msg(sess_ptr sess, msg_ptr msg);
    sess_ptr sdp_sess = nullptr;
    sess_ptr controller_sess = nullptr;
    std::mutex agent_lock;
    std::mt19937 rng;
    int current_sync_id = -1;
    std::mutex current_sync_id_lock;
};

#endif //OGP_CONTROLLER_SD_AGENT_H
