# @file /tip-server/src/tip/db/pg/CMakeLists.txt
#    @author: zmij
# Created on: Jul 9, 2015

cmake_minimum_required(VERSION 2.6)

set(
    pgsql_lib_SRCS
    common.cpp
    connection.cpp
    database.cpp
    error.cpp
    resultset.cpp
    query.cpp
    sqlstates.cpp
    detail/md5.cpp
    detail/protocol.cpp
    detail/basic_connection.cpp
    detail/connection_impl.cpp
    detail/basic_state.cpp
    detail/startup.cpp
    detail/idle_state.cpp
    detail/simple_query_state.cpp
    detail/transaction_state.cpp
    detail/terminated_state.cpp
    detail/result_impl.cpp
    detail/database_impl.cpp
    detail/connection_pool.cpp
    detail/connection_lock.cpp
)

add_library(tip-pgsql SHARED ${pgsql_lib_SRCS})
set_target_properties(
    tip-pgsql PROPERTIES
    VERSION 0.0.0
    SOVERSION 0
)

set(pgsql_lib_LIBRARIES
    ${Boost_SYSTEM_LIBRARIES}
    ${Boost_THREAD_LIBRARIES}
    ${Boost_REGEX_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TIP_LOG_LIBRARY}
)

message(STATUS ${pgsql_lib_LIBRARIES})

target_link_libraries(tip-pgsql ${pgsql_lib_LIBRARIES})

add_executable(pg-async-test pg_async.cpp)

target_link_libraries(pg-async-test 
    tip-pgsql
)
