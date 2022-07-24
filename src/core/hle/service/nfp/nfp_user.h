// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/nfp/nfp.h"

namespace Service::NFP {

class NFP_User final : public Module::Interface {
public:
    explicit NFP_User(std::shared_ptr<Module> module_, Core::System& system_);
    ~NFP_User() override;
};

} // namespace Service::NFP
