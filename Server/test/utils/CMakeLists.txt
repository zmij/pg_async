#	/awm-server/test/utils/CMakeLists.txt
#	
#	@author zmij
#	@date Dec 16, 2015

cmake_minimum_required(VERSION 2.6)

set(util_test_SRCS
    ranges_test.cpp
)

message(STATUS "Thread libs ${CMAKE_THREAD_LIBS_INIT}")

add_executable(test-utils ${util_test_SRCS})
target_link_libraries(
    test-utils
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

if (GTEST_XML_OUTPUT)
    set (
        TEST_ARGS
        --gtest_output=xml:test-utils-detail.xml
    )
endif()

add_test(
    NAME test-utils
    COMMAND test-utils ${TEST_ARGS}
)
