// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/thread.h"
#include "core/arm/cpu_interrupt_handler.h"

namespace Core {

CPUInterruptHandler::CPUInterruptHandler() : interrupt_event{std::make_unique<Common::Event>()} {}

CPUInterruptHandler::~CPUInterruptHandler() = default;

void CPUInterruptHandler::SetInterrupt(bool is_interrupted_) {
    if (is_interrupted_) {
        interrupt_event->Set();
    }
    is_interrupted = is_interrupted_;
}

void CPUInterruptHandler::AwaitInterrupt() {
    interrupt_event->Wait();
}

} // namespace Core
