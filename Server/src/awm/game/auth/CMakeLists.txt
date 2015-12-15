#	CMakeLists.txt
#
#	@author zmij
#	@date Aug 30, 2015

cmake_minimum_required(VERSION 2.6)

set(game_auth_SRCS
    configuration.cpp
    protocol.cpp
    user.cpp
    password.cpp
    password_authn_handler.cpp
    session.cpp
    vendor_user_id.cpp
    token.cpp
    user_language.cpp
    stats.cpp stats.hpp
)
set(LIB_NAME ${PROJECT_PREFIX}-auth)

include_directories(${CRYPTOPP_INCLUDE_DIR})

add_library(${LIB_NAME} SHARED ${game_auth_SRCS})
set_target_properties(
	${LIB_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION 1
)
set(auth_LIBRARIES
	${ASIO_LIBRARIES}
    ${CRYPTOPP_LIBRARIES}
	${TIP_LOG_LIB}
	${TIP_DB_LIB}
	${TIP_HTTP_SERVER_LIB}
)
target_link_libraries(${LIB_NAME} ${auth_LIBRARIES})

install (
    TARGETS ${LIB_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

set(L10N_DOMAINS ${L10N_DOMAINS} PARENT_SCOPE)

