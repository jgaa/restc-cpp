
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



# We create a configuration file so that other code that
# include our header files gets the correct configuration.
set(CONF_PATH ${RESTC_ROOT_DIR}/include/restc-cpp/config.h)

message(STATUS "Using ${CMAKE_CXX_COMPILER}")

if (NOT EMBEDDED_RESTC_CPP)
    find_package(OpenSSL REQUIRED)

    include(cmake_scripts/boost.cmake)
    include(cmake_scripts/pch.cmake)

    if (UNIX)
    # For now, assume we use g++/clang++
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG=1 -o3 ")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -D_DEBUG=1 -o0 -g ")
        add_definitions(-D__USE_LARGEFILE64=1 -D__USE_FILE_OFFSET64=1
            -Wall -fPIC -std=c++1y -pthread
        )
    elseif(WIN32)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG=1 ")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -D_DEBUG=1 ")
        # We will support windows from Windows Vista
        add_definitions(-D_WIN32_WINNT=0x0600)
    endif()

    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib CACHE PATH "Destination location")
    link_directories(${LIBRARY_OUTPUT_PATH})

    include_directories(include)
endif()

add_subdirectory(src)

if (RESTC_CPP_WITH_UNIT_TESTS)
    add_subdirectory(tests)
endif()


message(STATUS "Writing the current configuration to ${CONF_PATH}")
CONFIGURE_FILE(config.h.template ${CONF_PATH})

include(cmake_scripts/doxygen.cmake)
