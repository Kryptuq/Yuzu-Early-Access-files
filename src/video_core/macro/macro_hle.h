// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

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
    std::unordered_map<u64, std::function<std::unique_ptr<CachedMacro>(Engines::Maxwell3D&)>>
        builders;
};

} // namespace Tegra
