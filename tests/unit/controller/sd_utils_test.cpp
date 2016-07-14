//
// Created by thuanqin on 16/7/14.
//
#include "gmock/gmock.h"

#include "controller/controller.h"
#include "controller/sd_proxy.h"
#include "controller/sd_utils.h"
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


TEST(SDUtilsTest, check_diff_with_same_args) {
    SDUtils sd_utils;
    ogp_msg::ServiceSyncData service_sync_data;
    service_sync_data.set_uniq_id(100);
    ASSERT_FALSE(sd_utils.check_diff(service_sync_data, service_sync_data));
}

TEST(SDUtilsTest, check_diff_with_diff_args_01) {
    SDUtils sd_utils;
    ogp_msg::ServiceSyncData service_sync_data_01, service_sync_data_02;
    service_sync_data_01.set_uniq_id(100);
    service_sync_data_02.set_uniq_id(200);
    ASSERT_FALSE(sd_utils.check_diff(service_sync_data_01, service_sync_data_02));
}

TEST(SDUtilsTest, check_diff_with_diff_args_02) {
    SDUtils sd_utils;
    ogp_msg::ServiceSyncData service_sync_data_01, service_sync_data_02;

    for (int i=0; i<=9; i++) {
        auto service_sync_info = service_sync_data_01.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    for (int i=10; i<=19; i++) {
        auto service_sync_info = service_sync_data_02.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    ASSERT_TRUE(sd_utils.check_diff(service_sync_data_01, service_sync_data_02));
}

TEST(SDUtilsTest, check_diff_with_diff_args_03) {
    SDUtils sd_utils;
    ogp_msg::ServiceSyncData service_sync_data_01, service_sync_data_02;

    for (int i=0; i<=9; i++) {
        auto service_sync_info = service_sync_data_01.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    for (int i=0; i<=1; i++) {
        auto service_sync_info = service_sync_data_02.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    ASSERT_TRUE(sd_utils.check_diff(service_sync_data_01, service_sync_data_02));
}

TEST(SDUtilsTest, check_diff_with_diff_args_04) {
    SDUtils sd_utils;
    ogp_msg::ServiceSyncData service_sync_data_01, service_sync_data_02;

    for (int i=0; i<=1; i++) {
        auto service_sync_info = service_sync_data_01.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    for (int i=0; i<=9; i++) {
        auto service_sync_info = service_sync_data_02.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    ASSERT_TRUE(sd_utils.check_diff(service_sync_data_01, service_sync_data_02));
}

TEST(SDUtilsTest, check_diff_with_diff_args_05) {
    SDUtils sd_utils;
    ogp_msg::ServiceSyncData service_sync_data_01, service_sync_data_02;

    for (int i=0; i<=9; i++) {
        auto service_sync_info = service_sync_data_01.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    for (int i=0; i<=9; i++) {
        auto service_sync_info = service_sync_data_02.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=8; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    ASSERT_TRUE(sd_utils.check_diff(service_sync_data_01, service_sync_data_02));
}

TEST(SDUtilsTest, check_diff_with_diff_args_06) {
    SDUtils sd_utils;
    ogp_msg::ServiceSyncData service_sync_data_01, service_sync_data_02;

    for (int i=0; i<=9; i++) {
        auto service_sync_info = service_sync_data_01.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=8; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    for (int i=0; i<=9; i++) {
        auto service_sync_info = service_sync_data_02.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    ASSERT_TRUE(sd_utils.check_diff(service_sync_data_01, service_sync_data_02));
}

TEST(SDUtilsTest, check_diff_with_diff_args_07) {
    SDUtils sd_utils;
    ogp_msg::ServiceSyncData service_sync_data_01, service_sync_data_02;

    for (int i=0; i<=9; i++) {
        auto service_sync_info = service_sync_data_01.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    for (int i=0; i<=9; i++) {
        auto service_sync_info = service_sync_data_02.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(301+i);
        }
    }

    ASSERT_TRUE(sd_utils.check_diff(service_sync_data_01, service_sync_data_02));
}

TEST(SDUtilsTest, check_diff_with_diff_args_08) {
    SDUtils sd_utils;
    ogp_msg::ServiceSyncData service_sync_data_01, service_sync_data_02;
    service_sync_data_01.set_uniq_id(100);
    service_sync_data_02.set_uniq_id(100);
    ASSERT_FALSE(sd_utils.check_diff(service_sync_data_01, service_sync_data_02));
}

TEST(SDUtilsTest, check_diff_with_diff_args_09) {
    SDUtils sd_utils;
    ogp_msg::ServiceSyncData service_sync_data_01, service_sync_data_02;

    for (int i=0; i<=9; i++) {
        auto service_sync_info = service_sync_data_01.add_infos();
        service_sync_info->set_service_id(500+i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    for (int i=0; i<=9; i++) {
        auto service_sync_info = service_sync_data_02.add_infos();
        service_sync_info->set_service_id(i);
        service_sync_info->set_service_type(SDP_NAME);
        service_sync_info->set_app_id(100+i);
        service_sync_info->set_app_name("TEST");
        service_sync_info->set_service_port(200+i);

        for (int j=0; j<=9; j++) {
            auto service_sync_item = service_sync_info->add_items();
            service_sync_item->set_ma_ip("192.168.9.9");
            service_sync_item->set_public_port(300+i);
        }
    }

    ASSERT_TRUE(sd_utils.check_diff(service_sync_data_01, service_sync_data_02));
}
