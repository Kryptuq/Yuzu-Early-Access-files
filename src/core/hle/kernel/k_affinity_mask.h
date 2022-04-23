// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/common_types.h"
#include "core/hardware_properties.h"

namespace Kernel {

class KAffinityMask {
public:
    constexpr KAffinityMask() = default;

    [[nodiscard]] constexpr u64 GetAffinityMask() const {
        return this->mask;
    }

    constexpr void SetAffinityMask(u64 new_mask) {
        ASSERT((new_mask & ~AllowedAffinityMask) == 0);
        this->mask = new_mask;
    }

    [[nodiscard]] constexpr bool GetAffinity(s32 core) const {
        return (this->mask & GetCoreBit(core)) != 0;
    }

    constexpr void SetAffinity(s32 core, bool set) {
        if (set) {
            this->mask |= GetCoreBit(core);
        } else {
            this->mask &= ~GetCoreBit(core);
        }
    }

    constexpr void SetAll() {
        this->mask = AllowedAffinityMask;
    }

private:
    [[nodiscard]] static constexpr u64 GetCoreBit(s32 core) {
        ASSERT(0 <= core && core < static_cast<s32>(Core::Hardware::NUM_CPU_CORES));
        return (1ULL << core);
    }

    static constexpr u64 AllowedAffinityMask = (1ULL << Core::Hardware::NUM_CPU_CORES) - 1;

    u64 mask{};
};

} // namespace Kernel
