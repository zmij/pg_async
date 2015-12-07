#	CMakeLists.txt
#	
#	@author zmij
#	@date Aug 28, 2015

cmake_minimum_required(VERSION 2.6)
set(
    server_lib_SRCS
    connection.cpp
    mime_types.cpp
    reply.cpp
    server.cpp
    request_handler.cpp
    request_dispatcher.cpp
    error.cpp
    detail/context_registry.cpp
    detail/path_matcher.cpp
    locale_name.cpp
    locale_manager.cpp
    language_context.cpp
    json_body_context.cpp
    remote_address.cpp
)

add_library(${HTTP_SERVER_LIB} SHARED ${server_lib_SRCS})

set_target_properties(
    ${HTTP_SERVER_LIB} 
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

target_link_libraries(${HTTP_SERVER_LIB} 
    ${Boost_SYSTEM_LIBRARIES}
    ${Boost_THREAD_LIBRARIES}
    ${Boost_LOCALE_LIBRARIES}
    ${TIP_LOG_LIB}
    ${HTTP_COMMON_LIB}
)

install (
    TARGETS ${HTTP_SERVER_LIB}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
