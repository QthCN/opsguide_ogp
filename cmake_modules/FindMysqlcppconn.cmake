# - Try to find libmysqlcppconn (official MySQL cpp connector library)
# Once done this will define
#  LIBMYSQLCPPCONN_FOUND - System has libmysqlcppconn
#  LIBMYSQLCPPCONN_INCLUDE_DIRS - The libmysqlcppconn include directories
#  LIBMYSQLCPPCONN_LIBRARIES - The libraries needed to use libmysqlcppconn
#  LIBMYSQLCPPCONN_DEFINITIONS - Compiler switches required for using libmysqlcppconn

find_package(PkgConfig)
pkg_check_modules(PC_MYSQLCPPCONN mysqlcppconn)
set(LIBMYSQLCPPCONN_DEFINITIONS ${PC_MYSQLCPPCONN_CFLAGS_OTHER})

find_path(LIBMYSQLCPPCONN_INCLUDE_DIR
        mysql_connection.h
        HINTS ${PC_MYSQLCPPCONN_INCLUDEDIR} ${PC_MYSQLCPPCONN_INCLUDE_DIRS})
find_library(LIBMYSQLCPPCONN_LIBRARY mysqlcppconn mysqlcppconn-static
        HINTS ${PC_MYSQLCPPCONN_LIBDIR} ${PC_MYSQLCPPCONN_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
        mysqlcppconn DEFAULT_MSG
        LIBMYSQLCPPCONN_INCLUDE_DIR LIBMYSQLCPPCONN_LIBRARY)
mark_as_advanced(LIBMYSQLCPPCONN_INCLUDE_DIR LIBMYSQLCPPCONN_LIBRARY)

set(LIBMYSQLCPPCONN_INCLUDE_DIRS ${LIBMYSQLCPPCONN_INCLUDE_DIR})
set(LIBMYSQLCPPCONN_LIBRARIES ${LIBMYSQLCPPCONN_LIBRARY})