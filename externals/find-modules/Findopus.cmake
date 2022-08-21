# SPDX-FileCopyrightText: 2022 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
    pkg_search_module(opus IMPORTED_TARGET GLOBAL opus)
    if (opus_FOUND)
        add_library(Opus::opus ALIAS PkgConfig::opus)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Opus
    REQUIRED_VARS
        opus_LINK_LIBRARIES
        opus_FOUND
    VERSION_VAR opus_VERSION
)
