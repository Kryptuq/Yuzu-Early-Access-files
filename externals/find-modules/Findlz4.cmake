# SPDX-FileCopyrightText: 2022 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
    pkg_search_module(liblz4 IMPORTED_TARGET GLOBAL liblz4)
    if (liblz4_FOUND)
        add_library(lz4::lz4 ALIAS PkgConfig::liblz4)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(lz4
    REQUIRED_VARS
        liblz4_LINK_LIBRARIES
        liblz4_FOUND
    VERSION_VAR liblz4_VERSION
)
