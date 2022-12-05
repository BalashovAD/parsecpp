cmake_minimum_required(VERSION 3.13)

add_executable(quick quick.cpp)

target_include_directories(quick PRIVATE
        ${PARSECPP_INCLUDE_DIR})
