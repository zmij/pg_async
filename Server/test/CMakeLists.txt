#	/awm-server/test/CMakeLists.txtg
#	
#	@author zmij
#	@date Dec 16, 2015

cmake_minimum_required(VERSION 2.6)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_subdirectory(utils)
