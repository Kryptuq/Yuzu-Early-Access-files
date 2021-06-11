// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace Kernel {
using Handle = u32;
}

namespace Kernel::Svc {

constexpr s32 ArgumentHandleCountMax = 0x40;
constexpr u32 HandleWaitMask{1u << 30};

constexpr inline Handle InvalidHandle = Handle(0);

enum PseudoHandle : Handle {
    CurrentThread = 0xFFFF8000,
    CurrentProcess = 0xFFFF8001,
};

constexpr bool IsPseudoHandle(Handle handle) {
    return handle == PseudoHandle::CurrentProcess || handle == PseudoHandle::CurrentThread;
}

} // namespace Kernel::Svc
