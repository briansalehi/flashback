cmake_minimum_required(VERSION 3.31)
project(client LANGUAGES CXX DESCRIPTION "Client library linked to user interfaces")

set(CMAKE_MESSAGE_CONTEXT "client")

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
list(REMOVE_ITEM source_files src/main.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/units/*.cpp)
file(GLOB mock_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/mocks/flashback/*.hpp)

add_library(client STATIC)
target_sources(client PUBLIC ${source_files})
target_link_libraries(client PUBLIC common)
target_include_directories(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(client PUBLIC cxx_std_23)
target_compile_definitions(client PRIVATE LIBRARY_VERSION="${CMAKE_PROJECT_VERSION}")
define_tests(TARGET client FILES "${test_files}" MOCK_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/test/mocks")
