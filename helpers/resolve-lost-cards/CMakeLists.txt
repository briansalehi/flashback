cmake_minimum_required(VERSION 3.31.0)
project(ResolveLostCards LANGUAGES CXX VERSION 1.0 DESCRIPTION "Temporary subdirectory to resolve lost cards")

find_package(Boost 1.83.0 CONFIG REQUIRED COMPONENTS system)
find_package(PostgreSQL REQUIRED)

file(GLOB source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

add_executable(resolve-lost-cards)
target_sources(resolve-lost-cards PRIVATE ${source_files})
target_include_directories(resolve-lost-cards PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(resolve-lost-cards PRIVATE Boost::system pq pqxx)
target_compile_features(resolve-lost-cards PRIVATE cxx_std_23)
set_target_properties(resolve-lost-cards PRIVATE CXX_EXTENSIONS OFF)
