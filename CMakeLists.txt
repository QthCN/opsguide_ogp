cmake_minimum_required(VERSION 3.5)
project(ogp)

include_directories("./src")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(Boost 1.60.0 COMPONENTS regex system date_time filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

set(SOURCE_FILES src/main.cpp
        src/common/utils.h
        src/common/utils.cpp
        src/service/restful.h
        src/service/restful.cpp
        src/service/rpc.h
        src/service/rpc.cpp
        src/controller/controller.h
        src/controller/controller.cpp
        src/common/config.h
        src/common/config.cpp)
add_executable(ogp ${SOURCE_FILES})
target_link_libraries(ogp ${Boost_LIBRARIES})