# @file /tip-server/test/db/CMakeLists.txt
#    @author: zmij
# Created on: Jul 10, 2015

cmake_minimum_required(VERSION 2.6)

option(TEST_PG_ASYNC_FSM "Build test for pg_async FSM" OFF)

configure_file(config.in.hpp config.hpp)

# Test SQL script files
set (test_pg_SQL results-parse-test.sql)

foreach(sql_file ${test_pg_SQL})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${sql_file}
        DEPENDS ${sql_file}
        COMMENT "Copy test SQL script ${sql_file}"
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/${sql_file} ${CMAKE_CURRENT_BINARY_DIR}/${sql_file}
    )
endforeach()

add_custom_target(test-sql-scripts
    DEPENDS ${test_pg_SQL}
    COMMENT "Copy test SQL scripts"
)

set(test_pg_SRCS
    test_main.cpp
    test-environment.cpp
    internals_tests.cpp
    db_io_tests.cpp
    query_tests.cpp
    errors_tests.cpp
    array_support_test.cpp
    timestamp_io_test.cpp
    uuid_io_test.cpp
)

if(TEST_PG_ASYNC_FSM)
    list(APPEND test_pg_SRCS fsm_tests.cpp)
endif()

set(PGTEST test-pg-async)

add_executable( ${PGTEST} ${test_pg_SRCS})

target_link_libraries(
    ${PGTEST}
    ${Boost_PROGRAM_OPTIONS_LIBRARIES}
    ${GTEST_LIBRARIES}
    ${PGASYNC_LIB_NAME}
)

if (TEST_DATABASE)
    set(DB_TEST_ARGS
        "-d${TEST_DATABASE}" -s2 -x4 -q10 --run-deadline=1
    )
else()
    set(DB_TEST_ARGS)
endif()

if (GTEST_XML_OUTPUT)
    set (
        TEST_ARGS ${TEST_ARGS}
        --gtest_output=xml:test-pg-async-detail.xml
    )
endif()


add_test(
    NAME test-pg-async
    COMMAND ${PGTEST} ${DB_TEST_ARGS} ${TEST_ARGS}
)


add_dependencies(${PGTEST} test-sql-scripts)

if(WITH_BOOST_FIBER)

set(fiber_test_SRCS
    test_main.cpp
    test-environment.cpp
    fiber_query_test.cpp
)
add_executable(test-pg-fiber ${fiber_test_SRCS})
target_link_libraries(
    test-pg-fiber
    ${Boost_PROGRAM_OPTIONS_LIBRARIES}
    ${GTEST_LIBRARIES}
    ${PGFIBER_LIB_NAME}
)

if (GTEST_XML_OUTPUT)
    set (
        FIBER_TEST_ARGS
        --gtest_output=xml:test-pg-fiber-detail.xml
    )
endif()


add_test(
    NAME test-pg-fiber
    COMMAND test-pg-fiber ${DB_TEST_ARGS} ${FIBER_TEST_ARGS}
)


endif()

