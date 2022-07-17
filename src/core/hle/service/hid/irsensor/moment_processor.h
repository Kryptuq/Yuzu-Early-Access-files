// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/bit_field.h"
#include "common/common_types.h"
#include "core/hid/irs_types.h"
#include "core/hle/service/hid/irsensor/processor_base.h"

namespace Service::IRS {
class MomentProcessor final : public ProcessorBase {
public:
    explicit MomentProcessor(Core::IrSensor::DeviceFormat& device_format);
    ~MomentProcessor() override;

    // Called when the processor is initialized
    void StartProcessor() override;

    // Called when the processor is suspended
    void SuspendProcessor() override;

    // Called when the processor is stopped
    void StopProcessor() override;

    // Sets config parameters of the camera
    void SetConfig(Core::IrSensor::PackedMomentProcessorConfig config);

private:
    // This is nn::irsensor::MomentProcessorConfig
    struct MomentProcessorConfig {
        Core::IrSensor::CameraConfig camera_config;
        Core::IrSensor::IrsRect window_of_interest;
        Core::IrSensor::MomentProcessorPreprocess preprocess;
        u32 preprocess_intensity_threshold;
    };
    static_assert(sizeof(MomentProcessorConfig) == 0x28,
                  "MomentProcessorConfig is an invalid size");

    // This is nn::irsensor::MomentStatistic
    struct MomentStatistic {
        f32 average_intensity;
        Core::IrSensor::IrsCentroid centroid;
    };
    static_assert(sizeof(MomentStatistic) == 0xC, "MomentStatistic is an invalid size");

    // This is nn::irsensor::MomentProcessorState
    struct MomentProcessorState {
        s64 sampling_number;
        u64 timestamp;
        Core::IrSensor::CameraAmbientNoiseLevel ambient_noise_level;
        INSERT_PADDING_BYTES(4);
        std::array<MomentStatistic, 0x30> stadistic;
    };
    static_assert(sizeof(MomentProcessorState) == 0x258, "MomentProcessorState is an invalid size");

    MomentProcessorConfig current_config{};
    Core::IrSensor::DeviceFormat& device;
};

} // namespace Service::IRS
