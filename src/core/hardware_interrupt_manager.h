// Copyright 2019 Yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
