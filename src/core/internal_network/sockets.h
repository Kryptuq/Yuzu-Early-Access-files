// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <memory>
#include <utility>

#if defined(_WIN32)
#elif !YUZU_UNIX
#error "Platform not implemented"
#endif

#include "common/common_types.h"
#include "core/internal_network/network.h"
#include "network/network.h"

// TODO: C++20 Replace std::vector usages with std::span

namespace Network {

class SocketBase {
public:
#ifdef YUZU_UNIX
    using SOCKET = int;
    static constexpr SOCKET INVALID_SOCKET = -1;
    static constexpr SOCKET SOCKET_ERROR = -1;
#endif

    struct AcceptResult {
        std::unique_ptr<SocketBase> socket;
        SockAddrIn sockaddr_in;
    };

    SocketBase() = default;
    explicit SocketBase(SOCKET fd_) : fd{fd_} {}

    virtual ~SocketBase() = default;

    virtual SocketBase& operator=(const SocketBase&) = delete;

    // Avoid closing sockets implicitly
    virtual SocketBase& operator=(SocketBase&&) noexcept = delete;

    virtual Errno Initialize(Domain domain, Type type, Protocol protocol) = 0;

    virtual Errno Close() = 0;

    virtual std::pair<AcceptResult, Errno> Accept() = 0;

    virtual Errno Connect(SockAddrIn addr_in) = 0;

    virtual std::pair<SockAddrIn, Errno> GetPeerName() = 0;

    virtual std::pair<SockAddrIn, Errno> GetSockName() = 0;

    virtual Errno Bind(SockAddrIn addr) = 0;

    virtual Errno Listen(s32 backlog) = 0;

    virtual Errno Shutdown(ShutdownHow how) = 0;

    virtual std::pair<s32, Errno> Recv(int flags, std::vector<u8>& message) = 0;

    virtual std::pair<s32, Errno> RecvFrom(int flags, std::vector<u8>& message,
                                           SockAddrIn* addr) = 0;

    virtual std::pair<s32, Errno> Send(const std::vector<u8>& message, int flags) = 0;

    virtual std::pair<s32, Errno> SendTo(u32 flags, const std::vector<u8>& message,
                                         const SockAddrIn* addr) = 0;

    virtual Errno SetLinger(bool enable, u32 linger) = 0;

    virtual Errno SetReuseAddr(bool enable) = 0;

    virtual Errno SetKeepAlive(bool enable) = 0;

    virtual Errno SetBroadcast(bool enable) = 0;

    virtual Errno SetSndBuf(u32 value) = 0;

    virtual Errno SetRcvBuf(u32 value) = 0;

    virtual Errno SetSndTimeo(u32 value) = 0;

    virtual Errno SetRcvTimeo(u32 value) = 0;

    virtual Errno SetNonBlock(bool enable) = 0;

    virtual bool IsOpened() const = 0;

    virtual void HandleProxyPacket(const ProxyPacket& packet) = 0;

    [[nodiscard]] SOCKET GetFD() const {
        return fd;
    }

protected:
    SOCKET fd = INVALID_SOCKET;
};

class Socket : public SocketBase {
public:
    Socket() = default;
    explicit Socket(SOCKET fd_) : SocketBase{fd_} {}

    ~Socket() override;

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& rhs) noexcept;

    // Avoid closing sockets implicitly
    Socket& operator=(Socket&&) noexcept = delete;

    Errno Initialize(Domain domain, Type type, Protocol protocol) override;

    Errno Close() override;

    std::pair<AcceptResult, Errno> Accept() override;

    Errno Connect(SockAddrIn addr_in) override;

    std::pair<SockAddrIn, Errno> GetPeerName() override;

    std::pair<SockAddrIn, Errno> GetSockName() override;

    Errno Bind(SockAddrIn addr) override;

    Errno Listen(s32 backlog) override;

    Errno Shutdown(ShutdownHow how) override;

    std::pair<s32, Errno> Recv(int flags, std::vector<u8>& message) override;

    std::pair<s32, Errno> RecvFrom(int flags, std::vector<u8>& message, SockAddrIn* addr) override;

    std::pair<s32, Errno> Send(const std::vector<u8>& message, int flags) override;

    std::pair<s32, Errno> SendTo(u32 flags, const std::vector<u8>& message,
                                 const SockAddrIn* addr) override;

    Errno SetLinger(bool enable, u32 linger) override;

    Errno SetReuseAddr(bool enable) override;

    Errno SetKeepAlive(bool enable) override;

    Errno SetBroadcast(bool enable) override;

    Errno SetSndBuf(u32 value) override;

    Errno SetRcvBuf(u32 value) override;

    Errno SetSndTimeo(u32 value) override;

    Errno SetRcvTimeo(u32 value) override;

    Errno SetNonBlock(bool enable) override;

    template <typename T>
    Errno SetSockOpt(SOCKET fd, int option, T value);

    bool IsOpened() const override;

    void HandleProxyPacket(const ProxyPacket& packet) override;
};

std::pair<s32, Errno> Poll(std::vector<PollFD>& poll_fds, s32 timeout);

} // namespace Network
