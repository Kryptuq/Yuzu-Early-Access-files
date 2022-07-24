// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Audio {

class HwOpus final : public ServiceFramework<HwOpus> {
public:
    explicit HwOpus(Core::System& system_);
    ~HwOpus() override;

private:
    void OpenHardwareOpusDecoder(Kernel::HLERequestContext& ctx);
    void OpenHardwareOpusDecoderEx(Kernel::HLERequestContext& ctx);
    void GetWorkBufferSize(Kernel::HLERequestContext& ctx);
    void GetWorkBufferSizeEx(Kernel::HLERequestContext& ctx);
};

} // namespace Service::Audio
