// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "common/common_types.h"

namespace Tegra {

namespace Engines {
class Maxwell3D;
}

class HLEMacro {
public:
    explicit HLEMacro(Engines::Maxwell3D& maxwell3d_);
    ~HLEMacro();

    // Allocates and returns a cached macro if the hash matches a known function.
    // Returns nullptr otherwise.
    [[nodiscard]] std::unique_ptr<CachedMacro> GetHLEProgram(u64 hash) const;

private:
    Engines::Maxwell3D& maxwell3d;
};

} // namespace Tegra
