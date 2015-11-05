# @file /tip-server/src/tip/db/pg/CMakeLists.txt
#    @author: zmij
# Created on: Jul 9, 2015

cmake_minimum_required(VERSION 2.6)

# Set all log default level
if(NOT PGASYNC_LOG_ALL)
    set(PGASYNC_LOG_ALL OFF)
endif()

set(LOG_CATEGORIES SERVICE CONNECTION QUERY INTERNALS)

foreach(category ${LOG_CATEGORIES})
    if (NOT PGASYNC_LOG_${category})
        set(PGASYNC_LOG_${category} ${PGASYNC_LOG_ALL})
    endif()
endforeach()

configure_file(log_config.in.hpp log_config.hpp)

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
