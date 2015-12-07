# CMakeLists.txt
# Created on: Aug 17, 2015
#     Author: zmij

cmake_minimum_required(VERSION 2.6)

set(iri_lib_SRCS
    iri.cpp)

add_library(${TIP_IRI_LIB} SHARED ${iri_lib_SRCS})
set_target_properties(
    ${TIP_IRI_LIB} 
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

install (
    TARGETS ${TIP_IRI_LIB}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
