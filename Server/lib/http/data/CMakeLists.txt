#	/tip-http/data/CMakeLists.txt
#	
#	@author zmij
#	@date Dec 7, 2015

cmake_minimum_required(VERSION 2.6)

set(
    IANA_HEADER_DEFS
    ${CMAKE_CURRENT_SOURCE_DIR}/perm-headers.csv
    ${CMAKE_CURRENT_SOURCE_DIR}/prov-headers.csv
    ${CMAKE_CURRENT_SOURCE_DIR}/sec-headers.csv
)

set(
    HTTP_HEADER_DEFS
    ${IANA_HEADER_DEFS}
    ${TIP_HTTP_EXTRA_HEADERS}
)

filter_http_headers(
    http_headers.csv
    ${HTTP_HEADER_DEFS}
)

set(HTTP_HEADERS_DEF ${CMAKE_CURRENT_BINARY_DIR}/http_headers.csv PARENT_SCOPE)
