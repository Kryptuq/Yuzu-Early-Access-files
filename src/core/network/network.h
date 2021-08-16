// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <optional>
#include <utility>

#include "common/common_funcs.h"
#include "common/common_types.h"

#ifdef _WIN32
#include <winsock2.h>
#elif YUZU_UNIX
#include <netinet/in.h>
#endif

namespace Network {

class Socket;

/// Error code for network functions
enum class Errno {
    SUCCESS,
    BADF,
    INVAL,
    MFILE,
    NOTCONN,
    AGAIN,
    CONNREFUSED,
    HOSTUNREACH,
    NETDOWN,
    NETUNREACH,
    OTHER,
};

/// Address families
enum class Domain {
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
enum class Protocol {
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

/// Cross-platform poll fd structure

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
    Socket* socket;
    PollEvents events;
    PollEvents revents;
};

class NetworkInstance {
public:
    explicit NetworkInstance();
    ~NetworkInstance();
};

#ifdef _WIN32
constexpr IPv4Address TranslateIPv4(in_addr addr) {
    auto& bytes = addr.S_un.S_un_b;
    return IPv4Address{bytes.s_b1, bytes.s_b2, bytes.s_b3, bytes.s_b4};
}
#elif YUZU_UNIX
constexpr IPv4Address TranslateIPv4(in_addr addr) {
    const u32 bytes = addr.s_addr;
    return IPv4Address{static_cast<u8>(bytes), static_cast<u8>(bytes >> 8),
                       static_cast<u8>(bytes >> 16), static_cast<u8>(bytes >> 24)};
}
#endif

/// @brief Returns host's IPv4 address
/// @return human ordered IPv4 address (e.g. 192.168.0.1) as an array
std::optional<IPv4Address> GetHostIPv4Address();

} // namespace Network
