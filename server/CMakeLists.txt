cmake_minimum_required(VERSION 4.0.0)
project("Flashback Daemon" VERSION 0.1 LANGUAGES CXX DESCRIPTION "Technical reference training program")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Boost 1.83.0 CONFIG REQUIRED COMPONENTS system program_options)
find_package(Protobuf 3.19.0 REQUIRED)
find_package(gRPC 1.48.0 REQUIRED)
find_package(GTest 1.15.0 REQUIRED)
find_package(PostgreSQL 7.10 REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_RPATH $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_BINDIR}>)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PACKAGE_DESCRIPTION}")
set(CPACK_PACKAGE_NAME "flashback-server")
set(CPACK_RPM_PACKAGE_REQUIRES
    "boost-devel >= 1.83.0,
    libpqxx-devel >= 7.10,
    grpc-devel >= 1.48.0,
    protobuf-compiler >= 3.19.0,
    libargon2-devel >= 20190702,
    gtest-devel >= 1.15.0"
)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libboost-system-dev (>= 1.83.0),
	libboost-program-options-dev (>= 1.83.0),
    libpqxx-dev >= 7.10,
	libgrpc-dev (>= 1.48.0),
	protobuf-compiler (>= 3.19.0),
    libargon2-dev >= 20190702,
	libgtest-devel (>= 1.15.0)"
)
string(TOLOWER CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)

add_executable(flashback-daemon)
target_sources(flashback-daemon private ${source_files})
target_include_directories(flashback-daemon private ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(flashback-daemon private Boost::system Boost::program_options pq pqxx argon2)

enable_testing()
list(APPEND test_files ${source_files})
list(REMOVE_ITEM test_files src/main.cpp)
add_executable(test-flashback-daemon)
target_sources(test-flashback-daemon private ${test_files})
target_include_directories(test-flashback-daemon private ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(test-flashback-daemon private Boost::system Boost::program_options pq pqxx argon2 gtest gtest_main)
add_test(NAME test-flashback-daemon COMMAND test-flashback-daemon)

install(TARGETS flashback-daemon RUNTIME CONFIGURATIONS Release MinSizeRel)