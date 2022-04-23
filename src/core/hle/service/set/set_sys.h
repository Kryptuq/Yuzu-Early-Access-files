// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Set {

class SET_SYS final : public ServiceFramework<SET_SYS> {
public:
    explicit SET_SYS(Core::System& system_);
    ~SET_SYS() override;

private:
    /// Indicates the current theme set by the system settings
    enum class ColorSet : u32 {
        BasicWhite = 0,
        BasicBlack = 1,
    };

    void GetFirmwareVersion(Kernel::HLERequestContext& ctx);
    void GetFirmwareVersion2(Kernel::HLERequestContext& ctx);
    void GetColorSetId(Kernel::HLERequestContext& ctx);
    void SetColorSetId(Kernel::HLERequestContext& ctx);

    ColorSet color_set = ColorSet::BasicWhite;
};

} // namespace Service::Set
