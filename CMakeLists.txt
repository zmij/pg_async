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
option(WITH_BOOST_FIBER "Build wire with boost::fiber support" OFF)

# Dependencies
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/afsm/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/afsm/lib/meta/cmake")

set(BOOST_COMPONENTS
    system
    thread
)
set(BOOST_VERSION 1.58)
if (WITH_BOOST_FIBER)
set(BOOST_VERSION 1.61) # Boost.Fiber was introduced in version 1.61
list(APPEND BOOST_COMPONENTS context fiber)
endif()

find_package(Boost ${BOOST_VERSION} COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
set(
    ASIO_LIBRARIES
    ${Boost_SYSTEM_LIBRARIES}
    ${Boost_THREAD_LIBRARIES}
)
add_definitions(-DWITH_BOOST_ASIO)

if(WITH_BOOST_FIBER)
set(FIBER_LIBS
    ${Boost_CONTEXT_LIBRARIES}
    ${Boost_FIBER_LIBRARIES}
)
endif()

find_package(Threads REQUIRED)
find_package(AFSM)

add_definitions("-std=c++11")

if(USE_TIP_LOG)
    if(NOT TIP_LOG_INCLUDE_DIRS)
        add_subdirectory(lib/log)
    endif()
    link_directories(${TIP_LOG_LINK_DIR})
endif()

if(NOT TIP_UTIL_INCLUDE_DIRS)
    set(TIP_UTIL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib/util/include)
endif()

if(NOT AFSM_FOUND)
add_subdirectory(lib/afsm)
endif()

if (NOT PUSHKIN_ASIO_FIBERS_INCLUDE_DIRECTORIES)
    set(_PUSHKIN_ASIO_FIBERS_SUBTREE ON)
endif()

if (_PUSHKIN_ASIO_FIBERS_SUBTREE)
    add_subdirectory(lib/asio-fiber)
endif()


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${Boost_INCLUDE_DIR}
    ${METAPUSHKIN_INCLUDE_DIRS}
    ${AFSM_INCLUDE_DIRS}
    ${TIP_UTIL_INCLUDE_DIRS}
    ${TIP_LOG_INCLUDE_DIRS}
    ${PUSHKIN_ASIO_FIBERS_INCLUDE_DIRECTORIES}
)

if (USE_TIP_LOG)
    add_definitions(-DWITH_TIP_LOG)
endif()

set(PGASYNC_LIB_NAME ${PROJECT_PREFIX}-psql)
if(WITH_BOOST_FIBER)
set(PGFIBER_LIB_NAME ${PROJECT_PREFIX}-psql-fiber)
endif()

add_subdirectory(src/tip/db/pg)
add_subdirectory(include/tip/db)

if (BUILD_TESTS)
enable_testing()
add_subdirectory(test)
endif()

get_directory_property(has_parent PARENT_DIRECTORY)
if (has_parent)
    set(TIP_DB_LIB ${PGASYNC_LIB_NAME} CACHE INTERNAL "Name of tip psql library target")
    set(TIP_DB_FIBER_LIB ${PGFIBER_LIB_NAME} CACHE INTERNAL "Name of tip psql fiber library target")
    set(TIP_DB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${TIP_LOG_INCLUDE_DIRS} CACHE INTERNAL "Paths to tip psql library includes")
endif()
