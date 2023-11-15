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

if(${Parsecpp_HugePagesBenchmark})
    target_link_options(benchmarks PRIVATE "-B/hugetlbfs")
endif ()

if (Parsecpp_EnableHardBenchmark)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_HARD_BENCHMARK")
endif()

target_link_libraries(benchmarks ${BENCHMARK_LIBRARIES})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_custom_command(
            TARGET benchmarks POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/data/*
            ${CMAKE_CURRENT_BINARY_DIR}/)
else ()
    # -E copy src/* dest doesn't work in windows

    set(BENCHMARK_DATA_FILES 100k.json binance.json canada.json colors.css tags.txt 5mb.json 5mb-min.json 64kb.json 64kb-min.json 256kb.json 256kb-min.json)

    foreach(file_i ${BENCHMARK_DATA_FILES})
        add_custom_command(
                TARGET benchmarks POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/data/${file_i}
                ${CMAKE_CURRENT_BINARY_DIR}/)
    endforeach(file_i)
endif ()

