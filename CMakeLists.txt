cmake_minimum_required(VERSION 3.31)
project(Flashback VERSION 0.3 LANGUAGES CXX DESCRIPTION "A program to transform complex technical subjects into muscle memory" HOMEPAGE_URL "https://flashback.eu.com")

include(cmake/test_utils.cmake)

add_subdirectory(common)
add_subdirectory(database)
add_subdirectory(flashbackd)
add_subdirectory(client)
add_subdirectory(flashback)
#add_subdirectory(gui)

add_test(NAME all_tests COMMAND ${tests})
