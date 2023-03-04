// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/audio_in_manager.h"
#include "audio_core/in/audio_in.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace AudioCore::AudioOut {
class Manager;
class In;
} // namespace AudioCore::AudioOut

namespace Service::Audio {

class AudInU final : public ServiceFramework<AudInU> {
public:
    explicit AudInU(Core::System& system_);
    ~AudInU() override;

private:
    void ListAudioIns(HLERequestContext& ctx);
    void ListAudioInsAutoFiltered(HLERequestContext& ctx);
    void OpenInOutImpl(HLERequestContext& ctx);
    void OpenAudioIn(HLERequestContext& ctx);
    void OpenAudioInProtocolSpecified(HLERequestContext& ctx);

    KernelHelpers::ServiceContext service_context;
    std::unique_ptr<AudioCore::AudioIn::Manager> impl;
};

} // namespace Service::Audio
