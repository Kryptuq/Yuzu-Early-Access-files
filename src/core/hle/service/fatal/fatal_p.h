// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/fatal/fatal.h"

namespace Service::Fatal {

class Fatal_P final : public Module::Interface {
public:
    explicit Fatal_P(std::shared_ptr<Module> module_, Core::System& system_);
    ~Fatal_P() override;
};

} // namespace Service::Fatal
