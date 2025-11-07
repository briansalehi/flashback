cmake_minimum_required(VERSION 4.0.0)

project(common
    VERSION 0.2
    LANGUAGES CXX
    DESCRIPTION "A program to transform complex technical subjects into muscle memory"
    HOMEPAGE_URL "https://flashback.eu.com"
)

set(CMAKE_MESSAGE_CONTEXT "common")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

enable_testing()

find_package(Protobuf 3.19.0 REQUIRED)
find_package(gRPC 1.48.0 REQUIRED)
find_package(GTest 1.15.0)
find_program(grpc_cpp_plugin NAMES grpc_cpp_plugin REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
file(GLOB proto_files ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
list(APPEND test_files ${source_files})

set(server_headers
    ${CMAKE_CURRENT_BINARY_DIR}/types.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/types.grpc.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/server.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/server.grpc.pb.h
)

set(server_sources
    ${CMAKE_CURRENT_BINARY_DIR}/types.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/types.grpc.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/server.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/server.grpc.pb.cc
)

add_custom_command(
    OUTPUT ${server_sources} ${server_headers}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS
        --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        --plugin=protoc-gen-grpc=${grpc_cpp_plugin}
        -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
        -I ${Protobuf_INCLUDE_DIRS}
        ${proto_files}
    DEPENDS
        ${proto_files}
)

add_library(common STATIC ${server_sources} ${server_headers})
target_sources(common PUBLIC
    FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR} FILES ${server_headers}
    FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES ${header_files}
)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR} ${Protobuf_INCLUDE_DIRS})
target_link_libraries(common PUBLIC protobuf::libprotobuf gRPC::grpc++)

add_executable(common-test ${test_files})
target_include_directories(common-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(common-test PRIVATE common gtest gtest_main)
