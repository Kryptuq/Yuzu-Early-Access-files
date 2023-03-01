# SPDX-FileCopyrightText: 2022 Andrea Pappacoda <andrea@pappacoda.it>
#
# SPDX-License-Identifier: GPL-2.0-or-later

include(FindPackageHandleStandardArgs)

find_package(httplib QUIET CONFIG)
if (httplib_CONSIDERED_CONFIGS)
    find_package_handle_standard_args(httplib HANDLE_COMPONENTS CONFIG_MODE)
else()
    find_package(PkgConfig QUIET)
    pkg_search_module(HTTPLIB QUIET IMPORTED_TARGET cpp-httplib)
    if ("-DCPPHTTPLIB_OPENSSL_SUPPORT" IN_LIST HTTPLIB_CFLAGS_OTHER)
        set(httplib_OpenSSL_FOUND TRUE)
    endif()
    if ("-DCPPHTTPLIB_ZLIB_SUPPORT" IN_LIST HTTPLIB_CFLAGS_OTHER)
        set(httplib_ZLIB_FOUND TRUE)
    endif()
    if ("-DCPPHTTPLIB_BROTLI_SUPPORT" IN_LIST HTTPLIB_CFLAGS_OTHER)
        set(httplib_Brotli_FOUND TRUE)
    endif()
    find_package_handle_standard_args(httplib
        REQUIRED_VARS HTTPLIB_INCLUDEDIR
        VERSION_VAR HTTPLIB_VERSION
        HANDLE_COMPONENTS
    )
endif()

if (httplib_FOUND AND NOT TARGET httplib::httplib)
    add_library(httplib::httplib ALIAS PkgConfig::HTTPLIB)
endif()
