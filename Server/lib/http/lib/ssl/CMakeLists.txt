#    CMakeLists.txt for tip-ssl-context library
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

set(_pname ${PROJECT_PREFIX}-ssl-context)

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
        filesystem
)

find_package(Boost COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
find_package (Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
    ${Boost_INCLUDE_DIRS} 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(TIP_SSLCONTEXT_LIB ${PROJECT_PREFIX}-ssl)

add_subdirectory(include/tip)
add_subdirectory(src)

get_directory_property(has_parent PARENT_DIRECTORY)
if (has_parent)
    set(TIP_SSLCONTEXT_LIB ${PROJECT_PREFIX}-ssl PARENT_SCOPE)
    set(TIP_SSL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)
endif()
