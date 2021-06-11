/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace Dynarmic::FP {

class FPCR;
class FPSR;

enum class FPExc {
    InvalidOp,
    DivideByZero,
    Overflow,
    Underflow,
    Inexact,
    InputDenorm,
};

void FPProcessException(FPExc exception, FPCR fpcr, FPSR& fpsr);

}  // namespace Dynarmic::FP
