// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include "common/common_types.h"
#include "common/math_util.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"
#include "core/hle/service/nvflinger/buffer_transform_flags.h"
#include "core/hle/service/nvflinger/pixel_format.h"

namespace Service::Nvidia::Devices {

class nvmap;

class nvdisp_disp0 final : public nvdevice {
public:
    explicit nvdisp_disp0(Core::System& system_, std::shared_ptr<nvmap> nvmap_dev_);
    ~nvdisp_disp0() override;

    NvResult Ioctl1(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output) override;
    NvResult Ioctl2(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    const std::vector<u8>& inline_input, std::vector<u8>& output) override;
    NvResult Ioctl3(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output, std::vector<u8>& inline_output) override;

    void OnOpen(DeviceFD fd) override;
    void OnClose(DeviceFD fd) override;

    /// Performs a screen flip, drawing the buffer pointed to by the handle.
    void flip(u32 buffer_handle, u32 offset, android::PixelFormat format, u32 width, u32 height,
              u32 stride, android::BufferTransformFlags transform,
              const Common::Rectangle<int>& crop_rect);

private:
    std::shared_ptr<nvmap> nvmap_dev;
};

} // namespace Service::Nvidia::Devices
