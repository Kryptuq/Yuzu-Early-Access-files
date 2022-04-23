// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
