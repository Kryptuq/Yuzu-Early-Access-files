// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hid/irs_types.h"
#include "core/hle/service/hid/irsensor/processor_base.h"

namespace Service::IRS {
class ClusteringProcessor final : public ProcessorBase {
public:
    explicit ClusteringProcessor(Core::IrSensor::DeviceFormat& device_format);
    ~ClusteringProcessor() override;

    // Called when the processor is initialized
    void StartProcessor() override;

    // Called when the processor is suspended
    void SuspendProcessor() override;

    // Called when the processor is stopped
    void StopProcessor() override;

    // Sets config parameters of the camera
    void SetConfig(Core::IrSensor::PackedClusteringProcessorConfig config);

private:
    // This is nn::irsensor::ClusteringProcessorConfig
    struct ClusteringProcessorConfig {
        Core::IrSensor::CameraConfig camera_config;
        Core::IrSensor::IrsRect window_of_interest;
        u32 pixel_count_min;
        u32 pixel_count_max;
        u32 object_intensity_min;
        bool is_external_light_filter_enabled;
        INSERT_PADDING_BYTES(3);
    };
    static_assert(sizeof(ClusteringProcessorConfig) == 0x30,
                  "ClusteringProcessorConfig is an invalid size");

    // This is nn::irsensor::AdaptiveClusteringProcessorConfig
    struct AdaptiveClusteringProcessorConfig {
        Core::IrSensor::AdaptiveClusteringMode mode;
        Core::IrSensor::AdaptiveClusteringTargetDistance target_distance;
    };
    static_assert(sizeof(AdaptiveClusteringProcessorConfig) == 0x8,
                  "AdaptiveClusteringProcessorConfig is an invalid size");

    // This is nn::irsensor::ClusteringData
    struct ClusteringData {
        f32 average_intensity;
        Core::IrSensor::IrsCentroid centroid;
        u32 pixel_count;
        Core::IrSensor::IrsRect bound;
    };
    static_assert(sizeof(ClusteringData) == 0x18, "ClusteringData is an invalid size");

    // This is nn::irsensor::ClusteringProcessorState
    struct ClusteringProcessorState {
        s64 sampling_number;
        u64 timestamp;
        u8 object_count;
        INSERT_PADDING_BYTES(3);
        Core::IrSensor::CameraAmbientNoiseLevel ambient_noise_level;
        std::array<ClusteringData, 0x10> data;
    };
    static_assert(sizeof(ClusteringProcessorState) == 0x198,
                  "ClusteringProcessorState is an invalid size");

    ClusteringProcessorConfig current_config{};
    Core::IrSensor::DeviceFormat& device;
};
} // namespace Service::IRS
