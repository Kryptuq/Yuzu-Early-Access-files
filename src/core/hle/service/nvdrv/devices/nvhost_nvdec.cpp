// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_core.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/service/nvdrv/core/container.h"
#include "core/hle/service/nvdrv/devices/nvhost_nvdec.h"
#include "video_core/renderer_base.h"

namespace Service::Nvidia::Devices {

nvhost_nvdec::nvhost_nvdec(Core::System& system_, NvCore::Container& core_)
    : nvhost_nvdec_common{system_, core_, NvCore::ChannelType::NvDec} {}
nvhost_nvdec::~nvhost_nvdec() = default;

NvResult nvhost_nvdec::Ioctl1(DeviceFD fd, Ioctl command, std::span<const u8> input,
                              std::vector<u8>& output) {
    switch (command.group) {
    case 0x0:
        switch (command.cmd) {
        case 0x1: {
            auto& host1x_file = core.Host1xDeviceFile();
            if (!host1x_file.fd_to_id.contains(fd)) {
                host1x_file.fd_to_id[fd] = host1x_file.nvdec_next_id++;
            }
            return Submit(fd, input, output);
        }
        case 0x2:
            return GetSyncpoint(input, output);
        case 0x3:
            return GetWaitbase(input, output);
        case 0x7:
            return SetSubmitTimeout(input, output);
        case 0x9:
            return MapBuffer(input, output);
        case 0xa:
            return UnmapBuffer(input, output);
        default:
            break;
        }
        break;
    case 'H':
        switch (command.cmd) {
        case 0x1:
            return SetNVMAPfd(input);
        default:
            break;
        }
        break;
    }

    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_nvdec::Ioctl2(DeviceFD fd, Ioctl command, std::span<const u8> input,
                              std::span<const u8> inline_input, std::vector<u8>& output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_nvdec::Ioctl3(DeviceFD fd, Ioctl command, std::span<const u8> input,
                              std::vector<u8>& output, std::vector<u8>& inline_output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

void nvhost_nvdec::OnOpen(DeviceFD fd) {
    LOG_INFO(Service_NVDRV, "NVDEC video stream started");
    system.AudioCore().SetNVDECActive(true);
}

void nvhost_nvdec::OnClose(DeviceFD fd) {
    LOG_INFO(Service_NVDRV, "NVDEC video stream ended");
    auto& host1x_file = core.Host1xDeviceFile();
    const auto iter = host1x_file.fd_to_id.find(fd);
    if (iter != host1x_file.fd_to_id.end()) {
        system.GPU().ClearCdmaInstance(iter->second);
    }
    system.AudioCore().SetNVDECActive(false);
}

} // namespace Service::Nvidia::Devices
