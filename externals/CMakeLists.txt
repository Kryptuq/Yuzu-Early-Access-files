# SPDX-FileCopyrightText: 2016 Citra Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/externals/find-modules")
include(DownloadExternals)

# xbyak
if (ARCHITECTURE_x86 OR ARCHITECTURE_x86_64)
    add_subdirectory(xbyak)
endif()

# Dynarmic
if (ARCHITECTURE_x86_64 OR ARCHITECTURE_arm64)
    if (ARCHITECTURE_arm64)
       set(DYNARMIC_FRONTENDS "A32")
    endif()
    set(DYNARMIC_NO_BUNDLED_FMT ON)
    set(DYNARMIC_IGNORE_ASSERTS ON CACHE BOOL "" FORCE)
    add_subdirectory(dynarmic)
endif()

# getopt
if (MSVC)
    add_subdirectory(getopt)
endif()

# Glad
add_subdirectory(glad)

# inih
add_subdirectory(inih)

# mbedtls
add_subdirectory(mbedtls EXCLUDE_FROM_ALL)
target_include_directories(mbedtls PUBLIC ./mbedtls/include)

# MicroProfile
add_library(microprofile INTERFACE)
target_include_directories(microprofile INTERFACE ./microprofile)

# GCC bugs
if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND MINGW)
    target_compile_options(microprofile INTERFACE "-Wno-array-bounds")
endif()

# libusb
if (NOT LIBUSB_FOUND OR YUZU_USE_BUNDLED_LIBUSB)
    add_subdirectory(libusb)
endif()

# SDL2
if (YUZU_USE_EXTERNAL_SDL2)
    if (NOT WIN32)
        # Yuzu itself needs: Atomic Audio Events Joystick Haptic Sensor Threads Timers
        # Since 2.0.18 Atomic+Threads required for HIDAPI/libusb (see https://github.com/libsdl-org/SDL/issues/5095)
        # Yuzu-cmd also needs: Video (depends on Loadso/Dlopen)
        set(SDL_UNUSED_SUBSYSTEMS
            CPUinfo File Filesystem
            Locale Power Render)
        foreach(_SUB ${SDL_UNUSED_SUBSYSTEMS})
          string(TOUPPER ${_SUB} _OPT)
          option(SDL_${_OPT} "" OFF)
        endforeach()

        option(HIDAPI "" ON)
    endif()
    set(SDL_STATIC ON)
    set(SDL_SHARED OFF)
    if (APPLE)
        set(SDL_FILE ON)
    endif()

    add_subdirectory(SDL EXCLUDE_FROM_ALL)
    add_library(SDL2 ALIAS SDL2-static)
endif()

# ENet
add_subdirectory(enet)
target_include_directories(enet INTERFACE ./enet/include)

# Cubeb
if(ENABLE_CUBEB)
    set(BUILD_TESTS OFF CACHE BOOL "")
    add_subdirectory(cubeb EXCLUDE_FROM_ALL)
endif()

# DiscordRPC
if (USE_DISCORD_PRESENCE)
    add_subdirectory(discord-rpc EXCLUDE_FROM_ALL)
    target_include_directories(discord-rpc INTERFACE ./discord-rpc/include)
endif()

# Sirit
add_subdirectory(sirit)

if (ENABLE_WEB_SERVICE)
    if (NOT WIN32)
        find_package(OpenSSL 1.1)
        if (OPENSSL_FOUND)
            set(OPENSSL_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
        endif()
    endif()

    if (WIN32 OR NOT OPENSSL_FOUND)
        # LibreSSL
        set(LIBRESSL_SKIP_INSTALL ON CACHE BOOL "")
        set(OPENSSLDIR "/etc/ssl/")
        add_subdirectory(libressl EXCLUDE_FROM_ALL)
        target_include_directories(ssl INTERFACE ./libressl/include)
        target_compile_definitions(ssl PRIVATE -DHAVE_INET_NTOP)
        get_directory_property(OPENSSL_LIBRARIES
            DIRECTORY libressl
            DEFINITION OPENSSL_LIBS)
    endif()

    # httplib
    add_library(httplib INTERFACE)
    target_include_directories(httplib INTERFACE ./cpp-httplib)
    target_compile_definitions(httplib INTERFACE -DCPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(httplib INTERFACE ${OPENSSL_LIBRARIES})
    if (WIN32)
        target_link_libraries(httplib INTERFACE crypt32 cryptui ws2_32)
    endif()
	
    # cpp-jwt
    add_library(cpp-jwt INTERFACE)
    target_include_directories(cpp-jwt INTERFACE ./cpp-jwt/include)
    target_compile_definitions(cpp-jwt INTERFACE CPP_JWT_USE_VENDORED_NLOHMANN_JSON)
endif()

# Opus
if (YUZU_USE_BUNDLED_OPUS)
    add_subdirectory(opus EXCLUDE_FROM_ALL)
else()
    find_package(Opus 1.3 REQUIRED)
endif()

# FFMpeg
if (YUZU_USE_BUNDLED_FFMPEG)
    add_subdirectory(ffmpeg)
    set(FFmpeg_PATH "${FFmpeg_PATH}" PARENT_SCOPE)
    set(FFmpeg_LDFLAGS "${FFmpeg_LDFLAGS}" PARENT_SCOPE)
    set(FFmpeg_LIBRARIES "${FFmpeg_LIBRARIES}" PARENT_SCOPE)
    set(FFmpeg_INCLUDE_DIR "${FFmpeg_INCLUDE_DIR}" PARENT_SCOPE)
endif()
