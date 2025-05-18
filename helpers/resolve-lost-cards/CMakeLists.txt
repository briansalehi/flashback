cmake_minimum_required(VERSION 4.0.0)
project("Resolve Lost Cards" LANGUAGES CXX VERSION 1.0 DESCRIPTION "Temporary subdirectory to resolve lost cards")

find_package(Boost 1.83.0 CONFIG REQUIRED COMPONENTS system)
find_package(PostgreSQL REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

add_executable(resolve-lost-cards)
target_sources(resolve-lost-cards PRIVATE ${source_files})
target_include_directories(resolve-lost-cards PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(resolve-lost-cards PRIVATE Boost::system pq pqxx)
