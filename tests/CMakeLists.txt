project(tests)

if (NOT WIN32)
    add_definitions(-Wno-missing-braces)
endif()

# Lest can't be used as ExternalProject yet https://github.com/martinmoene/lest/issues/51
include_directories(${RESTC_ROOT_DIR}/externals/lest/include)

if (RESTC_CPP_WITH_UNIT_TESTS)
    add_subdirectory(unit)
endif()

if (RESTC_CPP_WITH_FUNCTIONALT_TESTS)
    add_subdirectory(functional)
endif()

