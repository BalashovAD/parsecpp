cmake_minimum_required(VERSION 3.13)

add_executable(calc calc.cpp)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU") # GCC
    message("Turn on opt-info (GCC)")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fopt-info-vec -fopt-info-vec-missed -fopt-info-inline")
    #  -fopt-info-optall -fopt-info
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")  # Clang
    message("Turn on opt-info (Clang)")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Rpass-analysis=.* -Rpass=.* -Rpass-missed=.*")
else ()
    message("Unknown compiler ${CMAKE_CXX_COMPILER_ID}")
endif ()

target_include_directories(calc PRIVATE
        ${PARSECPP_INCLUDE_DIR})
