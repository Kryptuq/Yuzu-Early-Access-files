// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/bcat/bcat.h"

namespace Service::BCAT {

BCAT::BCAT(Core::System& system_, std::shared_ptr<Module> module_,
           FileSystem::FileSystemController& fsc_, const char* name_)
    : Interface(system_, std::move(module_), fsc_, name_) {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &BCAT::CreateBcatService, "CreateBcatService"},
        {1, &BCAT::CreateDeliveryCacheStorageService, "CreateDeliveryCacheStorageService"},
        {2, &BCAT::CreateDeliveryCacheStorageServiceWithApplicationId, "CreateDeliveryCacheStorageServiceWithApplicationId"},
        {3, nullptr, "CreateDeliveryCacheProgressService"},
        {4, nullptr, "CreateDeliveryCacheProgressServiceWithApplicationId"},
    };
    // clang-format on
    RegisterHandlers(functions);
}

BCAT::~BCAT() = default;
} // namespace Service::BCAT
