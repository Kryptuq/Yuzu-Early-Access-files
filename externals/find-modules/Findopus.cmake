# SPDX-FileCopyrightText: 2022 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_search_module(OPUS QUIET IMPORTED_TARGET opus)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Opus
    REQUIRED_VARS OPUS_LINK_LIBRARIES
    VERSION_VAR OPUS_VERSION
)

if (Opus_FOUND AND NOT TARGET Opus::opus)
    add_library(Opus::opus ALIAS PkgConfig::OPUS)
endif()
