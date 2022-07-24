// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::HID {

class XCD_SYS final : public ServiceFramework<XCD_SYS> {
public:
    explicit XCD_SYS(Core::System& system_);
    ~XCD_SYS() override;
};

} // namespace Service::HID
