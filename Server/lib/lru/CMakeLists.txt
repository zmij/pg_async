#	CMakeLists.txt
#	
#	@author zmij
#	@date Nov 30, 2015

cmake_minimum_required(VERSION 2.6)

set(
    lru_cache_SRCS
    include/tip/lru-cache/lru_cache.hpp
    include/tip/lru-cache/lru_cache_service.hpp
)

install(
    FILES ${lru_cache_SRCS}
    DESTINATION include/tip/lru-cache
)

# TODO Tests
