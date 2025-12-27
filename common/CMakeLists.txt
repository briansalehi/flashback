cmake_minimum_required(VERSION 3.31.0)
project(common LANGUAGES CXX DESCRIPTION "Flashback Base Library" HOMEPAGE_URL "https://flashback.eu.com")

set(CMAKE_MESSAGE_CONTEXT "common")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

find_package(Protobuf 3.19.0 REQUIRED)
find_package(gRPC 1.48.0 REQUIRED)
find_package(GTest 1.15.0)
find_program(grpc_cpp_plugin NAMES grpc_cpp_plugin REQUIRED)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB proto_files ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/units/*.cpp)
file(GLOB mock_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/mocks/flashback/*.hpp)

set(server_headers
    ${CMAKE_CURRENT_BINARY_DIR}/types.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/types.grpc.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/requests.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/requests.grpc.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/server.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/server.grpc.pb.h
)

set(server_sources
    ${CMAKE_CURRENT_BINARY_DIR}/types.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/types.grpc.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/requests.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/requests.grpc.pb.cc
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

add_library(common STATIC)
target_sources(common PUBLIC ${source_files} ${server_sources})
target_sources(common PUBLIC
    FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR} FILES ${server_headers}
    FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES ${header_files}
)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR} ${Protobuf_INCLUDE_DIRS})
target_link_libraries(common PUBLIC protobuf::libprotobuf gRPC::grpc++)
target_compile_features(common PUBLIC cxx_std_23)
target_compile_definitions(common PUBLIC PROGRAM_VERSION="${CMAKE_PROJECT_VERSION}")
set_target_properties(common PROPERTIES CXX_EXTENSIONS OFF)
define_tests(TARGET common FILES ${test_files} MOCK_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/test/mocks)
