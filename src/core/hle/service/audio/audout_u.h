// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/audio_out_manager.h"
#include "audio_core/out/audio_out.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Kernel {
class HLERequestContext;
}

namespace AudioCore::AudioOut {
class Manager;
class Out;
} // namespace AudioCore::AudioOut

namespace Service::Audio {

class IAudioOut;

class AudOutU final : public ServiceFramework<AudOutU> {
public:
    explicit AudOutU(Core::System& system_);
    ~AudOutU() override;

private:
    void ListAudioOuts(Kernel::HLERequestContext& ctx);
    void OpenAudioOut(Kernel::HLERequestContext& ctx);

    KernelHelpers::ServiceContext service_context;
    std::unique_ptr<AudioCore::AudioOut::Manager> impl;
};

} // namespace Service::Audio
