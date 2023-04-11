cmake_minimum_required(VERSION 3.13)

enable_testing()

set(BENCHMARK_ENABLE_TESTING off)

set(BENCHMARK_DIR ${PROJECT_SOURCE_DIR}/thirdparty/benchmark)

add_subdirectory(${BENCHMARK_DIR} ${CMAKE_BINARY_DIR}/benchmerk)

set(BENCHMARK_LIBRARIES benchmark::benchmark)

file(GLOB_RECURSE BENCHMARK_SOURCE_LIST ./*.cpp)

add_executable(benchmarks ${BENCHMARK_SOURCE_LIST} ../include/parsecpp/core/baseTypes.h ../include/parsecpp/core/concept.h)

target_include_directories(benchmarks PRIVATE
        ${PARSECPP_INCLUDE_DIR}
        ${BENCHMARK_DIR}/include)

target_link_libraries(benchmarks ${BENCHMARK_LIBRARIES})

add_custom_command(
        TARGET benchmarks POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/data/*
        ${CMAKE_CURRENT_BINARY_DIR}/)
