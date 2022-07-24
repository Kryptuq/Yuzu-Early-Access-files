/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/process_exception.h"

#include <mcl/assert.hpp>

#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"

namespace Dynarmic::FP {

void FPProcessException(FPExc exception, FPCR fpcr, FPSR& fpsr) {
    switch (exception) {
    case FPExc::InvalidOp:
        if (fpcr.IOE()) {
            ASSERT_FALSE("Raising floating point exceptions unimplemented");
        }
        fpsr.IOC(true);
        break;
    case FPExc::DivideByZero:
        if (fpcr.DZE()) {
            ASSERT_FALSE("Raising floating point exceptions unimplemented");
        }
        fpsr.DZC(true);
        break;
    case FPExc::Overflow:
        if (fpcr.OFE()) {
            ASSERT_FALSE("Raising floating point exceptions unimplemented");
        }
        fpsr.OFC(true);
        break;
    case FPExc::Underflow:
        if (fpcr.UFE()) {
            ASSERT_FALSE("Raising floating point exceptions unimplemented");
        }
        fpsr.UFC(true);
        break;
    case FPExc::Inexact:
        if (fpcr.IXE()) {
            ASSERT_FALSE("Raising floating point exceptions unimplemented");
        }
        fpsr.IXC(true);
        break;
    case FPExc::InputDenorm:
        if (fpcr.IDE()) {
            ASSERT_FALSE("Raising floating point exceptions unimplemented");
        }
        fpsr.IDC(true);
        break;
    default:
        UNREACHABLE();
        break;
    }
}

}  // namespace Dynarmic::FP
