// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Glue {

class NOTIF_A final : public ServiceFramework<NOTIF_A> {
public:
    explicit NOTIF_A(Core::System& system_);
    ~NOTIF_A() override;

private:
    void ListAlarmSettings(Kernel::HLERequestContext& ctx);
    void Initialize(Kernel::HLERequestContext& ctx);
};

} // namespace Service::Glue
