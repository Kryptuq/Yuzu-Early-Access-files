/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/x64/hostloc.h"

#include <xbyak.h>

#include "dynarmic/backend/x64/abi.h"
#include "dynarmic/backend/x64/stack_layout.h"

namespace Dynarmic::Backend::X64 {

Xbyak::Reg64 HostLocToReg64(HostLoc loc) {
    ASSERT(HostLocIsGPR(loc));
    return Xbyak::Reg64(static_cast<int>(loc));
}

Xbyak::Xmm HostLocToXmm(HostLoc loc) {
    ASSERT(HostLocIsXMM(loc));
    return Xbyak::Xmm(static_cast<int>(loc) - static_cast<int>(HostLoc::XMM0));
}

}  // namespace Dynarmic::Backend::X64
