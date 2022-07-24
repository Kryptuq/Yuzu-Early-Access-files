/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <xbyak/xbyak.h>

namespace Dynarmic {

void EmitSpinLockLock(Xbyak::CodeGenerator& code, Xbyak::Reg64 ptr, Xbyak::Reg32 tmp);
void EmitSpinLockUnlock(Xbyak::CodeGenerator& code, Xbyak::Reg64 ptr, Xbyak::Reg32 tmp);

}  // namespace Dynarmic
