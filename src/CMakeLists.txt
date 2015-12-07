# @file /tip-server/src/tip/log/CMakeLists.txt
#    @author: zmij
# Created on: Jul 8, 2015

cmake_minimum_required(VERSION 2.6)

set(log_lib_SRCS
    log.cpp
    ansi_colors.cpp
)

set(
    LIB_NAME "${PROJECT_PREFIX}-log"
)

add_library(${LIB_NAME} SHARED ${log_lib_SRCS})
target_link_libraries(${LIB_NAME}
    ${Boost_SYSTEM_LIBRARIES}
    ${Boost_THREAD_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

set_target_properties(
    ${LIB_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

install (
    TARGETS ${LIB_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
