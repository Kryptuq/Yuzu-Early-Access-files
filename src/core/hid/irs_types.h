// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/hid/hid_types.h"

namespace Core::IrSensor {

// This is nn::irsensor::CameraAmbientNoiseLevel
enum class CameraAmbientNoiseLevel : u32 {
    Low,
    Medium,
    High,
    Unkown3, // This level can't be reached
};

// This is nn::irsensor::CameraLightTarget
enum class CameraLightTarget : u32 {
    AllLeds,
    BrightLeds,
    DimLeds,
    None,
};

// This is nn::irsensor::PackedCameraLightTarget
enum class PackedCameraLightTarget : u8 {
    AllLeds,
    BrightLeds,
    DimLeds,
    None,
};

// This is nn::irsensor::AdaptiveClusteringMode
enum class AdaptiveClusteringMode : u32 {
    StaticFov,
    DynamicFov,
};

// This is nn::irsensor::AdaptiveClusteringTargetDistance
enum class AdaptiveClusteringTargetDistance : u32 {
    Near,
    Middle,
    Far,
};

// This is nn::irsensor::ImageTransferProcessorFormat
enum class ImageTransferProcessorFormat : u32 {
    Size320x240,
    Size160x120,
    Size80x60,
    Size40x30,
    Size20x15,
};

// This is nn::irsensor::PackedImageTransferProcessorFormat
enum class PackedImageTransferProcessorFormat : u8 {
    Size320x240,
    Size160x120,
    Size80x60,
    Size40x30,
    Size20x15,
};

// This is nn::irsensor::IrCameraStatus
enum class IrCameraStatus : u32 {
    Available,
    Unsupported,
    Unconnected,
};

// This is nn::irsensor::IrCameraInternalStatus
enum class IrCameraInternalStatus : u32 {
    Stopped,
    FirmwareUpdateNeeded,
    Unkown2,
    Unkown3,
    Unkown4,
    FirmwareVersionRequested,
    FirmwareVersionIsInvalid,
    Ready,
    Setting,
};

// This is nn::irsensor::detail::StatusManager::IrSensorMode
enum class IrSensorMode : u64 {
    None,
    MomentProcessor,
    ClusteringProcessor,
    ImageTransferProcessor,
    PointingProcessorMarker,
    TeraPluginProcessor,
    IrLedProcessor,
};

// This is nn::irsensor::ImageProcessorStatus
enum ImageProcessorStatus : u32 {
    Stopped,
    Running,
};

// This is nn::irsensor::HandAnalysisMode
enum class HandAnalysisMode : u32 {
    None,
    Silhouette,
    Image,
    SilhoueteAndImage,
    SilhuetteOnly,
};

// This is nn::irsensor::IrSensorFunctionLevel
enum class IrSensorFunctionLevel : u8 {
    unknown0,
    unknown1,
    unknown2,
    unknown3,
    unknown4,
};

// This is nn::irsensor::MomentProcessorPreprocess
enum class MomentProcessorPreprocess : u32 {
    Unkown0,
    Unkown1,
};

// This is nn::irsensor::PackedMomentProcessorPreprocess
enum class PackedMomentProcessorPreprocess : u8 {
    Unkown0,
    Unkown1,
};

// This is nn::irsensor::PointingStatus
enum class PointingStatus : u32 {
    Unkown0,
    Unkown1,
};

struct IrsRect {
    s16 x;
    s16 y;
    s16 width;
    s16 height;
};

struct IrsCentroid {
    f32 x;
    f32 y;
};

struct CameraConfig {
    u64 exposure_time;
    CameraLightTarget light_target;
    u32 gain;
    bool is_negative_used;
    INSERT_PADDING_BYTES(7);
};
static_assert(sizeof(CameraConfig) == 0x18, "CameraConfig is an invalid size");

struct PackedCameraConfig {
    u64 exposure_time;
    PackedCameraLightTarget light_target;
    u8 gain;
    bool is_negative_used;
    INSERT_PADDING_BYTES(5);
};
static_assert(sizeof(PackedCameraConfig) == 0x10, "PackedCameraConfig is an invalid size");

// This is nn::irsensor::IrCameraHandle
struct IrCameraHandle {
    u8 npad_id{};
    Core::HID::NpadStyleIndex npad_type{Core::HID::NpadStyleIndex::None};
    INSERT_PADDING_BYTES(2);
};
static_assert(sizeof(IrCameraHandle) == 4, "IrCameraHandle is an invalid size");

// This is nn::irsensor::PackedMcuVersion
struct PackedMcuVersion {
    u16 major;
    u16 minor;
};
static_assert(sizeof(PackedMcuVersion) == 4, "PackedMcuVersion is an invalid size");

// This is nn::irsensor::PackedMomentProcessorConfig
struct PackedMomentProcessorConfig {
    PackedCameraConfig camera_config;
    IrsRect window_of_interest;
    PackedMcuVersion required_mcu_version;
    PackedMomentProcessorPreprocess preprocess;
    u8 preprocess_intensity_threshold;
    INSERT_PADDING_BYTES(2);
};
static_assert(sizeof(PackedMomentProcessorConfig) == 0x20,
              "PackedMomentProcessorConfig is an invalid size");

// This is nn::irsensor::PackedClusteringProcessorConfig
struct PackedClusteringProcessorConfig {
    PackedCameraConfig camera_config;
    IrsRect window_of_interest;
    PackedMcuVersion required_mcu_version;
    u32 pixel_count_min;
    u32 pixel_count_max;
    u8 object_intensity_min;
    bool is_external_light_filter_enabled;
    INSERT_PADDING_BYTES(2);
};
static_assert(sizeof(PackedClusteringProcessorConfig) == 0x28,
              "PackedClusteringProcessorConfig is an invalid size");

// This is nn::irsensor::PackedImageTransferProcessorConfig
struct PackedImageTransferProcessorConfig {
    PackedCameraConfig camera_config;
    PackedMcuVersion required_mcu_version;
    PackedImageTransferProcessorFormat format;
    INSERT_PADDING_BYTES(3);
};
static_assert(sizeof(PackedImageTransferProcessorConfig) == 0x18,
              "PackedImageTransferProcessorConfig is an invalid size");

// This is nn::irsensor::PackedTeraPluginProcessorConfig
struct PackedTeraPluginProcessorConfig {
    PackedMcuVersion required_mcu_version;
    u8 mode;
    u8 unknown_1;
    u8 unknown_2;
    u8 unknown_3;
};
static_assert(sizeof(PackedTeraPluginProcessorConfig) == 0x8,
              "PackedTeraPluginProcessorConfig is an invalid size");

// This is nn::irsensor::PackedPointingProcessorConfig
struct PackedPointingProcessorConfig {
    IrsRect window_of_interest;
    PackedMcuVersion required_mcu_version;
};
static_assert(sizeof(PackedPointingProcessorConfig) == 0xC,
              "PackedPointingProcessorConfig is an invalid size");

// This is nn::irsensor::PackedFunctionLevel
struct PackedFunctionLevel {
    IrSensorFunctionLevel function_level;
    INSERT_PADDING_BYTES(3);
};
static_assert(sizeof(PackedFunctionLevel) == 0x4, "PackedFunctionLevel is an invalid size");

// This is nn::irsensor::PackedImageTransferProcessorExConfig
struct PackedImageTransferProcessorExConfig {
    PackedCameraConfig camera_config;
    PackedMcuVersion required_mcu_version;
    PackedImageTransferProcessorFormat origin_format;
    PackedImageTransferProcessorFormat trimming_format;
    u16 trimming_start_x;
    u16 trimming_start_y;
    bool is_external_light_filter_enabled;
    INSERT_PADDING_BYTES(5);
};
static_assert(sizeof(PackedImageTransferProcessorExConfig) == 0x20,
              "PackedImageTransferProcessorExConfig is an invalid size");

// This is nn::irsensor::PackedIrLedProcessorConfig
struct PackedIrLedProcessorConfig {
    PackedMcuVersion required_mcu_version;
    u8 light_target;
    INSERT_PADDING_BYTES(3);
};
static_assert(sizeof(PackedIrLedProcessorConfig) == 0x8,
              "PackedIrLedProcessorConfig is an invalid size");

// This is nn::irsensor::HandAnalysisConfig
struct HandAnalysisConfig {
    HandAnalysisMode mode;
};
static_assert(sizeof(HandAnalysisConfig) == 0x4, "HandAnalysisConfig is an invalid size");

// This is nn::irsensor::detail::ProcessorState contents are different for each processor
struct ProcessorState {
    std::array<u8, 0xE20> processor_raw_data{};
};
static_assert(sizeof(ProcessorState) == 0xE20, "ProcessorState is an invalid size");

// This is nn::irsensor::detail::DeviceFormat
struct DeviceFormat {
    Core::IrSensor::IrCameraStatus camera_status{Core::IrSensor::IrCameraStatus::Unconnected};
    Core::IrSensor::IrCameraInternalStatus camera_internal_status{
        Core::IrSensor::IrCameraInternalStatus::Ready};
    Core::IrSensor::IrSensorMode mode{Core::IrSensor::IrSensorMode::None};
    ProcessorState state{};
};
static_assert(sizeof(DeviceFormat) == 0xE30, "DeviceFormat is an invalid size");

// This is nn::irsensor::ImageTransferProcessorState
struct ImageTransferProcessorState {
    u64 sampling_number;
    Core::IrSensor::CameraAmbientNoiseLevel ambient_noise_level;
    INSERT_PADDING_BYTES(4);
};
static_assert(sizeof(ImageTransferProcessorState) == 0x10,
              "ImageTransferProcessorState is an invalid size");

} // namespace Core::IrSensor
