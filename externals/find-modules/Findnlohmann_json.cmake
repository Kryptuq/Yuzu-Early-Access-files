# SPDX-FileCopyrightText: 2020 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
pkg_check_modules(PC_nlohmann_json QUIET nlohmann_json)

find_path(nlohmann_json_INCLUDE_DIR
  NAMES json.hpp
  PATHS ${PC_nlohmann_json_INCLUDE_DIRS}
  PATH_SUFFIXES nlohmann
)

if(nlohmann_json_INCLUDE_DIR)
  file(STRINGS "${nlohmann_json_INCLUDE_DIR}/json.hpp" _nlohmann_json_version_lines
    REGEX "#define[ \t]+NLOHMANN_JSON_VERSION_(MAJOR|MINOR|PATCH)")
  string(REGEX REPLACE ".*NLOHMANN_JSON_VERSION_MAJOR +\([0-9]+\).*" "\\1" _nlohmann_json_version_major "${_nlohmann_json_version_lines}")
  string(REGEX REPLACE ".*NLOHMANN_JSON_VERSION_MINOR +\([0-9]+\).*" "\\1" _nlohmann_json_version_minor "${_nlohmann_json_version_lines}")
  string(REGEX REPLACE ".*NLOHMANN_JSON_VERSION_PATCH +\([0-9]+\).*" "\\1" _nlohmann_json_version_patch "${_nlohmann_json_version_lines}")
  set(nlohmann_json_VERSION "${_nlohmann_json_version_major}.${_nlohmann_json_version_minor}.${_nlohmann_json_version_patch}")
  unset(_nlohmann_json_version_major)
  unset(_nlohmann_json_version_minor)
  unset(_nlohmann_json_version_patch)
  unset(_nlohmann_json_version_lines)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(nlohmann_json
  FOUND_VAR nlohmann_json_FOUND
  REQUIRED_VARS
    nlohmann_json_INCLUDE_DIR
    nlohmann_json_VERSION
  VERSION_VAR nlohmann_json_VERSION
)

if(nlohmann_json_FOUND)
  set(nlohmann_json_INCLUDE_DIRS ${nlohmann_json_INCLUDE_DIR})
  set(nlohmann_json_DEFINITIONS ${PC_nlohmann_json_CFLAGS_OTHER})
endif()

if(nlohmann_json_FOUND AND NOT TARGET nlohmann_json::nlohmann_json)
  add_library(nlohmann_json::nlohmann_json UNKNOWN IMPORTED)
  set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
    IMPORTED_LOCATION "${nlohmann_json_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_nlohmann_json_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${nlohmann_json_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
    nlohmann_json_INCLUDE_DIR
)
