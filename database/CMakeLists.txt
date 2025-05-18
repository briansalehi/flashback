cmake_minimum_required(VERSION 4.0.0)

project("Flashback Database"
    VERSION 0.1
    LANGUAGES CXX
    DESCRIPTION "A program to transform complex technical subjects into muscle memory"
    HOMEPAGE_URL "flashback.eu.com"
)

include(GNUInstallDirs)

find_package(GTest 1.15.0 REQUIRED)
find_package(Boost 1.83.0 CONFIG REQUIRED COMPONENTS program_options)
#find_package(PostgreSQL REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
add_library(flashback-database STATIC)
target_sources(flashback-database PUBLIC ${source_files})
target_include_directories(flashback-database PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(flashback-database PUBLIC Boost::program_options pq pqxx)

enable_testing()

file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
add_executable(flashback-database-test ${test_files})
target_link_libraries(flashback-database-test PRIVATE gtest gtest_main flashback-database)
add_test(NAME flashback-database-test COMMAND flashback-database-test)
