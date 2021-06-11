// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Sockets {

enum class Errno : u32 {
    SUCCESS = 0,
    BADF = 9,
    AGAIN = 11,
    INVAL = 22,
    MFILE = 24,
    NOTCONN = 107,
};

enum class Domain : u32 {
    INET = 2,
};

enum class Type : u32 {
    STREAM = 1,
    DGRAM = 2,
    RAW = 3,
    SEQPACKET = 5,
};

enum class Protocol : u32 {
    UNSPECIFIED = 0,
    ICMP = 1,
    TCP = 6,
    UDP = 17,
};

enum class OptName : u32 {
    REUSEADDR = 0x4,
    BROADCAST = 0x20,
    LINGER = 0x80,
    SNDBUF = 0x1001,
    RCVBUF = 0x1002,
    SNDTIMEO = 0x1005,
    RCVTIMEO = 0x1006,
};

enum class ShutdownHow : s32 {
    RD = 0,
    WR = 1,
    RDWR = 2,
};

enum class FcntlCmd : s32 {
    GETFL = 3,
    SETFL = 4,
};

struct SockAddrIn {
    u8 len;
    u8 family;
    u16 portno;
    std::array<u8, 4> ip;
    std::array<u8, 8> zeroes;
};

enum class PollEvents : u16 {
    // Using Pascal case because IN is a macro on Windows.
    In = 1 << 0,
    Pri = 1 << 1,
    Out = 1 << 2,
    Err = 1 << 3,
    Hup = 1 << 4,
    Nval = 1 << 5,
};

DECLARE_ENUM_FLAG_OPERATORS(PollEvents);

struct PollFD {
    s32 fd;
    PollEvents events;
    PollEvents revents;
};

struct Linger {
    u32 onoff;
    u32 linger;
};

constexpr u32 FLAG_MSG_DONTWAIT = 0x80;

constexpr u32 FLAG_O_NONBLOCK = 0x800;

/// Registers all Sockets services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system);

} // namespace Service::Sockets
