//
// Created by thuanqin on 16/6/17.
//

#include "gmock/gmock.h"

#include "controller/controller.h"
#include "ogp_msg.pb.h"
#include "service/message.h"
#include "unit/controller/agents_mock.h"
#include "unit/controller/applications_mock.h"
#include "unit/controller/service_mock.h"
#include "unit/model/model_mock.h"
#include "unit/service/session_mock.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::SaveArg;
using ::testing::Return;

class ControllerTest: public ::testing::Test {
protected:
    virtual void SetUp() {
        model_mgr = new NiceMock<MockModelMgr>();
        agents = new NiceMock<MockAgents>();
        applications = new NiceMock<MockApplications>();
        services = new NiceMock<MockServices>();
    }

    virtual void TearDown() {

    }

    NiceMock<MockModelMgr> *model_mgr;
    NiceMock<MockAgents> *agents;
    NiceMock<MockApplications> *applications;
    NiceMock<MockServices> *services;
};

TEST_F(ControllerTest, handle_da_say_hi_msg_da_not_exist) {
    Controller controller(model_mgr, agents, applications, services);
    std::string address = "192.8.8.8";
    auto sess = std::make_shared<NiceMock<MockSession>>();
    auto msg = std::make_shared<Message>(MsgType::DA_DOCKER_SAY_HI, new char[0], 0);
    agent_ptr agent = std::make_shared<DockerAgent>();
    agent_ptr arg_agent;
    agent->set_machine_ip(address);
    agent->set_sess(sess);

    EXPECT_CALL(*sess, get_address())
            .WillRepeatedly(Return(address));
    EXPECT_CALL(*agents, get_key(_))
            .Times(2)
            .WillOnce(Return(address+":"+DA_NAME))
            .WillOnce(Return(address+":"+DA_NAME));
    EXPECT_CALL(*agents, add_agent(address+":"+DA_NAME, _))
            .Times(1)
            .WillOnce(SaveArg<1>(&arg_agent));

    controller.handle_msg(sess, msg);

    EXPECT_EQ(arg_agent->get_machine_ip(), address);
    EXPECT_EQ(arg_agent->get_sess(), sess);
}

TEST_F(ControllerTest, handle_da_say_hi_msg_da_exist_without_sess) {
    Controller controller(model_mgr, agents, applications, services);
    std::string address = "192.8.8.8";
    auto sess = std::make_shared<NiceMock<MockSession>>();
    auto msg = std::make_shared<Message>(MsgType::DA_DOCKER_SAY_HI, new char[0], 0);
    agent_ptr agent = std::make_shared<DockerAgent>();
    agent->set_machine_ip(address);
    agent->set_sess(nullptr);

    EXPECT_CALL(*agents, get_agent_by_sess(_, DA_NAME))
            .Times(1)
            .WillRepeatedly(Return(agent));
    EXPECT_CALL(*agents, add_agent(_, _))
            .Times(0);

    controller.handle_msg(sess, msg);

}

TEST_F(ControllerTest, handle_da_say_hi_msg_da_exist_with_sess) {
    Controller controller(model_mgr, agents, applications, services);
    std::string address = "192.8.8.8";
    auto sess = std::make_shared<NiceMock<MockSession>>();
    auto msg = std::make_shared<Message>(MsgType::DA_DOCKER_SAY_HI, new char[0], 0);
    agent_ptr agent = std::make_shared<DockerAgent>();
    agent->set_machine_ip(address);
    agent->set_sess(sess);

    EXPECT_CALL(*agents, get_agent_by_sess(_, DA_NAME))
            .Times(1)
            .WillRepeatedly(Return(agent));
    EXPECT_CALL(*agents, add_agent(_, _))
            .Times(0);

    controller.handle_msg(sess, msg);
}

TEST_F(ControllerTest, handle_da_say_hi_msg_da_not_exist_but_ip_is_localhost) {
    Controller controller(model_mgr, agents, applications, services);
    std::string address = "127.0.0.1";
    auto sess = std::make_shared<NiceMock<MockSession>>();
    auto msg = std::make_shared<Message>(MsgType::DA_DOCKER_SAY_HI, new char[0], 0);
    agent_ptr agent = std::make_shared<DockerAgent>();
    agent_ptr arg_agent;
    agent->set_machine_ip(address);
    agent->set_sess(sess);

    EXPECT_CALL(*sess, get_address())
            .WillRepeatedly(Return(address));
    EXPECT_CALL(*agents, get_key(_))
            .WillRepeatedly(Return(address+":"+DA_NAME));
    EXPECT_CALL(*agents, add_agent(address+":"+DA_NAME, _))
            .Times(0);
    EXPECT_CALL(*sess, invalid_sess()).Times(1);

    controller.handle_msg(sess, msg);
}

TEST_F(ControllerTest, handle_po_add_service_msg_with_correct_args_in_port_type) {
    Controller controller(model_mgr, agents, applications, services);
    auto sess = std::make_shared<NiceMock<MockSession>>();
    auto req = ogp_msg::AddServiceReq();
    req.set_app_id(100);
    req.set_service_type(ST_PORT_SERVICE);
    auto req_port_body = req.mutable_port_service_body();
    req_port_body->set_service_port(9999);
    auto msg_size = req.ByteSize();
    char *msg_data = new char[msg_size];
    req.SerializeToArray(msg_data, msg_size);
    auto msg = std::make_shared<Message>(MsgType::PO_PORTAL_ADD_SERVICE_REQ,
                                         msg_data,
                                         msg_size);
    EXPECT_CALL(*applications, get_application(100))
            .WillRepeatedly(Return(std::make_shared<Application>(100, "", "", "")));
    EXPECT_CALL(*services, add_service(_, ST_PORT_SERVICE, 100, 9999, -1)).Times(1);
    EXPECT_CALL(*services, sync_services()).Times(1);
    controller.handle_msg(sess, msg);
}

TEST_F(ControllerTest, handle_po_add_service_msg_with_not_exist_service_type) {
    Controller controller(model_mgr, agents, applications, services);
    auto sess = std::make_shared<NiceMock<MockSession>>();
    auto req = ogp_msg::AddServiceReq();
    req.set_app_id(100);
    req.set_service_type("UNKNOWN_SERVICE_TYPE");
    auto req_port_body = req.mutable_port_service_body();
    req_port_body->set_service_port(9999);
    auto msg_size = req.ByteSize();
    char *msg_data = new char[msg_size];
    req.SerializeToArray(msg_data, msg_size);
    auto msg = std::make_shared<Message>(MsgType::PO_PORTAL_ADD_SERVICE_REQ,
                                         msg_data,
                                         msg_size);
    EXPECT_CALL(*applications, get_application(100))
            .WillRepeatedly(Return(std::make_shared<Application>(100, "", "", "")));
    EXPECT_CALL(*services, add_service(_, "UNKNOWN_SERVICE_TYPE", 100, 9999, -1)).Times(0);
    EXPECT_CALL(*services, sync_services()).Times(0);
    controller.handle_msg(sess, msg);
}

TEST_F(ControllerTest, handle_po_add_service_msg_with_not_exist_app_id) {
    Controller controller(model_mgr, agents, applications, services);
    auto sess = std::make_shared<NiceMock<MockSession>>();
    auto req = ogp_msg::AddServiceReq();
    req.set_app_id(-1);
    req.set_service_type(ST_PORT_SERVICE);
    req.set_app_id(100);
    req.set_service_type(ST_PORT_SERVICE);
    auto req_port_body = req.mutable_port_service_body();
    req_port_body->set_service_port(9999);
    auto msg_size = req.ByteSize();
    char *msg_data = new char[msg_size];
    req.SerializeToArray(msg_data, msg_size);
    auto msg = std::make_shared<Message>(MsgType::PO_PORTAL_ADD_SERVICE_REQ,
                                         msg_data,
                                         msg_size);
    EXPECT_CALL(*applications, get_application(100))
            .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*services, add_service(_, ST_PORT_SERVICE, 100, 9999, -1)).Times(0);
    EXPECT_CALL(*services, sync_services()).Times(0);
    controller.handle_msg(sess, msg);
}

TEST_F(ControllerTest, handle_po_del_service_msg_with_correct_args) {
    Controller controller(model_mgr, agents, applications, services);
    auto sess = std::make_shared<NiceMock<MockSession>>();
    auto req = ogp_msg::DelServiceReq();
    req.set_service_id(100);
    auto msg_size = req.ByteSize();
    char *msg_data = new char[msg_size];
    req.SerializeToArray(msg_data, msg_size);
    auto msg = std::make_shared<Message>(MsgType::PO_PORTAL_DEL_SERVICE_REQ,
                                         msg_data,
                                         msg_size);
    EXPECT_CALL(*services, remove_service(100)).Times(1);
    EXPECT_CALL(*services, sync_services()).Times(1);
    controller.handle_msg(sess, msg);
}


TEST_F(ControllerTest, handle_po_list_services_msg_with_correct_args) {
    Controller controller(model_mgr, agents, applications, services);
    auto sess = std::make_shared<NiceMock<MockSession>>();
    auto msg = std::make_shared<Message>(MsgType::PO_PORTAL_LIST_SERVICES_REQ,
                                         new char[0],
                                         0);
    EXPECT_CALL(*services, list_services()).Times(1);
    controller.handle_msg(sess, msg);
}