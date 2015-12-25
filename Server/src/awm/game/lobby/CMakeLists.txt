#	CMakeLists.txt
#
#	@author zavyalov
#	@date Dec 25, 2015

cmake_minimum_required(VERSION 2.6)

set(game_lobby_SRCS
    tank.cpp tank.hpp
)
set(LIB_NAME ${PROJECT_PREFIX}-lobby)

include_directories(${CRYPTOPP_INCLUDE_DIR})

add_library(${LIB_NAME} SHARED ${game_lobby_SRCS})
set_target_properties(
	${LIB_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION 1
)
set(lobby_LIBRARIES
	${ASIO_LIBRARIES}
	${CRYPTOPP_LIBRARIES}
	${TIP_LOG_LIB}
	${TIP_DB_LIB}
	${TIP_HTTP_SERVER_LIB}
	awm-config
)
target_link_libraries(${LIB_NAME} ${lobby_LIBRARIES})

install (
    TARGETS ${LIB_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

set(L10N_DOMAINS ${L10N_DOMAINS} PARENT_SCOPE)
