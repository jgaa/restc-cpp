project(functionaltests)


# ======================================
message("DEFAULT_LIBRARIES: ${DEFAULT_LIBRARIES}")

add_executable(basic_tests BasicTests.cpp)
SET_CPP_STANDARD(basic_tests)
add_dependencies(basic_tests restc-cpp ${DEPENDS_GTEST})
target_link_libraries(basic_tests PRIVATE ${GTEST_LIBRARIES} restc-cpp ${DEFAULT_LIBRARIES})
add_test(BASIC_TESTS basic_tests)


# ======================================

add_executable(readme_tests ReadmeTests.cpp)
target_link_libraries(readme_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
SET_CPP_STANDARD(readme_tests)
add_dependencies(readme_tests restc-cpp ${DEPENDS_GTEST})
add_test(README_TESTS readme_tests)

# ======================================

add_executable(auth_tests AuthTest.cpp)
target_link_libraries(auth_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(auth_tests  restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(auth_tests)
add_test(AUTH_TESTS auth_tests)


# ======================================

add_executable(upload_tests UploadTests.cpp)
target_link_libraries(upload_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(upload_tests restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(upload_tests)
add_test(UPLOAD_TESTS upload_tests)


# # ======================================

add_executable(inserter_serializer_tests InsertSerializerTest.cpp)
target_link_libraries(inserter_serializer_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(inserter_serializer_tests restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(inserter_serializer_tests)
add_test(INSERTER_SERIALIZER_TESTS inserter_serializer_tests)


# # ======================================

add_executable(ccache_tests ConnectionCacheTests.cpp)
target_link_libraries(ccache_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(ccache_tests restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(ccache_tests)
add_test(CCACHE_FUNCTIONAL_TESTS ccache_tests)

# # ======================================

add_executable(redirect_tests RedirectTests.cpp)
target_link_libraries(redirect_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(redirect_tests restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(redirect_tests)
add_test(REDIRECT_FUNCTIONAL_TESTS redirect_tests)


# # ======================================

add_executable(crud_tests CRUD_test.cpp)
target_link_libraries(crud_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(crud_tests restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(crud_tests)
add_test(CRUD_FUNCTIONAL_TESTS crud_tests)


# ======================================

add_executable(many_connections_tests ManyConnectionsTest.cpp)
target_link_libraries(many_connections_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(many_connections_tests restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(many_connections_tests)
add_test(MANY_CONNECTIONS_FUNCTIONAL_TESTS many_connections_tests)


# ======================================

if (RESTC_CPP_WITH_TLS)
    add_executable(https_tests HttpsTest.cpp)
    target_link_libraries(https_tests
        ${GTEST_LIBRARIES}
        restc-cpp
        ${DEFAULT_LIBRARIES}
    )
    add_dependencies(https_tests restc-cpp ${DEPENDS_GTEST})
    SET_CPP_STANDARD(https_tests)
    add_test(HTTPS_FUNCTIONAL_TESTS https_tests)
endif()


# ======================================

add_executable(own_ioservice_tests OwnIoserviceTests.cpp)
target_link_libraries(own_ioservice_tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(own_ioservice_tests restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(own_ioservice_tests)
add_test(OWN_IOSERVICE_TESTS own_ioservice_tests)



# ======================================

add_executable(connection_pool_instances_test ConnectionPoolInstancesTest.cpp)
target_link_libraries(connection_pool_instances_test
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(connection_pool_instances_test restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(connection_pool_instances_test)
add_test(CONNECTION_POOL_INSTANCES_TEST connection_pool_instances_test)



# ======================================

add_executable(proxy-tests ProxyTests.cpp)
target_link_libraries(proxy-tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(proxy-tests restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(proxy-tests)
add_test(PROXY_TESTS proxy-tests)


# ======================================

add_executable(cookie-tests CookieTests.cpp)
target_link_libraries(cookie-tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(cookie-tests restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(cookie-tests)
add_test(COOKIE_TESTS cookie-tests)

# ======================================

add_executable(properties-tests PropertiesTests.cpp)
target_link_libraries(properties-tests
    ${GTEST_LIBRARIES}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
add_dependencies(properties-tests restc-cpp ${DEPENDS_GTEST})
SET_CPP_STANDARD(properties-tests)
add_test(PROPERTIES_TESTS properties-tests)

