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
configure_file(version.in.hpp version.hpp)

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

add_library(${PGASYNC_LIB_NAME} SHARED ${pgsql_lib_SRCS})
set_target_properties(
    ${PGASYNC_LIB_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

set(pgsql_lib_LIBRARIES
    ${ASIO_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TIP_LOG_LIB}
)

target_link_libraries(${PGASYNC_LIB_NAME} ${pgsql_lib_LIBRARIES})

install (
    TARGETS ${PGASYNC_LIB_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

if(WITH_BOOST_FIBER)
#-----------------------------------------------------------------------------
#   PG async library built with fibers
#-----------------------------------------------------------------------------

add_library(${PGFIBER_LIB_NAME} SHARED ${pgsql_lib_SRCS})
set_target_properties(
    ${PGFIBER_LIB_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)
target_compile_definitions(${PGFIBER_LIB_NAME} PRIVATE WITH_BOOST_FIBERS)
target_link_libraries(${PGFIBER_LIB_NAME}
    ${pgsql_lib_LIBRARIES}
    ${Boost_CONTEXT_LIBRARIES}
    ${Boost_FIBER_LIBRARIES}
)
install (
    TARGETS ${PGFIBER_LIB_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
#-----------------------------------------------------------------------------
endif()
