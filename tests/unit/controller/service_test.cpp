//
// Created by thuanqin on 16/7/13.
//
#include "gmock/gmock.h"

#include "controller/controller.h"
#include "controller/service.h"
#include "ogp_msg.pb.h"
#include "service/message.h"
#include "unit/controller/agents_mock.h"
#include "unit/controller/applications_mock.h"
#include "unit/controller/service_mock.h"
#include "unit/model/model_mock.h"
#include "unit/service/session_mock.h"


using ::testing::_;
using ::testing::NiceMock;
using ::testing::Matcher;
using ::testing::SaveArg;
using ::testing::Return;

class ServicesTest: public ::testing::Test {
protected:
    virtual void SetUp() {
        model_mgr = new NiceMock<MockModelMgr>();
        agents = new NiceMock<MockAgents>();
        applications = new NiceMock<MockApplications>();
        services = new Services();
        services->init(agents, applications, model_mgr);
    }

    virtual void TearDown() {

    }

    NiceMock<MockAgents> *agents;
    NiceMock<MockApplications> *applications;
    NiceMock<MockModelMgr> *model_mgr;
    ServicesBase *services;
};

TEST_F(ServicesTest, add_service) {
    auto app = std::make_shared<Application>(100, "TEST", "TEST", "TEST");
    EXPECT_CALL(*applications, get_application(Matcher<int>(_)))
            .WillRepeatedly(Return(app));
    for (int i = 0; i<=9; i++) {
        services->add_service(900+i, ST_PORT_SERVICE, 100+i, 200+i, 300+i);
    }
    auto current_services = services->list_services();
    ASSERT_EQ(10, current_services.size());
}

TEST_F(ServicesTest, del_service) {
    auto app = std::make_shared<Application>(100, "TEST", "TEST", "TEST");
    EXPECT_CALL(*applications, get_application(Matcher<int>(_)))
            .WillRepeatedly(Return(app));
    for (int i = 0; i<=9; i++) {
        services->add_service(900+i, ST_PORT_SERVICE, 100+i, 200+i, 300+i);
    }
    auto current_services = services->list_services();
    ASSERT_EQ(10, current_services.size());

    for (int i = 0; i<=9; i++) {
        services->remove_service(900+i);
        auto current_services = services->list_services();
        ASSERT_EQ(9-i, current_services.size());
    }
}

TEST_F(ServicesTest, del_service_with_no_exist_id) {
    auto app = std::make_shared<Application>(100, "TEST", "TEST", "TEST");
    EXPECT_CALL(*applications, get_application(Matcher<int>(_)))
            .WillRepeatedly(Return(app));
    for (int i = 0; i<=9; i++) {
        services->add_service(900+i, ST_PORT_SERVICE, 100+i, 200+i, 300+i);
    }
    auto current_services = services->list_services();
    ASSERT_EQ(10, current_services.size());

    for (int i = 0; i<=9; i++) {
        services->remove_service(90000+i);
        auto current_services = services->list_services();
        ASSERT_EQ(10, current_services.size());
    }
}

TEST_F(ServicesTest, sync_services) {
    EXPECT_CALL(*agents, get_agents_by_type(DA_NAME)).Times(1);
    EXPECT_CALL(*agents, get_agents_by_type(SDP_NAME)).Times(2);
    services->sync_services();
}