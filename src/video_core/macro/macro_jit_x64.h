// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "video_core/macro/macro.h"

namespace Tegra {

namespace Engines {
class Maxwell3D;
}

class MacroJITx64 final : public MacroEngine {
public:
    explicit MacroJITx64(Engines::Maxwell3D& maxwell3d_);

protected:
    std::unique_ptr<CachedMacro> Compile(const std::vector<u32>& code) override;

private:
    Engines::Maxwell3D& maxwell3d;
};

} // namespace Tegra
