// SPDX-FileCopyrightText: 2022 yuzu emulator team and Skyline Team and Contributors
// (https://github.com/skyline-emu/)
// SPDX-License-Identifier: GPL-3.0-or-later Licensed under GPLv3
// or any later version Refer to the license.txt file included.

#include "core/hle/service/nvdrv/core/container.h"
#include "core/hle/service/nvdrv/core/nvmap.h"
#include "core/hle/service/nvdrv/core/syncpoint_manager.h"
#include "video_core/host1x/host1x.h"

namespace Service::Nvidia::NvCore {

struct ContainerImpl {
    ContainerImpl(Tegra::Host1x::Host1x& host1x_) : file{host1x_}, manager{host1x_} {}
    NvMap file;
    SyncpointManager manager;
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

SyncpointManager& Container::GetSyncpointManager() {
    return impl->manager;
}

const SyncpointManager& Container::GetSyncpointManager() const {
    return impl->manager;
}

} // namespace Service::Nvidia::NvCore
