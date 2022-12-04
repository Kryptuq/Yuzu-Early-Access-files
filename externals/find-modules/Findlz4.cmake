# SPDX-FileCopyrightText: 2022 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

include(FindPackageHandleStandardArgs)

find_package(lz4 QUIET CONFIG)
if (lz4_FOUND)
    find_package_handle_standard_args(lz4 CONFIG_MODE)
    if (NOT TARGET lz4::lz4)
        if (TARGET LZ4::lz4_shared)
            set_target_properties(LZ4::lz4_shared PROPERTIES IMPORTED_GLOBAL TRUE)
            add_library(lz4::lz4 ALIAS LZ4::lz4_shared)
        else()
            set_target_properties(LZ4::lz4_static PROPERTIES IMPORTED_GLOBAL TRUE)
            add_library(lz4::lz4 ALIAS LZ4::lz4_static)
        endif()
    endif()
else()
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_search_module(liblz4 QUIET IMPORTED_TARGET GLOBAL liblz4)
        if (liblz4_FOUND)
            add_library(lz4::lz4 ALIAS PkgConfig::liblz4)
        endif()
    endif()
    find_package_handle_standard_args(lz4
        REQUIRED_VARS liblz4_LINK_LIBRARIES
        VERSION_VAR liblz4_VERSION
    )
endif()
