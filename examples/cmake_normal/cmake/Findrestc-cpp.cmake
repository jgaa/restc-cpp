# - Try to find librestc-cpp
# Once done this will define
#  LIBRESTC_CPP_FOUND - System has librestc-cpp
#  LIBRESTC_CPP_INCLUDE_DIRS - The librestc-cpp include directories
#  LIBRESTC_CPP_LIBRARIES - The libraries needed to use librestc-cpp
#  LIBRESTC_CPP_DEFINITIONS - Compiler switches required for using librestc-cpp

find_package(PkgConfig)
pkg_check_modules(PC_RESTC_CPP QUIET librestc-cpp)
set(LIBRESTC_CPP_DEFINITIONS ${PC_RESTC_CPP_CFLAGS_OTHER})

find_path(LIBRESTC_CPP_INCLUDE_DIR restc-cpp/restc-cpp.h
          HINTS ${PC_RESTC_CPP_INCLUDEDIR} ${PC_RESTC_CPP_INCLUDE_DIRS}
          PATH_SUFFIXES restc-cpp )

find_library(LIBRESTC_CPP_LIBRARY NAMES restc-cpp
             HINTS ${PC_RESTC_CPP_LIBDIR} ${PC_RESTC_CPP_LIBRARY_DIRS} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBRESTC_CPP_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(librestc-cpp  DEFAULT_MSG
                                  LIBRESTC_CPP_LIBRARY LIBRESTC_CPP_INCLUDE_DIR)

mark_as_advanced(LIBRESTC_CPP_INCLUDE_DIR LIBRESTC_CPP_LIBRARY )

set(LIBRESTC_CPP_LIBRARIES ${LIBRESTC_CPP_LIBRARY} )
set(LIBRESTC_CPP_INCLUDE_DIRS ${LIBRESTC_CPP_INCLUDE_DIR} )

