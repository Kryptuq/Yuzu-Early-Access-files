// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hid/irs_types.h"
#include "core/hle/service/hid/irs_ring_lifo.h"
#include "core/hle/service/hid/irsensor/processor_base.h"

namespace Core::HID {
class EmulatedController;
} // namespace Core::HID

namespace Service::IRS {
class ClusteringProcessor final : public ProcessorBase {
public:
    explicit ClusteringProcessor(Core::HID::HIDCore& hid_core_,
                                 Core::IrSensor::DeviceFormat& device_format,
                                 std::size_t npad_index);
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
    static constexpr auto format = Core::IrSensor::ImageTransferProcessorFormat::Size320x240;
    static constexpr std::size_t width = 320;
    static constexpr std::size_t height = 240;

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

    struct ClusteringSharedMemory {
        Service::IRS::Lifo<ClusteringProcessorState, 6> clustering_lifo;
        static_assert(sizeof(clustering_lifo) == 0x9A0, "clustering_lifo is an invalid size");
        INSERT_PADDING_WORDS(0x11F);
    };
    static_assert(sizeof(ClusteringSharedMemory) == 0xE20,
                  "ClusteringSharedMemory is an invalid size");

    void OnControllerUpdate(Core::HID::ControllerTriggerType type);
    void RemoveLowIntensityData(std::vector<u8>& data);
    ClusteringData GetClusterProperties(std::vector<u8>& data, std::size_t x, std::size_t y);
    ClusteringData GetPixelProperties(const std::vector<u8>& data, std::size_t x,
                                      std::size_t y) const;
    ClusteringData MergeCluster(const ClusteringData a, const ClusteringData b) const;
    u8 GetPixel(const std::vector<u8>& data, std::size_t x, std::size_t y) const;
    void SetPixel(std::vector<u8>& data, std::size_t x, std::size_t y, u8 value);

    // Sets config parameters of the camera
    void SetDefaultConfig();

    ClusteringSharedMemory* shared_memory = nullptr;
    ClusteringProcessorState next_state{};

    ClusteringProcessorConfig current_config{};
    Core::IrSensor::DeviceFormat& device;
    Core::HID::EmulatedController* npad_device;
    int callback_key{};
};
} // namespace Service::IRS
