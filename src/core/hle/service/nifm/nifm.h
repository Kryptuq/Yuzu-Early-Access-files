// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"
#include "network/network.h"
#include "network/room.h"
#include "network/room_member.h"

namespace Core {
class System;
}

namespace Service::NIFM {

void LoopProcess(Core::System& system);

class IGeneralService final : public ServiceFramework<IGeneralService> {
public:
    explicit IGeneralService(Core::System& system_);
    ~IGeneralService() override;

private:
    void GetClientId(Kernel::HLERequestContext& ctx);
    void CreateScanRequest(Kernel::HLERequestContext& ctx);
    void CreateRequest(Kernel::HLERequestContext& ctx);
    void GetCurrentNetworkProfile(Kernel::HLERequestContext& ctx);
    void RemoveNetworkProfile(Kernel::HLERequestContext& ctx);
    void GetCurrentIpAddress(Kernel::HLERequestContext& ctx);
    void CreateTemporaryNetworkProfile(Kernel::HLERequestContext& ctx);
    void GetCurrentIpConfigInfo(Kernel::HLERequestContext& ctx);
    void IsWirelessCommunicationEnabled(Kernel::HLERequestContext& ctx);
    void GetInternetConnectionStatus(Kernel::HLERequestContext& ctx);
    void IsEthernetCommunicationEnabled(Kernel::HLERequestContext& ctx);
    void IsAnyInternetRequestAccepted(Kernel::HLERequestContext& ctx);

    Network::RoomNetwork& network;
};

} // namespace Service::NIFM
