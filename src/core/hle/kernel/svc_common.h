// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "common/literals.h"

namespace Kernel {
using Handle = u32;
}

namespace Kernel::Svc {

using namespace Common::Literals;

enum {
    HandleWaitMask = (1u << 30),
};

constexpr inline s32 ArgumentHandleCountMax = 0x40;

constexpr inline s64 WaitInfinite = -1;

constexpr inline std::size_t HeapSizeAlignment = 2_MiB;

constexpr inline Handle InvalidHandle = Handle(0);

enum PseudoHandle : Handle {
    CurrentThread = 0xFFFF8000,
    CurrentProcess = 0xFFFF8001,
};

constexpr bool IsPseudoHandle(Handle handle) {
    return handle == PseudoHandle::CurrentProcess || handle == PseudoHandle::CurrentThread;
}

} // namespace Kernel::Svc
