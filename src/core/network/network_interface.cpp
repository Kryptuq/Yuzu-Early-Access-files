// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>

#include "common/bit_cast.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/network/network_interface.h"

#ifdef _WIN32
#include <iphlpapi.h>
#else
#include <cerrno>
#include <ifaddrs.h>
#include <net/if.h>
#endif

namespace Network {

#ifdef _WIN32

std::vector<NetworkInterface> GetAvailableNetworkInterfaces() {
    std::vector<u8> adapter_addresses_raw;
    auto adapter_addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(adapter_addresses_raw.data());
    DWORD ret = ERROR_BUFFER_OVERFLOW;
    DWORD buf_size = 0;

    // retry up to 5 times
    for (int i = 0; i < 5 && ret == ERROR_BUFFER_OVERFLOW; i++) {
        ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                                   nullptr, adapter_addresses, &buf_size);

        if (ret == ERROR_BUFFER_OVERFLOW) {
            adapter_addresses_raw.resize(buf_size);
            adapter_addresses =
                reinterpret_cast<PIP_ADAPTER_ADDRESSES>(adapter_addresses_raw.data());
        } else {
            break;
        }
    }

    if (ret == NO_ERROR) {
        std::vector<NetworkInterface> result;

        for (auto current_address = adapter_addresses; current_address != nullptr;
             current_address = current_address->Next) {
            if (current_address->FirstUnicastAddress == nullptr ||
                current_address->FirstUnicastAddress->Address.lpSockaddr == nullptr) {
                continue;
            }

            if (current_address->OperStatus != IfOperStatusUp) {
                continue;
            }

            const auto ip_addr = Common::BitCast<struct sockaddr_in>(
                                     *current_address->FirstUnicastAddress->Address.lpSockaddr)
                                     .sin_addr;

            result.push_back(NetworkInterface{
                .name{Common::UTF16ToUTF8(std::wstring{current_address->FriendlyName})},
                .ip_address{ip_addr}});
        }

        return result;
    } else {
        LOG_ERROR(Network, "Failed to get network interfaces with GetAdaptersAddresses");
        return {};
    }
}

#else

std::vector<NetworkInterface> GetAvailableNetworkInterfaces() {
    std::vector<NetworkInterface> result;

    struct ifaddrs* ifaddr = nullptr;

    if (getifaddrs(&ifaddr) != 0) {
        LOG_ERROR(Network, "Failed to get network interfaces with getifaddrs: {}",
                  std::strerror(errno));
        return result;
    }

    for (auto ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }

        if (ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        if ((ifa->ifa_flags & IFF_UP) == 0 || (ifa->ifa_flags & IFF_LOOPBACK) != 0) {
            continue;
        }

        result.push_back(NetworkInterface{
            .name{ifa->ifa_name},
            .ip_address{Common::BitCast<struct sockaddr_in>(*ifa->ifa_addr).sin_addr}});
    }

    freeifaddrs(ifaddr);

    return result;
}

#endif

} // namespace Network
