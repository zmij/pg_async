#	CMake build system for AWM server
#	
#	@author zmij
#	@date Nov 30, 2015

cmake_minimum_required(VERSION 2.6)

set(_pversion 0.1.0)

if(${CMAKE_VERSION} VERSION_GREATER 3.0)
    cmake_policy(SET CMP0048 NEW)
    project(awm-server VERSION ${_pversion})
else()
    project(awm-server)
    set(PROJECT_VERSION ${_pversion})
endif()

set(PROJECT_PREFIX awm)

# CMake helper scripts
include(${CMAKE_SOURCE_DIR}/cmake/scripts/argn.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/scripts/subproject.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/scripts/initializer_list.cmake)

include(${CMAKE_SOURCE_DIR}/cmake/scripts/psql.cmake)

# Dependencies
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules" )

# Options
option(BUILD_TESTS "Build tests for the project" ON)
option(BUILD_DATABASE "Build database for the project" OFF)

find_program(GIT NAMES git)
find_program(SED NAMES sed)

if (NOT _GITVERSION_SET)
    execute_process(COMMAND ${GIT} describe --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git branch ${GIT_BRANCH} version ${GIT_VERSION}")
    set(_GITVERSION_SET ${GIT_VERSION})
endif(NOT _GITVERSION_SET)

add_definitions("-std=c++11")
add_definitions(-DWITH_TIP_LOG)
add_definitions(-DWITH_BOOST_ASIO)
set(USE_TIP_LOG 'ON')

set(TIP_LOG_INCLUDE_DIRS lib/log/include)
set(TIP_LOG_LIBRARY awm-log)
set(TIP_LOG_LIB ${TIP_LOG_LIBRARY})

set(TIP_DB_LIB awm-psql)

set(TIP_IRI_INCLUDE_DIRS lib/iri/include)
set(TIP_IRI_LIB awm-iri)

set(TIP_HTTP_SERVER_LIB awm-http-server)

set(TIP_HTTP_EXTRA_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/data/awm-headers.csv)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set(BOOST_COMPONENTS
    system
    thread
    regex
    filesystem
    locale
    program_options
)

find_package(Boost 1.58 COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
find_package (Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Crypto++ REQUIRED)

include_directories(${Boost_INCLUDE_DIR})

if(BUILD_TESTS)
    enable_testing()
    include(sql/create/set_db_options.cmake)
endif()

# Library subprojects
add_subproject(log ROOT lib/log
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/lib/log/include)
add_subproject(pg ROOT lib/pg_async
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/lib/pg_async/include)
add_subproject(facet INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/lib/facet/include)
add_subproject(lru INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/lib/lru/include)
add_subproject(iri ROOT lib/iri
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/lib/iri/include)
add_subproject(http ROOT lib/http
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/lib/http/include
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/http/lib/cereal/include
            ${CMAKE_CURRENT_BINARY_DIR}/lib/http/include)
add_subproject(l10n ROOT lib/l10n
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/lib/l10n/include)
add_subproject(pstreams INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/pstreams)

# Game source
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

add_subdirectory(include/tip)
add_subdirectory(src/awm)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()

# Game databases
add_subdirectory(sql/create)
