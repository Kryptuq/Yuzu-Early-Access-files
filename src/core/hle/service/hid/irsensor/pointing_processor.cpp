// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/hid/irsensor/pointing_processor.h"

namespace Service::IRS {
PointingProcessor::PointingProcessor(Core::IrSensor::DeviceFormat& device_format)
    : device(device_format) {
    device.mode = Core::IrSensor::IrSensorMode::PointingProcessorMarker;
    device.camera_status = Core::IrSensor::IrCameraStatus::Unconnected;
    device.camera_internal_status = Core::IrSensor::IrCameraInternalStatus::Stopped;
}

PointingProcessor::~PointingProcessor() = default;

void PointingProcessor::StartProcessor() {}

void PointingProcessor::SuspendProcessor() {}

void PointingProcessor::StopProcessor() {}

void PointingProcessor::SetConfig(Core::IrSensor::PackedPointingProcessorConfig config) {
    current_config.window_of_interest = config.window_of_interest;
}

} // namespace Service::IRS
