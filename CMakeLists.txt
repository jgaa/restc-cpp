
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
    if (RESTC_CPP_WITH_TLS)
        if (UNIX)
            find_package(OpenSSL REQUIRED)
        else()
            # find_package fails to do anything sane with openssl 1.1 under Windows
			if (NOT DEFINED OPENSSL_ROOT)
				SET (OPENSSL_ROOT C:/devel/openssl/lib-386)
				message(STATUS "Using OpenSSL from ${OPENSSL_ROOT}")
			endIF()
			
            if (NOT DEFINED OPENSSL_INCLUDE_DIR)
                    set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT}/include)
            endif()
            if (NOT DEFINED OPENSSL_LIB_DIR)
                    set(OPENSSL_LIB_DIR ${OPENSSL_ROOT}/lib)
            endif()
            include_directories(${OPENSSL_INCLUDE_DIR})
			link_directories(${OPENSSL_LIB_DIR})
			message(STATUS "Using OpenSSL headers from ${OPENSSL_INCLUDE_DIR}")
        endif()
        set(SSL_LIBS libeay32 ssleay32)
    endif()

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

endif()

include_directories(
    ${RESTC_ROOT_DIR}/include
    ${RESTC_ROOT_DIR}/externals/rapidjson/include/
    )

add_subdirectory(src)

if (RESTC_CPP_WITH_UNIT_TESTS)
    add_subdirectory(tests)
endif()


message(STATUS "Writing the current configuration to ${CONF_PATH}")
CONFIGURE_FILE(config.h.template ${CONF_PATH})

include(cmake_scripts/doxygen.cmake)
