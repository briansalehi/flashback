cmake_minimum_required(VERSION 4.0.0)
project("Flashback Library" VERSION 0.1 LANGUAGES CXX DESCRIPTION "Technical reference training program")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
enable_testing()

find_package(Protobuf 3.19.0 REQUIRED)
find_package(gRPC 1.48.0 REQUIRED)
find_package(GTest 1.15.0 REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
file(GLOB proto_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} proto/*.proto)
list(APPEND test_files ${source_files})
list(REMOVE_ITEM test_files src/main.cpp)

