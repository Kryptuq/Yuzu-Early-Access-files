// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Glue {

class BGTC_T final : public ServiceFramework<BGTC_T> {
public:
    explicit BGTC_T(Core::System& system_);
    ~BGTC_T() override;

    void OpenTaskService(Kernel::HLERequestContext& ctx);
};

class ITaskService final : public ServiceFramework<ITaskService> {
public:
    explicit ITaskService(Core::System& system_);
    ~ITaskService() override;
};

class BGTC_SC final : public ServiceFramework<BGTC_SC> {
public:
    explicit BGTC_SC(Core::System& system_);
    ~BGTC_SC() override;
};

} // namespace Service::Glue
