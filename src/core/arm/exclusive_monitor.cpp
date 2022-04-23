// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef ARCHITECTURE_x86_64
#include "core/arm/dynarmic/arm_exclusive_monitor.h"
#endif
#include "core/arm/exclusive_monitor.h"
#include "core/memory.h"

namespace Core {

ExclusiveMonitor::~ExclusiveMonitor() = default;

std::unique_ptr<Core::ExclusiveMonitor> MakeExclusiveMonitor(Memory::Memory& memory,
                                                             std::size_t num_cores) {
#ifdef ARCHITECTURE_x86_64
    return std::make_unique<Core::DynarmicExclusiveMonitor>(memory, num_cores);
#else
    // TODO(merry): Passthrough exclusive monitor
    return nullptr;
#endif
}

} // namespace Core
