cmake_minimum_required(VERSION 3.22)
project(parsercpp)

set(CMAKE_CXX_STANDARD 20)

if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments -fcolor-diagnostics")
    endif()
endif(CCACHE_FOUND)

set(PARSECPP_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE HEADER_LIST ${PARSECPP_INCLUDE_DIR}/*.h)


add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(benchmark)
