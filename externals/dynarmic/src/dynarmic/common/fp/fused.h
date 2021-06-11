/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace Dynarmic::FP {

struct FPUnpacked;

/// This function assumes all arguments have been normalized.
FPUnpacked FusedMulAdd(FPUnpacked addend, FPUnpacked op1, FPUnpacked op2);

}  // namespace Dynarmic::FP
