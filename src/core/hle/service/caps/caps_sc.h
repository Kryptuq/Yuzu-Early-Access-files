// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Capture {

class CAPS_SC final : public ServiceFramework<CAPS_SC> {
public:
    explicit CAPS_SC(Core::System& system_);
    ~CAPS_SC() override;
};

} // namespace Service::Capture
