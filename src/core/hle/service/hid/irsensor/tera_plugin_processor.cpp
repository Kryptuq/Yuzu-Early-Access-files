// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/hid/irsensor/tera_plugin_processor.h"

namespace Service::IRS {
TeraPluginProcessor::TeraPluginProcessor(Core::IrSensor::DeviceFormat& device_format)
    : device(device_format) {
    device.mode = Core::IrSensor::IrSensorMode::TeraPluginProcessor;
    device.camera_status = Core::IrSensor::IrCameraStatus::Unconnected;
    device.camera_internal_status = Core::IrSensor::IrCameraInternalStatus::Stopped;
}

TeraPluginProcessor::~TeraPluginProcessor() = default;

void TeraPluginProcessor::StartProcessor() {}

void TeraPluginProcessor::SuspendProcessor() {}

void TeraPluginProcessor::StopProcessor() {}

void TeraPluginProcessor::SetConfig(Core::IrSensor::PackedTeraPluginProcessorConfig config) {
    current_config.mode = config.mode;
    current_config.unknown_1 = config.unknown_1;
    current_config.unknown_2 = config.unknown_2;
    current_config.unknown_3 = config.unknown_3;
}

} // namespace Service::IRS
