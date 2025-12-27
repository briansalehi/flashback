cmake_minimum_required(VERSION 3.31.0)
project(database LANGUAGES CXX DESCRIPTION "Flashback Database" HOMEPAGE_URL "https://flashback.eu.com")

set(CMAKE_MESSAGE_CONTEXT "database")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(FetchContent)
include(CTest)

find_package(PostgreSQL 16.0.0 REQUIRED)
find_package(GTest 1.15.0)
find_library(pqxx 7.0.0 NAMES pqxx REQUIRED)
find_path(pqxx_INCLUDE_DIRS pqxx NAMES pqxx)

#FetchContent_Declare(pqxx GIT_REPOSITORY https://github.com/jtv/libpqxx GIT_TAG 7.10.4)
#FetchContent_MakeAvailable(pqxx)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/units/*.cpp)
file(GLOB mock_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/mocks/flashback/*.hpp)

add_library(database STATIC)
target_sources(database
    PRIVATE ${source_files}
    PUBLIC FILE_SET HEADERS BASE_DIRS include test/mocks NAMESPACE flashback:: FILES ${header_files} ${mock_files}
)
target_include_directories(database PUBLIC ${PQXX_INCLUDE_DIR} test/mocks)
target_link_libraries(database PUBLIC common pq pqxx)
target_compile_features(database PUBLIC cxx_std_23)
define_tests(TARGET database FILES ${test_files} MOCK_DIRS test/mocks)
