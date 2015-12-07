#	/tip-ssl/include/tip/CMakeLists.txt
#	
#	@author zmij
#	@date Dec 7, 2015

cmake_minimum_required(VERSION 2.6)
set(
        tip_HDRS
        l10n/message.hpp
)

install(
    FILES ${tip_HDRS}
    DESTINATION include/tip/l10n
)
