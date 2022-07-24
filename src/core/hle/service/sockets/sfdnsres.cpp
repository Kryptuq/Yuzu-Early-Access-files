// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string_view>
#include <utility>
#include <vector>

#include "common/string_util.h"
#include "common/swap.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/sockets/sfdnsres.h"
#include "core/memory.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#elif YUZU_UNIX
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#ifndef EAI_NODATA
#define EAI_NODATA EAI_NONAME
#endif
#endif

namespace Service::Sockets {

SFDNSRES::SFDNSRES(Core::System& system_) : ServiceFramework{system_, "sfdnsres"} {
    static const FunctionInfo functions[] = {
        {0, nullptr, "SetDnsAddressesPrivateRequest"},
        {1, nullptr, "GetDnsAddressPrivateRequest"},
        {2, nullptr, "GetHostByNameRequest"},
        {3, nullptr, "GetHostByAddrRequest"},
        {4, nullptr, "GetHostStringErrorRequest"},
        {5, nullptr, "GetGaiStringErrorRequest"},
        {6, &SFDNSRES::GetAddrInfoRequest, "GetAddrInfoRequest"},
        {7, nullptr, "GetNameInfoRequest"},
        {8, nullptr, "RequestCancelHandleRequest"},
        {9, nullptr, "CancelRequest"},
        {10, nullptr, "GetHostByNameRequestWithOptions"},
        {11, nullptr, "GetHostByAddrRequestWithOptions"},
        {12, &SFDNSRES::GetAddrInfoRequestWithOptions, "GetAddrInfoRequestWithOptions"},
        {13, nullptr, "GetNameInfoRequestWithOptions"},
        {14, nullptr, "ResolverSetOptionRequest"},
        {15, nullptr, "ResolverGetOptionRequest"},
    };
    RegisterHandlers(functions);
}

SFDNSRES::~SFDNSRES() = default;

enum class NetDbError : s32 {
    Internal = -1,
    Success = 0,
    HostNotFound = 1,
    TryAgain = 2,
    NoRecovery = 3,
    NoData = 4,
};

static NetDbError AddrInfoErrorToNetDbError(s32 result) {
    // Best effort guess to map errors
    switch (result) {
    case 0:
        return NetDbError::Success;
    case EAI_AGAIN:
        return NetDbError::TryAgain;
    case EAI_NODATA:
        return NetDbError::NoData;
    default:
        return NetDbError::HostNotFound;
    }
}

static std::vector<u8> SerializeAddrInfo(const addrinfo* addrinfo, s32 result_code,
                                         std::string_view host) {
    // Adapted from
    // https://github.com/switchbrew/libnx/blob/c5a9a909a91657a9818a3b7e18c9b91ff0cbb6e3/nx/source/runtime/resolver.c#L190
    std::vector<u8> data;

    auto* current = addrinfo;
    while (current != nullptr) {
        struct SerializedResponseHeader {
            u32 magic;
            s32 flags;
            s32 family;
            s32 socket_type;
            s32 protocol;
            u32 address_length;
        };
        static_assert(sizeof(SerializedResponseHeader) == 0x18,
                      "Response header size must be 0x18 bytes");

        constexpr auto header_size = sizeof(SerializedResponseHeader);
        const auto addr_size =
            current->ai_addr && current->ai_addrlen > 0 ? current->ai_addrlen : 4;
        const auto canonname_size = current->ai_canonname ? strlen(current->ai_canonname) + 1 : 1;

        const auto last_size = data.size();
        data.resize(last_size + header_size + addr_size + canonname_size);

        // Header in network byte order
        SerializedResponseHeader header{};

        constexpr auto HEADER_MAGIC = 0xBEEFCAFE;
        header.magic = htonl(HEADER_MAGIC);
        header.family = htonl(current->ai_family);
        header.flags = htonl(current->ai_flags);
        header.socket_type = htonl(current->ai_socktype);
        header.protocol = htonl(current->ai_protocol);
        header.address_length = current->ai_addr ? htonl((u32)current->ai_addrlen) : 0;

        auto* header_ptr = data.data() + last_size;
        std::memcpy(header_ptr, &header, header_size);

        if (header.address_length == 0) {
            std::memset(header_ptr + header_size, 0, 4);
        } else {
            switch (current->ai_family) {
            case AF_INET: {
                struct SockAddrIn {
                    s16 sin_family;
                    u16 sin_port;
                    u32 sin_addr;
                    u8 sin_zero[8];
                };

                SockAddrIn serialized_addr{};
                const auto addr = *reinterpret_cast<sockaddr_in*>(current->ai_addr);
                serialized_addr.sin_port = htons(addr.sin_port);
                serialized_addr.sin_family = htons(addr.sin_family);
                serialized_addr.sin_addr = htonl(addr.sin_addr.s_addr);
                std::memcpy(header_ptr + header_size, &serialized_addr, sizeof(SockAddrIn));

                char addr_string_buf[64]{};
                inet_ntop(AF_INET, &addr.sin_addr, addr_string_buf, std::size(addr_string_buf));
                LOG_INFO(Service, "Resolved host '{}' to IPv4 address {}", host, addr_string_buf);
                break;
            }
            case AF_INET6: {
                struct SockAddrIn6 {
                    s16 sin6_family;
                    u16 sin6_port;
                    u32 sin6_flowinfo;
                    u8 sin6_addr[16];
                    u32 sin6_scope_id;
                };

                SockAddrIn6 serialized_addr{};
                const auto addr = *reinterpret_cast<sockaddr_in6*>(current->ai_addr);
                serialized_addr.sin6_family = htons(addr.sin6_family);
                serialized_addr.sin6_port = htons(addr.sin6_port);
                serialized_addr.sin6_flowinfo = htonl(addr.sin6_flowinfo);
                serialized_addr.sin6_scope_id = htonl(addr.sin6_scope_id);
                std::memcpy(serialized_addr.sin6_addr, &addr.sin6_addr,
                            sizeof(SockAddrIn6::sin6_addr));
                std::memcpy(header_ptr + header_size, &serialized_addr, sizeof(SockAddrIn6));

                char addr_string_buf[64]{};
                inet_ntop(AF_INET6, &addr.sin6_addr, addr_string_buf, std::size(addr_string_buf));
                LOG_INFO(Service, "Resolved host '{}' to IPv6 address {}", host, addr_string_buf);
                break;
            }
            default:
                std::memcpy(header_ptr + header_size, current->ai_addr, addr_size);
                break;
            }
        }
        if (current->ai_canonname) {
            std::memcpy(header_ptr + addr_size, current->ai_canonname, canonname_size);
        } else {
            *(header_ptr + header_size + addr_size) = 0;
        }

        current = current->ai_next;
    }

    // 4-byte sentinel value
    data.push_back(0);
    data.push_back(0);
    data.push_back(0);
    data.push_back(0);

    return data;
}

static std::pair<u32, s32> GetAddrInfoRequestImpl(Kernel::HLERequestContext& ctx) {
    struct Parameters {
        u8 use_nsd_resolve;
        u32 unknown;
        u64 process_id;
    };

    IPC::RequestParser rp{ctx};
    const auto parameters = rp.PopRaw<Parameters>();

    LOG_WARNING(Service,
                "called with ignored parameters: use_nsd_resolve={}, unknown={}, process_id={}",
                parameters.use_nsd_resolve, parameters.unknown, parameters.process_id);

    const auto host_buffer = ctx.ReadBuffer(0);
    const std::string host = Common::StringFromBuffer(host_buffer);

    const auto service_buffer = ctx.ReadBuffer(1);
    const std::string service = Common::StringFromBuffer(service_buffer);

    addrinfo* addrinfo;
    // Pass null for hints. Serialized hints are also passed in a buffer, but are ignored for now
    s32 result_code = getaddrinfo(host.c_str(), service.c_str(), nullptr, &addrinfo);

    u32 data_size = 0;
    if (result_code == 0 && addrinfo != nullptr) {
        const std::vector<u8>& data = SerializeAddrInfo(addrinfo, result_code, host);
        data_size = static_cast<u32>(data.size());
        freeaddrinfo(addrinfo);

        ctx.WriteBuffer(data, 0);
    }

    return std::make_pair(data_size, result_code);
}

void SFDNSRES::GetAddrInfoRequest(Kernel::HLERequestContext& ctx) {
    auto [data_size, result_code] = GetAddrInfoRequestImpl(ctx);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push(static_cast<s32>(AddrInfoErrorToNetDbError(result_code))); // NetDBErrorCode
    rb.Push(result_code);                                              // errno
    rb.Push(data_size);                                                // serialized size
}

void SFDNSRES::GetAddrInfoRequestWithOptions(Kernel::HLERequestContext& ctx) {
    // Additional options are ignored
    auto [data_size, result_code] = GetAddrInfoRequestImpl(ctx);

    IPC::ResponseBuilder rb{ctx, 5};
    rb.Push(ResultSuccess);
    rb.Push(data_size);                                                // serialized size
    rb.Push(result_code);                                              // errno
    rb.Push(static_cast<s32>(AddrInfoErrorToNetDbError(result_code))); // NetDBErrorCode
    rb.Push(0);
}

} // namespace Service::Sockets