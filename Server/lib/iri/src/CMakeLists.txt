# CMakeLists.txt
# Created on: Aug 17, 2015
#     Author: zmij

cmake_minimum_required(VERSION 2.6)

set(iri_lib_SRCS
    iri.cpp)

set(LIBRARY_NAME ${PROJECT_PREFIX}-iri)

add_library(${LIBRARY_NAME} SHARED ${iri_lib_SRCS})
set_target_properties(
    ${LIBRARY_NAME} 
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

install (
    TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
