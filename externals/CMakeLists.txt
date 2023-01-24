# SPDX-FileCopyrightText: 2016 Citra Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

# Dynarmic has cmake_minimum_required(3.12) and we may want to override
# some of its variables, which is only possible in 3.13+
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# Disable tests in all externals supporting the standard option name
set(BUILD_TESTING OFF)

# xbyak
if ((ARCHITECTURE_x86 OR ARCHITECTURE_x86_64) AND NOT TARGET xbyak::xbyak)
    add_subdirectory(xbyak EXCLUDE_FROM_ALL)
endif()

# Dynarmic
if ((ARCHITECTURE_x86_64 OR ARCHITECTURE_arm64) AND NOT TARGET dynarmic::dynarmic)
    set(DYNARMIC_IGNORE_ASSERTS ON)
    add_subdirectory(dynarmic EXCLUDE_FROM_ALL)
    add_library(dynarmic::dynarmic ALIAS dynarmic)
endif()

# getopt
if (MSVC)
    add_subdirectory(getopt)
endif()

# Glad
add_subdirectory(glad)

# inih
if (NOT TARGET inih::INIReader)
    add_subdirectory(inih)
endif()

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
if (ENABLE_LIBUSB AND NOT TARGET libusb::usb)
    add_subdirectory(libusb EXCLUDE_FROM_ALL)
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
          set(SDL_${_OPT} OFF)
        endforeach()

        set(HIDAPI ON)
    endif()
    set(SDL_STATIC ON)
    set(SDL_SHARED OFF)
    if (APPLE)
        set(SDL_FILE ON)
    endif()

    add_subdirectory(SDL EXCLUDE_FROM_ALL)
endif()

# ENet
if (NOT TARGET enet::enet)
    add_subdirectory(enet EXCLUDE_FROM_ALL)
    target_include_directories(enet INTERFACE ./enet/include)
    add_library(enet::enet ALIAS enet)
endif()

# Cubeb
if (ENABLE_CUBEB AND NOT TARGET cubeb::cubeb)
    set(BUILD_TESTS OFF)
    add_subdirectory(cubeb EXCLUDE_FROM_ALL)
    add_library(cubeb::cubeb ALIAS cubeb)
endif()

# DiscordRPC
if (USE_DISCORD_PRESENCE AND NOT TARGET DiscordRPC::discord-rpc)
    add_subdirectory(discord-rpc EXCLUDE_FROM_ALL)
    target_include_directories(discord-rpc INTERFACE ./discord-rpc/include)
    add_library(DiscordRPC::discord-rpc ALIAS discord-rpc)
endif()

# Sirit
add_subdirectory(sirit EXCLUDE_FROM_ALL)

# httplib
if (ENABLE_WEB_SERVICE AND NOT TARGET httplib::httplib)
    if (NOT WIN32)
        find_package(OpenSSL 1.1)
        if (OPENSSL_FOUND)
            set(OPENSSL_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
        endif()
    endif()

    if (WIN32 OR NOT OPENSSL_FOUND)
        # LibreSSL
        set(LIBRESSL_SKIP_INSTALL ON)
        set(OPENSSLDIR "/etc/ssl/")
        add_subdirectory(libressl EXCLUDE_FROM_ALL)
        target_include_directories(ssl INTERFACE ./libressl/include)
        target_compile_definitions(ssl PRIVATE -DHAVE_INET_NTOP)
        get_directory_property(OPENSSL_LIBRARIES
            DIRECTORY libressl
            DEFINITION OPENSSL_LIBS)
    endif()

    add_library(httplib INTERFACE)
    target_include_directories(httplib INTERFACE ./cpp-httplib)
    target_compile_definitions(httplib INTERFACE -DCPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(httplib INTERFACE ${OPENSSL_LIBRARIES})
    if (WIN32)
        target_link_libraries(httplib INTERFACE crypt32 cryptui ws2_32)
    endif()
    add_library(httplib::httplib ALIAS httplib)
endif()

# cpp-jwt
if (ENABLE_WEB_SERVICE AND NOT TARGET cpp-jwt::cpp-jwt)
    add_library(cpp-jwt INTERFACE)
    target_include_directories(cpp-jwt INTERFACE ./cpp-jwt/include)
    target_compile_definitions(cpp-jwt INTERFACE CPP_JWT_USE_VENDORED_NLOHMANN_JSON)
    add_library(cpp-jwt::cpp-jwt ALIAS cpp-jwt)
endif()

# Opus
if (NOT TARGET Opus::opus)
    add_subdirectory(opus EXCLUDE_FROM_ALL)
endif()

# FFMpeg
if (YUZU_USE_BUNDLED_FFMPEG)
    add_subdirectory(ffmpeg)
    set(FFmpeg_PATH "${FFmpeg_PATH}" PARENT_SCOPE)
    set(FFmpeg_LDFLAGS "${FFmpeg_LDFLAGS}" PARENT_SCOPE)
    set(FFmpeg_LIBRARIES "${FFmpeg_LIBRARIES}" PARENT_SCOPE)
    set(FFmpeg_INCLUDE_DIR "${FFmpeg_INCLUDE_DIR}" PARENT_SCOPE)
endif()

# Vulkan-Headers
if (YUZU_USE_EXTERNAL_VULKAN_HEADERS)
    add_subdirectory(Vulkan-Headers EXCLUDE_FROM_ALL)
endif()

if (NOT TARGET LLVM::Demangle)
    add_library(demangle STATIC)
    target_include_directories(demangle PUBLIC ./demangle)
    target_sources(demangle PRIVATE demangle/ItaniumDemangle.cpp)
    add_library(LLVM::Demangle ALIAS demangle)
endif()
