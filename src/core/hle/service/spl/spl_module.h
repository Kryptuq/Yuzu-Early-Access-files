// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <random>
#include "core/hle/service/service.h"
#include "core/hle/service/spl/spl_results.h"
#include "core/hle/service/spl/spl_types.h"

namespace Core {
class System;
}

namespace Service::SPL {

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(Core::System& system_, std::shared_ptr<Module> module_,
                           const char* name);
        ~Interface() override;

        // General
        void GetConfig(HLERequestContext& ctx);
        void ModularExponentiate(HLERequestContext& ctx);
        void SetConfig(HLERequestContext& ctx);
        void GenerateRandomBytes(HLERequestContext& ctx);
        void IsDevelopment(HLERequestContext& ctx);
        void SetBootReason(HLERequestContext& ctx);
        void GetBootReason(HLERequestContext& ctx);

    protected:
        std::shared_ptr<Module> module;

    private:
        ResultVal<u64> GetConfigImpl(ConfigItem config_item) const;

        std::mt19937 rng;
    };
};

void LoopProcess(Core::System& system);

} // namespace Service::SPL
