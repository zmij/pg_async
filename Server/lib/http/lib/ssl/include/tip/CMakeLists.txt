#	/tip-ssl/include/tip/CMakeLists.txt
#	
#	@author zmij
#	@date Dec 7, 2015

cmake_minimum_required(VERSION 2.6)
set(
        tip_ssl_HDRS
        ssl_context_service.hpp
)

install(
    FILES ${tip_ssl_HDRS}
    DESTINATION include/tip
)
