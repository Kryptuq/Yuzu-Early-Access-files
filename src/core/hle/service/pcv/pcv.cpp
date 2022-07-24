// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "core/hle/service/pcv/pcv.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::PCV {

class PCV final : public ServiceFramework<PCV> {
public:
    explicit PCV(Core::System& system_) : ServiceFramework{system_, "pcv"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "SetPowerEnabled"},
            {1, nullptr, "SetClockEnabled"},
            {2, nullptr, "SetClockRate"},
            {3, nullptr, "GetClockRate"},
            {4, nullptr, "GetState"},
            {5, nullptr, "GetPossibleClockRates"},
            {6, nullptr, "SetMinVClockRate"},
            {7, nullptr, "SetReset"},
            {8, nullptr, "SetVoltageEnabled"},
            {9, nullptr, "GetVoltageEnabled"},
            {10, nullptr, "GetVoltageRange"},
            {11, nullptr, "SetVoltageValue"},
            {12, nullptr, "GetVoltageValue"},
            {13, nullptr, "GetTemperatureThresholds"},
            {14, nullptr, "SetTemperature"},
            {15, nullptr, "Initialize"},
            {16, nullptr, "IsInitialized"},
            {17, nullptr, "Finalize"},
            {18, nullptr, "PowerOn"},
            {19, nullptr, "PowerOff"},
            {20, nullptr, "ChangeVoltage"},
            {21, nullptr, "GetPowerClockInfoEvent"},
            {22, nullptr, "GetOscillatorClock"},
            {23, nullptr, "GetDvfsTable"},
            {24, nullptr, "GetModuleStateTable"},
            {25, nullptr, "GetPowerDomainStateTable"},
            {26, nullptr, "GetFuseInfo"},
            {27, nullptr, "GetDramId"},
            {28, nullptr, "IsPoweredOn"},
            {29, nullptr, "GetVoltage"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class PCV_ARB final : public ServiceFramework<PCV_ARB> {
public:
    explicit PCV_ARB(Core::System& system_) : ServiceFramework{system_, "pcv:arb"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "ReleaseControl"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class PCV_IMM final : public ServiceFramework<PCV_IMM> {
public:
    explicit PCV_IMM(Core::System& system_) : ServiceFramework{system_, "pcv:imm"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "SetClockRate"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<PCV>(system)->InstallAsService(sm);
    std::make_shared<PCV_ARB>(system)->InstallAsService(sm);
    std::make_shared<PCV_IMM>(system)->InstallAsService(sm);
}

} // namespace Service::PCV
