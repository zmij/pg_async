# Find crypto++ library
include(FindPackageHandleStandardArgs)
if (CRYPTOPP_INCLUDE_DIR AND CRYPTOPP_LIBRARIES)
    set(CRYPTOPP_FOUND TRUE)
else()
    find_path(
        CRYPTOPP_INCLUDE_DIR
        NAMES  cryptlib.h
        HINTS 
            /usr/include/crypto++
            /usr/local/include/crypto++
            /opt/local/include/crypto++
            /usr/include/cryptopp
            /usr/local/include/cryptopp
            /opt/local/include/cryptopp
    )

    find_library(CRYPTOPP_LIBRARIES
        NAMES cryptopp
        HINTS /usr/lib /usr/local/lib /opt/local/lib
    )
    
    add_definitions(-DCRYPTOPP_DISABLE_ASM)
    
    find_package_handle_standard_args(Crypto++ DEFAULT_MSG
        CRYPTOPP_LIBRARIES CRYPTOPP_INCLUDE_DIR)
    
    mark_as_advanced(CRYPTOPP_LIBRARIES CRYPTOPP_INCLUDE_DIR)
endif()
