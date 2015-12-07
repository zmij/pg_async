#    CMakeLists.txt for tip-l10n library
#	
#	@author zmij

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

set(_pname ${PROJECT_PREFIX}-l10n)

if (${CMAKE_VERSION} VERSION_GREATER "3.0")
    cmake_policy(SET CMP0048 NEW)
    project(${_pname} VERSION ${_pversion})
else()
    project(${_pname})
    set(PROJECT_VERSION ${_pversion})
endif()

add_definitions("-std=c++11")

set(BOOST_COMPONENTS
        system
        thread
)

find_package(Boost COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
find_package (Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
    ${Boost_INCLUDE_DIRS} 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(TIP_L10N_LIB ${PROJECT_PREFIX}-l10n)

add_subdirectory(include/tip)
add_subdirectory(src)

get_directory_property(has_parent PARENT_DIRECTORY)
if (has_parent)
    set(TIP_L10N_LIB ${PROJECT_PREFIX}-l10n PARENT_SCOPE)
endif()
