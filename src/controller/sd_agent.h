//
// Created by thuanqin on 16/7/18.
//

#ifndef OGP_CONTROLLER_SD_AGENT_H
#define OGP_CONTROLLER_SD_AGENT_H

#include <mutex>
#include <random>
#include <queue>

#include "common/config.h"
#include "common/haproxy.h"
#include "controller/base.h"
#include "controller/sd_utils.h"
#include "ogp_msg.pb.h"
#include "service/message.h"
#include "service/session.h"

class SDAgent: public BaseController {
public:
    SDAgent(){
        haproxy_helper = new HAProxyHelper(config_mgr.get_item("haproxy_cfg_file")->get_str(),
                                           config_mgr.get_item("haproxy_bin")->get_str(),
                                           config_mgr.get_item("haproxy_pid_file")->get_str());
    };
    ~SDAgent() {
        delete haproxy_helper;
    }
    void init();
    void associate_sess(sess_ptr sess);
    void handle_msg(sess_ptr sess, msg_ptr msg);
    // 心跳同步线程
    void send_heartbeat();
    // 与sdp之间的状态同步线程
    void sync();
    // haproxy的状态维护现场
    void haproxy_sync();
    void invalid_sess(sess_ptr sess);

private:
    void handle_ct_list_sdp_msg(sess_ptr sess, msg_ptr msg);
    void handle_sp_sync_service_msg(sess_ptr sess, msg_ptr msg);
    sess_ptr sdp_sess = nullptr;
    sess_ptr controller_sess = nullptr;
    std::mutex agent_lock;
    std::mt19937 rng;
    ogp_msg::ServiceSyncData current_services;
    SDUtils sd_utils;
    bool do_haproxy_sync;
    HAProxyHelper *haproxy_helper;
};

#endif //OGP_CONTROLLER_SD_AGENT_H
