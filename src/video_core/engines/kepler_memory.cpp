// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "video_core/engines/kepler_memory.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/renderer_base.h"
#include "video_core/textures/decoders.h"

namespace Tegra::Engines {

KeplerMemory::KeplerMemory(Core::System& system_, MemoryManager& memory_manager)
    : system{system_}, upload_state{memory_manager, regs.upload} {}

KeplerMemory::~KeplerMemory() = default;

void KeplerMemory::CallMethod(u32 method, u32 method_argument, bool is_last_call) {
    ASSERT_MSG(method < Regs::NUM_REGS,
               "Invalid KeplerMemory register, increase the size of the Regs structure");

    regs.reg_array[method] = method_argument;

    switch (method) {
    case KEPLERMEMORY_REG_INDEX(exec): {
        upload_state.ProcessExec(regs.exec.linear != 0);
        break;
    }
    case KEPLERMEMORY_REG_INDEX(data): {
        upload_state.ProcessData(method_argument, is_last_call);
        if (is_last_call) {
        }
        break;
    }
    }
}

void KeplerMemory::CallMultiMethod(u32 method, const u32* base_start, u32 amount,
                                   u32 methods_pending) {
    for (std::size_t i = 0; i < amount; i++) {
        CallMethod(method, base_start[i], methods_pending - static_cast<u32>(i) <= 1);
    }
}

} // namespace Tegra::Engines
