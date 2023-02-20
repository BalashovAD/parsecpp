cmake_minimum_required(VERSION 3.13)

include(CodeCoverage.cmake)

enable_testing()

set(GTEST_DIR ${PROJECT_SOURCE_DIR}/thirdparty/googletest)

add_subdirectory(${GTEST_DIR} ${CMAKE_BINARY_DIR}/googletest)

set(TEST_LIBRARIES gtest gtest_main)

file(GLOB_RECURSE TEST_SOURCE_LIST ./*.cpp)

add_executable(unit-tests ${TEST_SOURCE_LIST} ${HEADER_LIST})

target_link_libraries(unit-tests PRIVATE ${TEST_LIBRARIES})

if (CODE_COVERAGE)
        target_code_coverage(unit-tests EXCLUDE .*Test.cpp testHelper.h unit.cpp)
        target_compile_options(unit-tests PRIVATE -g -O0 --coverage -fprofile-arcs -ftest-coverage)
endif ()

target_compile_options(unit-tests PRIVATE -Werror -Wall -Wextra -pedantic -Wno-unused-parameter)

target_include_directories(unit-tests PRIVATE
        ${PARSECPP_INCLUDE_DIR}
        ${GTEST_DIR}/googletest/include)

add_test(NAME unit-tests
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

