// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/assert.h"
#include "common/common_types.h"
#include "core/hle/kernel/k_condition_variable.h"
#include "core/hle/kernel/svc_types.h"

union ResultCode;

namespace Core {
class System;
}

namespace Kernel {

class KernelCore;

class KAddressArbiter {
public:
    using ThreadTree = KConditionVariable::ThreadTree;

    explicit KAddressArbiter(Core::System& system_);
    ~KAddressArbiter();

    [[nodiscard]] ResultCode SignalToAddress(VAddr addr, Svc::SignalType type, s32 value,
                                             s32 count) {
        switch (type) {
        case Svc::SignalType::Signal:
            return Signal(addr, count);
        case Svc::SignalType::SignalAndIncrementIfEqual:
            return SignalAndIncrementIfEqual(addr, value, count);
        case Svc::SignalType::SignalAndModifyByWaitingCountIfEqual:
            return SignalAndModifyByWaitingCountIfEqual(addr, value, count);
        }
        UNREACHABLE();
        return ResultUnknown;
    }

    [[nodiscard]] ResultCode WaitForAddress(VAddr addr, Svc::ArbitrationType type, s32 value,
                                            s64 timeout) {
        switch (type) {
        case Svc::ArbitrationType::WaitIfLessThan:
            return WaitIfLessThan(addr, value, false, timeout);
        case Svc::ArbitrationType::DecrementAndWaitIfLessThan:
            return WaitIfLessThan(addr, value, true, timeout);
        case Svc::ArbitrationType::WaitIfEqual:
            return WaitIfEqual(addr, value, timeout);
        }
        UNREACHABLE();
        return ResultUnknown;
    }

private:
    [[nodiscard]] ResultCode Signal(VAddr addr, s32 count);
    [[nodiscard]] ResultCode SignalAndIncrementIfEqual(VAddr addr, s32 value, s32 count);
    [[nodiscard]] ResultCode SignalAndModifyByWaitingCountIfEqual(VAddr addr, s32 value, s32 count);
    [[nodiscard]] ResultCode WaitIfLessThan(VAddr addr, s32 value, bool decrement, s64 timeout);
    [[nodiscard]] ResultCode WaitIfEqual(VAddr addr, s32 value, s64 timeout);

    ThreadTree thread_tree;

    Core::System& system;
    KernelCore& kernel;
};

} // namespace Kernel
