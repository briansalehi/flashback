cmake_minimum_required(VERSION 3.31)

function(define_tests)
    set(options)
    set(single_args TARGET)
    set(multi_args FILES MOCK_DIRS)
    cmake_parse_arguments(DEFINE_TESTS "${options}" "${single_args}" "${multi_args}" ${ARGN})

    if(NOT TARGET ${DEFINE_TESTS_TARGET})
        message(FATAL_ERROR "${DEFINE_TESTS_TARGET} is not a target, but define_tests() requires a library target")
    endif()

    # defines an individual target for each unit test
    foreach(test_file ${DEFINE_TESTS_FILES})
        get_filename_component(test_unit ${test_file} NAME_WE)
        add_executable(${test_unit})
        target_sources(${test_unit} PRIVATE ${test_file})
        target_include_directories(${test_unit} PRIVATE ${DEFINE_TESTS_MOCK_DIRS})
        target_link_libraries(${test_unit} PRIVATE ${DEFINE_TESTS_TARGET} gmock gtest gtest_main)
        add_test(NAME ${test_unit} COMMAND ${test_unit})
        list(APPEND unit_tests ${test_unit})
    endforeach()

    # defines a target running all unit tests in this module
    add_executable(${DEFINE_TESTS_TARGET}-tests)
    target_sources(${DEFINE_TESTS_TARGET}-tests PRIVATE ${DEFINE_TESTS_FILES})
    target_include_directories(${DEFINE_TESTS_TARGET}-tests PRIVATE ${DEFINE_TESTS_MOCK_DIRS})
    target_link_libraries(${DEFINE_TESTS_TARGET}-tests PRIVATE ${DEFINE_TESTS_TARGET} gmock gtest gtest_main)
    add_test(NAME ${DEFINE_TESTS_TARGET}-tests COMMAND ${DEFINE_TESTS_TARGET}-tests)
    if(INSTALL_TESTS)
        install(TARGETS ${unit_tests} ${DEFINE_TESTS_TARGET}-tests RUNTIME COMPONENT ${DEFINE_TESTS_TARGET}-tests)
    endif()
endfunction()
