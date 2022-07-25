// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <queue>

#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/irsensor/clustering_processor.h"

namespace Service::IRS {
ClusteringProcessor::ClusteringProcessor(Core::HID::HIDCore& hid_core_,
                                         Core::IrSensor::DeviceFormat& device_format,
                                         std::size_t npad_index)
    : device{device_format} {
    npad_device = hid_core_.GetEmulatedControllerByIndex(npad_index);

    device.mode = Core::IrSensor::IrSensorMode::ClusteringProcessor;
    device.camera_status = Core::IrSensor::IrCameraStatus::Unconnected;
    device.camera_internal_status = Core::IrSensor::IrCameraInternalStatus::Stopped;
    SetDefaultConfig();

    shared_memory = std::construct_at(
        reinterpret_cast<ClusteringSharedMemory*>(&device_format.state.processor_raw_data));

    Core::HID::ControllerUpdateCallback engine_callback{
        .on_change = [this](Core::HID::ControllerTriggerType type) { OnControllerUpdate(type); },
        .is_npad_service = true,
    };
    callback_key = npad_device->SetCallback(engine_callback);
}

ClusteringProcessor::~ClusteringProcessor() {
    npad_device->DeleteCallback(callback_key);
};

void ClusteringProcessor::StartProcessor() {
    device.camera_status = Core::IrSensor::IrCameraStatus::Available;
    device.camera_internal_status = Core::IrSensor::IrCameraInternalStatus::Ready;
}

void ClusteringProcessor::SuspendProcessor() {}

void ClusteringProcessor::StopProcessor() {}

void ClusteringProcessor::OnControllerUpdate(Core::HID::ControllerTriggerType type) {
    if (type != Core::HID::ControllerTriggerType::IrSensor) {
        return;
    }

    next_state = {};
    const auto camera_data = npad_device->GetCamera();
    auto filtered_image = camera_data.data;

    RemoveLowIntensityData(filtered_image);

    const auto window_start_x = static_cast<std::size_t>(current_config.window_of_interest.x);
    const auto window_start_y = static_cast<std::size_t>(current_config.window_of_interest.y);
    const auto window_end_x =
        window_start_x + static_cast<std::size_t>(current_config.window_of_interest.width);
    const auto window_end_y =
        window_start_y + static_cast<std::size_t>(current_config.window_of_interest.height);

    for (std::size_t y = window_start_y; y < window_end_y; y++) {
        for (std::size_t x = window_start_x; x < window_end_x; x++) {
            u8 pixel = GetPixel(filtered_image, x, y);
            if (pixel == 0) {
                continue;
            }
            const auto cluster = GetClusterProperties(filtered_image, x, y);
            if (cluster.pixel_count > current_config.pixel_count_max) {
                continue;
            }
            if (cluster.pixel_count < current_config.pixel_count_min) {
                continue;
            }
            // Cluster object limit reached
            if (next_state.object_count >= next_state.data.size()) {
                continue;
            }
            next_state.data[next_state.object_count] = cluster;
            next_state.object_count++;
        }
    }

    next_state.sampling_number = camera_data.sample;
    next_state.timestamp = next_state.timestamp + 131;
    next_state.ambient_noise_level = Core::IrSensor::CameraAmbientNoiseLevel::Low;
    shared_memory->clustering_lifo.WriteNextEntry(next_state);

    if (!IsProcessorActive()) {
        StartProcessor();
    }
}

void ClusteringProcessor::RemoveLowIntensityData(std::vector<u8>& data) {
    for (u8& pixel : data) {
        if (pixel < current_config.pixel_count_min) {
            pixel = 0;
        }
    }
}

ClusteringProcessor::ClusteringData ClusteringProcessor::GetClusterProperties(std::vector<u8>& data,
                                                                              std::size_t x,
                                                                              std::size_t y) {
    std::queue<Common::Point<std::size_t>> search_points{};
    ClusteringData current_cluster = GetPixelProperties(data, x, y);
    SetPixel(data, x, y, 0);
    search_points.emplace(x, y);

    while (!search_points.empty()) {
        const auto point = search_points.front();
        search_points.pop();

        // Avoid negative numbers
        if (point.x == 0 || point.y == 0) {
            continue;
        }

        std::array<Common::Point<std::size_t>, 4> new_points{
            Common::Point<std::size_t>{point.x - 1, point.y},
            {point.x, point.y - 1},
            {point.x + 1, point.y},
            {point.x, point.y + 1},
        };

        for (const auto new_point : new_points) {
            if (new_point.x >= width) {
                continue;
            }
            if (new_point.y >= height) {
                continue;
            }
            if (GetPixel(data, new_point.x, new_point.y) < current_config.object_intensity_min) {
                continue;
            }
            const ClusteringData cluster = GetPixelProperties(data, new_point.x, new_point.y);
            current_cluster = MergeCluster(current_cluster, cluster);
            SetPixel(data, new_point.x, new_point.y, 0);
            search_points.emplace(new_point.x, new_point.y);
        }
    }

    return current_cluster;
}

ClusteringProcessor::ClusteringData ClusteringProcessor::GetPixelProperties(
    const std::vector<u8>& data, std::size_t x, std::size_t y) const {
    return {
        .average_intensity = GetPixel(data, x, y) / 255.0f,
        .centroid =
            {
                .x = static_cast<f32>(x),
                .y = static_cast<f32>(y),

            },
        .pixel_count = 1,
        .bound =
            {
                .x = static_cast<s16>(x),
                .y = static_cast<s16>(y),
                .width = 1,
                .height = 1,
            },
    };
}

ClusteringProcessor::ClusteringData ClusteringProcessor::MergeCluster(
    const ClusteringData a, const ClusteringData b) const {
    const f32 a_pixel_count = static_cast<f32>(a.pixel_count);
    const f32 b_pixel_count = static_cast<f32>(b.pixel_count);
    const f32 pixel_count = a_pixel_count + b_pixel_count;
    const f32 average_intensity =
        (a.average_intensity * a_pixel_count + b.average_intensity * b_pixel_count) / pixel_count;
    const Core::IrSensor::IrsCentroid centroid = {
        .x = (a.centroid.x * a_pixel_count + b.centroid.x * b_pixel_count) / pixel_count,
        .y = (a.centroid.y * a_pixel_count + b.centroid.y * b_pixel_count) / pixel_count,
    };
    s16 bound_start_x = a.bound.x < b.bound.x ? a.bound.x : b.bound.x;
    s16 bound_start_y = a.bound.y < b.bound.y ? a.bound.y : b.bound.y;
    s16 a_bound_end_x = a.bound.x + a.bound.width;
    s16 a_bound_end_y = a.bound.y + a.bound.height;
    s16 b_bound_end_x = b.bound.x + b.bound.width;
    s16 b_bound_end_y = b.bound.y + b.bound.height;

    const Core::IrSensor::IrsRect bound = {
        .x = bound_start_x,
        .y = bound_start_y,
        .width = a_bound_end_x > b_bound_end_x ? static_cast<s16>(a_bound_end_x - bound_start_x)
                                               : static_cast<s16>(b_bound_end_x - bound_start_x),
        .height = a_bound_end_y > b_bound_end_y ? static_cast<s16>(a_bound_end_y - bound_start_y)
                                                : static_cast<s16>(b_bound_end_y - bound_start_y),
    };

    return {
        .average_intensity = average_intensity,
        .centroid = centroid,
        .pixel_count = static_cast<u32>(pixel_count),
        .bound = bound,
    };
}

u8 ClusteringProcessor::GetPixel(const std::vector<u8>& data, std::size_t x, std::size_t y) const {
    if ((y * width) + x > data.size()) {
        return 0;
    }
    return data[(y * width) + x];
}

void ClusteringProcessor::SetPixel(std::vector<u8>& data, std::size_t x, std::size_t y, u8 value) {
    if ((y * width) + x > data.size()) {
        return;
    }
    data[(y * width) + x] = value;
}

void ClusteringProcessor::SetDefaultConfig() {
    using namespace std::literals::chrono_literals;
    current_config.camera_config.exposure_time = std::chrono::microseconds(200ms).count();
    current_config.camera_config.gain = 2;
    current_config.camera_config.is_negative_used = false;
    current_config.camera_config.light_target = Core::IrSensor::CameraLightTarget::BrightLeds;
    current_config.window_of_interest = {
        .x = 0,
        .y = 0,
        .width = width,
        .height = height,
    };
    current_config.pixel_count_min = 3;
    current_config.pixel_count_max = static_cast<u32>(GetDataSize(format));
    current_config.is_external_light_filter_enabled = true;
    current_config.object_intensity_min = 150;

    npad_device->SetCameraFormat(format);
}

void ClusteringProcessor::SetConfig(Core::IrSensor::PackedClusteringProcessorConfig config) {
    current_config.camera_config.exposure_time = config.camera_config.exposure_time;
    current_config.camera_config.gain = config.camera_config.gain;
    current_config.camera_config.is_negative_used = config.camera_config.is_negative_used;
    current_config.camera_config.light_target =
        static_cast<Core::IrSensor::CameraLightTarget>(config.camera_config.light_target);
    current_config.window_of_interest = config.window_of_interest;
    current_config.pixel_count_min = config.pixel_count_min;
    current_config.pixel_count_max = config.pixel_count_max;
    current_config.is_external_light_filter_enabled = config.is_external_light_filter_enabled;
    current_config.object_intensity_min = config.object_intensity_min;

    LOG_INFO(Service_IRS,
             "Processor config, exposure_time={}, gain={}, is_negative_used={}, "
             "light_target={}, window_of_interest=({}, {}, {}, {}), pixel_count_min={}, "
             "pixel_count_max={}, is_external_light_filter_enabled={}, object_intensity_min={}",
             current_config.camera_config.exposure_time, current_config.camera_config.gain,
             current_config.camera_config.is_negative_used,
             current_config.camera_config.light_target, current_config.window_of_interest.x,
             current_config.window_of_interest.y, current_config.window_of_interest.width,
             current_config.window_of_interest.height, current_config.pixel_count_min,
             current_config.pixel_count_max, current_config.is_external_light_filter_enabled,
             current_config.object_intensity_min);

    npad_device->SetCameraFormat(format);
}

} // namespace Service::IRS
