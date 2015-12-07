#	CMakeLists.txt
#	
#	@author zmij
#	@date Aug 21, 2015

cmake_minimum_required(VERSION 2.6)

set(
    client_lib_SRCS
    service.cpp
    session.cpp
)

add_library(${HTTP_CLIENT_LIB} SHARED ${client_lib_SRCS})

set_target_properties(
    ${HTTP_CLIENT_LIB} 
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

target_link_libraries(
    ${HTTP_CLIENT_LIB} 
    ${Boost_SYSTEM_LIBRARIES}
    ${Boost_THREAD_LIBRARIES}
    ${TIP_LOG_LIB}
    ${HTTP_COMMON_LIB}
    ${TIP_SSLCONTEXT_LIB}
)

install (
    TARGETS ${HTTP_CLIENT_LIB}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
