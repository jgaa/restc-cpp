project(graphapi)


# ======================================

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME}
    restc-cpp
    ${DEFAULT_LIBRARIES}
)
SET_CPP_STANDARD(${PROJECT_NAME})
