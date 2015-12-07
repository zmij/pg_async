#	CMakeLists.txt for tip-iri library
#	
#	@author zmij
#	@date Nov 30, 2015

cmake_minimum_required(VERSION 2.6)

if (NOT PROJECT_PREFIX)
    set(PROJECT_PREFIX tip)
endif()
set(_pname ${PROJECT_PREFIX}-iri)
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

set(TIP_IRI_LIB ${PROJECT_PREFIX}-iri)

add_subdirectory(include/tip)
add_subdirectory(src)

get_directory_property(has_parent PARENT_DIRECTORY)

if (has_parent)
    set(TIP_IRI_LIB ${PROJECT_PREFIX}-iri PARENT_SCOPE)
    set(TIP_IRI_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)
endif()

