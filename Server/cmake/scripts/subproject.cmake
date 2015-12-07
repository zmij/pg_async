#	subproject.cmake
#	
#	@author zmij
#	@date Nov 30, 2015

function(add_subproject NAME)
    set(argnames
        ROOT INCLUDES)
    parse_argn("" argnames ${ARGN})
    if (INCLUDES)
        include_directories(${INCLUDES})
    endif()
    if (ROOT)
        add_subdirectory(${ROOT})
    endif()
endfunction()