# - Try to find GMock 
# Once done this will define
#  GMOCK_FOUND - System has GMock
#  GMOCK_INCLUDE_DIRS - The GMock include directories
#  GMOCK_LIBRARIES - The libraries needed to use GMock
#  GMOCK_DEFINITIONS - Compiler switches required for using GMock

find_package(PkgConfig)
pkg_check_modules(PC_GMOCK gmock)
set(GMOCK_DEFINITIONS ${PC_GMOCK_CFLAGS_OTHER})

find_path(GMOCK_INCLUDE_DIR
          gmock/gmock.h
          HINTS ${PC_GMOCK_INCLUDEDIR} ${PC_GMOCK_INCLUDE_DIRS}
          PATH_SUFFIXES gmock)
SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
find_library(GMOCK_LIBRARY gmock
             HINTS ${PC_GMOCK_LIBDIR} ${PC_GMOCK_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
        gmock DEFAULT_MSG
        GMOCK_INCLUDE_DIR GMOCK_LIBRARY)
mark_as_advanced(GMOCK_INCLUDE_DIR GMOCK_LIBRARY)

set(GMOCK_INCLUDE_DIRS ${GMOCK_INCLUDE_DIR})
set(GMOCK_LIBRARIES ${GMOCK_LIBRARY})