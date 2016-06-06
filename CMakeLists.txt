#    CMakeLists.txt for pg_async library
#
#    @author zmij
#    @date Jul 18, 2015

cmake_minimum_required(VERSION 2.6)

if (NOT PROJECT_PREFIX)
    set(PROJECT_PREFIX tip)
endif()

if (PROJECT_VERSION)
    # Built as a subproject
    set(_pversion ${PROJECT_VERSION})
else()
    set(_pversion 0.1.0)
endif()

set(_pname ${PROJECT_PREFIX}-pg-async)

if (${CMAKE_VERSION} VERSION_GREATER "3.0")
    cmake_policy(SET CMP0048 NEW)
    project(${_pname} VERSION ${_pversion})
else()
    project(${_pname})
    set(PROJECT_VERSION ${_pversion})
endif()

option(USE_TIP_LOG "Use tip::log logger library" OFF)
option(BUILD_TESTS "Build tests for the library" OFF)
option(USE_BOOST_ASIO "Use Boost.Asio instead of Standalone Asio library" ON)

# Dependencies
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules" )

if (USE_BOOST_ASIO)
set(BOOST_COMPONENTS
    system
    thread
)
find_package(Boost 1.58 COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
set(
    ASIO_LIBRARIES
    ${Boost_SYSTEM_LIBRARIES}
    ${Boost_THREAD_LIBRARIES}
)
add_definitions(-DWITH_BOOST_ASIO)
else()
find_package(asio 1.10 REQUIRED)
endif()

find_package(Threads REQUIRED)
add_definitions("-std=c++11")

if(NOT TIP_UTIL_INCLUDE_DIRS)
    set(TIP_UTIL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib/util/include)
endif()

if(USE_TIP_LOG)
if(NOT TIP_LOG_INCLUDE_DIRS)
add_subdirectory(lib/log)
endif()
endif()

if(NOT AFSM_INCLUDE_DIR)
add_subdirectory(lib/afsm)
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${Boost_INCLUDE_DIR}
    ${METAPUSHKIN_INCLUDE_DIR}
    ${AFSM_INCLUDE_DIR}
    ${TIP_UTIL_INCLUDE_DIRS}
    ${TIP_LOG_INCLUDE_DIRS}
)

link_directories(${TIP_LOG_LINK_DIR})

if (USE_TIP_LOG)
    add_definitions(-DWITH_TIP_LOG)
endif()

set(PGASYNC_LIB_NAME ${PROJECT_PREFIX}-psql)

add_subdirectory(src/tip/db/pg)
add_subdirectory(include/tip/db)

if (BUILD_TESTS)
enable_testing()
add_subdirectory(test)
endif()

get_directory_property(has_parent PARENT_DIRECTORY)
if (has_parent)
    set(TIP_DB_LIB ${PROJECT_PREFIX}-psql CACHE INTERNAL "Name of tip psql library target")
    set(TIP_DB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${TIP_LOG_INCLUDE_DIRS} CACHE INTERNAL "Paths to tip psql library includes")
endif()
