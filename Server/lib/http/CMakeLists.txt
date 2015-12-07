#	CMakeLists.txt
#	
#	@author zmij
#	@date Dec 7, 2015

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

set(_pname ${PROJECT_PREFIX}-http)

if (NOT TIP_IRI_INCLUDE_DIRS)
    message(FATAL_ERROR "Please set path to tip-iri includes (-DTIP_IRI_INCLUDE_DIRS=)")
endif()

if (NOT TIP_LOG_INCLUDE_DIRS)
    message(FATAL_ERROR "Please set path to tip-log includes (-DTIP_LOG_INCLUDE_DIRS=)")
endif()

if (NOT TIP_IRI_LIB)
    set(TIP_IRI_LIB tip-iri)
endif()
if (NOT TIP_LOG_LIB)
    set(TIP_LOG_LIB tip-log)
endif()

if (${CMAKE_VERSION} VERSION_GREATER "3.0")
    cmake_policy(SET CMP0048 NEW)
    project(${_pname} VERSION ${_pversion})
else()
    project(${_pname})
    set(PROJECT_VERSION ${_pversion})
endif()

set(HTTP_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(HTTP_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR})

include(${CMAKE_CURRENT_SOURCE_DIR}/scripts/header_gen.cmake)

add_definitions("-std=c++11")

set(HTTP_COMMON_LIB ${PROJECT_PREFIX}-http-common)
set(HTTP_CLIENT_LIB ${PROJECT_PREFIX}-http-client)
set(HTTP_SERVER_LIB ${PROJECT_PREFIX}-http-server)

add_subdirectory(data)

add_subdirectory(lib/ssl)

set(CEREAL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/cereal/include)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${TIP_IRI_INCLUDE_DIRS}
    ${TIP_LOG_INCLUDE_DIRS}
    ${TIP_SSL_INCLUDE_DIRS}
    ${CEREAL_INCLUDE_DIR}
)

link_directories(${TIP_IRI_LIB_DIR} ${TIP_LOG_LIB_DIR})

add_subdirectory(include/tip/http)
add_subdirectory(src/tip/http)

get_directory_property(has_parent PARENT_DIRECTORY)
if (has_parent)
    set(HTTP_COMMON_LIB ${PROJECT_PREFIX}-http-common PARENT_SCOPE)
    set(HTTP_CLIENT_LIB ${PROJECT_PREFIX}-http-client PARENT_SCOPE)
    set(HTTP_SERVER_LIB ${PROJECT_PREFIX}-http-server PARENT_SCOPE)
    set(
        TIP_HTTP_INCLIDE_DIRS 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CEREAL_INCLUDE_DIR}
        PARENT_SCOPE
    )
endif()
