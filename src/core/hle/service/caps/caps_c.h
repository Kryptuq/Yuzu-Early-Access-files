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

class CAPS_C final : public ServiceFramework<CAPS_C> {
public:
    explicit CAPS_C(Core::System& system_);
    ~CAPS_C() override;

private:
    void SetShimLibraryVersion(Kernel::HLERequestContext& ctx);
};

} // namespace Service::Capture
