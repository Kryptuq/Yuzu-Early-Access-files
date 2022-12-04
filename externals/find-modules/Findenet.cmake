# SPDX-FileCopyrightText: 2022 Alexandre Bouvier <contact@amb.tf>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_search_module(ENET QUIET IMPORTED_TARGET GLOBAL libenet)
    if (ENET_FOUND)
        add_library(enet::enet ALIAS PkgConfig::ENET)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(enet
    REQUIRED_VARS ENET_LINK_LIBRARIES
    VERSION_VAR ENET_VERSION
)
