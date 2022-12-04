# SPDX-FileCopyrightText: 2022 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

include(FindPackageHandleStandardArgs)

find_package(zstd QUIET CONFIG)
if (zstd_FOUND)
    find_package_handle_standard_args(zstd CONFIG_MODE)
    if (NOT TARGET zstd::zstd)
        if (TARGET zstd::libzstd_shared)
            set_target_properties(zstd::libzstd_shared PROPERTIES IMPORTED_GLOBAL TRUE)
            add_library(zstd::zstd ALIAS zstd::libzstd_shared)
        else()
            set_target_properties(zstd::libzstd_static PROPERTIES IMPORTED_GLOBAL TRUE)
            add_library(zstd::zstd ALIAS zstd::libzstd_static)
        endif()
    endif()
else()
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_search_module(libzstd QUIET IMPORTED_TARGET GLOBAL libzstd)
        if (libzstd_FOUND)
            add_library(zstd::zstd ALIAS PkgConfig::libzstd)
        endif()
    endif()
    find_package_handle_standard_args(zstd
        REQUIRED_VARS libzstd_LINK_LIBRARIES
        VERSION_VAR libzstd_VERSION
    )
endif()
