// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>

#include "common/param_package.h"
#include "input_common/drivers/camera.h"

namespace InputCommon {
constexpr PadIdentifier identifier = {
    .guid = Common::UUID{},
    .port = 0,
    .pad = 0,
};

Camera::Camera(std::string input_engine_) : InputEngine(std::move(input_engine_)) {
    PreSetController(identifier);
}

void Camera::SetCameraData(std::size_t width, std::size_t height, std::vector<u32> data) {
    const std::size_t desired_width = getImageWidth();
    const std::size_t desired_height = getImageHeight();
    status.data.resize(desired_width * desired_height);

    // Resize image to desired format
    for (std::size_t y = 0; y < desired_height; y++) {
        for (std::size_t x = 0; x < desired_width; x++) {
            const std::size_t pixel_index = y * desired_width + x;
            const std::size_t old_x = width * x / desired_width;
            const std::size_t old_y = height * y / desired_height;
            const std::size_t data_pixel_index = old_y * width + old_x;
            status.data[pixel_index] = static_cast<u8>(data[data_pixel_index] & 0xFF);
        }
    }

    SetCamera(identifier, status);
}

std::size_t Camera::getImageWidth() const {
    switch (status.format) {
    case Common::Input::CameraFormat::Size320x240:
        return 320;
    case Common::Input::CameraFormat::Size160x120:
        return 160;
    case Common::Input::CameraFormat::Size80x60:
        return 80;
    case Common::Input::CameraFormat::Size40x30:
        return 40;
    case Common::Input::CameraFormat::Size20x15:
        return 20;
    case Common::Input::CameraFormat::None:
    default:
        return 0;
    }
}

std::size_t Camera::getImageHeight() const {
    switch (status.format) {
    case Common::Input::CameraFormat::Size320x240:
        return 240;
    case Common::Input::CameraFormat::Size160x120:
        return 120;
    case Common::Input::CameraFormat::Size80x60:
        return 60;
    case Common::Input::CameraFormat::Size40x30:
        return 30;
    case Common::Input::CameraFormat::Size20x15:
        return 15;
    case Common::Input::CameraFormat::None:
    default:
        return 0;
    }
}

Common::Input::CameraError Camera::SetCameraFormat(
    [[maybe_unused]] const PadIdentifier& identifier_,
    const Common::Input::CameraFormat camera_format) {
    status.format = camera_format;
    return Common::Input::CameraError::None;
}

} // namespace InputCommon
