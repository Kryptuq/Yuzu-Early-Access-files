// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvdrv/devices/nvhost_nvjpg.h"

namespace Service::Nvidia::Devices {

nvhost_nvjpg::nvhost_nvjpg(Core::System& system_) : nvdevice{system_} {}
nvhost_nvjpg::~nvhost_nvjpg() = default;

NvResult nvhost_nvjpg::Ioctl1(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                              std::vector<u8>& output) {
    switch (command.group) {
    case 'H':
        switch (command.cmd) {
        case 0x1:
            return SetNVMAPfd(input, output);
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

NvResult nvhost_nvjpg::Ioctl2(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                              const std::vector<u8>& inline_input, std::vector<u8>& output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_nvjpg::Ioctl3(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                              std::vector<u8>& output, std::vector<u8>& inline_output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

void nvhost_nvjpg::OnOpen(DeviceFD fd) {}
void nvhost_nvjpg::OnClose(DeviceFD fd) {}

NvResult nvhost_nvjpg::SetNVMAPfd(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlSetNvmapFD params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, fd={}", params.nvmap_fd);

    nvmap_fd = params.nvmap_fd;
    return NvResult::Success;
}

} // namespace Service::Nvidia::Devices
