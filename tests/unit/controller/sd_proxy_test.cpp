//
// Created by thuanqin on 16/7/14.
//

#include "gmock/gmock.h"

#include "controller/controller.h"
#include "controller/sd_proxy.h"
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

class SDProxyTest: public ::testing::Test {
protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {

    }
    SDProxy sd_proxy;
};

TEST_F(SDProxyTest, handle_ct_sync_service_data_req_with_good_uniq_id) {
    auto sess = std::make_shared<NiceMock<MockSession>>();
    ASSERT_EQ(sd_proxy.get_current_services().uniq_id(), -1);
    ogp_msg::ServiceSyncData service_sync_data;
    service_sync_data.set_uniq_id(100);
    auto header = service_sync_data.mutable_header();
    int rc = 0;
    std::string ret_msg = "";
    header->set_rc(rc);
    header->set_message(ret_msg);
    auto msg_size = service_sync_data.ByteSize();
    char *msg_data = new char[msg_size];
    service_sync_data.SerializeToArray(msg_data, msg_size);
    auto msg = std::make_shared<Message>(MsgType::CT_SDPROXY_SERVICE_DATA_SYNC_REQ,
                                         msg_data,
                                         msg_size);
    sd_proxy.handle_msg(sess, msg);
    ASSERT_EQ(sd_proxy.get_current_services().uniq_id(), 100);
}

TEST_F(SDProxyTest, handle_ct_sync_service_data_req_with_bad_uniq_id) {
    auto sess = std::make_shared<NiceMock<MockSession>>();
    ASSERT_EQ(sd_proxy.get_current_services().uniq_id(), -1);
    ogp_msg::ServiceSyncData service_sync_data;
    auto header = service_sync_data.mutable_header();
    int rc = 0;
    std::string ret_msg = "";
    header->set_rc(rc);
    header->set_message(ret_msg);
    service_sync_data.set_uniq_id(-2);
    auto msg_size = service_sync_data.ByteSize();
    char *msg_data = new char[msg_size];
    service_sync_data.SerializeToArray(msg_data, msg_size);
    auto msg = std::make_shared<Message>(MsgType::CT_SDPROXY_SERVICE_DATA_SYNC_REQ,
                                         msg_data,
                                         msg_size);
    sd_proxy.handle_msg(sess, msg);
    ASSERT_EQ(sd_proxy.get_current_services().uniq_id(), -1);
}
