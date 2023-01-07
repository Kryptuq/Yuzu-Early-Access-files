// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <span>
#include <vector>
#include <queue>

#include "common/common_funcs.h"
#include "core/internal_network/sockets.h"
#include "network/network.h"

namespace Network {

class ProxySocket : public SocketBase {
public:
    YUZU_NON_COPYABLE(ProxySocket);
    YUZU_NON_MOVEABLE(ProxySocket);

    explicit ProxySocket(RoomNetwork& room_network_) noexcept;
    ~ProxySocket() override;

    void HandleProxyPacket(const ProxyPacket& packet) override;

    Errno Initialize(Domain domain, Type type, Protocol socket_protocol) override;

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

    std::pair<s32, Errno> ReceivePacket(int flags, std::vector<u8>& message, SockAddrIn* addr,
                                        std::size_t max_length);

    std::pair<s32, Errno> Send(std::span<const u8> message, int flags) override;

    void SendPacket(ProxyPacket& packet);

    std::pair<s32, Errno> SendTo(u32 flags, std::span<const u8> message,
                                 const SockAddrIn* addr) override;

    Errno SetLinger(bool enable, u32 linger) override;

    Errno SetReuseAddr(bool enable) override;

    Errno SetBroadcast(bool enable) override;

    Errno SetKeepAlive(bool enable) override;

    Errno SetSndBuf(u32 value) override;

    Errno SetRcvBuf(u32 value) override;

    Errno SetSndTimeo(u32 value) override;

    Errno SetRcvTimeo(u32 value) override;

    Errno SetNonBlock(bool enable) override;

    template <typename T>
    Errno SetSockOpt(SOCKET fd, int option, T value);

    bool IsOpened() const override;

private:
    bool broadcast = false;
    bool closed = false;
    u32 send_timeout = 0;
    u32 receive_timeout = 0;
    bool is_bound = false;
    SockAddrIn local_endpoint{};
    bool blocking = true;
    std::queue<ProxyPacket> received_packets;
    Protocol protocol;

    std::mutex packets_mutex;

    RoomNetwork& room_network;
};

} // namespace Network
