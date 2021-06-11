// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/fatal/fatal_p.h"

namespace Service::Fatal {

Fatal_P::Fatal_P(std::shared_ptr<Module> module_, Core::System& system_)
    : Interface(std::move(module_), system_, "fatal:p") {}

Fatal_P::~Fatal_P() = default;

} // namespace Service::Fatal
