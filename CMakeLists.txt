
cmake_minimum_required(VERSION 3.0)
project (restc-cpp)

if (NOT DEFINED RESTC_ROOT_DIR)
    set(RESTC_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if (NOT DEFINED WITH_APIDOC)
    option(WITH_APIDOC "Generate Doxygen documentation")
endif()

if (NOT DEFINED RESTC_CPP_WITH_UNIT_TESTS)
    option(RESTC_CPP_WITH_UNIT_TESTS "Enable Unit Testing" ON)
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

if (WIN32 AND NOT DEFINED RESTC_CPP_USE_WIN32_DEFAULTS)
    option(RESTC_CPP_USE_WIN32_DEFAULTS "Use C:/devel ... for openssl, zlib" ON)
endif()

# We create a configuration file so that other code that
# include our header files gets the correct configuration.
set(CONF_PATH ${RESTC_ROOT_DIR}/include/restc-cpp/config.h)

message(STATUS "Using ${CMAKE_CXX_COMPILER}")

if (NOT EMBEDDED_RESTC_CPP)

    if (NOT CMAKE_PREFIX_PATH AND WIN32 AND RESTC_CPP_USE_WIN32_DEFAULTS)
        set(CMAKE_PREFIX_PATH
            C:/devel/zlib-1.2.9
            C:/devel/zlib-1.2.9/build/RelWithDebInfo
        )
    endif()

    if (NOT OPENSSL_ROOT_DIR AND WIN32 AND RESTC_CPP_USE_WIN32_DEFAULTS)
        # Cmake's find_package(OpenSSL) is badly broken, and fails to work
        # with the default openssl build on windows.
        # In order to resolve the problems, make a 64 (or 32) bit build of
        # openssl and copy inc32/openssl to ${OPENSSL_ROOT_DIR}/include, and
        # the content of out32dll to ${OPENSSL_ROOT_DIR}/lib/VC
        # Make sure to also add ${OPENSSL_ROOT_DIR}/lib/VC to your path
        # when you run programs compiled with restc_cpp under windows, or
        # copy the dll's to the directory with your binaries.
        # (Using 3rd party C libraries under Windows with their own build systems
        # is a major pain-point. I'm planning to see if I can use Windows crypto
        # library in sted of openssl, (or somerthing that can be built as a
        # submodule to git with cmake)).
        set(OPENSSL_ROOT_DIR C:/devel/openssl)
        message(STATUS "Setting OPENSSL_ROOT_DIR to ${OPENSSL_ROOT_DIR}")
    endif()

    if (RESTC_CPP_WITH_ZLIB)
        find_package(ZLIB REQUIRED)
    endif()

    if (RESTC_CPP_WITH_TLS)
        find_package(OpenSSL REQUIRED)
    endif()

    if (UNIX)
        set(THREADLIBS pthread)
    endif()


    set (DEFAULT_LIBRARIES
        ${DEFAULT_LIBRARIES}
        ${THREADLIBS}
        ${OPENSSL_LIBRARIES}
        ${ZLIB_LIBRARIES}
    )

    include(${RESTC_ROOT_DIR}/cmake_scripts/boost.cmake)
    include(cmake_scripts/pch.cmake)

    if (UNIX)
    # For now, assume we use g++/clang++
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG=1 -o3 ")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -D_DEBUG=1 -o0 -g ")
        add_definitions(-D__USE_LARGEFILE64=1 -D__USE_FILE_OFFSET64=1
            -Wall -fPIC -std=c++14 -pthread
        )
    elseif(WIN32)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG=1 ")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -D_DEBUG=1 ")
        # We will support windows from Windows Vista
        add_definitions(-D_WIN32_WINNT=0x0600)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    endif()

    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib CACHE PATH "Destination location")
    link_directories(${LIBRARY_OUTPUT_PATH})

    include(cmake_scripts/doxygen.cmake)

endif()

set(DEFAULT_LIBRARIES ${DEFAULT_LIBRARIES} ${ZLIB_LIBRARIES})

include_directories(
    ${RESTC_ROOT_DIR}/include
    ${RESTC_ROOT_DIR}/externals/rapidjson/include/
    ${ZLIB_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR}/build
    ${OPENSSL_INCLUDE_DIR}
    )

add_subdirectory(src)

if (RESTC_CPP_WITH_UNIT_TESTS)
    add_subdirectory(tests)
endif()


message(STATUS "Writing the current configuration to ${CONF_PATH}")
CONFIGURE_FILE(config.h.template ${CONF_PATH})

