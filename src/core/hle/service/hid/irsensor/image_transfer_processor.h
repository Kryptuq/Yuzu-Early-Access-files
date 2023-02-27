// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hid/irs_types.h"
#include "core/hle/service/hid/irsensor/processor_base.h"

namespace Core {
class System;
}

namespace Core::HID {
class EmulatedController;
} // namespace Core::HID

namespace Service::IRS {
class ImageTransferProcessor final : public ProcessorBase {
public:
    explicit ImageTransferProcessor(Core::System& system_,
                                    Core::IrSensor::DeviceFormat& device_format,
                                    std::size_t npad_index);
    ~ImageTransferProcessor() override;

    // Called when the processor is initialized
    void StartProcessor() override;

    // Called when the processor is suspended
    void SuspendProcessor() override;

    // Called when the processor is stopped
    void StopProcessor() override;

    // Sets config parameters of the camera
    void SetConfig(Core::IrSensor::PackedImageTransferProcessorConfig config);
    void SetConfig(Core::IrSensor::PackedImageTransferProcessorExConfig config);

    // Transfer memory where the image data will be stored
    void SetTransferMemoryAddress(VAddr t_mem);

    Core::IrSensor::ImageTransferProcessorState GetState(std::vector<u8>& data) const;

private:
    // This is nn::irsensor::ImageTransferProcessorConfig
    struct ImageTransferProcessorConfig {
        Core::IrSensor::CameraConfig camera_config;
        Core::IrSensor::ImageTransferProcessorFormat format;
    };
    static_assert(sizeof(ImageTransferProcessorConfig) == 0x20,
                  "ImageTransferProcessorConfig is an invalid size");

    // This is nn::irsensor::ImageTransferProcessorExConfig
    struct ImageTransferProcessorExConfig {
        Core::IrSensor::CameraConfig camera_config;
        Core::IrSensor::ImageTransferProcessorFormat origin_format;
        Core::IrSensor::ImageTransferProcessorFormat trimming_format;
        u16 trimming_start_x;
        u16 trimming_start_y;
        bool is_external_light_filter_enabled;
        INSERT_PADDING_BYTES(3);
    };
    static_assert(sizeof(ImageTransferProcessorExConfig) == 0x28,
                  "ImageTransferProcessorExConfig is an invalid size");

    void OnControllerUpdate(Core::HID::ControllerTriggerType type);

    ImageTransferProcessorExConfig current_config{};
    Core::IrSensor::ImageTransferProcessorState processor_state{};
    Core::IrSensor::DeviceFormat& device;
    Core::HID::EmulatedController* npad_device;
    int callback_key{};

    Core::System& system;
    VAddr transfer_memory{};
};
} // namespace Service::IRS
