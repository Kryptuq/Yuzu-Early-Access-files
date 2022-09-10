// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <thread>

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/zstd_compression.h"
#include "core/internal_network/network.h"
#include "core/internal_network/network_interface.h"
#include "core/internal_network/socket_proxy.h"

namespace Network {

ProxySocket::ProxySocket(RoomNetwork& room_network_) noexcept : room_network{room_network_} {}

ProxySocket::~ProxySocket() {
    if (fd == INVALID_SOCKET) {
        return;
    }
    fd = INVALID_SOCKET;
}

void ProxySocket::HandleProxyPacket(const ProxyPacket& packet) {
    if (protocol != packet.protocol || local_endpoint.portno != packet.remote_endpoint.portno ||
        closed) {
        return;
    }

    if (!broadcast && packet.broadcast) {
        LOG_INFO(Network, "Received broadcast packet, but not configured for broadcast mode");
        return;
    }

    auto decompressed = packet;
    decompressed.data = Common::Compression::DecompressDataZSTD(packet.data);

    std::lock_guard guard(packets_mutex);
    received_packets.push(decompressed);
}

template <typename T>
Errno ProxySocket::SetSockOpt(SOCKET fd_, int option, T value) {
    LOG_DEBUG(Network, "(STUBBED) called");
    return Errno::SUCCESS;
}

Errno ProxySocket::Initialize(Domain domain, Type type, Protocol socket_protocol) {
    protocol = socket_protocol;
    SetSockOpt(fd, SO_TYPE, type);

    return Errno::SUCCESS;
}

std::pair<ProxySocket::AcceptResult, Errno> ProxySocket::Accept() {
    LOG_WARNING(Network, "(STUBBED) called");
    return {AcceptResult{}, Errno::SUCCESS};
}

Errno ProxySocket::Connect(SockAddrIn addr_in) {
    LOG_WARNING(Network, "(STUBBED) called");
    return Errno::SUCCESS;
}

std::pair<SockAddrIn, Errno> ProxySocket::GetPeerName() {
    LOG_WARNING(Network, "(STUBBED) called");
    return {SockAddrIn{}, Errno::SUCCESS};
}

std::pair<SockAddrIn, Errno> ProxySocket::GetSockName() {
    LOG_WARNING(Network, "(STUBBED) called");
    return {SockAddrIn{}, Errno::SUCCESS};
}

Errno ProxySocket::Bind(SockAddrIn addr) {
    if (is_bound) {
        LOG_WARNING(Network, "Rebinding Socket is unimplemented!");
        return Errno::SUCCESS;
    }
    local_endpoint = addr;
    is_bound = true;

    return Errno::SUCCESS;
}

Errno ProxySocket::Listen(s32 backlog) {
    LOG_WARNING(Network, "(STUBBED) called");
    return Errno::SUCCESS;
}

Errno ProxySocket::Shutdown(ShutdownHow how) {
    LOG_WARNING(Network, "(STUBBED) called");
    return Errno::SUCCESS;
}

std::pair<s32, Errno> ProxySocket::Recv(int flags, std::vector<u8>& message) {
    LOG_WARNING(Network, "(STUBBED) called");
    ASSERT(flags == 0);
    ASSERT(message.size() < static_cast<size_t>(std::numeric_limits<int>::max()));

    return {static_cast<s32>(0), Errno::SUCCESS};
}

std::pair<s32, Errno> ProxySocket::RecvFrom(int flags, std::vector<u8>& message, SockAddrIn* addr) {
    ASSERT(flags == 0);
    ASSERT(message.size() < static_cast<size_t>(std::numeric_limits<int>::max()));

    // TODO (flTobi): Verify the timeout behavior and break when connection is lost
    const auto timestamp = std::chrono::steady_clock::now();
    // When receive_timeout is set to zero, the socket is supposed to wait indefinitely until a
    // packet arrives. In order to prevent lost packets from hanging the emulation thread, we set
    // the timeout to 5s instead
    const auto timeout = receive_timeout == 0 ? 5000 : receive_timeout;
    while (true) {
        {
            std::lock_guard guard(packets_mutex);
            if (received_packets.size() > 0) {
                return ReceivePacket(flags, message, addr, message.size());
            }
        }

        if (!blocking) {
            return {-1, Errno::AGAIN};
        }

        std::this_thread::yield();

        const auto time_diff = std::chrono::steady_clock::now() - timestamp;
        const auto time_diff_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(time_diff).count();

        if (time_diff_ms > timeout) {
            return {-1, Errno::TIMEDOUT};
        }
    }
}

std::pair<s32, Errno> ProxySocket::ReceivePacket(int flags, std::vector<u8>& message,
                                                 SockAddrIn* addr, std::size_t max_length) {
    ProxyPacket& packet = received_packets.front();
    if (addr) {
        addr->family = Domain::INET;
        addr->ip = packet.local_endpoint.ip;         // The senders ip address
        addr->portno = packet.local_endpoint.portno; // The senders port number
    }

    bool peek = (flags & FLAG_MSG_PEEK) != 0;
    std::size_t read_bytes;
    if (packet.data.size() > max_length) {
        read_bytes = max_length;
        message.clear();
        std::copy(packet.data.begin(), packet.data.begin() + read_bytes,
                  std::back_inserter(message));
        message.resize(max_length);

        if (protocol == Protocol::UDP) {
            if (!peek) {
                received_packets.pop();
            }
            return {-1, Errno::MSGSIZE};
        } else if (protocol == Protocol::TCP) {
            std::vector<u8> numArray(packet.data.size() - max_length);
            std::copy(packet.data.begin() + max_length, packet.data.end(),
                      std::back_inserter(numArray));
            packet.data = numArray;
        }
    } else {
        read_bytes = packet.data.size();
        message.clear();
        std::copy(packet.data.begin(), packet.data.end(), std::back_inserter(message));
        message.resize(max_length);
        if (!peek) {
            received_packets.pop();
        }
    }

    return {static_cast<u32>(read_bytes), Errno::SUCCESS};
}

std::pair<s32, Errno> ProxySocket::Send(const std::vector<u8>& message, int flags) {
    LOG_WARNING(Network, "(STUBBED) called");
    ASSERT(message.size() < static_cast<size_t>(std::numeric_limits<int>::max()));
    ASSERT(flags == 0);

    return {static_cast<s32>(0), Errno::SUCCESS};
}

void ProxySocket::SendPacket(ProxyPacket& packet) {
    if (auto room_member = room_network.GetRoomMember().lock()) {
        if (room_member->IsConnected()) {
            packet.data = Common::Compression::CompressDataZSTDDefault(packet.data.data(),
                                                                       packet.data.size());
            room_member->SendProxyPacket(packet);
        }
    }
}

std::pair<s32, Errno> ProxySocket::SendTo(u32 flags, const std::vector<u8>& message,
                                          const SockAddrIn* addr) {
    ASSERT(flags == 0);

    if (!is_bound) {
        LOG_ERROR(Network, "ProxySocket is not bound!");
        return {static_cast<s32>(message.size()), Errno::SUCCESS};
    }

    if (auto room_member = room_network.GetRoomMember().lock()) {
        if (!room_member->IsConnected()) {
            return {static_cast<s32>(message.size()), Errno::SUCCESS};
        }
    }

    ProxyPacket packet;
    packet.local_endpoint = local_endpoint;
    packet.remote_endpoint = *addr;
    packet.protocol = protocol;
    packet.broadcast = broadcast && packet.remote_endpoint.ip[3] == 255;

    auto& ip = local_endpoint.ip;
    auto ipv4 = Network::GetHostIPv4Address();
    // If the ip is all zeroes (INADDR_ANY) or if it matches the hosts ip address,
    // replace it with a "fake" routing address
    if (std::all_of(ip.begin(), ip.end(), [](u8 i) { return i == 0; }) || (ipv4 && ipv4 == ip)) {
        if (auto room_member = room_network.GetRoomMember().lock()) {
            packet.local_endpoint.ip = room_member->GetFakeIpAddress();
        }
    }

    packet.data.clear();
    std::copy(message.begin(), message.end(), std::back_inserter(packet.data));

    SendPacket(packet);

    return {static_cast<s32>(message.size()), Errno::SUCCESS};
}

Errno ProxySocket::Close() {
    fd = INVALID_SOCKET;
    closed = true;

    return Errno::SUCCESS;
}

Errno ProxySocket::SetLinger(bool enable, u32 linger) {
    struct Linger {
        u16 linger_enable;
        u16 linger_time;
    } values;
    values.linger_enable = enable ? 1 : 0;
    values.linger_time = static_cast<u16>(linger);

    return SetSockOpt(fd, SO_LINGER, values);
}

Errno ProxySocket::SetReuseAddr(bool enable) {
    return SetSockOpt<u32>(fd, SO_REUSEADDR, enable ? 1 : 0);
}

Errno ProxySocket::SetBroadcast(bool enable) {
    broadcast = enable;
    return SetSockOpt<u32>(fd, SO_BROADCAST, enable ? 1 : 0);
}

Errno ProxySocket::SetSndBuf(u32 value) {
    return SetSockOpt(fd, SO_SNDBUF, value);
}

Errno ProxySocket::SetKeepAlive(bool enable) {
    return Errno::SUCCESS;
}

Errno ProxySocket::SetRcvBuf(u32 value) {
    return SetSockOpt(fd, SO_RCVBUF, value);
}

Errno ProxySocket::SetSndTimeo(u32 value) {
    send_timeout = value;
    return SetSockOpt(fd, SO_SNDTIMEO, static_cast<int>(value));
}

Errno ProxySocket::SetRcvTimeo(u32 value) {
    receive_timeout = value;
    return SetSockOpt(fd, SO_RCVTIMEO, static_cast<int>(value));
}

Errno ProxySocket::SetNonBlock(bool enable) {
    blocking = !enable;
    return Errno::SUCCESS;
}

bool ProxySocket::IsOpened() const {
    return fd != INVALID_SOCKET;
}

} // namespace Network
