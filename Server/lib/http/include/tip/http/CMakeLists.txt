#	CMakeLists.txt
#	
#	@author zmij
#	@date Dec 7, 2015

cmake_minimum_required(VERSION 2.6)

configure_file(version.in.hpp version.hpp)

add_subdirectory(common)
add_subdirectory(client)
#add_subdirectory(server)
