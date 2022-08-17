// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

namespace Network {

/// Address families
enum class Domain : u8 {
    INET, ///< Address family for IPv4
};

/// Socket types
enum class Type {
    STREAM,
    DGRAM,
    RAW,
    SEQPACKET,
};

/// Protocol values for sockets
enum class Protocol : u8 {
    ICMP,
    TCP,
    UDP,
};

/// Shutdown mode
enum class ShutdownHow {
    RD,
    WR,
    RDWR,
};

/// Array of IPv4 address
using IPv4Address = std::array<u8, 4>;

/// Cross-platform sockaddr structure
struct SockAddrIn {
    Domain family;
    IPv4Address ip;
    u16 portno;
};

constexpr u32 FLAG_MSG_PEEK = 0x2;
constexpr u32 FLAG_MSG_DONTWAIT = 0x80;
constexpr u32 FLAG_O_NONBLOCK = 0x800;

} // namespace Network
