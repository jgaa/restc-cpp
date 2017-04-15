project(unittests)

macro (ADD_AND_RUN_UNITTEST testname targetname)
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
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
ADD_AND_RUN_UNITTEST(URL_UNITTESTS url_tests)


# ======================================

add_executable(json_serialize_tests JsonSerializeTests.cpp)
target_link_libraries(json_serialize_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
ADD_AND_RUN_UNITTEST(JSON_UNITTESTS json_serialize_tests)


# ======================================

add_executable(http_reply_tests HttpReplyTests.cpp)
target_link_libraries(http_reply_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    ${UNITTEST_LIB}
)
ADD_AND_RUN_UNITTEST(HTTP_REPLY_UNITTESTS http_reply_tests)
