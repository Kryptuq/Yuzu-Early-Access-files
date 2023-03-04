// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_funcs.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::PCTL {

enum class Capability : u32 {
    None = 0,
    Application = 1 << 0,
    SnsPost = 1 << 1,
    Recovery = 1 << 6,
    Status = 1 << 8,
    StereoVision = 1 << 9,
    System = 1 << 15,
};
DECLARE_ENUM_FLAG_OPERATORS(Capability);

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(Core::System& system_, std::shared_ptr<Module> module_,
                           const char* name_, Capability capability_);
        ~Interface() override;

        void CreateService(HLERequestContext& ctx);
        void CreateServiceWithoutInitialize(HLERequestContext& ctx);

    protected:
        std::shared_ptr<Module> module;

    private:
        Capability capability{};
    };
};

void LoopProcess(Core::System& system);

} // namespace Service::PCTL
