// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "common/swap.h"

namespace Core::Timing {
class CoreTiming;
}

namespace Core {
class System;
}

namespace Service::HID {
class ControllerBase {
public:
    explicit ControllerBase(Core::System& system_);
    virtual ~ControllerBase();

    // Called when the controller is initialized
    virtual void OnInit() = 0;

    // When the controller is released
    virtual void OnRelease() = 0;

    // When the controller is requesting an update for the shared memory
    virtual void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                          std::size_t size) = 0;

    // When the controller is requesting a motion update for the shared memory
    virtual void OnMotionUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                std::size_t size) {}

    void ActivateController();

    void DeactivateController();

    bool IsControllerActivated() const;

protected:
    bool is_activated{false};

    Core::System& system;
};
} // namespace Service::HID
