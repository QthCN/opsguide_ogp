//
// Created by thuanqin on 16/6/17.
//

#ifndef OGP_TEST_AGENTS_MOCK_H
#define OGP_TEST_AGENTS_MOCK_H

#include "gmock/gmock.h"

#include "controller/agents.h"

class MockAgents: public AgentsBase {
public:
    MOCK_METHOD1(get_key, std::string(agent_ptr agent));
    MOCK_METHOD2(get_key, std::string(std::string address, std::string agent_type));
    MOCK_METHOD1(get_agent_by_key, agent_ptr(std::string key));
    MOCK_METHOD1(get_agent_by_sess, agent_ptr(sess_ptr sess));
    MOCK_METHOD2(get_agent_by_sess, agent_ptr(sess_ptr sess, std::string agent_type));
    MOCK_METHOD2(add_agent, void(std::string key, agent_ptr agent));
    MOCK_METHOD0(get_agents, std::vector<agent_ptr>());
    MOCK_METHOD0(dump_status, void());
    MOCK_METHOD2(init, void((ModelMgrBase *model_mgr_, ApplicationsBase *applications_)));
    MOCK_METHOD1(get_agents_by_type, std::vector<agent_ptr>(std::string agent_type));
};


#endif //OGP_TEST_AGENTS_MOCK_H
