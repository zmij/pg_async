# @file /tip-server/src/tip/db/pg/CMakeLists.txt
#    @author: zmij
# Created on: Jul 9, 2015

cmake_minimum_required(VERSION 2.6)

set(
    pgsql_lib_SRCS
    common.cpp
    database.cpp
    error.cpp
    resultset.cpp
    query.cpp
    sqlstates.cpp
    pg_types.cpp
    protocol_io_traits.cpp
    transaction.cpp
    detail/md5.cpp
    detail/protocol.cpp
    detail/protocol_parsers.cpp
    detail/basic_connection.cpp
    detail/transport.cpp
    detail/result_impl.cpp
    detail/database_impl.cpp
    detail/connection_pool.cpp
)

add_library(tip-pgsql SHARED ${pgsql_lib_SRCS})
set_target_properties(
    tip-pgsql PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

set(pgsql_lib_LIBRARIES
    ${ASIO_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TIP_LOG_LIBRARY}
)

target_link_libraries(tip-pgsql ${pgsql_lib_LIBRARIES})

install (
    TARGETS tip-pgsql
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
