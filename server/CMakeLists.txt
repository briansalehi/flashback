cmake_minimum_required(VERSION 3.31.0)

project(flashbackd
    VERSION 0.2
    LANGUAGES CXX
    DESCRIPTION "A program to transform complex technical subjects into muscle memory"
    HOMEPAGE_URL "https://flashback.eu.com"
)

set(CMAKE_MESSAGE_CONTEXT "server")

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

FetchContent_Declare(pqxx GIT_REPOSITORY https://github.com/jtv/libpqxx GIT_TAG 7.10.3)

find_package(Boost 1.83.0 CONFIG REQUIRED COMPONENTS system program_options)
find_package(GTest 1.15.0)
find_library(sodium NAMES sodium REQUIRED)
find_path(SODIUM_INCLUDE_DIR sodium)

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
    libpq-devel >= 16.0.0,
    libsodium-devel >= 1.0.0,
    grpc-devel >= 1.48.0,
    protobuf-compiler >= 3.19.0"
)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libboost-system-dev (>= 1.83.0),
	libboost-program-options-dev (>= 1.83.0),
    libpq-dev (>= 16.0.0),
    libsodium-dev (>= 1.0.0),
	libgrpc-dev (>= 1.48.0),
	protobuf-compiler (>= 3.19.0)"
)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)

FetchContent_MakeAvailable(pqxx)

add_executable(flashbackd ${source_files} ${server_sources})
target_include_directories(flashbackd
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(flashbackd PRIVATE common pqxx pq sodium Boost::system Boost::program_options)

enable_testing()

list(APPEND test_files ${source_files})
list(REMOVE_ITEM test_files src/main.cpp)

add_executable(flashbackd-test ${test_files})
target_include_directories(flashbackd-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(flashbackd-test PRIVATE common pq pqxx sodium Boost::system Boost::program_options gtest gtest_main)
add_test(NAME flashbackd-test COMMAND flashbackd-test)

install(TARGETS flashbackd RUNTIME CONFIGURATIONS Release MinSizeRel COMPONENT server)
install(FILES res/flashbackd.service CONFIGURATIONS Release MinSizeRel COMPONENT server DESTINATION /usr/local/lib/systemd/system)
