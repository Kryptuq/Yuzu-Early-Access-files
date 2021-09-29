// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Kernel {
class HLERequestContext;
}

namespace Service::Audio {

class AudRenU final : public ServiceFramework<AudRenU> {
public:
    explicit AudRenU(Core::System& system_);
    ~AudRenU() override;

private:
    void OpenAudioRenderer(Kernel::HLERequestContext& ctx);
    void GetAudioRendererWorkBufferSize(Kernel::HLERequestContext& ctx);
    void GetAudioDeviceService(Kernel::HLERequestContext& ctx);
    void OpenAudioRendererForManualExecution(Kernel::HLERequestContext& ctx);
    void GetAudioDeviceServiceWithRevisionInfo(Kernel::HLERequestContext& ctx);

    void OpenAudioRendererImpl(Kernel::HLERequestContext& ctx);

    KernelHelpers::ServiceContext service_context;

    std::size_t audren_instance_count = 0;
    Kernel::KEvent* buffer_event;
};

// Describes a particular audio feature that may be supported in a particular revision.
enum class AudioFeatures : u32 {
    AudioUSBDeviceOutput,
    Splitter,
    PerformanceMetricsVersion2,
    VariadicCommandBuffer,
};

// Tests if a particular audio feature is supported with a given audio revision.
bool IsFeatureSupported(AudioFeatures feature, u32_le revision);

} // namespace Service::Audio
