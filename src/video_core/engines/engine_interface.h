// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <bitset>
#include <limits>
#include <vector>

#include "common/common_types.h"

namespace Tegra::Engines {

class EngineInterface {
public:
    virtual ~EngineInterface() = default;

    /// Write the value to the register identified by method.
    virtual void CallMethod(u32 method, u32 method_argument, bool is_last_call) = 0;

    /// Write multiple values to the register identified by method.
    virtual void CallMultiMethod(u32 method, const u32* base_start, u32 amount,
                                 u32 methods_pending) = 0;

    void ConsumeSink() {
        if (method_sink.empty()) {
            return;
        }
        ConsumeSinkImpl();
    }

    std::bitset<std::numeric_limits<u16>::max()> execution_mask{};
    std::vector<std::pair<u32, u32>> method_sink{};
    bool current_dirty{};
    GPUVAddr current_dma_segment;

protected:
    virtual void ConsumeSinkImpl() {
        for (auto [method, value] : method_sink) {
            CallMethod(method, value, true);
        }
        method_sink.clear();
    }
};

} // namespace Tegra::Engines
