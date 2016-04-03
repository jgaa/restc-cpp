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
set(UNITTEST_CPP_LIBPATH ${UNITTEST_CPP_PATH}/build)

add_executable(basic_tests BasicTests.cpp)
target_link_libraries(basic_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
)

include_directories(${UNITTEST_CPP_INCLUDE_PATH})
link_directories(${UNITTEST_CPP_LIBPATH})

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
