// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/acc/acc.h"

namespace Service::Account {

class ACC_U1 final : public Module::Interface {
public:
    explicit ACC_U1(std::shared_ptr<Module> module_,
                    std::shared_ptr<ProfileManager> profile_manager_, Core::System& system_);
    ~ACC_U1() override;
};

} // namespace Service::Account
