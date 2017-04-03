#    CMakeLists.txt for pushkin-asio-fibers library
#
#    @author zmij
#    @date Nov 30, 2015

cmake_minimum_required(VERSION 2.6)

# Set library name here
set(lib_name pushkin-asio-fibers)
string(TOUPPER PUSHKIN_ASIO_FIBERS LIB_NAME)

set(_pname ${lib_name})
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

option(BUILD_TESTS "Build tests for ${lib_name} library" ON)
# Without fibers only non-fiber utilities can be used
option(WITH_BOOST_FIBER "Build wire with boost::fiber support" ON)

set(
    BOOST_COMPONENTS
    system
    thread
)
set(BOOST_VERSION 1.58)
if (WITH_BOOST_FIBER)
set(BOOST_VERSION 1.61) # Boost.Fiber was introduced in version 1.61
list(APPEND BOOST_COMPONENTS context fiber)
endif()

find_package(Boost ${BOOST_VERSION} COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
find_package(Threads REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRECTORIES}
)
add_definitions("-std=c++14")

#set(TIP_${LIB_NAME}_LIB ${PROJECT_PREFIX}-${lib_name})

# Add subdirectories here

get_directory_property(has_parent PARENT_DIRECTORY)
if (has_parent)
    set(${LIB_NAME}_INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        CACHE INTERNAL "Path to pushkin-asio-fibers library includes" )
endif()
