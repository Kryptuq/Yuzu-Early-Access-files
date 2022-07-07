// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/audio_render_manager.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Kernel {
class HLERequestContext;
}

namespace Service::Audio {
class IAudioRenderer;

class AudRenU final : public ServiceFramework<AudRenU> {
public:
    explicit AudRenU(Core::System& system_);
    ~AudRenU() override;

private:
    void OpenAudioRenderer(Kernel::HLERequestContext& ctx);
    void GetWorkBufferSize(Kernel::HLERequestContext& ctx);
    void GetAudioDeviceService(Kernel::HLERequestContext& ctx);
    void OpenAudioRendererForManualExecution(Kernel::HLERequestContext& ctx);
    void GetAudioDeviceServiceWithRevisionInfo(Kernel::HLERequestContext& ctx);

    KernelHelpers::ServiceContext service_context;
    std::unique_ptr<AudioCore::AudioRenderer::Manager> impl;
    u32 num_audio_devices{0};
};

} // namespace Service::Audio
