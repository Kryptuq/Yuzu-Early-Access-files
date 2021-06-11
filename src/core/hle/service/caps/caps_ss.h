// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Capture {

class CAPS_SS final : public ServiceFramework<CAPS_SS> {
public:
    explicit CAPS_SS(Core::System& system_);
    ~CAPS_SS() override;
};

} // namespace Service::Capture
