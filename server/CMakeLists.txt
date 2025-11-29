cmake_minimum_required(VERSION 3.31.0)

project(flashbackd
    VERSION 0.2
    LANGUAGES CXX
    DESCRIPTION "A program to transform complex technical subjects into muscle memory"
    HOMEPAGE_URL "https://flashback.eu.com"
)

set(CMAKE_MESSAGE_CONTEXT "server")

enable_testing()

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Boost 1.83.0 CONFIG REQUIRED COMPONENTS system program_options)
find_package(GTest 1.15.0)
find_library(sodium NAMES sodium REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
string(TOLOWER "${CMAKE_SYSTEM_NAME}" CPACK_SYSTEM_NAME)
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME}")
set(CPACK_PACKAGE_VENDOR "Brian Salehi <briansalehi@proton.me>")
set(CPACK_PACKAGE_CONTACT "Brian Salehi <briansalehi@proton.me>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RPM_PACKAGE_REQUIRES "boost >= 1.83.0 libsodium >= 1.0.0 grpc >= 1.48.0")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-system (>= 1.83.0), libboost-program-options (>= 1.83.0), libsodium (>= 1.0.0), libgrpc (>= 1.48.0)")

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/units/*.cpp)
file(GLOB mock_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/mocks/flashback/*.hpp)

add_executable(flashbackd ${source_files} ${server_sources})
target_include_directories(flashbackd PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(flashbackd PRIVATE common database sodium Boost::system Boost::program_options)
target_compile_definitions(flashbackd PRIVATE INSTALL_RPATH="$ORIGIN")

define_tests(
    EXECUTABLE
    NAME server
    TARGET flashbackd
    SOURCES "${test_files}"
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/test/mocks
)

install(TARGETS flashbackd RUNTIME CONFIGURATIONS Release MinSizeRel COMPONENT server)
install(FILES res/flashbackd.service CONFIGURATIONS Release MinSizeRel COMPONENT server DESTINATION ${CMAKE_INSTALL_LIBDIR}/lib/systemd/system)
