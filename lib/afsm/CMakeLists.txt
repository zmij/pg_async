#    CMakeLists.txt for afsm library
#
#    @author zmij
#    @date Nov 30, 2015

cmake_minimum_required(VERSION 2.6)

# Set library name here
set(lib_name afsm)
string(TOUPPER ${lib_name} LIB_NAME)

if (PROJECT_VERSION)
    set(_pversion ${PROJECT_VERSION})
else()
    set(_pversion 0.1.0)
endif()

if (${CMAKE_VERSION} VERSION_GREATER "3.0")
    cmake_policy(SET CMP0048 NEW)
    project(${lib_name} VERSION ${_pversion})
else()
    project(${lib_name})
    set(PROJECT_VERSION ${_pversion})
endif()

option(BUILD_TESTS "Build test programs" ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_definitions("-std=c++11")
add_definitions(-Wall -Werror -Wextra -pedantic -Weffc++
    -Wno-non-virtual-dtor # I really know what I am exactly doing
)

set(${LIB_NAME}_LIB ${lib_name})

# Add subdirectories here

if (NOT METAPUSHKIN_INCLUDE_DIR)
    add_subdirectory(lib/meta)
endif()

message(STATUS "Metapushkin include dir ${METAPUSHKIN_INCLUDE_DIR}")
include_directories(${METAPUSHKIN_INCLUDE_DIR})

if (BUILD_TESTS)
    enable_testing()
    if (NOT PSST_ANSI_COLORS_INCLUDE_DIR)
    add_subdirectory(lib/ansi-colors)
    endif()
    message(STATUS ${PSST_ANSI_COLORS_LINK_DIR})

    include_directories(${PSST_ANSI_COLORS_INCLUDE_DIR})
    link_directories(${PSST_ANSI_COLORS_LINK_DIR})
    add_subdirectory(test)
endif()

get_directory_property(has_parent PARENT_DIRECTORY)
if (has_parent)
    set(${LIB_NAME}_LIB ${lib_name} CACHE INTERNAL "Name of afsm library target")
    set(${LIB_NAME}_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "Path to afsm libaray includes" )
endif()