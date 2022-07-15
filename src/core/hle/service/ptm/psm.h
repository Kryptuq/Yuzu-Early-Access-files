// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Service::PTM {

class PSM final : public ServiceFramework<PSM> {
public:
    explicit PSM(Core::System& system_);
    ~PSM() override;

private:
    enum class ChargerType : u32 {
        Unplugged = 0,
        RegularCharger = 1,
        LowPowerCharger = 2,
        Unknown = 3,
    };

    void GetBatteryChargePercentage(Kernel::HLERequestContext& ctx);
    void GetChargerType(Kernel::HLERequestContext& ctx);
    void OpenSession(Kernel::HLERequestContext& ctx);

    u32 battery_charge_percentage{100};
    ChargerType charger_type{ChargerType::RegularCharger};
};

} // namespace Service::PTM
