// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/bcat/bcat_module.h"

namespace Core {
class System;
}

namespace Service::BCAT {

class BCAT final : public Module::Interface {
public:
    explicit BCAT(Core::System& system_, std::shared_ptr<Module> module_,
                  FileSystem::FileSystemController& fsc_, const char* name_);
    ~BCAT() override;
};

} // namespace Service::BCAT
