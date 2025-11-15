cmake_minimum_required(VERSION 3.20)
project(bearlang_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/app)

file(GLOB_RECURSE CORE_SOURCES
    ${SRC_DIR}/core/lexer/*.cpp
    ${SRC_DIR}/core/parser/*.cpp
    ${SRC_DIR}/core/codegen/*.cpp
)

add_executable(bearlang_app
    ${SRC_DIR}/app.cpp
    ${CORE_SOURCES}
)

target_include_directories(bearlang_app PRIVATE ${SRC_DIR})

target_compile_features(bearlang_app PRIVATE cxx_std_20)
