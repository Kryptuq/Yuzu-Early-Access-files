// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Capture {

class CAPS_A final : public ServiceFramework<CAPS_A> {
public:
    explicit CAPS_A(Core::System& system_);
    ~CAPS_A() override;
};

} // namespace Service::Capture
