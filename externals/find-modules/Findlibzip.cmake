
find_package(PkgConfig QUIET)
pkg_check_modules(PC_libzip QUIET libzip)

find_path(libzip_INCLUDE_DIR
  NAMES zip.h
  PATHS ${PC_libzip_INCLUDE_DIRS}
  "$ENV{LIB_DIR}/include"
  "$ENV{INCLUDE}"
  /usr/local/include
  /usr/include
)
find_path(libzip_INCLUDE_DIR_ZIPCONF
  NAMES zipconf.h
  HINTS ${PC_libzip_INCLUDE_DIRS}
  "$ENV{LIB_DIR}/include"
  "$ENV{LIB_DIR}/lib/libzip/include"
  "$ENV{LIB}/lib/libzip/include"
  /usr/local/lib/libzip/include
  /usr/lib/libzip/include
  /usr/local/include
  /usr/include
  "$ENV{INCLUDE}"
)
find_library(libzip_LIBRARY
  NAMES zip
  PATHS ${PC_libzip_LIBRARY_DIRS}
  "$ENV{LIB_DIR}/lib" "$ENV{LIB}" /usr/local/lib /usr/lib
)

if (libzip_INCLUDE_DIR_ZIPCONF)
  FILE(READ "${libzip_INCLUDE_DIR_ZIPCONF}/zipconf.h" _libzip_VERSION_CONTENTS)
  if (_libzip_VERSION_CONTENTS)
    STRING(REGEX REPLACE ".*#define LIBZIP_VERSION \"([0-9.]+)\".*" "\\1" libzip_VERSION "${_libzip_VERSION_CONTENTS}")
  endif()
  unset(_libzip_VERSION_CONTENTS)
endif()

set(libzip_VERSION ${libzip_VERSION} CACHE STRING "Version number of libzip")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libzip
  FOUND_VAR libzip_FOUND
  REQUIRED_VARS
    libzip_LIBRARY
    libzip_INCLUDE_DIR
    libzip_INCLUDE_DIR_ZIPCONF
    libzip_VERSION
  VERSION_VAR libzip_VERSION
)

if(libzip_FOUND)
  set(libzip_LIBRARIES ${libzip_LIBRARY})
  set(libzip_INCLUDE_DIRS ${libzip_INCLUDE_DIR})
  set(libzip_DEFINITIONS ${PC_libzip_CFLAGS_OTHER})
endif()

if(libzip_FOUND AND NOT TARGET libzip::libzip)
  add_library(libzip::libzip UNKNOWN IMPORTED)
  set_target_properties(libzip::libzip PROPERTIES
    IMPORTED_LOCATION "${libzip_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_libzip_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${libzip_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
    libzip_INCLUDE_DIR
    libzip_INCLUDE_DIR_ZIPCONF
    libzip_LIBRARY
    libzip_VERSION
)
