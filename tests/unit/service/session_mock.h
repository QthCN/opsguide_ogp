//
// Created by thuanqin on 16/6/17.
//

#ifndef OGP_TEST_SESSION_MOCK_H
#define OGP_TEST_SESSION_MOCK_H

#include "gmock/gmock.h"

#include "service/session.h"

class MockSession: public Session {
public:
    MOCK_METHOD1(set_address, Session &(std::string address));
    MOCK_CONST_METHOD0(get_address, std::string());
    MOCK_CONST_METHOD0(get_port, unsigned short());
    MOCK_METHOD1(set_port, Session &(unsigned short port));
    MOCK_METHOD1(send_msg, void(msg_ptr msg));
    MOCK_CONST_METHOD0(valid, bool());
    MOCK_METHOD0(invalid_sess, void());
};

#endif //OGP_TEST_SESSION_MOCK_H
