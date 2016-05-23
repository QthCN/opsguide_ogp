cmake_minimum_required(VERSION 3.5)
project(ogp)

include_directories("./src")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(Boost 1.60.0 COMPONENTS regex system date_time program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

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
        src/service/message.h)
add_executable(ogp src/cli/controller.cpp ${CONTROLLER_SOURCE_FILES})
target_link_libraries(ogp ${Boost_LIBRARIES})

# test
add_executable(opg_test tests/common_tests.cpp ${CONTROLLER_SOURCE_FILES})
target_link_libraries(opg_test ${GTEST_BOTH_LIBRARIES})
target_link_libraries(opg_test ${Boost_LIBRARIES})