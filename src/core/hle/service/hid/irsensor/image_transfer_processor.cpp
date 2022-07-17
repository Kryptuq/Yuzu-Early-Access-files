// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/irsensor/image_transfer_processor.h"

namespace Service::IRS {
ImageTransferProcessor::ImageTransferProcessor(Core::HID::HIDCore& hid_core_,
                                               Core::IrSensor::DeviceFormat& device_format,
                                               std::size_t npad_index)
    : device{device_format} {
    npad_device = hid_core_.GetEmulatedControllerByIndex(npad_index);

    Core::HID::ControllerUpdateCallback engine_callback{
        .on_change = [this](Core::HID::ControllerTriggerType type) { OnControllerUpdate(type); },
        .is_npad_service = true,
    };
    callback_key = npad_device->SetCallback(engine_callback);

    device.mode = Core::IrSensor::IrSensorMode::ImageTransferProcessor;
    device.camera_status = Core::IrSensor::IrCameraStatus::Unconnected;
    device.camera_internal_status = Core::IrSensor::IrCameraInternalStatus::Stopped;
}

ImageTransferProcessor::~ImageTransferProcessor() {
    npad_device->DeleteCallback(callback_key);
};

void ImageTransferProcessor::StartProcessor() {
    is_active = true;
    device.camera_status = Core::IrSensor::IrCameraStatus::Available;
    device.camera_internal_status = Core::IrSensor::IrCameraInternalStatus::Ready;
    processor_state.sampling_number = 0;
    processor_state.ambient_noise_level = Core::IrSensor::CameraAmbientNoiseLevel::Low;
}

void ImageTransferProcessor::SuspendProcessor() {}

void ImageTransferProcessor::StopProcessor() {}

void ImageTransferProcessor::OnControllerUpdate(Core::HID::ControllerTriggerType type) {
    if (type != Core::HID::ControllerTriggerType::IrSensor) {
        return;
    }
    if (!is_transfer_memory_set) {
        return;
    }

    const auto camera_data = npad_device->GetCamera();

    // This indicates how much ambient light is precent
    processor_state.ambient_noise_level = Core::IrSensor::CameraAmbientNoiseLevel::Low;
    processor_state.sampling_number = camera_data.sample;

    if (camera_data.format != current_config.origin_format) {
        LOG_WARNING(Service_IRS, "Wrong Input format {} expected {}", camera_data.format,
                    current_config.origin_format);
        memset(transfer_memory, 0, GetDataSize(current_config.trimming_format));
        return;
    }

    if (current_config.origin_format > current_config.trimming_format) {
        LOG_WARNING(Service_IRS, "Origin format {} is smaller than trimming format {}",
                    current_config.origin_format, current_config.trimming_format);
        memset(transfer_memory, 0, GetDataSize(current_config.trimming_format));
        return;
    }

    std::vector<u8> window_data{};
    const auto origin_width = GetDataWidth(current_config.origin_format);
    const auto origin_height = GetDataHeight(current_config.origin_format);
    const auto trimming_width = GetDataWidth(current_config.trimming_format);
    const auto trimming_height = GetDataHeight(current_config.trimming_format);
    window_data.resize(GetDataSize(current_config.trimming_format));

    if (trimming_width + current_config.trimming_start_x > origin_width ||
        trimming_height + current_config.trimming_start_y > origin_height) {
        LOG_WARNING(Service_IRS,
                    "Trimming area ({}, {}, {}, {}) is outside of origin area ({}, {})",
                    current_config.trimming_start_x, current_config.trimming_start_y,
                    trimming_width, trimming_height, origin_width, origin_height);
        memset(transfer_memory, 0, GetDataSize(current_config.trimming_format));
        return;
    }

    for (std::size_t y = 0; y < trimming_height; y++) {
        for (std::size_t x = 0; x < trimming_width; x++) {
            const std::size_t window_index = (y * trimming_width) + x;
            const std::size_t origin_index =
                ((y + current_config.trimming_start_y) * origin_width) + x +
                current_config.trimming_start_x;
            window_data[window_index] = camera_data.data[origin_index];
        }
    }

    memcpy(transfer_memory, window_data.data(), GetDataSize(current_config.trimming_format));

    if (!IsProcessorActive()) {
        StartProcessor();
    }
}

void ImageTransferProcessor::SetConfig(Core::IrSensor::PackedImageTransferProcessorConfig config) {
    current_config.camera_config.exposure_time = config.camera_config.exposure_time;
    current_config.camera_config.gain = config.camera_config.gain;
    current_config.camera_config.is_negative_used = config.camera_config.is_negative_used;
    current_config.camera_config.light_target =
        static_cast<Core::IrSensor::CameraLightTarget>(config.camera_config.light_target);
    current_config.origin_format =
        static_cast<Core::IrSensor::ImageTransferProcessorFormat>(config.format);
    current_config.trimming_format =
        static_cast<Core::IrSensor::ImageTransferProcessorFormat>(config.format);
    current_config.trimming_start_x = 0;
    current_config.trimming_start_y = 0;

    npad_device->SetCameraFormat(current_config.origin_format);
}

void ImageTransferProcessor::SetConfig(
    Core::IrSensor::PackedImageTransferProcessorExConfig config) {
    current_config.camera_config.exposure_time = config.camera_config.exposure_time;
    current_config.camera_config.gain = config.camera_config.gain;
    current_config.camera_config.is_negative_used = config.camera_config.is_negative_used;
    current_config.camera_config.light_target =
        static_cast<Core::IrSensor::CameraLightTarget>(config.camera_config.light_target);
    current_config.origin_format =
        static_cast<Core::IrSensor::ImageTransferProcessorFormat>(config.origin_format);
    current_config.trimming_format =
        static_cast<Core::IrSensor::ImageTransferProcessorFormat>(config.trimming_format);
    current_config.trimming_start_x = config.trimming_start_x;
    current_config.trimming_start_y = config.trimming_start_y;

    npad_device->SetCameraFormat(current_config.origin_format);
}

void ImageTransferProcessor::SetTransferMemoryPointer(u8* t_mem) {
    is_transfer_memory_set = true;
    transfer_memory = t_mem;
}

Core::IrSensor::ImageTransferProcessorState ImageTransferProcessor::GetState(
    std::vector<u8>& data) const {
    const auto size = GetDataSize(current_config.trimming_format);
    data.resize(size);
    memcpy(data.data(), transfer_memory, size);
    return processor_state;
}

} // namespace Service::IRS
