# SPDX-FileCopyrightText: 2022 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
    pkg_search_module(libzstd IMPORTED_TARGET GLOBAL libzstd)
    if (libzstd_FOUND)
        add_library(zstd::zstd ALIAS PkgConfig::libzstd)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zstd
    REQUIRED_VARS
        libzstd_LINK_LIBRARIES
        libzstd_FOUND
    VERSION_VAR libzstd_VERSION
)
