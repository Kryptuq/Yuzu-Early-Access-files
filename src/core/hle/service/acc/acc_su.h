// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/acc/acc.h"

namespace Service::Account {

class ACC_SU final : public Module::Interface {
public:
    explicit ACC_SU(std::shared_ptr<Module> module_,
                    std::shared_ptr<ProfileManager> profile_manager_, Core::System& system_);
    ~ACC_SU() override;
};

} // namespace Service::Account
