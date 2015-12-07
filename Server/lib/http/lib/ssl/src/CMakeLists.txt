#	CMakeLists.txt
#	
#	@author zmij
#	@date Aug 21, 2015

cmake_minimum_required(VERSION 2.6)

set(
    tip_ssl_SRCS
    ssl_context_service.cpp
)

add_library(${TIP_SSLCONTEXT_LIB} SHARED ${tip_ssl_SRCS})

set_target_properties(
    ${TIP_SSLCONTEXT_LIB}
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

target_link_libraries(
    ${TIP_SSLCONTEXT_LIB} 
    ${Boost_SYSTEM_LIBRARIES}
    ${Boost_FILESYSTEM_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

install (
    TARGETS ${TIP_SSLCONTEXT_LIB}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
