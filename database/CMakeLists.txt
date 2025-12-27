cmake_minimum_required(VERSION 3.31.0)

project(database
    VERSION 0.3
    LANGUAGES CXX
    DESCRIPTION "A program to transform complex technical subjects into muscle memory"
    HOMEPAGE_URL "https://flashback.eu.com"
)

set(CMAKE_MESSAGE_CONTEXT "database")

enable_testing()

include(GNUInstallDirs)
include(FetchContent)

find_package(PostgreSQL 16.0.0 REQUIRED)
find_package(GTest 1.15.0)
find_library(pqxx 7.0.0 NAMES pqxx REQUIRED)
find_path(pqxx_INCLUDE_DIRS pqxx NAMES pqxx)

#FetchContent_Declare(pqxx GIT_REPOSITORY https://github.com/jtv/libpqxx GIT_TAG 7.10.4)
#FetchContent_MakeAvailable(pqxx)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
string(TOLOWER "${CMAKE_SYSTEM_NAME}" CPACK_SYSTEM_NAME)
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME}")
set(CPACK_PACKAGE_VENDOR "Brian Salehi <briansalehi@proton.me>")
set(CPACK_PACKAGE_CONTACT "Brian Salehi <briansalehi@proton.me>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RPM_PACKAGE_REQUIRES "")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "")

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
