project(examples)


# ======================================

add_executable(logip logip.cpp)
target_link_libraries(logip
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
SET_CPP_STANDARD(logip)
