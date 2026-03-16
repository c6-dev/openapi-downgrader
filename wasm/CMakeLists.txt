cmake_minimum_required(VERSION 3.15)
project(openapi-downgrader-wasm)

include(FetchContent)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        0.8.0
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)

add_executable(downgrader
    ../code/Converter.cpp
    ../code/Util.cpp
    wasm_entry.cpp
)

target_include_directories(downgrader PRIVATE ../include)
target_link_libraries(downgrader PRIVATE yaml-cpp)

set_target_properties(downgrader PROPERTIES
    OUTPUT_NAME "downgrader"
    SUFFIX ".js"
)

target_link_options(downgrader PRIVATE
    --bind
    -O2
    -sWASM=1
    -sALLOW_MEMORY_GROWTH=1
    -sMODULARIZE=1
    -sEXPORT_NAME=createModule
    -sENVIRONMENT=web
)
