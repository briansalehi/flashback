cmake_minimum_required(VERSION 4.1)
project(Flashback LANGUAGES NONE DESCRIPTION "Cross platform Flashback application")

find_program(JAVA_EXEC java REQUIRED)
find_program(CARGO_EXEC cargo REQUIRED)

execute_process(COMMAND ${JAVA_EXEC} --version OUTPUT_VARIABLE JAVA_VERSION_RAW OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" JAVA_VERSION ${JAVA_VERSION_RAW})

execute_process(COMMAND ${CARGO_EXEC} --version OUTPUT_VARIABLE CARGO_VERSION_RAW OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" CARGO_VERSION ${CARGO_VERSION_RAW})

if(JAVA_VERSION VERSION_LESS "21.0.0")
    message(FATAL_ERROR "java 21.0.0 or more recent is required but found java ${JAVA_VERSION}")
else()
    message(STATUS "Found Java ${JAVA_VERSION}: ${JAVA_EXEC}")
endif()

if(CARGO_VERSION VERSION_LESS "1.94.0")
    message(FATAL_ERROR "cargo 1.94.0 or more recent is required  but found cargo ${CARGO_VERSION}")
else()
    message(STATUS "Found cargo ${CARGO_VERSION}: ${CARGO_EXEC}")
endif()

if(EXISTS $ENV{JAVA_HOME})
    message(STATUS "JAVA_HOME: $ENV{JAVA_HOME}")
else()
    message(FATAL_ERROR "JAVA_HOME is not set")
endif()

if(EXISTS $ENV{ANDROID_HOME})
    message(STATUS "ANDROID_HOME: $ENV{ANDROID_HOME}")
else()
    message(FATAL_ERROR "ANDROID_HOME is not set")
endif()

if(EXISTS $ENV{NDK_HOME})
    message(STATUS "NDK_HOME: $ENV{NDK_HOME}")
else()
    message(FATAL_ERROR "NDK_HOME is not set")
endif()

add_custom_target(linux
    COMMAND ${CARGO_EXEC} tauri build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Linux desktop application"
    USES_TERMINAL
    VERBATIM
)

add_custom_target(windows
    COMMAND ${CARGO_EXEC} tauri build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src-tauri
    COMMENT "Building Windows desktop application"
    USES_TERMINAL
    VERBATIM
)

add_custom_target(macos
    COMMAND ${CARGO_EXEC} tauri build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src-tauri
    COMMENT "Building Mac desktop application"
    USES_TERMINAL
    VERBATIM
)

add_custom_target(android
    COMMAND
        PATH=$ENV{PATH}
        JAVA_HOME=$ENV{JAVA_HOME}
        ANDROID_HOME=$ENV{ANDROID_HOME}
        NDK_HOME=$ENV{NDK_HOME}
        ${CARGO_EXEC} tauri android build --debug
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Android application"
    USES_TERMINAL
    VERBATIM
)

file(GLOB artifacts RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    app/src-tauri/target/release/bundle/deb/*.deb
    app/src-tauri/target/release/bundle/rpm/*.rpm
    app/src-tauri/target/release/bundle/appimage/*.AppImage
)

install(FILES ${artifacts} COMPONENT client DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
