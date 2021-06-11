// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Kernel {
class HLERequestContext;
}

namespace Service::Capture {

class CAPS_A final : public ServiceFramework<CAPS_A> {
public:
    explicit CAPS_A(Core::System& system_);
    ~CAPS_A() override;
};

} // namespace Service::Capture
