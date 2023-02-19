// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service {

namespace FileSystem {
class FileSystemController;
} // namespace FileSystem

namespace BCAT {

class Backend;

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(Core::System& system_, std::shared_ptr<Module> module_,
                           FileSystem::FileSystemController& fsc_, const char* name);
        ~Interface() override;

        void CreateBcatService(Kernel::HLERequestContext& ctx);
        void CreateDeliveryCacheStorageService(Kernel::HLERequestContext& ctx);
        void CreateDeliveryCacheStorageServiceWithApplicationId(Kernel::HLERequestContext& ctx);

    protected:
        FileSystem::FileSystemController& fsc;

        std::shared_ptr<Module> module;
        std::unique_ptr<Backend> backend;
    };
};

void LoopProcess(Core::System& system);

} // namespace BCAT

} // namespace Service
