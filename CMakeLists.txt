#	CMakeLists.txt
#	
#	@author zmij
#	@date Jul 18, 2015

cmake_minimum_required(VERSION 2.6)

project(pg-async)
set(PROJECT_VERSION 0.1.0)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set(BOOST_COMPONENTS 
    system
    thread
)

find_package(Boost 1.58 COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
find_package(Threads REQUIRED)
add_definitions("-std=c++11")

include_directories(
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIR}
)

option(USE_TIP_LOG "Use tip::log logger library" OFF)
option(BUILD_TESTS "Build tests for the library" OFF)

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
