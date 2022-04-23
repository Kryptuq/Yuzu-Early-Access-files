// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Service::APM {

class Controller;
class Module;

class APM final : public ServiceFramework<APM> {
public:
    explicit APM(Core::System& system_, std::shared_ptr<Module> apm_, Controller& controller_,
                 const char* name);
    ~APM() override;

private:
    void OpenSession(Kernel::HLERequestContext& ctx);
    void GetPerformanceMode(Kernel::HLERequestContext& ctx);
    void IsCpuOverclockEnabled(Kernel::HLERequestContext& ctx);

    std::shared_ptr<Module> apm;
    Controller& controller;
};

class APM_Sys final : public ServiceFramework<APM_Sys> {
public:
    explicit APM_Sys(Core::System& system_, Controller& controller);
    ~APM_Sys() override;

    void SetCpuBoostMode(Kernel::HLERequestContext& ctx);

private:
    void GetPerformanceEvent(Kernel::HLERequestContext& ctx);
    void GetCurrentPerformanceConfiguration(Kernel::HLERequestContext& ctx);

    Controller& controller;
};

} // namespace Service::APM
