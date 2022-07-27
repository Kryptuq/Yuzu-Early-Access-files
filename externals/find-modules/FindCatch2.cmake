# SPDX-FileCopyrightText: 2020 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
pkg_check_modules(PC_Catch2 QUIET Catch2)

find_path(Catch2_INCLUDE_DIR
  NAMES catch.hpp
  PATHS ${PC_Catch2_INCLUDE_DIRS} ${CONAN_CATCH2_ROOT}
  PATH_SUFFIXES catch2
)

if(Catch2_INCLUDE_DIR)
  file(STRINGS "${Catch2_INCLUDE_DIR}/catch.hpp" _Catch2_version_lines
    REGEX "#define[ \t]+CATCH_VERSION_(MAJOR|MINOR|PATCH)")
  string(REGEX REPLACE ".*CATCH_VERSION_MAJOR +\([0-9]+\).*" "\\1" _Catch2_version_major "${_Catch2_version_lines}")
  string(REGEX REPLACE ".*CATCH_VERSION_MINOR +\([0-9]+\).*" "\\1" _Catch2_version_minor "${_Catch2_version_lines}")
  string(REGEX REPLACE ".*CATCH_VERSION_PATCH +\([0-9]+\).*" "\\1" _Catch2_version_patch "${_Catch2_version_lines}")
  set(Catch2_VERSION "${_Catch2_version_major}.${_Catch2_version_minor}.${_Catch2_version_patch}")
  unset(_Catch2_version_major)
  unset(_Catch2_version_minor)
  unset(_Catch2_version_patch)
  unset(_Catch2_version_lines)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Catch2
  FOUND_VAR Catch2_FOUND
  REQUIRED_VARS
    Catch2_INCLUDE_DIR
    Catch2_VERSION
  VERSION_VAR Catch2_VERSION
)

if(Catch2_FOUND)
  set(Catch2_INCLUDE_DIRS ${Catch2_INCLUDE_DIR})
  set(Catch2_DEFINITIONS ${PC_Catch2_CFLAGS_OTHER})
endif()

if(Catch2_FOUND AND NOT TARGET Catch2::Catch2)
  add_library(Catch2::Catch2 UNKNOWN IMPORTED)
  set_target_properties(Catch2::Catch2 PROPERTIES
    IMPORTED_LOCATION "${Catch2_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_Catch2_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${Catch2_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
    Catch2_INCLUDE_DIR
)
