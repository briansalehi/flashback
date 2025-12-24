cmake_minimum_required(VERSION 3.31.0)

project(Flashback
    VERSION 0.3
    LANGUAGES CXX
    DESCRIPTION "A program to transform complex technical subjects into muscle memory"
    HOMEPAGE_URL "https://flashback.eu.com"
)

include(cmake/test_utils.cmake)

add_subdirectory(common)
add_subdirectory(database)
add_subdirectory(server)
add_subdirectory(cli)
#add_subdirectory(gui)
