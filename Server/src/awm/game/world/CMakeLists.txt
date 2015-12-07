# CMakeLists.txt
# Created on: 3 сен. 2015 г.
#     Author: zelenov

cmake_minimum_required(VERSION 2.6)

find_package( Boost COMPONENTS 
    program_options
    filesystem
    REQUIRED )

set(
    tip_world_SRCS
    world.cpp
)

set(LIB_NAME ${PROJECT_PREFIX}-world)
add_library(${LIB_NAME} SHARED ${tip_world_SRCS})
target_link_libraries(${LIB_NAME}
    ${ASIO_LIBRARIES}
    ${Boost_PROGRAM_OPTIONS_LIBRARIES}
    ${Boost_SYSTEM_LIBRARIES}
    ${Boost_FILESYSTEM_LIBRARIES}
    ${TIP_LOG_LIB}
)
set_target_properties(
	${LIB_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION 1
)

install (
    TARGETS ${LIB_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
