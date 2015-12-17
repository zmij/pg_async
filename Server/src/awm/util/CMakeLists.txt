#	/awm-server/src/awm/util/CMakeLists.txt
#	
#	@author zmij
#	@date Dec 16, 2015

cmake_minimum_required(VERSION 2.6)



add_library(awm-config SHARED configuration.cpp)
set_target_properties(
	awm-config PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION 1
)
target_link_libraries(
    awm-config
    ${Boost_PROGRAM_OPTIONS_LIBRARIES}
    ${TIP_LOG_LIB}
)
