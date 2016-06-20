//
// Created by thuanqin on 16/5/13.
//

#ifndef OG_CONTROLLER_CONTROLLER_H
#define OG_CONTROLLER_CONTROLLER_H

#include "controller/agents.h"
#include "controller/applications.h"
#include "controller/base.h"
#include "controller/scheduler.h"
#include "model/model.h"
#include "service/message.h"
#include "service/session.h"

class Controller: public BaseController {
public:
    Controller(ModelMgrBase *model_mgr, AgentsBase *agents, ApplicationsBase *applications):
            model_mgr(model_mgr), agents(agents), applications(applications) {};
    ~Controller() {
        delete model_mgr;
        delete agents;
        delete applications;
    }
    void init();
    void associate_sess(sess_ptr sess);
    void handle_msg(sess_ptr sess, msg_ptr msg);
    void invalid_sess(sess_ptr sess);

private:
    void handle_da_sync_msg(sess_ptr sess, msg_ptr msg);
    void handle_da_say_hi_msg(sess_ptr sess, msg_ptr msg);
    void handle_po_get_apps_msg(sess_ptr sess, msg_ptr msg);
    void handle_po_get_agents_msg(sess_ptr sess, msg_ptr msg);
    void handle_po_publish_app_msg(sess_ptr sess, msg_ptr msg);
    void handle_po_remove_appver_msg(sess_ptr sess, msg_ptr msg);
    void handle_po_upgrade_appver_msg(sess_ptr sess, msg_ptr msg);
    void handle_ci_add_app(sess_ptr sess, msg_ptr msg);
    std::mutex g_lock;
    Scheduler scheduler;
    ModelMgrBase *model_mgr;
    AgentsBase *agents;
    ApplicationsBase *applications;
};

#endif //OG_CONTROLLER_CONTROLLER_H
