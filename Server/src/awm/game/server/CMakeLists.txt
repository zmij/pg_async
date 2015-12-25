# CMakeLists.txt
# Created on: 30 авг. 2015 г.
#     Author: zmij

cmake_minimum_required(VERSION 2.6)

set(
    tip_server_SRCS
    srvd.cpp
)

quoted_initializer_list(L10N_DOMAINS_INIT ${L10N_DOMAINS})
configure_file(config.in.hpp config.hpp)

add_executable(awm-srvd ${tip_server_SRCS})
target_link_libraries(awm-srvd
    ${ASIO_LIBRARIES}
    ${Boost_PROGRAM_OPTIONS_LIBRARIES}
    awm-auth
    awm-world
)

install (
    TARGETS awm-srvd
    RUNTIME DESTINATION bin
)
