# SPDX-FileCopyrightText: 2022 Andrea Pappacoda <andrea@pappacoda.it>
#
# SPDX-License-Identifier: GPL-2.0-or-later

include(FindPackageHandleStandardArgs)

find_package(httplib QUIET CONFIG)
if (httplib_FOUND)
    find_package_handle_standard_args(httplib CONFIG_MODE)
else()
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_search_module(HTTPLIB QUIET IMPORTED_TARGET GLOBAL cpp-httplib)
        if (HTTPLIB_FOUND)
            add_library(httplib::httplib ALIAS PkgConfig::HTTPLIB)
        endif()
    endif()
    find_package_handle_standard_args(httplib
        REQUIRED_VARS HTTPLIB_INCLUDEDIR
        VERSION_VAR HTTPLIB_VERSION
    )
endif()
