// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
