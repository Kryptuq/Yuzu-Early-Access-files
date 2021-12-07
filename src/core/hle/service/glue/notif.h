// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
