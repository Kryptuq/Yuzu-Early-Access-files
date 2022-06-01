// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hid/hid_types.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::HID {

class IRS final : public ServiceFramework<IRS> {
public:
    explicit IRS(Core::System& system_);
    ~IRS() override;

private:
    // This is nn::irsensor::IrCameraStatus
    enum IrCameraStatus : u32 {
        Available,
        Unsupported,
        Unconnected,
    };

    // This is nn::irsensor::IrCameraInternalStatus
    enum IrCameraInternalStatus : u32 {
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
    enum IrSensorMode : u64 {
        None,
        MomentProcessor,
        ClusteringProcessor,
        ImageTransferProcessor,
        PointingProcessorMarker,
        TeraPluginProcessor,
        IrLedProcessor,
    };

    // This is nn::irsensor::ImageProcessorStatus
    enum ImageProcessorStatus : u8 {
        stopped,
        running,
    };

    // This is nn::irsensor::ImageTransferProcessorFormat
    enum ImageTransferProcessorFormat : u8 {
        Size320x240,
        Size160x120,
        Size80x60,
        Size40x30,
        Size20x15,
    };

    // This is nn::irsensor::AdaptiveClusteringMode
    enum AdaptiveClusteringMode : u8 {
        StaticFov,
        DynamicFov,
    };

    // This is nn::irsensor::AdaptiveClusteringTargetDistance
    enum AdaptiveClusteringTargetDistance : u8 {
        Near,
        Middle,
        Far,
    };

    // This is nn::irsensor::IrsHandAnalysisMode
    enum IrsHandAnalysisMode : u8 {
        Silhouette,
        Image,
        SilhoueteAndImage,
        SilhuetteOnly,
    };

    // This is nn::irsensor::IrSensorFunctionLevel
    enum IrSensorFunctionLevel : u8 {
        unknown0,
        unknown1,
        unknown2,
        unknown3,
        unknown4,
    };

    // This is nn::irsensor::IrCameraHandle
    struct IrCameraHandle {
        u8 npad_id{};
        Core::HID::NpadStyleIndex npad_type{Core::HID::NpadStyleIndex::None};
        INSERT_PADDING_BYTES(2);
    };
    static_assert(sizeof(IrCameraHandle) == 4, "IrCameraHandle is an invalid size");

    struct IrsRect {
        s16 x;
        s16 y;
        s16 width;
        s16 height;
    };

    // This is nn::irsensor::PackedMcuVersion
    struct PackedMcuVersion {
        u16 major;
        u16 minor;
    };
    static_assert(sizeof(PackedMcuVersion) == 4, "PackedMcuVersion is an invalid size");

    // This is nn::irsensor::MomentProcessorConfig
    struct MomentProcessorConfig {
        u64 exposire_time;
        u8 light_target;
        u8 gain;
        u8 is_negative_used;
        INSERT_PADDING_BYTES(7);
        IrsRect window_of_interest;
        u8 preprocess;
        u8 preprocess_intensity_threshold;
        INSERT_PADDING_BYTES(5);
    };
    static_assert(sizeof(MomentProcessorConfig) == 0x28,
                  "MomentProcessorConfig is an invalid size");

    // This is nn::irsensor::PackedMomentProcessorConfig
    struct PackedMomentProcessorConfig {
        u64 exposire_time;
        u8 light_target;
        u8 gain;
        u8 is_negative_used;
        INSERT_PADDING_BYTES(5);
        IrsRect window_of_interest;
        PackedMcuVersion required_mcu_version;
        u8 preprocess;
        u8 preprocess_intensity_threshold;
        INSERT_PADDING_BYTES(2);
    };
    static_assert(sizeof(PackedMomentProcessorConfig) == 0x20,
                  "PackedMomentProcessorConfig is an invalid size");

    // This is nn::irsensor::ClusteringProcessorConfig
    struct ClusteringProcessorConfig {
        u64 exposire_time;
        u32 light_target;
        u32 gain;
        u8 is_negative_used;
        INSERT_PADDING_BYTES(7);
        IrsRect window_of_interest;
        u32 pixel_count_min;
        u32 pixel_count_max;
        u32 object_intensity_min;
        u8 is_external_light_filter_enabled;
        INSERT_PADDING_BYTES(3);
    };
    static_assert(sizeof(ClusteringProcessorConfig) == 0x30,
                  "ClusteringProcessorConfig is an invalid size");

    // This is nn::irsensor::PackedClusteringProcessorConfig
    struct PackedClusteringProcessorConfig {
        u64 exposire_time;
        u8 light_target;
        u8 gain;
        u8 is_negative_used;
        INSERT_PADDING_BYTES(5);
        IrsRect window_of_interest;
        PackedMcuVersion required_mcu_version;
        u32 pixel_count_min;
        u32 pixel_count_max;
        u32 object_intensity_min;
        u8 is_external_light_filter_enabled;
        INSERT_PADDING_BYTES(2);
    };
    static_assert(sizeof(PackedClusteringProcessorConfig) == 0x30,
                  "PackedClusteringProcessorConfig is an invalid size");

    // This is nn::irsensor::PackedImageTransferProcessorConfig
    struct PackedImageTransferProcessorConfig {
        u64 exposire_time;
        u8 light_target;
        u8 gain;
        u8 is_negative_used;
        INSERT_PADDING_BYTES(5);
        PackedMcuVersion required_mcu_version;
        u8 format;
        INSERT_PADDING_BYTES(3);
    };
    static_assert(sizeof(PackedImageTransferProcessorConfig) == 0x18,
                  "PackedImageTransferProcessorConfig is an invalid size");

    // This is nn::irsensor::PackedTeraPluginProcessorConfig
    struct PackedTeraPluginProcessorConfig {
        PackedMcuVersion required_mcu_version;
        u8 mode;
        INSERT_PADDING_BYTES(3);
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
        u64 exposire_time;
        u8 light_target;
        u8 gain;
        u8 is_negative_used;
        INSERT_PADDING_BYTES(5);
        PackedMcuVersion required_mcu_version;
        ImageTransferProcessorFormat origin_format;
        ImageTransferProcessorFormat trimming_format;
        u16 trimming_start_x;
        u16 trimming_start_y;
        u8 is_external_light_filter_enabled;
        INSERT_PADDING_BYTES(3);
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

    void ActivateIrsensor(Kernel::HLERequestContext& ctx);
    void DeactivateIrsensor(Kernel::HLERequestContext& ctx);
    void GetIrsensorSharedMemoryHandle(Kernel::HLERequestContext& ctx);
    void StopImageProcessor(Kernel::HLERequestContext& ctx);
    void RunMomentProcessor(Kernel::HLERequestContext& ctx);
    void RunClusteringProcessor(Kernel::HLERequestContext& ctx);
    void RunImageTransferProcessor(Kernel::HLERequestContext& ctx);
    void GetImageTransferProcessorState(Kernel::HLERequestContext& ctx);
    void RunTeraPluginProcessor(Kernel::HLERequestContext& ctx);
    void GetNpadIrCameraHandle(Kernel::HLERequestContext& ctx);
    void RunPointingProcessor(Kernel::HLERequestContext& ctx);
    void SuspendImageProcessor(Kernel::HLERequestContext& ctx);
    void CheckFirmwareVersion(Kernel::HLERequestContext& ctx);
    void SetFunctionLevel(Kernel::HLERequestContext& ctx);
    void RunImageTransferExProcessor(Kernel::HLERequestContext& ctx);
    void RunIrLedProcessor(Kernel::HLERequestContext& ctx);
    void StopImageProcessorAsync(Kernel::HLERequestContext& ctx);
    void ActivateIrsensorWithFunctionLevel(Kernel::HLERequestContext& ctx);
};

class IRS_SYS final : public ServiceFramework<IRS_SYS> {
public:
    explicit IRS_SYS(Core::System& system);
    ~IRS_SYS() override;
};

} // namespace Service::HID
