#	CMakeLists.txt
#	
#	@author zmij
#	@date Dec 7, 2015

cmake_minimum_required(VERSION 2.6)

generate_http_headers_enum( ${HTTP_HEADERS_DEF} header_names.hpp)

set(
    HTTP_COMMON_HEADERS
    cookie.hpp
    header.hpp
    request_fwd.hpp
    request.hpp
    response_fwd.hpp
    response_status.hpp
    response.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/header_names.hpp
)

set(
    HTTP_GRAMMAR_HEADERS
    grammar/base64_generate.hpp
    grammar/base64_parse.hpp
    grammar/cookie_generate.hpp
    grammar/cookie_parse.hpp
    grammar/datetime_generate.hpp
    grammar/datetime_parse.hpp
    grammar/header_fields_parse.hpp
    grammar/header_generate.hpp
    grammar/header_parse.hpp
    grammar/request_generate.hpp
    grammar/request_parse.hpp
    grammar/response_generate.hpp
    grammar/response_parse.hpp
)

install(
    FILES ${HTTP_COMMON_HEADERS}
    DESTINATION tip/http/common
)

install(
    FILES ${HTTP_GRAMMAR_HEADERS}
    DESTINATION tip/http/common/grammar
)
