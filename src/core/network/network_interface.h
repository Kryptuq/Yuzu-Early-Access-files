// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

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
};

std::vector<NetworkInterface> GetAvailableNetworkInterfaces();

} // namespace Network
