// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
