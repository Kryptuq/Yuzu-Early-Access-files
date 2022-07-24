// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/fatal/fatal_u.h"

namespace Service::Fatal {

Fatal_U::Fatal_U(std::shared_ptr<Module> module_, Core::System& system_)
    : Interface(std::move(module_), system_, "fatal:u") {
    static const FunctionInfo functions[] = {
        {0, &Fatal_U::ThrowFatal, "ThrowFatal"},
        {1, &Fatal_U::ThrowFatalWithPolicy, "ThrowFatalWithPolicy"},
        {2, &Fatal_U::ThrowFatalWithCpuContext, "ThrowFatalWithCpuContext"},
    };
    RegisterHandlers(functions);
}

Fatal_U::~Fatal_U() = default;

} // namespace Service::Fatal
