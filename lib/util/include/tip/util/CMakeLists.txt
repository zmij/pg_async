#	CMakeLists.txt
#	
#	@author zmij
#	@date Aug 3, 2015

cmake_minimum_required(VERSION 2.6)

set(
    pg_async_util_HDRS
    endian.hpp
    meta_helpers.hpp
    streambuf.hpp
)

install(
    FILES ${pg_async_util_HDRS}
    DESTINATION include/tip/util
)
