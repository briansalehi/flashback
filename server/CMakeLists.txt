cmake_minimum_required(VERSION 3.31.0)

project(flashbackd
    VERSION 0.3
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
list(REMOVE_ITEM source_files src/main.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/units/*.cpp)
file(GLOB mock_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/mocks/flashback/*.hpp)

# server library
add_library(server)
target_sources(server PUBLIC ${source_files})
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(server PUBLIC common database sodium Boost::system Boost::program_options)
target_compile_definitions(server PUBLIC INSTALL_RPATH="$ORIGIN")
target_compile_features(server PUBLIC cxx_std_23)

# server executable
add_executable(flashbackd)
target_sources(flashbackd PRIVATE src/main.cpp)
target_link_libraries(flashbackd PRIVATE server)

define_tests(TARGET server FILES "${test_files}" MOCK_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/test/mocks)

install(TARGETS flashbackd RUNTIME CONFIGURATIONS Release MinSizeRel COMPONENT server)
install(FILES res/flashbackd.service CONFIGURATIONS Release MinSizeRel COMPONENT server DESTINATION ${CMAKE_INSTALL_LIBDIR}/lib/systemd/system)
