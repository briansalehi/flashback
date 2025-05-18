cmake_minimum_required(VERSION 4.0.0)

project(Flashback
    VERSION 0.1
    LANGUAGES CXX
    DESCRIPTION "A program to transform complex technical subjects into muscle memory"
    HOMEPAGE_URL "flashback.eu.com"
)

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# dependencies
find_package(Boost 1.83.0 REQUIRED COMPONENTS program_options)
find_package(ftxui 5.0.0 CONFIG REQUIRED COMPONENTS component dom screen)
#find_package(Protobuf 3.19.0 REQUIRED)
find_package(gRPC 1.48.0 REQUIRED)
find_package(GTest 1.15.0)

# configurations
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_RPATH $<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PROJECT_NAME "flashback-cli")
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
string(TOLOWER CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
file(GLOB proto_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} proto/*.proto)
list(APPEND test_files ${source_files})
list(REMOVE_ITEM test_files src/main.cpp)

# client executable
add_executable(flashback ${source_files} ${header_files})
#protobuf_generate(TARGET flashback PROTOS ${proto_files})
target_include_directories(flashback
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(flashback PRIVATE Boost::program_options ftxui::component ftxui::dom ftxui::screen)
target_compile_definitions(flashback PRIVATE PROGRAM_VERSION="${PROJECT_VERSION}")

# client test
enable_testing()
add_executable(flashback-test ${test_files})
#protobuf_generate(TARGET flashback-test PROTOS ${proto_files})
target_include_directories(flashback-test PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(flashback-test PRIVATE Boost::program_options gtest gtest_main)
target_compile_definitions(flashback-test PRIVATE PROGRAM_VERSION="${PROJECT_VERSION}")
add_test(NAME flashback-test COMMAND flashback-test)

# installation
write_basic_package_version_file(${CMAKE_BINARY_DIR}/FlashbackCliVersion.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(TARGETS flashback RUNTIME)
install(FILES ${CMAKE_BINARY_DIR}/FlashbackCliVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake)

# packaging
include(CPack)
