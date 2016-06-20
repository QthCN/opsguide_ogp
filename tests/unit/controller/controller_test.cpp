//
// Created by thuanqin on 16/6/17.
//

#include <ogp_msg.pb.h>
#include "gmock/gmock.h"

#include "controller/controller.h"
#include "ogp_msg.pb.h"
#include "service/message.h"
#include "unit/controller/agents_mock.h"
#include "unit/controller/applications_mock.h"
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
    }

    virtual void TearDown() {

    }

    NiceMock<MockModelMgr> *model_mgr;
    NiceMock<MockAgents> *agents;
    NiceMock<MockApplications> *applications;
};

TEST_F(ControllerTest, handle_da_say_hi_msg_da_not_exist) {
    Controller controller(model_mgr, agents, applications);
    std::string address = "192.8.8.8";
    auto sess = std::make_shared<NiceMock<MockSession>>();
    auto msg = std::make_shared<Message>(MsgType::DA_DOCKER_SAY_HI, new char[0], 0);
    agent_ptr agent = std::make_shared<DockerAgent>();
    agent_ptr arg_agent;
    agent->set_machine_ip(address);
    agent->set_sess(sess);

    EXPECT_CALL(*sess, get_address())
            .Times(1)
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
    Controller controller(model_mgr, agents, applications);
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
    Controller controller(model_mgr, agents, applications);
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

