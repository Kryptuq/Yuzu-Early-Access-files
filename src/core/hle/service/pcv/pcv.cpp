// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/pcv/pcv.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"

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

class IClkrstSession final : public ServiceFramework<IClkrstSession> {
public:
    explicit IClkrstSession(Core::System& system_, DeviceCode deivce_code_)
        : ServiceFramework{system_, "IClkrstSession"}, deivce_code(deivce_code_) {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "SetClockEnabled"},
            {1, nullptr, "SetClockDisabled"},
            {2, nullptr, "SetResetAsserted"},
            {3, nullptr, "SetResetDeasserted"},
            {4, nullptr, "SetPowerEnabled"},
            {5, nullptr, "SetPowerDisabled"},
            {6, nullptr, "GetState"},
            {7, &IClkrstSession::SetClockRate, "SetClockRate"},
            {8, &IClkrstSession::GetClockRate, "GetClockRate"},
            {9, nullptr, "SetMinVClockRate"},
            {10, nullptr, "GetPossibleClockRates"},
            {11, nullptr, "GetDvfsTable"},
        };
        // clang-format on
        RegisterHandlers(functions);
    }

private:
    void SetClockRate(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        clock_rate = rp.Pop<u32>();
        LOG_DEBUG(Service_PCV, "(STUBBED) called, clock_rate={}", clock_rate);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetClockRate(HLERequestContext& ctx) {
        LOG_DEBUG(Service_PCV, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(clock_rate);
    }

    DeviceCode deivce_code;
    u32 clock_rate{};
};

class CLKRST final : public ServiceFramework<CLKRST> {
public:
    explicit CLKRST(Core::System& system_, const char* name) : ServiceFramework{system_, name} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &CLKRST::OpenSession, "OpenSession"},
            {1, nullptr, "GetTemperatureThresholds"},
            {2, nullptr, "SetTemperature"},
            {3, nullptr, "GetModuleStateTable"},
            {4, nullptr, "GetModuleStateTableEvent"},
            {5, nullptr, "GetModuleStateTableMaxCount"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void OpenSession(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto device_code = static_cast<DeviceCode>(rp.Pop<u32>());
        const auto unkonwn_input = rp.Pop<u32>();

        LOG_DEBUG(Service_PCV, "called, device_code={}, input={}", device_code, unkonwn_input);

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IClkrstSession>(system, device_code);
    }
};

class CLKRST_A final : public ServiceFramework<CLKRST_A> {
public:
    explicit CLKRST_A(Core::System& system_) : ServiceFramework{system_, "clkrst:a"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "ReleaseControl"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("pcv", std::make_shared<PCV>(system));
    server_manager->RegisterNamedService("clkrst", std::make_shared<CLKRST>(system, "clkrst"));
    server_manager->RegisterNamedService("clkrst:i", std::make_shared<CLKRST>(system, "clkrst:i"));
    server_manager->RegisterNamedService("clkrst:a", std::make_shared<CLKRST_A>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::PCV
