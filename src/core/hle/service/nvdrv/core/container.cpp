// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-FileCopyrightText: 2022 Skyline Team and Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/nvdrv/core/container.h"
#include "core/hle/service/nvdrv/core/nvmap.h"
#include "core/hle/service/nvdrv/core/syncpoint_manager.h"
#include "video_core/host1x/host1x.h"

namespace Service::Nvidia::NvCore {

struct ContainerImpl {
    explicit ContainerImpl(Tegra::Host1x::Host1x& host1x_)
        : file{host1x_}, manager{host1x_}, device_file_data{} {}
    NvMap file;
    SyncpointManager manager;
    Container::Host1xDeviceFileData device_file_data;
};

Container::Container(Tegra::Host1x::Host1x& host1x_) {
    impl = std::make_unique<ContainerImpl>(host1x_);
}

Container::~Container() = default;

NvMap& Container::GetNvMapFile() {
    return impl->file;
}

const NvMap& Container::GetNvMapFile() const {
    return impl->file;
}

Container::Host1xDeviceFileData& Container::Host1xDeviceFile() {
    return impl->device_file_data;
}

const Container::Host1xDeviceFileData& Container::Host1xDeviceFile() const {
    return impl->device_file_data;
}

SyncpointManager& Container::GetSyncpointManager() {
    return impl->manager;
}

const SyncpointManager& Container::GetSyncpointManager() const {
    return impl->manager;
}

} // namespace Service::Nvidia::NvCore
