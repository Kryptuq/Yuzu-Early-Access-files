# SPDX-FileCopyrightText: 2022 Alexandre Bouvier <contact@amb.tf>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(PkgConfig QUIET)
pkg_search_module(INIH QUIET IMPORTED_TARGET inih)
if (INIReader IN_LIST inih_FIND_COMPONENTS)
    pkg_search_module(INIREADER QUIET IMPORTED_TARGET INIReader)
    if (INIREADER_FOUND)
        set(inih_INIReader_FOUND TRUE)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(inih
    REQUIRED_VARS INIH_LINK_LIBRARIES
    VERSION_VAR INIH_VERSION
    HANDLE_COMPONENTS
)

if (inih_FOUND AND NOT TARGET inih::inih)
    add_library(inih::inih ALIAS PkgConfig::INIH)
endif()

if (inih_FOUND AND inih_INIReader_FOUND AND NOT TARGET inih::INIReader)
    add_library(inih::INIReader ALIAS PkgConfig::INIREADER)
endif()
