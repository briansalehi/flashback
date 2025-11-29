cmake_minimum_required(VERSION 3.31)

function(define_tests)
    set(options EXECUTABLE LIBRARY)
    set(one_value NAME TARGET)
    set(multi_value SOURCES INCLUDE_DIRS)
    cmake_parse_arguments(TEST_MODULE "${options}" "${one_value}" "${multi_value}" ${ARGN})

    message(DEBUG "Unit test ${TEST_MODULE_NAME}")

    if(NOT TARGET ${TEST_MODULE_TARGET})
        message(FATAL_ERROR "${TEST_MODULE_TARGET} is not a target, but define_tests macro requires an executable or a library target")
    endif()

    get_target_property(dependencies ${TEST_MODULE_TARGET} LINK_LIBRARIES)

    # defines an individual target for each unit test
    foreach(test_file ${TEST_MODULE_SOURCES})
        get_filename_component(test_unit ${test_file} NAME_WE)
        add_executable(${test_unit} ${test_file})
        target_include_directories(${test_unit} PRIVATE ${TEST_MODULE_INCLUDE_DIRS})

        if(TEST_MODULE_LIBRARY)
            target_link_libraries(${test_unit} PRIVATE ${TEST_MODULE_TARGET} ${dependencies} gtest gtest_main)
        elseif(TEST_MODULE_EXECUTABLE)
            target_link_libraries(${test_unit} PRIVATE ${dependencies} gtest gtest_main)
        else()
            message(FATAL_ERROR "Test module type is not correct or not supported")
        endif()

        add_test(NAME ${test_unit} COMMAND ${test_unit})
        list(APPEND unit_tests ${test_unit})
    endforeach()

    # defines a target running all unit tests in this module
    add_executable(${TEST_MODULE_TARGET}-tests)
    target_sources(${TEST_MODULE_TARGET}-tests PRIVATE ${TEST_MODULE_SOURCES})
    target_include_directories(${TEST_MODULE_TARGET}-tests PRIVATE ${TEST_MODULE_INCLUDE_DIRS})

    if(TEST_MODULE_LIBRARY)
        target_link_libraries(${TEST_MODULE_TARGET}-tests PRIVATE ${TEST_MODULE_TARGET} ${dependencies} gtest gtest_main)
    elseif(TEST_MODULE_EXECUTABLE)
        target_link_libraries(${TEST_MODULE_TARGET}-tests PRIVATE ${dependencies} gtest gtest_main)
    endif()

    add_test(NAME ${TEST_MODULE_TARGET}-tests COMMAND ${TEST_MODULE_TARGET}-tests)
endfunction()