# SPDX-FileCopyrightText: 2022 Alexandre Bouvier <contact@amb.tf>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_search_module(INIREADER QUIET IMPORTED_TARGET INIReader)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(inih
    REQUIRED_VARS INIREADER_LINK_LIBRARIES
    VERSION_VAR INIREADER_VERSION
)

if (inih_FOUND AND NOT TARGET inih::INIReader)
    add_library(inih::INIReader ALIAS PkgConfig::INIREADER)
endif()
