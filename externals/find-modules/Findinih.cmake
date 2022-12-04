# SPDX-FileCopyrightText: 2022 Alexandre Bouvier <contact@amb.tf>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_search_module(INIREADER QUIET IMPORTED_TARGET GLOBAL INIReader)
    if (INIREADER_FOUND)
        add_library(inih::INIReader ALIAS PkgConfig::INIREADER)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(inih
    REQUIRED_VARS INIREADER_LINK_LIBRARIES
    VERSION_VAR INIREADER_VERSION
)
