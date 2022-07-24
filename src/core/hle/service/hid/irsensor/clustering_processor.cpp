// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/hid/irsensor/clustering_processor.h"

namespace Service::IRS {
ClusteringProcessor::ClusteringProcessor(Core::IrSensor::DeviceFormat& device_format)
    : device(device_format) {
    device.mode = Core::IrSensor::IrSensorMode::ClusteringProcessor;
    device.camera_status = Core::IrSensor::IrCameraStatus::Unconnected;
    device.camera_internal_status = Core::IrSensor::IrCameraInternalStatus::Stopped;
}

ClusteringProcessor::~ClusteringProcessor() = default;

void ClusteringProcessor::StartProcessor() {}

void ClusteringProcessor::SuspendProcessor() {}

void ClusteringProcessor::StopProcessor() {}

void ClusteringProcessor::SetConfig(Core::IrSensor::PackedClusteringProcessorConfig config) {
    current_config.camera_config.exposure_time = config.camera_config.exposure_time;
    current_config.camera_config.gain = config.camera_config.gain;
    current_config.camera_config.is_negative_used = config.camera_config.is_negative_used;
    current_config.camera_config.light_target =
        static_cast<Core::IrSensor::CameraLightTarget>(config.camera_config.light_target);
    current_config.pixel_count_min = config.pixel_count_min;
    current_config.pixel_count_max = config.pixel_count_max;
    current_config.is_external_light_filter_enabled = config.is_external_light_filter_enabled;
    current_config.object_intensity_min = config.object_intensity_min;
}

} // namespace Service::IRS
