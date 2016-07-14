cmake_minimum_required(VERSION 3.5)
project(ogp)

include_directories("./src")
include_directories("./tests")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.60.0 COMPONENTS regex system date_time program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(CURL REQUIRED)
include_directories($CURL_INCLUDE_DIRS)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

find_package(Restclient-cpp)
include_directories(${LIBRESTCLIENT_CPP_INCLUDE_DIRS})

find_package(Gmock)
include_directories(${GMOCK_INCLUDE_DIRS})

find_package(Mysqlcppconn)
include_directories(${LIBMYSQLCPPCONN_INCLUDE_DIRS})

find_package(Protobuf REQUIRED)
file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/*.proto")
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})
include_directories(${CMAKE_BINARY_DIR})

# controller
set(CONTROLLER_SOURCE_FILES
        src/common/utils.h
        src/common/utils.cpp
        src/controller/controller.h
        src/controller/controller.cpp
        src/common/config.h
        src/common/config.cpp
        src/common/log.h
        src/common/log.cpp
        src/service/controller.h
        src/service/controller.cpp
        src/service/session.h
        src/service/message.h
        src/controller/base.h
        src/model/model.h
        src/model/model.cpp
        src/controller/scheduler.h
        src/controller/scheduler.cpp
        src/controller/utils.cpp
        src/controller/utils.h
        src/controller/ma.h
        src/controller/agents.h
        src/controller/applications.h
        src/controller/service.h)
add_executable(ogp-controller src/cli/controller.cpp ${CONTROLLER_SOURCE_FILES} ${ProtoSources} ${ProtoHeaders})
target_link_libraries(ogp-controller ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} ${LIBMYSQLCPPCONN_LIBRARIES})

# docker agent
set(DOCKER_AGENT_SOURCE_FILES
        src/common/utils.h
        src/common/utils.cpp
        src/common/config.h
        src/common/config.cpp
        src/common/log.h
        src/common/log.cpp
        src/service/session.h
        src/service/message.h
        src/controller/docker_agent.h
        src/controller/docker_agent.cpp
        src/controller/base.h
        src/service/agent.h
        src/service/agent.cpp
        src/common/docker_client.h
        src/common/docker_client.cpp
        src/third/json/json.hpp
        src/controller/ma.h)
add_executable(ogp-docker-agent src/cli/docker_agent.cpp ${DOCKER_AGENT_SOURCE_FILES} ${ProtoSources} ${ProtoHeaders})
target_link_libraries(ogp-docker-agent ${Boost_LIBRARIES} ${LIBRESTCLIENT_CPP_LIBRARIES} ${PROTOBUF_LIBRARY} ${CURL_LIBRARIES})

# sd proxy
set(SD_PROXY_SOURCE_FILES
        src/common/utils.h
        src/common/utils.cpp
        src/common/config.h
        src/common/config.cpp
        src/common/log.h
        src/common/log.cpp
        src/service/session.h
        src/service/message.h
        src/controller/base.h
        src/service/proxy.h
        src/service/proxy.cpp
        src/controller/sd_proxy.h
        src/controller/sd_proxy.cpp
        src/controller/utils.cpp
        src/controller/utils.h
        src/controller/sd_utils.h
        src/controller/sd_utils.cpp)
add_executable(ogp-sd-proxy src/cli/sd_proxy.cpp ${SD_PROXY_SOURCE_FILES} ${ProtoSources} ${ProtoHeaders})
target_link_libraries(ogp-sd-proxy ${Boost_LIBRARIES} ${LIBRESTCLIENT_CPP_LIBRARIES} ${PROTOBUF_LIBRARY} ${CURL_LIBRARIES})

# ogp_cli
set(OGP_CLI_SOURCE_FILES
        src/common/log.h
        src/common/log.cpp
        src/common/utils.h
        src/common/utils.cpp)
add_executable(ogp-cli src/cli/cli.cpp ${OGP_CLI_SOURCE_FILES} ${ProtoSources} ${ProtoHeaders})
target_link_libraries(ogp-cli ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY})

# test
set(OGP_TEST_SOURCE_FILES
        tests/unit/model/model_mock.h
        tests/unit/controller/controller_test.cpp
        tests/unit/controller/applications_mock.h
        tests/unit/controller/agents_mock.h
        tests/unit/service/session_mock.h
        tests/unit/controller/service_mock.h
        tests/unit/controller/service_test.cpp
        tests/unit/controller/sd_proxy_test.cpp
        tests/unit/controller/sd_utils_test.cpp)
add_executable(ogp_test ${OGP_TEST_SOURCE_FILES}
                        ${ProtoSources}
                        ${ProtoHeaders}
                        src/common/utils.h
                        src/common/utils.cpp
                        src/common/config.h
                        src/common/config.cpp
                        src/common/log.h
                        src/common/log.cpp
                        src/service/session.h
                        src/service/message.h
                        src/controller/docker_agent.h
                        src/controller/docker_agent.cpp
                        src/controller/base.h
                        src/service/agent.h
                        src/service/agent.cpp
                        src/common/docker_client.h
                        src/common/docker_client.cpp
                        src/third/json/json.hpp
                        src/controller/ma.h
                        src/model/model.h
                        src/model/model.cpp
                        src/controller/scheduler.h
                        src/controller/scheduler.cpp
                        src/controller/utils.cpp
                        src/controller/utils.h
                        src/controller/controller.h
                        src/controller/controller.cpp
                        src/controller/service.h
                        src/controller/sd_proxy.cpp
                        src/controller/sd_proxy.h
                        src/controller/sd_utils.cpp
                        src/controller/sd_utils.h)
target_link_libraries(ogp_test ${GTEST_BOTH_LIBRARIES})
target_link_libraries(ogp_test ${Boost_LIBRARIES})
target_link_libraries(ogp_test ${LIBRESTCLIENT_CPP_LIBRARIES})
target_link_libraries(ogp_test ${PROTOBUF_LIBRARY})
target_link_libraries(ogp_test ${LIBMYSQLCPPCONN_LIBRARIES})
target_link_libraries(ogp_test ${GMOCK_LIBRARIES})
target_link_libraries(ogp_test ${CURL_LIBRARIES})