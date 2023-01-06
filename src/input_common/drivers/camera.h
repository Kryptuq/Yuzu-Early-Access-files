// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "input_common/input_engine.h"

namespace InputCommon {

/**
 * A button device factory representing a keyboard. It receives keyboard events and forward them
 * to all button devices it created.
 */
class Camera final : public InputEngine {
public:
    explicit Camera(std::string input_engine_);

    void SetCameraData(std::size_t width, std::size_t height, std::span<const u32> data);

    std::size_t getImageWidth() const;
    std::size_t getImageHeight() const;

    Common::Input::CameraError SetCameraFormat(const PadIdentifier& identifier_,
                                               Common::Input::CameraFormat camera_format) override;

private:
    Common::Input::CameraStatus status{};
};

} // namespace InputCommon
