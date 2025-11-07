cmake_minimum_required(VERSION 3.31.0)

project(flashback-cli
    VERSION 0.2
    LANGUAGES CXX
    DESCRIPTION "A program to transform complex technical subjects into muscle memory"
    HOMEPAGE_URL "https://flashback.eu.com"
)

set(CMAKE_MESSAGE_CONTEXT "cli")

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# dependencies
find_package(Boost 1.83.0 REQUIRED COMPONENTS program_options)
find_package(ftxui 5.0.0 COMPONENTS screen dom component)
find_package(GTest 1.15.0)

if(NOT ftxui_FOUND)
    message(STATUS "ftxui library falls back to cloning with git")
    FetchContent_Declare(ftxui GIT_REPOSITORY https://github.com/arthursonzogni/ftxui GIT_TAG v6.1.9)
    FetchContent_MakeAvailable(ftxui)
endif()

enable_testing()

# configurations
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_RPATH $<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>)
string(TOLOWER "${CMAKE_SYSTEM_NAME}" CPACK_SYSTEM_NAME)
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME}")
set(CPACK_PACKAGE_VENDOR "Brian Salehi <briansalehi@proton.me>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RPM_PACKAGE_REQUIRES
    "g++ >= 5.1,
    boost-devel >= 1.83.0,
    ftxui-devel >= 5.0.0,
    grpc-devel >= 1.48.0,
    protobuf-compiler >= 3.19.0"
)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "g++ (>= 5.1),
	libboost-dev (>= 1.83.0),
	ftxui-dev (>= 5.0.0),
	libgrpc-dev (>= 1.48.0),
	protobuf-compiler (>= 3.19.0)"
)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
list(APPEND test_files ${source_files})
list(REMOVE_ITEM test_files src/main.cpp)

set(proto_files
    ${CMAKE_SOURCE_DIR}/common/proto/types.proto
    ${CMAKE_SOURCE_DIR}/server/proto/server.proto
)

set(client_sources
    ${CMAKE_CURRENT_BINARY_DIR}/server.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/server.pb.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/server.grpc.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/server.grpc.pb.cpp
)

# client executable
add_executable(flashback ${source_files} ${header_files})
target_include_directories(flashback
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(flashback PRIVATE common Boost::program_options ftxui::screen ftxui::dom ftxui::component)
target_compile_definitions(flashback PRIVATE PROGRAM_VERSION="${PROJECT_VERSION}")

# client test
add_executable(flashback-test ${test_files})
target_include_directories(flashback-test PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(flashback-test PRIVATE common Boost::program_options ftxui::screen ftxui::dom ftxui::component gtest gtest_main)
target_compile_definitions(flashback-test PRIVATE PROGRAM_VERSION="${PROJECT_VERSION}")
add_test(NAME flashback-test COMMAND flashback-test)

# installation
write_basic_package_version_file(${CMAKE_BINARY_DIR}/FlashbackCliVersion.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(TARGETS flashback RUNTIME CONFIGURATIONS Release MinSizeRel COMPONENT client)
install(FILES ${CMAKE_BINARY_DIR}/FlashbackCliVersion.cmake CONFIGURATIONS Release MinSizeRel COMPONENT client DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake)

# packaging
include(CPack)
