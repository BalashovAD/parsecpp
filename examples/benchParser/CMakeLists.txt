cmake_minimum_required(VERSION 3.13)

add_executable(bench-parser bench-parser.cpp)

target_include_directories(bench-parser PRIVATE
        ${PARSECPP_INCLUDE_DIR})
