
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
    )

include_directories(${Boost_INCLUDE_DIRS})

if (WIN32)
    # Msvc and possible some other Windows-compilers will link
    # to the correct libraries trough #pragma directives in boost headers.
	#SET(BOOST_UNIT_TEST_FRAMEWORK libboost_test_exec_monitor-vc140-mt-sgd-1_57)
else ()
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
    set(BOOST_UNIT_TEST_FRAMEWORK boost_unit_test_framework)
endif ()

if (UNIX)
	set (BOOST_LIBRARIES
		${LIB_BOOST_SYSTEM}
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
    )

	set(BOOST_UNIT_TEST_LIBRARIES boost_unit_test_framework)
endif()

if (UNIX)
	set(THREADLIBS pthread)
	set(SSL_LIBS ssl crypto)
else()
	set(SSL_LIBS optimized libeay32MT debug libeay32MTd optimized ssleay32MT debug ssleay32MTd)
endif()

set (DEFAULT_LIBRARIES
    ${DEFAULT_LIBRARIES}
    ${THREADLIBS}
    ${SSL_LIBS}
    ${BOOST_LIBRARIES}
    )
