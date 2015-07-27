# @file /tip-server/test/db/CMakeLists.txt
#    @author: zmij
# Created on: Jul 10, 2015

cmake_minimum_required(VERSION 2.6)

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
    pg-tests.cpp
    db_io_tests.cpp
    query_tests.cpp
    test_main.cpp
    test-environment.cpp
)

set(PGTEST test-pg-async)

add_executable( ${PGTEST} ${test_pg_SRCS})

target_link_libraries(
    ${PGTEST} 
    ${Boost_PROGRAM_OPTIONS_LIBRARIES}
    ${GTEST_LIBRARIES}
    tip-pgsql
)

if (TEST_DATABASE)
    set(TEST_ARGS
        -b'${TEST_DATABASE}' -s2 -x8 -q10 --run-deadline=1
    )
else()
    set(TEST_ARGS)
endif()

add_test(
    NAME pg-async-test
    COMMAND ${PGTEST} ${TEST_ARGS}
)


add_dependencies(${PGTEST} test-sql-scripts)

add_executable(test-extended-query-mode extended_query_mode.cpp)
target_link_libraries(test-extended-query-mode   
    tip-pgsql
    tip-log
)

