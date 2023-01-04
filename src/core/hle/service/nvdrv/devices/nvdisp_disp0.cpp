// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/service/nvdrv/core/container.h"
#include "core/hle/service/nvdrv/core/nvmap.h"
#include "core/hle/service/nvdrv/devices/nvdisp_disp0.h"
#include "core/perf_stats.h"
#include "video_core/gpu.h"

namespace Service::Nvidia::Devices {

nvdisp_disp0::nvdisp_disp0(Core::System& system_, NvCore::Container& core)
    : nvdevice{system_}, container{core}, nvmap{core.GetNvMapFile()} {}
nvdisp_disp0::~nvdisp_disp0() = default;

NvResult nvdisp_disp0::Ioctl1(DeviceFD fd, Ioctl command, std::span<const u8> input,
                              std::vector<u8>& output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvdisp_disp0::Ioctl2(DeviceFD fd, Ioctl command, std::span<const u8> input,
                              std::span<const u8> inline_input, std::vector<u8>& output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvdisp_disp0::Ioctl3(DeviceFD fd, Ioctl command, std::span<const u8> input,
                              std::vector<u8>& output, std::vector<u8>& inline_output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

void nvdisp_disp0::OnOpen(DeviceFD fd) {}
void nvdisp_disp0::OnClose(DeviceFD fd) {}

void nvdisp_disp0::flip(u32 buffer_handle, u32 offset, android::PixelFormat format, u32 width,
                        u32 height, u32 stride, android::BufferTransformFlags transform,
                        const Common::Rectangle<int>& crop_rect,
                        std::array<Service::Nvidia::NvFence, 4>& fences, u32 num_fences) {
    const VAddr addr = nvmap.GetHandleAddress(buffer_handle);
    LOG_TRACE(Service,
              "Drawing from address {:X} offset {:08X} Width {} Height {} Stride {} Format {}",
              addr, offset, width, height, stride, format);

    const Tegra::FramebufferConfig framebuffer{addr,   offset, width,     height,
                                               stride, format, transform, crop_rect};

    system.GPU().RequestSwapBuffers(&framebuffer, fences, num_fences);
    system.GetPerfStats().EndSystemFrame();
    system.SpeedLimiter().DoSpeedLimiting(system.CoreTiming().GetGlobalTimeUs());
    system.GetPerfStats().BeginSystemFrame();
}

Kernel::KEvent* nvdisp_disp0::QueryEvent(u32 event_id) {
    LOG_CRITICAL(Service_NVDRV, "Unknown DISP Event {}", event_id);
    return nullptr;
}

} // namespace Service::Nvidia::Devices
