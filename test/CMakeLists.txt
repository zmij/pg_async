#	CMakeLists.txt
#	
#	@author zmij
#	@date Jul 18, 2015

cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
find_package( Boost COMPONENTS 
    program_options
    REQUIRED )
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_subdirectory(db)
