#	CMakeLists.txt
#	
#	@author zmij
#	@date Jul 18, 2015

cmake_minimum_required(VERSION 2.6)

project(pg-async)
set(PROJECT_VERSION 0.1.0)

option(USE_TIP_LOG "Use tip::log logger library" OFF)
option(BUILD_TESTS "Build tests for the library" OFF)
option(USE_BOOST_ASIO "Use Boost.Asio instead of Standalone Asio library" ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

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

include_directories(
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIR}
)

if (USE_TIP_LOG)
    add_definitions(-DWITH_TIP_LOG)
endif()

add_subdirectory(src/tip/db/pg)
add_subdirectory(include/tip/db)
add_subdirectory(include/tip/util)

if (BUILD_TESTS)
enable_testing()
add_subdirectory(test)
endif()

message(STATUS "Cmake module path ${CMAKE_MODULE_PATH}")
