// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <span>
#include <vector>

#include "common/common_types.h"
#include "common/socket_types.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sockets/sockets.h"
#include "network/network.h"

namespace Core {
class System;
}

namespace Network {
class SocketBase;
class Socket;
} // namespace Network

namespace Service::Sockets {

class BSD final : public ServiceFramework<BSD> {
public:
    explicit BSD(Core::System& system_, const char* name);
    ~BSD() override;

private:
    /// Maximum number of file descriptors
    static constexpr size_t MAX_FD = 128;

    struct FileDescriptor {
        std::unique_ptr<Network::SocketBase> socket;
        s32 flags = 0;
        bool is_connection_based = false;
    };

    struct PollWork {
        void Execute(BSD* bsd);
        void Response(Kernel::HLERequestContext& ctx);

        s32 nfds;
        s32 timeout;
        std::span<const u8> read_buffer;
        std::vector<u8> write_buffer;
        s32 ret{};
        Errno bsd_errno{};
    };

    struct AcceptWork {
        void Execute(BSD* bsd);
        void Response(Kernel::HLERequestContext& ctx);

        s32 fd;
        std::vector<u8> write_buffer;
        s32 ret{};
        Errno bsd_errno{};
    };

    struct ConnectWork {
        void Execute(BSD* bsd);
        void Response(Kernel::HLERequestContext& ctx);

        s32 fd;
        std::span<const u8> addr;
        Errno bsd_errno{};
    };

    struct RecvWork {
        void Execute(BSD* bsd);
        void Response(Kernel::HLERequestContext& ctx);

        s32 fd;
        u32 flags;
        std::vector<u8> message;
        s32 ret{};
        Errno bsd_errno{};
    };

    struct RecvFromWork {
        void Execute(BSD* bsd);
        void Response(Kernel::HLERequestContext& ctx);

        s32 fd;
        u32 flags;
        std::vector<u8> message;
        std::vector<u8> addr;
        s32 ret{};
        Errno bsd_errno{};
    };

    struct SendWork {
        void Execute(BSD* bsd);
        void Response(Kernel::HLERequestContext& ctx);

        s32 fd;
        u32 flags;
        std::span<const u8> message;
        s32 ret{};
        Errno bsd_errno{};
    };

    struct SendToWork {
        void Execute(BSD* bsd);
        void Response(Kernel::HLERequestContext& ctx);

        s32 fd;
        u32 flags;
        std::span<const u8> message;
        std::span<const u8> addr;
        s32 ret{};
        Errno bsd_errno{};
    };

    void RegisterClient(Kernel::HLERequestContext& ctx);
    void StartMonitoring(Kernel::HLERequestContext& ctx);
    void Socket(Kernel::HLERequestContext& ctx);
    void Select(Kernel::HLERequestContext& ctx);
    void Poll(Kernel::HLERequestContext& ctx);
    void Accept(Kernel::HLERequestContext& ctx);
    void Bind(Kernel::HLERequestContext& ctx);
    void Connect(Kernel::HLERequestContext& ctx);
    void GetPeerName(Kernel::HLERequestContext& ctx);
    void GetSockName(Kernel::HLERequestContext& ctx);
    void GetSockOpt(Kernel::HLERequestContext& ctx);
    void Listen(Kernel::HLERequestContext& ctx);
    void Fcntl(Kernel::HLERequestContext& ctx);
    void SetSockOpt(Kernel::HLERequestContext& ctx);
    void Shutdown(Kernel::HLERequestContext& ctx);
    void Recv(Kernel::HLERequestContext& ctx);
    void RecvFrom(Kernel::HLERequestContext& ctx);
    void Send(Kernel::HLERequestContext& ctx);
    void SendTo(Kernel::HLERequestContext& ctx);
    void Write(Kernel::HLERequestContext& ctx);
    void Read(Kernel::HLERequestContext& ctx);
    void Close(Kernel::HLERequestContext& ctx);
    void EventFd(Kernel::HLERequestContext& ctx);

    template <typename Work>
    void ExecuteWork(Kernel::HLERequestContext& ctx, Work work);

    std::pair<s32, Errno> SocketImpl(Domain domain, Type type, Protocol protocol);
    std::pair<s32, Errno> PollImpl(std::vector<u8>& write_buffer, std::span<const u8> read_buffer,
                                   s32 nfds, s32 timeout);
    std::pair<s32, Errno> AcceptImpl(s32 fd, std::vector<u8>& write_buffer);
    Errno BindImpl(s32 fd, std::span<const u8> addr);
    Errno ConnectImpl(s32 fd, std::span<const u8> addr);
    Errno GetPeerNameImpl(s32 fd, std::vector<u8>& write_buffer);
    Errno GetSockNameImpl(s32 fd, std::vector<u8>& write_buffer);
    Errno ListenImpl(s32 fd, s32 backlog);
    std::pair<s32, Errno> FcntlImpl(s32 fd, FcntlCmd cmd, s32 arg);
    Errno SetSockOptImpl(s32 fd, u32 level, OptName optname, size_t optlen, const void* optval);
    Errno ShutdownImpl(s32 fd, s32 how);
    std::pair<s32, Errno> RecvImpl(s32 fd, u32 flags, std::vector<u8>& message);
    std::pair<s32, Errno> RecvFromImpl(s32 fd, u32 flags, std::vector<u8>& message,
                                       std::vector<u8>& addr);
    std::pair<s32, Errno> SendImpl(s32 fd, u32 flags, std::span<const u8> message);
    std::pair<s32, Errno> SendToImpl(s32 fd, u32 flags, std::span<const u8> message,
                                     std::span<const u8> addr);
    Errno CloseImpl(s32 fd);

    s32 FindFreeFileDescriptorHandle() noexcept;
    bool IsFileDescriptorValid(s32 fd) const noexcept;

    void BuildErrnoResponse(Kernel::HLERequestContext& ctx, Errno bsd_errno) const noexcept;

    std::array<std::optional<FileDescriptor>, MAX_FD> file_descriptors;

    Network::RoomNetwork& room_network;

    /// Callback to parse and handle a received wifi packet.
    void OnProxyPacketReceived(const Network::ProxyPacket& packet);

    // Callback identifier for the OnProxyPacketReceived event.
    Network::RoomMember::CallbackHandle<Network::ProxyPacket> proxy_packet_received;
};

class BSDCFG final : public ServiceFramework<BSDCFG> {
public:
    explicit BSDCFG(Core::System& system_);
    ~BSDCFG() override;
};

} // namespace Service::Sockets
