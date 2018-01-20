cmake_minimum_required(VERSION 3.0)
project (restc-cpp VERSION 0.9.0)

if (NOT DEFINED INSTALL_RAPIDJSON_HEADERS)
    option(INSTALL_RAPIDJSON_HEADERS "Install rapidjson headers when make install is executed" ON)
endif()

include(cmake_scripts/external-projects.cmake)

if (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    message(STATUS "Using conan configuration: ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif()

if (NOT DEFINED RESTC_ROOT_DIR)
    set(RESTC_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if (NOT DEFINED WITH_APIDOC)
    option(WITH_APIDOC "Generate Doxygen documentation")
endif()

if (NOT DEFINED WITH_EXAMPLES)
    option(RESTC_CPP_WITH_EXAMPLES "Compile examples" ON)
endif()

if (NOT DEFINED RESTC_CPP_WITH_UNIT_TESTS)
    option(RESTC_CPP_WITH_UNIT_TESTS "Enable Unit Testing" ON)
endif()

if (NOT DEFINED RESTC_CPP_AUTORUN_UNIT_TESTS)
    option(RESTC_CPP_AUTORUN_UNIT_TESTS "Run Unit Tests automatically after build" OFF)
endif()

if (NOT DEFINED RESTC_CPP_WITH_FUNCTIONALT_TESTS)
    option(RESTC_CPP_WITH_FUNCTIONALT_TESTS "Enable Functional Testing" ON)
endif()

if (NOT DEFINED RESTC_CPP_WITH_TLS)
        option(RESTC_CPP_WITH_TLS "Enable TLS (Trough OpenSSL)" ON)
endif()

if (NOT DEFINED RESTC_CPP_LOG_WITH_BOOST_LOG)
    option(RESTC_CPP_LOG_WITH_BOOST_LOG "Use boost::log for logging" ON)
endif()

if (NOT DEFINED RESTC_CPP_LOG_JSON_SERIALIZATION)
    option(RESTC_CPP_LOG_JSON_SERIALIZATION "Enable trace logging for json serialization debugging")
endif()

if (NOT DEFINED RESTC_CPP_WITH_ZLIB)
    option(RESTC_CPP_WITH_ZLIB "Use zlib" ON)
endif()

if (NOT DEFINED RESTC_CPP_USE_CPP17)
    option(RESTC_CPP_USE_CPP17 "Use the C++17 standard" OFF)
endif()

message(STATUS "Using ${CMAKE_CXX_COMPILER}")

# We create a configuration file so that other code that include our header files gets the correct configuration.
CONFIGURE_FILE(config.h.template ${CMAKE_BINARY_DIR}/generated-include/${PROJECT_NAME}/config.h)

set(ACTUAL_SOURCES
    src/ChunkedReaderImpl.cpp
    src/ChunkedWriterImpl.cpp
    src/IoReaderImpl.cpp
    src/IoWriterImpl.cpp
    src/PlainReaderImpl.cpp
    src/PlainWriterImpl.cpp
    src/NoBodyReaderImpl.cpp
    src/DataReaderStream.cpp
    src/RestClientImpl.cpp
    src/RequestImpl.cpp
    src/ReplyImpl.cpp
    src/ConnectionPoolImpl.cpp
    src/Url.cpp
    src/RequestBodyStringImpl.cpp
    src/RequestBodyFileImpl.cpp
    src/url_encode.cpp
    )

set(HEADERS
    ${RESTC_ROOT_DIR}/include/restc-cpp/restc-cpp.h
    )

if (RESTC_CPP_WITH_ZLIB)
    set(ACTUAL_SOURCES ${ACTUAL_SOURCES} src/ZipReaderImpl.cpp)
endif()

if (WIN32)
    include(cmake_scripts/pch.cmake)
    ADD_MSVC_PRECOMPILED_HEADER(restc-cpp/restc-cpp.h src/pch.cpp ACTUAL_SOURCES)
    add_definitions(-DWAR_PCH)
    set(SOURCES ${ACTUAL_SOURCES} src/pch.cpp ${HEADERS} ${RESFILES})
else()
    set(SOURCES ${ACTUAL_SOURCES})
endif()

add_library(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_OUTPUT_NAME restc-cppD)
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated-include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

if (RESTC_CPP_USE_CPP17)
    message(STATUS "Using C++ 17")
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
else()
    message(STATUS "Using C++ 14")
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
endif()

add_dependencies(${PROJECT_NAME} externalRapidJson)

if (NOT EMBEDDED_RESTC_CPP)

    if (RESTC_CPP_WITH_ZLIB)
        find_package(ZLIB REQUIRED)
        target_include_directories(${PROJECT_NAME} PUBLIC ${ZLIB_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PUBLIC ${ZLIB_LIBRARIES})
    endif()

    if (RESTC_CPP_WITH_TLS)
        find_package(OpenSSL REQUIRED)
        target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES})
    endif()

    if (UNIX)
        find_package(Threads REQUIRED)
        target_link_libraries(${PROJECT_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
    endif()

    #set(Boost_USE_MULTITHREADED ON)
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
    target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DBOOST_COROUTINE_NO_DEPRECATION_WARNING=1)

    if (EXISTS ${Boost_INCLUDE_DIRS}/boost/type_index.hpp)
        set(RESTC_CPP_HAVE_BOOST_TYPEINDEX 1)
    endif()

    if (WIN32)
        link_directories(${Boost_LIBRARY_DIRS})
    endif()

    include(cmake_scripts/pch.cmake)
    
    set_property(TARGET  PROPERTY CXX_STANDARD 17)

#     if (UNIX)
#         # For now, assume we use g++/clang++
#         set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG=1 -o3 ")
#         set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -D_DEBUG=1 -o0 -g ")
#         add_definitions(-D__USE_LARGEFILE64=1 -D__USE_FILE_OFFSET64=1
#             -Wall -fPIC -std=c++14 -pthread
#             )
#     elseif(WIN32)
#         set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG=1 ")
#         set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -D_DEBUG=1 ")
#         # We will support windows from Windows Vista
#         add_definitions(-D_WIN32_WINNT=0x0600)
#         set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
#     endif()

    if(WIN32)
        add_definitions(-D_WIN32_WINNT=0x0600)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    endif()

    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib CACHE PATH "Destination location")
    link_directories(${LIBRARY_OUTPUT_PATH})

    include(cmake_scripts/doxygen.cmake)

    target_include_directories(${PROJECT_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${ZLIB_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${ZLIB_INCLUDE_DIR}/build>
            $<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>
    )

endif()

include(cmake_scripts/install.cmake)
install(DIRECTORY ${CMAKE_BINARY_DIR}/generated-include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if (RESTC_CPP_WITH_UNIT_TESTS OR RESTC_CPP_WITH_FUNCTIONALT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (RESTC_CPP_WITH_EXAMPLES)
    add_subdirectory(examples/logip)
endif()
