// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hle/service/service.h"

namespace Service::PTM {

class TS final : public ServiceFramework<TS> {
public:
    explicit TS(Core::System& system_);
    ~TS() override;

private:
    enum class Location : u8 {
        Internal,
        External,
    };

    void GetTemperature(HLERequestContext& ctx);
    void GetTemperatureMilliC(HLERequestContext& ctx);
};

} // namespace Service::PTM
