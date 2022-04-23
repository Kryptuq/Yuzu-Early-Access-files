// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/common_types.h"

namespace Core {
class System;
}

namespace Core::Timing {
struct EventType;
}

namespace Core::Hardware {

class InterruptManager {
public:
    explicit InterruptManager(Core::System& system);
    ~InterruptManager();

    void GPUInterruptSyncpt(u32 syncpoint_id, u32 value);

private:
    Core::System& system;
    std::shared_ptr<Core::Timing::EventType> gpu_interrupt_event;
};

} // namespace Core::Hardware
