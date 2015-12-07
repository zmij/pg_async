# CMakeLists.txt
# Created on: 11 окт. 2015 г.
#     Author: zmij

cmake_minimum_required(VERSION 2.6)

set(l10n_SRCS
    message.cpp
)

add_library(${TIP_L10N_LIB} SHARED ${l10n_SRCS})

set_target_properties(
    ${TIP_L10N_LIB}
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

target_link_libraries(
    ${TIP_L10N_LIB}
    ${Boost_LOCALE_LIBRARIES}
)

install (
    TARGETS ${TIP_L10N_LIB}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
