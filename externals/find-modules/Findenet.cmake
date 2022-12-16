# SPDX-FileCopyrightText: 2022 Alexandre Bouvier <contact@amb.tf>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(PkgConfig QUIET)
pkg_search_module(ENET QUIET IMPORTED_TARGET libenet)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(enet
    REQUIRED_VARS ENET_LINK_LIBRARIES
    VERSION_VAR ENET_VERSION
)

if (enet_FOUND AND NOT TARGET enet::enet)
    add_library(enet::enet ALIAS PkgConfig::ENET)
endif()
