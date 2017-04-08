#    CMakeLists.txt for tip-log library
#
#    @author zmij
#    @date Nov 30, 2015

cmake_minimum_required(VERSION 2.6)

if (NOT PROJECT_PREFIX)
    set(PROJECT_PREFIX tip)
endif()
set(_pname ${PROJECT_PREFIX}-log)
if (PROJECT_VERSION)
    set(_pversion ${PROJECT_VERSION})
else()
    set(_pversion 0.1.0)
endif()

if (${CMAKE_VERSION} VERSION_GREATER "3.0")
    cmake_policy(SET CMP0048 NEW)
    project(${_pname} VERSION ${_pversion})
else()
    project(${_pname})
    set(PROJECT_VERSION ${_pversion})
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_definitions("-std=c++11")

set(TIP_LOG_LIB ${PROJECT_PREFIX}-log)

add_subdirectory(include/pushkin)
add_subdirectory(src)

get_directory_property(has_parent PARENT_DIRECTORY)
if (has_parent)
    set(TIP_LOG_LIB ${PROJECT_PREFIX}-log CACHE INTERNAL "Name of tip log library target")
    set(TIP_LOG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "Path to tip log library includes" )
    set(TIP_LOG_LINK_DIR ${CMAKE_CURRENT_BINARY_DIR}/src CACHE INTERNAL "Path to tip log binary dir")
endif()
