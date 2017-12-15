cmake_minimum_required(VERSION 3.0)
project (example)

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS
    system
    program_options
    filesystem
    date_time
    context
    coroutine
    chrono
    log
    )

include(cmake/external-projects.cmake)

add_executable(${PROJECT_NAME} main.cpp)
add_dependencies(${PROJECT_NAME} externalRestcCpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
target_link_libraries(${PROJECT_NAME}
    restc-cpp
    ${Boost_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    )

add_definitions(
    -DBOOST_COROUTINE_NO_DEPRECATION_WARNING=1
    -DBOOST_ALL_DYN_LINK=1
    )
