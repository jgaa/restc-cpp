project(tests)

# For now, build the unittest lib manually
#
#  $ cd <project root dir>
#  $ git submodule init
#  $ cd externals/unittest-cpp
#  $ mkdir build
#  $ cd build
#  $ cmake ..
#  $ make

set(UNITTEST_CPP_PATH ${RESTC_ROOT_DIR}/externals/unittest-cpp)
set(UNITTEST_CPP_INCLUDE_PATH ${UNITTEST_CPP_PATH})
if (UNIX)
    set(UNITTEST_CPP_LIBPATH ${UNITTEST_CPP_PATH}/build)
else()
    set(UNITTEST_CPP_LIBPATH ${UNITTEST_CPP_PATH}/build/Debug)
endif()

include_directories(${UNITTEST_CPP_INCLUDE_PATH})
link_directories(
    ${UNITTEST_CPP_LIBPATH}
    ${BOOST_LIBRARYDIR}
    )

add_executable(basic_tests BasicTests.cpp)
target_link_libraries(basic_tests
    restc-cpp ${DEFAULT_LIBRARIES})

add_test(BASIC_TESTS basic_tests)

add_executable(readme_tests ReadmeTests.cpp)
target_link_libraries(readme_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
)

add_test(JSON_README_TESTS readme_tests)

add_executable(auth_tests AuthTest.cpp)
target_link_libraries(auth_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    UnitTest++
)

add_test(AUTH_TESTS auth_tests)


add_executable(upload_tests UploadTests.cpp)
target_link_libraries(upload_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    UnitTest++
)

add_test(UPLOAD_TESTS upload_tests)


add_executable(inserter_serializer_tests InsertSerializerTest.cpp)
target_link_libraries(inserter_serializer_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    UnitTest++
)

add_test(INSERTER_SERIALIZER_TESTS inserter_serializer_tests)


add_executable(ccache_tests ConnectionCacheTests.cpp)
target_link_libraries(ccache_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    UnitTest++
)

add_test(CCACHE_FUNCTIONAL_TESTS ccache_tests)


add_executable(redirect_tests RedirectTests.cpp)
target_link_libraries(redirect_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    UnitTest++
)

add_test(REDIRECT_FUNCTIONAL_TESTS redirect_tests)

add_executable(crud_tests CRUD_test.cpp)
target_link_libraries(crud_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    UnitTest++
)

add_test(CRUD_FUNCTIONAL_TESTS crud_tests)

add_executable(1000c_tests 1000ConnectionsTest.cpp)
target_link_libraries(1000c_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    UnitTest++
)

add_test(1000C_FUNCTIONAL_TESTS 1000c_tests)

if (RESTC_CPP_WITH_TLS)
    add_executable(https_tests HttpsTest.cpp)
    target_link_libraries(https_tests
        restc-cpp
        ${DEFAULT_LIBRARIES}
        UnitTest++
    )

    add_test(HTTPS_FUNCTIONAL_TESTS https_tests)
endif()

message(STATUS "UNITTEST_CPP_LIBPATH is ${UNITTEST_CPP_LIBPATH}")

add_executable(url_tests UrlTests.cpp)
target_link_libraries(url_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    UnitTest++
)

add_test(URL_UNITTESTS url_tests)

add_executable(json_serialize_tests JsonSerializeTests.cpp)
target_link_libraries(json_serialize_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    UnitTest++
)

add_test(JSON_UNITTESTS json_serialize_tests)

add_executable(http_reply_tests HttpReplyTests.cpp)
target_link_libraries(http_reply_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    UnitTest++
)

add_test(HTTP_REPLY_UNITTESTS http_reply_tests)


add_executable(own_ioservice_tests OwnIoserviceTests.cpp)
target_link_libraries(own_ioservice_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
    UnitTest++
)

add_test(OWN_IOSERVICE_TESTS own_ioservice_tests)

