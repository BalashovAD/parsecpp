cmake_minimum_required(VERSION 3.13)

add_executable(calc calc.cpp)

target_include_directories(calc PRIVATE
        ${PARSECPP_INCLUDE_DIR})
