# SPDX-FileCopyrightText: 2022 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

include(FindPackageHandleStandardArgs)

find_package(lz4 QUIET CONFIG)
if (lz4_FOUND)
    find_package_handle_standard_args(lz4 CONFIG_MODE)
else()
    find_package(PkgConfig QUIET)
    pkg_search_module(LZ4 QUIET IMPORTED_TARGET liblz4)
    find_package_handle_standard_args(lz4
        REQUIRED_VARS LZ4_LINK_LIBRARIES
        VERSION_VAR LZ4_VERSION
    )
endif()

if (lz4_FOUND AND NOT TARGET lz4::lz4)
    if (TARGET LZ4::lz4_shared)
        add_library(lz4::lz4 ALIAS LZ4::lz4_shared)
    elseif (TARGET LZ4::lz4_static)
        add_library(lz4::lz4 ALIAS LZ4::lz4_static)
    else()
        add_library(lz4::lz4 ALIAS PkgConfig::LZ4)
    endif()
endif()
