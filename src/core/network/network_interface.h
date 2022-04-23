// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

namespace Network {

struct NetworkInterface {
    std::string name;
    struct in_addr ip_address;
    struct in_addr subnet_mask;
    struct in_addr gateway;
};

std::vector<NetworkInterface> GetAvailableNetworkInterfaces();
std::optional<NetworkInterface> GetSelectedNetworkInterface();

} // namespace Network
