//
// Created by thuanqin on 16/7/5.
//

#ifndef OGP_CONTROLLER_SD_PROXY_H
#define OGP_CONTROLLER_SD_PROXY_H

#include <mutex>
#include <queue>

#include "controller/agents.h"
#include "controller/base.h"
#include "controller/sd_utils.h"
#include "ogp_msg.pb.h"
#include "service/message.h"
#include "service/session.h"


class SDProxy: public BaseController {
public:
    SDProxy(AgentsBase *agents): agents(agents) {
        cs_lock.lock();
        current_services.set_uniq_id(-1);
        cs_lock.unlock();
    };
    ~SDProxy() {
        delete agents;
    }
    void init();
    void associate_sess(sess_ptr sess);
    void handle_msg(sess_ptr sess, msg_ptr msg);
    void invalid_sess(sess_ptr sess);
    // 心跳同步线程
    void send_heartbeat();
    // SDAgent同步线程
    void sync_sda();
    void do_sync_sda(int sda_uid, ogp_msg::ServiceSyncData &service_data);
    ogp_msg::ServiceSyncData get_current_services() {return current_services;}
    void sync_controller();

private:
    std::mutex g_lock;
    sess_ptr controller_sess = nullptr;
    void handle_sa_sync_service_msg(sess_ptr sess, msg_ptr msg);
    void handle_ct_sync_service_data_msg(sess_ptr sess, msg_ptr msg);
    void handle_sa_say_hi_msg(sess_ptr sess, msg_ptr msg);
    void handle_sa_heartbeat_sync_msg(sess_ptr sess, msg_ptr msg);
    void disconnect_sdas();
    void send_listen_info();
    ogp_msg::ServiceSyncData current_services;
    std::mutex cs_lock;
    int sync_sda_current_uniq_id = -1;
    std::mutex ssda_id_lock;
    SDUtils sd_utils;
    AgentsBase *agents;
    std::mutex sda_sync_lock;
};

#endif //OGP_CONTROLLER_SD_PROXY_H
