// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/fatal/fatal_p.h"

namespace Service::Fatal {

Fatal_P::Fatal_P(std::shared_ptr<Module> module_, Core::System& system_)
    : Interface(std::move(module_), system_, "fatal:p") {}

Fatal_P::~Fatal_P() = default;

} // namespace Service::Fatal
