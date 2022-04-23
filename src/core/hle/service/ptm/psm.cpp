// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/ptm/psm.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::PSM {

class IPsmSession final : public ServiceFramework<IPsmSession> {
public:
    explicit IPsmSession(Core::System& system_)
        : ServiceFramework{system_, "IPsmSession"}, service_context{system_, "IPsmSession"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IPsmSession::BindStateChangeEvent, "BindStateChangeEvent"},
            {1, &IPsmSession::UnbindStateChangeEvent, "UnbindStateChangeEvent"},
            {2, &IPsmSession::SetChargerTypeChangeEventEnabled, "SetChargerTypeChangeEventEnabled"},
            {3, &IPsmSession::SetPowerSupplyChangeEventEnabled, "SetPowerSupplyChangeEventEnabled"},
            {4, &IPsmSession::SetBatteryVoltageStateChangeEventEnabled, "SetBatteryVoltageStateChangeEventEnabled"},
        };
        // clang-format on

        RegisterHandlers(functions);

        state_change_event = service_context.CreateEvent("IPsmSession::state_change_event");
    }

    ~IPsmSession() override {
        service_context.CloseEvent(state_change_event);
    }

    void SignalChargerTypeChanged() {
        if (should_signal && should_signal_charger_type) {
            state_change_event->GetWritableEvent().Signal();
        }
    }

    void SignalPowerSupplyChanged() {
        if (should_signal && should_signal_power_supply) {
            state_change_event->GetWritableEvent().Signal();
        }
    }

    void SignalBatteryVoltageStateChanged() {
        if (should_signal && should_signal_battery_voltage) {
            state_change_event->GetWritableEvent().Signal();
        }
    }

private:
    void BindStateChangeEvent(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PSM, "called");

        should_signal = true;

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(state_change_event->GetReadableEvent());
    }

    void UnbindStateChangeEvent(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PSM, "called");

        should_signal = false;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void SetChargerTypeChangeEventEnabled(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto state = rp.Pop<bool>();
        LOG_DEBUG(Service_PSM, "called, state={}", state);

        should_signal_charger_type = state;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void SetPowerSupplyChangeEventEnabled(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto state = rp.Pop<bool>();
        LOG_DEBUG(Service_PSM, "called, state={}", state);

        should_signal_power_supply = state;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void SetBatteryVoltageStateChangeEventEnabled(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto state = rp.Pop<bool>();
        LOG_DEBUG(Service_PSM, "called, state={}", state);

        should_signal_battery_voltage = state;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    KernelHelpers::ServiceContext service_context;

    bool should_signal_charger_type{};
    bool should_signal_power_supply{};
    bool should_signal_battery_voltage{};
    bool should_signal{};
    Kernel::KEvent* state_change_event;
};

class PSM final : public ServiceFramework<PSM> {
public:
    explicit PSM(Core::System& system_) : ServiceFramework{system_, "psm"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &PSM::GetBatteryChargePercentage, "GetBatteryChargePercentage"},
            {1, &PSM::GetChargerType, "GetChargerType"},
            {2, nullptr, "EnableBatteryCharging"},
            {3, nullptr, "DisableBatteryCharging"},
            {4, nullptr, "IsBatteryChargingEnabled"},
            {5, nullptr, "AcquireControllerPowerSupply"},
            {6, nullptr, "ReleaseControllerPowerSupply"},
            {7, &PSM::OpenSession, "OpenSession"},
            {8, nullptr, "EnableEnoughPowerChargeEmulation"},
            {9, nullptr, "DisableEnoughPowerChargeEmulation"},
            {10, nullptr, "EnableFastBatteryCharging"},
            {11, nullptr, "DisableFastBatteryCharging"},
            {12, nullptr, "GetBatteryVoltageState"},
            {13, nullptr, "GetRawBatteryChargePercentage"},
            {14, nullptr, "IsEnoughPowerSupplied"},
            {15, nullptr, "GetBatteryAgePercentage"},
            {16, nullptr, "GetBatteryChargeInfoEvent"},
            {17, nullptr, "GetBatteryChargeInfoFields"},
            {18, nullptr, "GetBatteryChargeCalibratedEvent"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

    ~PSM() override = default;

private:
    void GetBatteryChargePercentage(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PSM, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(battery_charge_percentage);
    }

    void GetChargerType(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PSM, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushEnum(charger_type);
    }

    void OpenSession(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PSM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IPsmSession>(system);
    }

    enum class ChargerType : u32 {
        Unplugged = 0,
        RegularCharger = 1,
        LowPowerCharger = 2,
        Unknown = 3,
    };

    u32 battery_charge_percentage{100}; // 100%
    ChargerType charger_type{ChargerType::RegularCharger};
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<PSM>(system)->InstallAsService(sm);
}

} // namespace Service::PSM
