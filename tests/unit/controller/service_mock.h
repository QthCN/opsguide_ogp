//
// Created by thuanqin on 16/7/12.
//

#ifndef OGP_TEST_SERVICE_MOCK_H
#define OGP_TEST_SERVICE_MOCK_H

#include "gmock/gmock.h"

#include "controller/service.h"

class MockServices: public ServicesBase {
public:
    MOCK_METHOD0(sync_services, void());
    MOCK_METHOD0(list_services, std::vector<service_ptr>());
    MOCK_METHOD1(remove_service, void(int service_id));
    MOCK_METHOD5(add_service, void(int service_id, std::string service_type, int app_id, int service_port, int private_port));
    MOCK_METHOD2(init, void(AgentsBase *agents_, ApplicationsBase *applications_));
};

#endif //OGP_TEST_SERVICE_MOCK_H
