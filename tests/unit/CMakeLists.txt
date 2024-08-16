project(unittests)

macro (ADD_AND_RUN_UNITTEST testname targetname)
SET_CPP_STANDARD(${targetname})
add_test(NAME ${testname} COMMAND ${targetname})
if (RESTC_CPP_AUTORUN_UNIT_TESTS)
    add_custom_command(
        TARGET ${targetname}
        POST_BUILD
        COMMENT "running ${testname}"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> -R "${testname}"  --output-on-failure
    )
endif()
endmacro()


add_executable(url_tests UrlTests.cpp)
target_link_libraries(url_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(url_tests restc-cpp ${DEPENDS_GTEST})
ADD_AND_RUN_UNITTEST(URL_UNITTESTS url_tests)


# ======================================

add_executable(json_serialize_tests JsonSerializeTests.cpp)
target_link_libraries(json_serialize_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)

add_dependencies(json_serialize_tests restc-cpp ${DEPENDS_GTEST})

ADD_AND_RUN_UNITTEST(JSON_UNITTESTS json_serialize_tests)


# ======================================

add_executable(json_iostream_tests Iostream2JsonTests.cpp)
target_link_libraries(json_iostream_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)

add_dependencies(json_iostream_tests restc-cpp ${DEPENDS_GTEST})

ADD_AND_RUN_UNITTEST(JSON_IOSTREAM_UNITTESTS json_iostream_tests)



# ======================================

add_executable(http_reply_tests HttpReplyTests.cpp)
target_link_libraries(http_reply_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(http_reply_tests restc-cpp ${DEPENDS_GTEST})
ADD_AND_RUN_UNITTEST(HTTP_REPLY_UNITTESTS http_reply_tests)


# ======================================

add_executable(async_sleep_tests AsyncSleepTests.cpp)
target_link_libraries(async_sleep_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(async_sleep_tests restc-cpp ${DEPENDS_GTEST})
ADD_AND_RUN_UNITTEST(ASYNC_SLEEP_TESTS async_sleep_tests)


# ======================================

add_executable(request_builder_tests RequestBuilder.cpp)
target_link_libraries(request_builder_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(request_builder_tests restc-cpp ${DEPENDS_GTEST})
ADD_AND_RUN_UNITTEST(REQUEST_BUILDER_TESTS request_builder_tests)

