// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <utility>

#if defined(_WIN32)
#include <winsock.h>
#elif !YUZU_UNIX
#error "Platform not implemented"
#endif

#include "common/common_types.h"
#include "core/network/network.h"

// TODO: C++20 Replace std::vector usages with std::span

namespace Network {

class Socket {
public:
    struct AcceptResult {
        std::unique_ptr<Socket> socket;
        SockAddrIn sockaddr_in;
    };

    explicit Socket() = default;
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& rhs) noexcept;

    // Avoid closing sockets implicitly
    Socket& operator=(Socket&&) noexcept = delete;

    Errno Initialize(Domain domain, Type type, Protocol protocol);

    Errno Close();

    std::pair<AcceptResult, Errno> Accept();

    Errno Connect(SockAddrIn addr_in);

    std::pair<SockAddrIn, Errno> GetPeerName();

    std::pair<SockAddrIn, Errno> GetSockName();

    Errno Bind(SockAddrIn addr);

    Errno Listen(s32 backlog);

    Errno Shutdown(ShutdownHow how);

    std::pair<s32, Errno> Recv(int flags, std::vector<u8>& message);

    std::pair<s32, Errno> RecvFrom(int flags, std::vector<u8>& message, SockAddrIn* addr);

    std::pair<s32, Errno> Send(const std::vector<u8>& message, int flags);

    std::pair<s32, Errno> SendTo(u32 flags, const std::vector<u8>& message, const SockAddrIn* addr);

    Errno SetLinger(bool enable, u32 linger);

    Errno SetReuseAddr(bool enable);

    Errno SetBroadcast(bool enable);

    Errno SetSndBuf(u32 value);

    Errno SetRcvBuf(u32 value);

    Errno SetSndTimeo(u32 value);

    Errno SetRcvTimeo(u32 value);

    Errno SetNonBlock(bool enable);

    bool IsOpened() const;

#if defined(_WIN32)
    SOCKET fd = INVALID_SOCKET;
#elif YUZU_UNIX
    int fd = -1;
#endif
};

std::pair<s32, Errno> Poll(std::vector<PollFD>& poll_fds, s32 timeout);

} // namespace Network
