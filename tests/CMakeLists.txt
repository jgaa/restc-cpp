project(tests)

if (NOT WIN32)
    add_definitions(-Wno-missing-braces)
endif()

if (RESTC_CPP_WITH_UNIT_TESTS)
    add_subdirectory(unit)
endif()

if (RESTC_CPP_WITH_FUNCTIONALT_TESTS)
    add_subdirectory(functional)
endif()
