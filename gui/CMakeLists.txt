cmake_minimum_required(VERSION 3.31.0)
project(flashback-gui LANGUAGES CXX DESCRIPTION "Flashback Graphical Client" HOMEPAGE_URL "https://flashback.eu.com")

set(CMAKE_MESSAGE_CONTEXT "gui")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INSTALL_RPATH $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Quick)

# iOS
set(asset_catalog_path ${CMAKE_CURRENT_SOURCE_DIR}/ios/Assets.xcassets)
set_source_files_properties(${asset_catalog_path} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

# macOS
set(app_icon ${CMAKE_CURRENT_SOURCE_DIR}/mac/Assets.xcassets)
set_source_files_properties(${app_icon} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB module_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cppm)
file(GLOB header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/flashback/*.hpp)
file(GLOB qml_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ui/*.qml)
file(GLOB proto_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} proto/*.proto)
list(APPEND test_files ${source_files})
list(REMOVE_ITEM test_files src/main.cpp)

message(NOTICE "Qt Version: ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}")
qt_standard_project_setup(REQUIRES ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR})
qt_add_executable(flashback-gui)
set_target_properties(flashback-gui
    PROPERTIES QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
    MACOSX_BUNDLE_GUI_IDENTIFIER com.eu.flashback
    MACOSX_BUNDLE_BUNDLE_VERSION ${CMAKE_PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
qt_add_qml_module(flashback-gui
    URI flashback
    VERSION ${CMAKE_PROJECT_VERSION}
    QML_FILES ${qml_files}
    RESOURCES ui/flashback.qrc
)
target_sources(flashback-gui
    PRIVATE ${source_files} ${app_icon} ${asset_catalog_path}
    PUBLIC FILE_SET HEADERS FILES ${header_files}
    PUBLIC FILE_SET modules TYPE CXX_MODULES FILES ${module_files}
)
target_link_libraries(flashback-gui PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Quick)
define_tests(TARGET gui FILES ${test_files} MOCK_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/test/mocks)
set(tests ${tests};gui PARENT_SCOPE)

install(TARGETS flashback-gui
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PACKAGE_DESCRIPTION}")
include(CPack)
