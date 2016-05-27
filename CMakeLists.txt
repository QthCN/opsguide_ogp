cmake_minimum_required(VERSION 3.5)
project(ogp)

include_directories("./src")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(Boost 1.60.0 COMPONENTS regex system date_time program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

find_package(Restclient-cpp)
include_directories(${LIBRESTCLIENT_CPP_INCLUDE_DIRS})

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
        src/controller/base.h)
add_executable(ogp-controller src/cli/controller.cpp ${CONTROLLER_SOURCE_FILES})
target_link_libraries(ogp-controller ${Boost_LIBRARIES})

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
        src/common/docker_client.cpp)
add_executable(ogp-docker-agent src/cli/docker_agent.cpp ${DOCKER_AGENT_SOURCE_FILES})
target_link_libraries(ogp-docker-agent ${Boost_LIBRARIES} ${LIBRESTCLIENT_CPP_LIBRARIES})

# test
add_executable(ogp_test tests/common_tests.cpp ${CONTROLLER_SOURCE_FILES})
target_link_libraries(ogp_test ${GTEST_BOTH_LIBRARIES})
target_link_libraries(ogp_test ${Boost_LIBRARIES})