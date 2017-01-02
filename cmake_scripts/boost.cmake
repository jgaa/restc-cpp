
include (CheckIncludeFiles)

if (WIN32)
    # The cmake find boost script is usually broken for Windows.
    # In stead, we will require BOOST_ROOT and BOOST_LIBRARYDIR
    # to be set manually.

    if (NOT DEFINED BOOST_ROOT)
            set (BOOST_ROOT C:/devel/boost_1_63_0)
            message(STATUS "Warning. No BOOST_ROOT defined. Defaulting to ${BOOST_ROOT}")
    else()
            message(STATUS "Using BOOST from ${BOOST_ROOT}")
    endif()

    if (NOT DEFINED BOOST_LIBRARYDIR)
            set (BOOST_LIBRARYDIR ${BOOST_ROOT}/stage/x64/lib)
            message(STATUS "Warning. No BOOST_ROOT defined. Defaulting to ${BOOST_ROOT}")
    endif()

    include_directories(${BOOST_ROOT})
    link_directories(${BOOST_LIBRARYDIR})

    if (EXISTS ${BOOST_ROOT}/boost/type_index.hpp)
        set(HAVE_BOOST_TYPEINDEX 1)
    endif()
	
	set(Boost_USE_STATIC_LIBS ON)
	set(Boost_USE_MULTITHREADED ON)
	unset(Boost_INCLUDE_DIR CACHE)
	unset(Boost_LIBRARY_DIRS CACHE)

# Msvc and possible some other Windows-compilers will link
# to the correct libraries trough #pragma directives in boost headers.
#SET(BOOST_UNIT_TEST_FRAMEWORK libboost_test_exec_monitor-vc140-mt-sgd-1_57)
endif()


if (UNIX)
    set(Boost_USE_MULTITHREADED ON)
    find_package(Boost REQUIRED COMPONENTS
        system
        thread
        program_options
        serialization
        filesystem
        date_time
        iostreams
        regex
        context
        coroutine
        chrono
        log
        )

    include_directories(${Boost_INCLUDE_DIRS})

    set(LIB_BOOST_PROGRAM_OPTIONS boost_program_options)
    set(LIB_BOOST_SERIALIZATION boost_serialization)
    set(LIB_BOOST_FILESYSTEM boost_filesystem)
    set(LIB_BOOST_DATE_TIME boost_date_time)
    set(LIB_BOOST_IOSTREAMS boost_iostreams)
    set(LIB_BOOST_SYSTEM boost_system)
    set(LIB_BOOST_REGEX boost_regex)
    set(LIB_BOOST_CONTEXT boost_context)
    set(LIB_BOOST_COROUTINE boost_coroutine)
    set(LIB_BOOST_CHRONO boost_chrono)
    set(LIB_BOOST_THREAD boost_thread)
    set(LIB_BOOST_LOG boost_log)
    set(BOOST_UNIT_TEST_FRAMEWORK boost_unit_test_framework)

    set (BOOST_LIBRARIES ${LIB_BOOST_SYSTEM}
        ${LIB_BOOST_PROGRAM_OPTIONS}
        ${LIB_BOOST_SERIALIZATION}
        ${LIB_BOOST_FILESYSTEM}
        ${LIB_BOOST_DATE_TIME}
        ${LIB_BOOST_IOSTREAMS}
        ${LIB_BOOST_REGEX}
        ${LIB_BOOST_COROUTINE}
        ${LIB_BOOST_CONTEXT}
        ${LIB_BOOST_CHRONO}
        ${LIB_BOOST_THREAD}
        ${LIB_BOOST_LOG})

    set(BOOST_UNIT_TEST_LIBRARIES boost_unit_test_framework)
    if (EXISTS ${Boost_INCLUDE_DIRS}/boost/type_index.hpp)
        set(HAVE_BOOST_TYPEINDEX 1)
    endif()
    #CHECK_INCLUDE_FILES(${Boost_INCLUDE_DIRS}/boost/type_index.hpp HAVE_BOOST_TYPEINDEX)
endif()

if (UNIX)
    set(THREADLIBS pthread)
    set(SSL_LIBS ssl crypto)
elseif(WIN32)
	set(SSL_LIBS ${OPENSSL_LIBRARIES})
endif()

set (DEFAULT_LIBRARIES 
		${DEFAULT_LIBRARIES}
		${THREADLIBS}
		${SSL_LIBS}
		${ZLIB_LIBRARIES}
		${BOOST_LIBRARIES}
		)
message(STATUS "Default libraries: ${DEFAULT_LIBRARIES}")





