// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/bit_field.h"
#include "common/common_types.h"
#include "core/hid/irs_types.h"
#include "core/hle/service/hid/irsensor/processor_base.h"

namespace Service::IRS {
class TeraPluginProcessor final : public ProcessorBase {
public:
    explicit TeraPluginProcessor(Core::IrSensor::DeviceFormat& device_format);
    ~TeraPluginProcessor() override;

    // Called when the processor is initialized
    void StartProcessor() override;

    // Called when the processor is suspended
    void SuspendProcessor() override;

    // Called when the processor is stopped
    void StopProcessor() override;

    // Sets config parameters of the camera
    void SetConfig(Core::IrSensor::PackedTeraPluginProcessorConfig config);

private:
    // This is nn::irsensor::TeraPluginProcessorConfig
    struct TeraPluginProcessorConfig {
        u8 mode;
        u8 unknown_1;
        u8 unknown_2;
        u8 unknown_3;
    };
    static_assert(sizeof(TeraPluginProcessorConfig) == 0x4,
                  "TeraPluginProcessorConfig is an invalid size");

    struct TeraPluginProcessorState {
        s64 sampling_number;
        u64 timestamp;
        Core::IrSensor::CameraAmbientNoiseLevel ambient_noise_level;
        std::array<u8, 0x12c> data;
    };
    static_assert(sizeof(TeraPluginProcessorState) == 0x140,
                  "TeraPluginProcessorState is an invalid size");

    TeraPluginProcessorConfig current_config{};
    Core::IrSensor::DeviceFormat& device;
};

} // namespace Service::IRS
