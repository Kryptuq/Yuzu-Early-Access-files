// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Nvidia {

class NVMEMP final : public ServiceFramework<NVMEMP> {
public:
    explicit NVMEMP(Core::System& system_);
    ~NVMEMP() override;

private:
    void Open(HLERequestContext& ctx);
    void GetAruid(HLERequestContext& ctx);
};

} // namespace Service::Nvidia
