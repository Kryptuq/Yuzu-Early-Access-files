/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/x64/exception_handler.h"

namespace Dynarmic::Backend::X64 {

struct ExceptionHandler::Impl final {
};

ExceptionHandler::ExceptionHandler() = default;
ExceptionHandler::~ExceptionHandler() = default;

void ExceptionHandler::Register(BlockOfCode&) {
    // Do nothing
}

bool ExceptionHandler::SupportsFastmem() const noexcept {
    return false;
}

void ExceptionHandler::SetFastmemCallback(std::function<FakeCall(u64)>) {
    // Do nothing
}

}  // namespace Dynarmic::Backend::X64
