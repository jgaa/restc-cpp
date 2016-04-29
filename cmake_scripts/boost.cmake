
if (WIN32)
	# The cmake find boost script is usually broken for Windows.
	# In stead, we will require BOOST_ROOT and BOOST_LIBRARYDIR
	# to be set manually.
	
	if (NOT DEFINED BOOST_ROOT)
		set (BOOST_ROOT C:/devel/boost_1_61_0_b1)
		message(STATUS "Warning. No BOOST_ROOT defined. Defaulting to ${BOOST_ROOT}")
	else()
		message(STATUS "Using BOOST from ${BOOST_ROOT}")
	endif()

	if (NOT DEFINED BOOST_LIBRARYDIR)
		set (BOOST_LIBRARYDIR ${BOOST_ROOT}/stage/lib)
		message(STATUS "Warning. No BOOST_ROOT defined. Defaulting to ${BOOST_ROOT}")
	endif()
	
	include_directories(${BOOST_ROOT})
	link_directories(${BOOST_LIBRARYDIR})
	
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
    set(BOOST_UNIT_TEST_FRAMEWORK boost_unit_test_framework)

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
	#set(SSL_LIBS libcrypto libssl)
endif()

set (DEFAULT_LIBRARIES
    ${DEFAULT_LIBRARIES}
    ${THREADLIBS}
    ${SSL_LIBS}
    ${BOOST_LIBRARIES}
    )
