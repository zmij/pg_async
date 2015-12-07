#	CMakeLists.txt
#	
#	@author zmij
#	@date Aug 19, 2015

cmake_minimum_required(VERSION 2.6)

generate_http_header_parser_generator(${HTTP_HEADERS_DEF})

set(
    common_lib_SRCS
    header.cpp
    request.cpp
    response.cpp
    grammar/header_parse.cpp
    grammar/header_generate.cpp
    grammar/request_parse.cpp
    grammar/request_generate.cpp
    header_names_parser.cpp
    header_names_generator.cpp
)

add_library(${HTTP_COMMON_LIB} SHARED ${common_lib_SRCS})
set_target_properties(
    ${HTTP_COMMON_LIB} 
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)
target_link_libraries(${HTTP_COMMON_LIB} ${TIP_IRI_LIB})
add_dependencies(${HTTP_COMMON_LIB} http_enum_header)

install (
    TARGETS ${HTTP_COMMON_LIB}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
