#	CMakeLists.txt
#	
#	@author zmij
#	@date Jul 18, 2015

cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
set (Boost_USE_STATIC_LIBS ON)
find_package( Boost COMPONENTS 
    unit_test_framework program_options
    REQUIRED )
add_subdirectory(db)
