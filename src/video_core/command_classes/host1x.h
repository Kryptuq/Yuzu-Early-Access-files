// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace Tegra {
class GPU;
class Nvdec;

class Host1x {
public:
    enum class Method : u32 {
        WaitSyncpt = 0x8,
        LoadSyncptPayload32 = 0x4e,
        WaitSyncpt32 = 0x50,
    };

    explicit Host1x(GPU& gpu);
    ~Host1x();

    /// Writes the method into the state, Invoke Execute() if encountered
    void ProcessMethod(Method method, u32 argument);

private:
    /// For Host1x, execute is waiting on a syncpoint previously written into the state
    void Execute(u32 data);

    u32 syncpoint_value{};
    GPU& gpu;
};

} // namespace Tegra
