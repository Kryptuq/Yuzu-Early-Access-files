// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Audio {

struct OpusMultiStreamParametersEx {
    u32 sample_rate;
    u32 channel_count;
    u32 number_streams;
    u32 number_stereo_streams;
    u32 use_large_frame_size;
    u32 padding;
    std::array<u32, 64> channel_mappings;
};

class HwOpus final : public ServiceFramework<HwOpus> {
public:
    explicit HwOpus(Core::System& system_);
    ~HwOpus() override;

private:
    void OpenHardwareOpusDecoder(Kernel::HLERequestContext& ctx);
    void OpenHardwareOpusDecoderEx(Kernel::HLERequestContext& ctx);
    void GetWorkBufferSize(Kernel::HLERequestContext& ctx);
    void GetWorkBufferSizeEx(Kernel::HLERequestContext& ctx);
    void GetWorkBufferSizeForMultiStreamEx(Kernel::HLERequestContext& ctx);
};

} // namespace Service::Audio
