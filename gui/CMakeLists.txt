cmake_minimum_required(VERSION 3.31.0)
project(flashback-gui VERSION 0.3 LANGUAGES CXX DESCRIPTION "Flashback Graphical Client" HOMEPAGE_URL "https://flashback.eu.com")

set(CMAKE_MESSAGE_CONTEXT "gui")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INSTALL_RPATH $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

find_package(Qt6 6.10.0 REQUIRED COMPONENTS Core Quick)

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

qt_standard_project_setup(REQUIRES 6.9.0)
qt_add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}
    PROPERTIES QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
    MACOSX_BUNDLE_GUI_IDENTIFIER com.eu.flashback
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
qt_add_qml_module(${PROJECT_NAME}
    URI flashback
    VERSION ${PROJECT_VERSION}
    QML_FILES ${qml_files}
    RESOURCES ui/flashback.qrc
)
target_sources(${PROJECT_NAME}
    PRIVATE ${source_files} ${app_icon} ${asset_catalog_path}
    PUBLIC FILE_SET HEADERS FILES ${header_files}
    PUBLIC FILE_SET modules TYPE CXX_MODULES FILES ${module_files}
)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Quick)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PACKAGE_DESCRIPTION}")
include(CPack)
