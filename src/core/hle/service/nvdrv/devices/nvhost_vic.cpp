// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/service/nvdrv/devices/nvhost_vic.h"
#include "video_core/renderer_base.h"

namespace Service::Nvidia::Devices {

u32 nvhost_vic::next_id{};

nvhost_vic::nvhost_vic(Core::System& system_, NvCore::Container& core_)
    : nvhost_nvdec_common{system_, core_, NvCore::ChannelType::VIC} {}

nvhost_vic::~nvhost_vic() = default;

NvResult nvhost_vic::Ioctl1(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                            std::vector<u8>& output) {
    switch (command.group) {
    case 0x0:
        switch (command.cmd) {
        case 0x1:
            if (!fd_to_id.contains(fd)) {
                fd_to_id[fd] = next_id++;
            }
            return Submit(fd, input, output);
        case 0x2:
            return GetSyncpoint(input, output);
        case 0x3:
            return GetWaitbase(input, output);
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
    default:
        break;
    }

    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_vic::Ioctl2(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                            const std::vector<u8>& inline_input, std::vector<u8>& output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_vic::Ioctl3(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                            std::vector<u8>& output, std::vector<u8>& inline_output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

void nvhost_vic::OnOpen(DeviceFD fd) {}

void nvhost_vic::OnClose(DeviceFD fd) {
    const auto iter = fd_to_id.find(fd);
    if (iter != fd_to_id.end()) {
        system.GPU().ClearCdmaInstance(iter->second);
    }
}

} // namespace Service::Nvidia::Devices
