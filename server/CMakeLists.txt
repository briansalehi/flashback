cmake_minimum_required(VERSION 4.0.0)

project(flashbackd
    VERSION 0.2
    LANGUAGES CXX
    DESCRIPTION "Flashback service"
    HOMEPAGE_URL "https://flashback.eu.com"
)

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Boost 1.83.0 CONFIG REQUIRED COMPONENTS system program_options)
find_package(PostgreSQL 7.10 REQUIRED)
find_package(Protobuf 3.19.0 REQUIRED)
find_package(gRPC 1.48.0 REQUIRED)
find_package(GTest 1.15.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_RPATH $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_BINDIR}>)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PACKAGE_DESCRIPTION}")
string(TOLOWER "${CMAKE_SYSTEM_NAME}" CPACK_SYSTEM_NAME)
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME}")
set(CPACK_PACKAGE_VENDOR "Brian Salehi <briansalehi@proton.me>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RPM_PACKAGE_REQUIRES
    "boost-devel >= 1.83.0,
    libpqxx-devel >= 7.10,
    grpc-devel >= 1.48.0,
    protobuf-compiler >= 3.19.0"
)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libboost-system-dev (>= 1.83.0),
	libboost-program-options-dev (>= 1.83.0),
    libpqxx-dev >= 7.10,
	libgrpc-dev (>= 1.48.0),
	protobuf-compiler (>= 3.19.0)"
)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)

add_executable(flashbackd ${source_files})
#target_sources(flashbackd PRIVATE )
target_include_directories(flashbackd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(flashbackd PRIVATE Boost::system Boost::program_options pq pqxx)

enable_testing()
#list(APPEND test_files ${source_files})
#list(REMOVE_ITEM test_files src/main.cpp)
#add_executable(test-flashbackd ${test_files})
##target_sources(test-flashbackd PRIVATE )
#target_include_directories(test-flashbackd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
#target_link_libraries(test-flashbackd PRIVATE Boost::system Boost::program_options pq pqxx gtest gtest_main)
#add_test(NAME test-flashbackd COMMAND test-flashbackd)

install(TARGETS flashbackd RUNTIME CONFIGURATIONS Release MinSizeRel)
