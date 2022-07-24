// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/spl/spl_module.h"

namespace Core {
class System;
}

namespace Service::SPL {

class CSRNG final : public Module::Interface {
public:
    explicit CSRNG(Core::System& system_, std::shared_ptr<Module> module_);
    ~CSRNG() override;
};

} // namespace Service::SPL
