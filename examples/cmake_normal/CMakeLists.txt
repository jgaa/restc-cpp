cmake_minimum_required(VERSION 3.0)
project (example)

find_package(restc-cpp REQUIRED)
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

add_executable(${PROJECT_NAME} main.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
target_link_libraries(${PROJECT_NAME}
    ${RESTC_CPP_LIBRARIES}
    ${Boost_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    )

add_definitions(
    -DBOOST_COROUTINE_NO_DEPRECATION_WARNING=1
    -DBOOST_ALL_DYN_LINK=1
    )

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${ZLIB_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZLIB_INCLUDE_DIR}/build>
    $<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>
    )
