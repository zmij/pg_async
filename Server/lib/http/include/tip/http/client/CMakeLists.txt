#	/tip-http/include/tip/http/client/CMakeLists.txt
#	
#	@author zmij
#	@date Dec 7, 2015

cmake_minimum_required(VERSION 2.6)

set (
    http_client_HDRS
    service.hpp
    session.hpp
)

install(
    FILES ${http_client_HDRS}
    DESTINATION include/tip/http/client
)
