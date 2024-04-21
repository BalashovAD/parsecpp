cmake_minimum_required(VERSION 3.22)
project(parsercpp)

set(CMAKE_CXX_STANDARD 20)

option(Parsecpp_DisableError   "Disable error" OFF)

option(Parsecpp_EnableHardBenchmark   "Enable hard benchmark" OFF)
option(Parsecpp_Sanitizer   "Enable sanitizer" OFF)
option(Parsecpp_SingleInclude   "Use single include file" OFF)
option(Parsecpp_AnnotateAssembler   "Annotate assembler -g3" OFF)

if(${CCACHE_FOUND})
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments -fcolor-diagnostics")
    endif()
endif()


if (${Parsecpp_AnnotateAssembler})
    message("Use -g3 to annotate assembler")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
endif ()

if (${Parsecpp_SingleInclude})
    message("Use single_include")
    set(PARSECPP_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/single_include)
else()
    message("Use include")
    set(PARSECPP_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
endif()

file(GLOB_RECURSE HEADER_LIST ${PARSECPP_INCLUDE_DIR}/*.h)

if (${Parsecpp_DisableError})
    message("Disable error log")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRS_DISABLE_ERROR_LOG")
else ()
    message("Build with error log")
endif ()

if (${Parsecpp_Sanitizer})
    message("Add sanitizer")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
    add_compile_options(-fsanitize=leak)
    add_link_options(-fsanitize=leak)
endif ()

add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(benchmarks)