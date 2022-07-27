# SPDX-FileCopyrightText: 2020 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
pkg_check_modules(PC_opus QUIET opus)

find_path(opus_INCLUDE_DIR
  NAMES opus.h
  PATHS ${PC_opus_INCLUDE_DIRS}
  PATH_SUFFIXES opus
)
find_library(opus_LIBRARY
  NAMES opus
  PATHS ${PC_opus_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(opus
  FOUND_VAR opus_FOUND
  REQUIRED_VARS
    opus_LIBRARY
    opus_INCLUDE_DIR
  VERSION_VAR opus_VERSION
)

if(opus_FOUND)
  set(Opus_LIBRARIES ${opus_LIBRARY})
  set(Opus_INCLUDE_DIRS ${opus_INCLUDE_DIR})
  set(Opus_DEFINITIONS ${PC_opus_CFLAGS_OTHER})
endif()

if(opus_FOUND AND NOT TARGET Opus::Opus)
  add_library(Opus::Opus UNKNOWN IMPORTED GLOBAL)
  set_target_properties(Opus::Opus PROPERTIES
    IMPORTED_LOCATION "${opus_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_opus_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${opus_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
    opus_INCLUDE_DIR
    opus_LIBRARY
)
