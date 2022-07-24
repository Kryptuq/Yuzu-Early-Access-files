// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: 2021 Skyline Team and Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"

namespace Tegra {

namespace Host1x {

class Host1x;
class Nvdec;

class Control {
public:
    enum class Method : u32 {
        WaitSyncpt = 0x8,
        LoadSyncptPayload32 = 0x4e,
        WaitSyncpt32 = 0x50,
    };

    explicit Control(Host1x& host1x);
    ~Control();

    /// Writes the method into the state, Invoke Execute() if encountered
    void ProcessMethod(Method method, u32 argument);

private:
    /// For Host1x, execute is waiting on a syncpoint previously written into the state
    void Execute(u32 data);

    u32 syncpoint_value{};
    Host1x& host1x;
};

} // namespace Host1x

} // namespace Tegra
