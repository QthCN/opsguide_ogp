//
// Created by thuanqin on 16/6/17.
//

#ifndef OGP_TESTS_APPLICATIONS_MOCK_H
#define OGP_TESTS_APPLICATIONS_MOCK_H

#include "gmock/gmock.h"

#include "controller/applications.h"

class MockApplications: public ApplicationsBase {
public:
    MOCK_METHOD1(add_application, void(application_ptr a));
    MOCK_METHOD1(get_application, application_ptr(int app_id));
    MOCK_METHOD1(get_application, application_ptr(std::string app_name));
    MOCK_METHOD0(get_applications, std::vector<application_ptr>());
};

#endif //OGP_TESTS_APPLICATIONS_MOCK_H
