# @file /tip-server/src/tip/log/CMakeLists.txt
#    @author: zmij
# Created on: Jul 8, 2015

cmake_minimum_required(VERSION 2.6)

set(log_lib_SRCS
    log.cpp
    ansi_colors.cpp
)

add_library(${TIP_LOG_LIB} SHARED ${log_lib_SRCS})
target_link_libraries(${TIP_LOG_LIB}
    ${Boost_SYSTEM_LIBRARIES}
    ${Boost_THREAD_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
set_target_properties(
    ${TIP_LOG_LIB} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

install (
    TARGETS ${TIP_LOG_LIB}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
