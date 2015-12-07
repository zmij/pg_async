# Generic CMake functions

#
# Function to parse function arguments
# usage
# parse_argn( <prefix> <arg_names> ${ARGN} )
# 
function(parse_argn _PREFIX _ARGNAMES)
	set(current_arg_name ${_PREFIX}DEFAULT_ARGS)	
	foreach (arg ${ARGN})
		list(FIND ${_ARGNAMES} ${arg} idx)
		if (NOT idx LESS 0)
			set(current_arg_name ${_PREFIX}${arg})
		else()
			list(APPEND ${current_arg_name} ${arg})
		endif()
	endforeach()
	
	foreach (arg_name ${${_ARGNAMES}})
		set(${_PREFIX}${arg_name} ${${_PREFIX}${arg_name}} PARENT_SCOPE)
	endforeach()
	
endfunction()
