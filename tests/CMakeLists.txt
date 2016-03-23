project(tests)

add_executable(basic_tests BasicTests.cpp)
target_link_libraries(basic_tests
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
