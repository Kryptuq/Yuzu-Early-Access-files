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

    constexpr u64 GetAffinityMask() const {
        return m_mask;
    }

    constexpr void SetAffinityMask(u64 new_mask) {
        ASSERT((new_mask & ~AllowedAffinityMask) == 0);
        m_mask = new_mask;
    }

    constexpr bool GetAffinity(s32 core) const {
        return (m_mask & GetCoreBit(core)) != 0;
    }

    constexpr void SetAffinity(s32 core, bool set) {
        if (set) {
            m_mask |= GetCoreBit(core);
        } else {
            m_mask &= ~GetCoreBit(core);
        }
    }

    constexpr void SetAll() {
        m_mask = AllowedAffinityMask;
    }

private:
    static constexpr u64 GetCoreBit(s32 core) {
        ASSERT(0 <= core && core < static_cast<s32>(Core::Hardware::NUM_CPU_CORES));
        return (1ULL << core);
    }

    static constexpr u64 AllowedAffinityMask = (1ULL << Core::Hardware::NUM_CPU_CORES) - 1;

    u64 m_mask{};
};

} // namespace Kernel
