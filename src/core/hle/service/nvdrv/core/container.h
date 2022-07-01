// SPDX-FileCopyrightText: 2022 yuzu emulator team and Skyline Team and Contributors
// (https://github.com/skyline-emu/)
// SPDX-License-Identifier: GPL-3.0-or-later Licensed under GPLv3
// or any later version Refer to the license.txt file included.

#pragma once

#include <memory>

namespace Tegra {

namespace Host1x {
class Host1x;
} // namespace Host1x

} // namespace Tegra

namespace Service::Nvidia::NvCore {

class NvMap;
class SyncpointManager;

struct ContainerImpl;

class Container {
public:
    Container(Tegra::Host1x::Host1x& host1x);
    ~Container();

    NvMap& GetNvMapFile();

    const NvMap& GetNvMapFile() const;

    SyncpointManager& GetSyncpointManager();

    const SyncpointManager& GetSyncpointManager() const;

private:
    std::unique_ptr<ContainerImpl> impl;
};

} // namespace Service::Nvidia::NvCore
