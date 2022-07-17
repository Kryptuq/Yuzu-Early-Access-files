// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/hid/irsensor/ir_led_processor.h"

namespace Service::IRS {
IrLedProcessor::IrLedProcessor(Core::IrSensor::DeviceFormat& device_format)
    : device(device_format) {
    device.mode = Core::IrSensor::IrSensorMode::IrLedProcessor;
    device.camera_status = Core::IrSensor::IrCameraStatus::Unconnected;
    device.camera_internal_status = Core::IrSensor::IrCameraInternalStatus::Stopped;
}

IrLedProcessor::~IrLedProcessor() = default;

void IrLedProcessor::StartProcessor() {}

void IrLedProcessor::SuspendProcessor() {}

void IrLedProcessor::StopProcessor() {}

void IrLedProcessor::SetConfig(Core::IrSensor::PackedIrLedProcessorConfig config) {
    current_config.light_target =
        static_cast<Core::IrSensor::CameraLightTarget>(config.light_target);
}

} // namespace Service::IRS
