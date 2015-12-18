# Find PSTreams library
include(FindPackageHandleStandardArgs)
if (PSTREAMS_INCLUDE_DIR)
    set(PSTREAMS_FOUND TRUE)
else()
    find_path(
        PSTREAMS_INCLUDE_DIR
        NAMES pstreams/pstream.h
        HINTS
              /usr/include
              /usr/local/include
    )
    find_package_handle_standard_args(PStreams DEFAULT_MSG
        "" ${PSTREAMS_INCLUDE_DIR})
    
    mark_as_advanced(PSTREAMS_INCLUDE_DIR)
endif()
