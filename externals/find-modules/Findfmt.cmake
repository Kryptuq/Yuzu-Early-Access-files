# SPDX-FileCopyrightText: 2020 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
pkg_check_modules(PC_fmt QUIET fmt)

find_path(fmt_INCLUDE_DIR
  NAMES format.h
  PATHS ${PC_fmt_INCLUDE_DIRS} ${CONAN_INCLUDE_DIRS_fmt}
  PATH_SUFFIXES fmt
)

find_library(fmt_LIBRARY
  NAMES fmt
  PATHS ${PC_fmt_LIBRARY_DIRS} ${CONAN_LIB_DIRS_fmt}
)

if(fmt_INCLUDE_DIR)
  set(_fmt_version_file "${fmt_INCLUDE_DIR}/core.h")
  if(NOT EXISTS "${_fmt_version_file}")
    set(_fmt_version_file "${fmt_INCLUDE_DIR}/format.h")
  endif()
  if(EXISTS "${_fmt_version_file}")
    # parse "#define FMT_VERSION 60200" to 6.2.0
    file(STRINGS "${_fmt_version_file}" fmt_VERSION_LINE
      REGEX "^#define[ \t]+FMT_VERSION[ \t]+[0-9]+$")
    string(REGEX REPLACE "^#define[ \t]+FMT_VERSION[ \t]+([0-9]+)$"
      "\\1" fmt_VERSION "${fmt_VERSION_LINE}")
    foreach(ver "fmt_VERSION_PATCH" "fmt_VERSION_MINOR" "fmt_VERSION_MAJOR")
      math(EXPR ${ver} "${fmt_VERSION} % 100")
      math(EXPR fmt_VERSION "(${fmt_VERSION} - ${${ver}}) / 100")
    endforeach()
    set(fmt_VERSION
      "${fmt_VERSION_MAJOR}.${fmt_VERSION_MINOR}.${fmt_VERSION_PATCH}")
  endif()
  unset(_fmt_version_file)
  unset(fmt_VERSION_LINE)
  unset(fmt_VERSION_MAJOR)
  unset(fmt_VERSION_MINOR)
  unset(fmt_VERSION_PATCH)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(fmt
  FOUND_VAR fmt_FOUND
  REQUIRED_VARS
    fmt_LIBRARY
    fmt_INCLUDE_DIR
    fmt_VERSION
  VERSION_VAR fmt_VERSION
)

if(fmt_FOUND)
  set(fmt_LIBRARIES ${fmt_LIBRARY})
  set(fmt_INCLUDE_DIRS ${fmt_INCLUDE_DIR})
  set(fmt_DEFINITIONS ${PC_fmt_CFLAGS_OTHER})
endif()

if(fmt_FOUND AND NOT TARGET fmt::fmt)
  add_library(fmt::fmt UNKNOWN IMPORTED)
  set_target_properties(fmt::fmt PROPERTIES
    IMPORTED_LOCATION "${fmt_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_fmt_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${fmt_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
    fmt_INCLUDE_DIR
    fmt_LIBRARY
)
