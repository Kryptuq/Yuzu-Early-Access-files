// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"
#include "core/hle/service/wlan/wlan.h"

namespace Service::WLAN {

class WLANInfra final : public ServiceFramework<WLANInfra> {
public:
    explicit WLANInfra(Core::System& system_) : ServiceFramework{system_, "wlan:inf"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "OpenMode"},
            {1, nullptr, "CloseMode"},
            {2, nullptr, "GetMacAddress"},
            {3, nullptr, "StartScan"},
            {4, nullptr, "StopScan"},
            {5, nullptr, "Connect"},
            {6, nullptr, "CancelConnect"},
            {7, nullptr, "Disconnect"},
            {8, nullptr, "GetConnectionEvent"},
            {9, nullptr, "GetConnectionStatus"},
            {10, nullptr, "GetState"},
            {11, nullptr, "GetScanResult"},
            {12, nullptr, "GetRssi"},
            {13, nullptr, "ChangeRxAntenna"},
            {14, nullptr, "GetFwVersion"},
            {15, nullptr, "RequestSleep"},
            {16, nullptr, "RequestWakeUp"},
            {17, nullptr, "RequestIfUpDown"},
            {18, nullptr, "Unknown18"},
            {19, nullptr, "Unknown19"},
            {20, nullptr, "Unknown20"},
            {21, nullptr, "Unknown21"},
            {22, nullptr, "Unknown22"},
            {23, nullptr, "Unknown23"},
            {24, nullptr, "Unknown24"},
            {25, nullptr, "Unknown25"},
            {26, nullptr, "Unknown26"},
            {27, nullptr, "Unknown27"},
            {28, nullptr, "Unknown28"},
            {29, nullptr, "Unknown29"},
            {30, nullptr, "Unknown30"},
            {31, nullptr, "Unknown31"},
            {32, nullptr, "Unknown32"},
            {33, nullptr, "Unknown33"},
            {34, nullptr, "Unknown34"},
            {35, nullptr, "Unknown35"},
            {36, nullptr, "Unknown36"},
            {37, nullptr, "Unknown37"},
            {38, nullptr, "Unknown38"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class WLANLocal final : public ServiceFramework<WLANLocal> {
public:
    explicit WLANLocal(Core::System& system_) : ServiceFramework{system_, "wlan:lcl"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Unknown0"},
            {1, nullptr, "Unknown1"},
            {2, nullptr, "Unknown2"},
            {3, nullptr, "Unknown3"},
            {4, nullptr, "Unknown4"},
            {5, nullptr, "Unknown5"},
            {6, nullptr, "GetMacAddress"},
            {7, nullptr, "CreateBss"},
            {8, nullptr, "DestroyBss"},
            {9, nullptr, "StartScan"},
            {10, nullptr, "StopScan"},
            {11, nullptr, "Connect"},
            {12, nullptr, "CancelConnect"},
            {13, nullptr, "Join"},
            {14, nullptr, "CancelJoin"},
            {15, nullptr, "Disconnect"},
            {16, nullptr, "SetBeaconLostCount"},
            {17, nullptr, "Unknown17"},
            {18, nullptr, "Unknown18"},
            {19, nullptr, "Unknown19"},
            {20, nullptr, "GetBssIndicationEvent"},
            {21, nullptr, "GetBssIndicationInfo"},
            {22, nullptr, "GetState"},
            {23, nullptr, "GetAllowedChannels"},
            {24, nullptr, "AddIe"},
            {25, nullptr, "DeleteIe"},
            {26, nullptr, "Unknown26"},
            {27, nullptr, "Unknown27"},
            {28, nullptr, "CreateRxEntry"},
            {29, nullptr, "DeleteRxEntry"},
            {30, nullptr, "Unknown30"},
            {31, nullptr, "Unknown31"},
            {32, nullptr, "AddMatchingDataToRxEntry"},
            {33, nullptr, "RemoveMatchingDataFromRxEntry"},
            {34, nullptr, "GetScanResult"},
            {35, nullptr, "Unknown35"},
            {36, nullptr, "SetActionFrameWithBeacon"},
            {37, nullptr, "CancelActionFrameWithBeacon"},
            {38, nullptr, "CreateRxEntryForActionFrame"},
            {39, nullptr, "DeleteRxEntryForActionFrame"},
            {40, nullptr, "Unknown40"},
            {41, nullptr, "Unknown41"},
            {42, nullptr, "CancelGetActionFrame"},
            {43, nullptr, "GetRssi"},
            {44, nullptr, "Unknown44"},
            {45, nullptr, "Unknown45"},
            {46, nullptr, "Unknown46"},
            {47, nullptr, "Unknown47"},
            {48, nullptr, "Unknown48"},
            {49, nullptr, "Unknown49"},
            {50, nullptr, "Unknown50"},
            {51, nullptr, "Unknown51"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class WLANLocalGetFrame final : public ServiceFramework<WLANLocalGetFrame> {
public:
    explicit WLANLocalGetFrame(Core::System& system_) : ServiceFramework{system_, "wlan:lg"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Unknown"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class WLANSocketGetFrame final : public ServiceFramework<WLANSocketGetFrame> {
public:
    explicit WLANSocketGetFrame(Core::System& system_) : ServiceFramework{system_, "wlan:sg"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Unknown"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class WLANSocketManager final : public ServiceFramework<WLANSocketManager> {
public:
    explicit WLANSocketManager(Core::System& system_) : ServiceFramework{system_, "wlan:soc"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Unknown0"},
            {1, nullptr, "Unknown1"},
            {2, nullptr, "Unknown2"},
            {3, nullptr, "Unknown3"},
            {4, nullptr, "Unknown4"},
            {5, nullptr, "Unknown5"},
            {6, nullptr, "GetMacAddress"},
            {7, nullptr, "SwitchTsfTimerFunction"},
            {8, nullptr, "Unknown8"},
            {9, nullptr, "Unknown9"},
            {10, nullptr, "Unknown10"},
            {11, nullptr, "Unknown11"},
            {12, nullptr, "Unknown12"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<WLANInfra>(system)->InstallAsService(sm);
    std::make_shared<WLANLocal>(system)->InstallAsService(sm);
    std::make_shared<WLANLocalGetFrame>(system)->InstallAsService(sm);
    std::make_shared<WLANSocketGetFrame>(system)->InstallAsService(sm);
    std::make_shared<WLANSocketManager>(system)->InstallAsService(sm);
}

} // namespace Service::WLAN
