#	CMakeLists.txt tip-log/include/tip
#	
#	@author zmij
#	@date Nov 30, 2015

cmake_minimum_required(VERSION 2.6)

install(
    FILES log.hpp
    DESTINATION include/tip
)

install(
    FILES log/ansi_colors.hpp
    DESTINATION include/tip/log
)
