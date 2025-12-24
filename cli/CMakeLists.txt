cmake_minimum_required(VERSION 3.31.0)

project(flashback-cli
    VERSION 0.3
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
find_package(GTest 1.15.0)

FetchContent_Declare(ftxui GIT_REPOSITORY https://github.com/arthursonzogni/ftxui GIT_TAG v6.1.9)
FetchContent_MakeAvailable(ftxui)

enable_testing()

# configurations
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
string(TOLOWER "${CMAKE_SYSTEM_NAME}" CPACK_SYSTEM_NAME)
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME}")
set(CPACK_PACKAGE_VENDOR "Brian Salehi <briansalehi@proton.me>")
set(CPACK_PACKAGE_CONTACT "Brian Salehi <briansalehi@proton.me>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RPM_PACKAGE_REQUIRES "boost >= 1.83.0 ftxui >= 5.0.0 grpc >= 1.48.0 libpq >= 18.0.0 libpqxx >= 7.0.0")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost (>= 1.83.0), ftxui (>= 5.0.0), libgrpc (>= 1.48.0), libpq (>= 18.0.0), libpqxx (>= 7.0.0)")

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
list(REMOVE_ITEM source_files src/main.cpp)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/units/*.cpp)
file(GLOB mock_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/mocks/flashback/*.hpp)

# client backend
add_library(client STATIC)
target_sources(client PUBLIC ${source_files})
target_include_directories(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(client PUBLIC common Boost::program_options ftxui::screen ftxui::dom ftxui::component)
target_compile_features(client PUBLIC cxx_std_23)

# client executable
add_executable(flashback)
target_sources(flashback PRIVATE src/main.cpp)
target_link_libraries(flashback PRIVATE client)

# client test
define_tests(TARGET client FILES ${test_files} MOCK_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/test/mocks)

# installation
write_basic_package_version_file(${CMAKE_BINARY_DIR}/FlashbackCliVersion.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(TARGETS flashback RUNTIME CONFIGURATIONS Release MinSizeRel COMPONENT client)
install(FILES ${CMAKE_BINARY_DIR}/FlashbackCliVersion.cmake CONFIGURATIONS Release MinSizeRel COMPONENT client DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake)

# packaging
include(CPack)
