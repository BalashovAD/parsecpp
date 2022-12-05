cmake_minimum_required(VERSION 3.13)

add_executable(json json.cpp)

target_include_directories(json PRIVATE
        ${PARSECPP_INCLUDE_DIR})
