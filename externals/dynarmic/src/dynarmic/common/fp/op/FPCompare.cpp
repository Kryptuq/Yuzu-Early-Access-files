/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/op/FPCompare.h"

#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/fp/process_exception.h"
#include "dynarmic/common/fp/unpacked.h"

namespace Dynarmic::FP {

template<typename FPT>
bool FPCompareEQ(FPT lhs, FPT rhs, FPCR fpcr, FPSR& fpsr) {
    const auto unpacked1 = FPUnpack(lhs, fpcr, fpsr);
    const auto unpacked2 = FPUnpack(rhs, fpcr, fpsr);
    const auto type1 = std::get<FPType>(unpacked1);
    const auto type2 = std::get<FPType>(unpacked2);
    const auto& value1 = std::get<FPUnpacked>(unpacked1);
    const auto& value2 = std::get<FPUnpacked>(unpacked2);

    if (type1 == FPType::QNaN || type1 == FPType::SNaN || type2 == FPType::QNaN || type2 == FPType::SNaN) {
        if (type1 == FPType::SNaN || type2 == FPType::SNaN) {
            FPProcessException(FPExc::InvalidOp, fpcr, fpsr);
        }

        // Comparisons against NaN are never equal.
        return false;
    }

    return value1 == value2 || (type1 == FPType::Zero && type2 == FPType::Zero);
}

template bool FPCompareEQ<u16>(u16 lhs, u16 rhs, FPCR fpcr, FPSR& fpsr);
template bool FPCompareEQ<u32>(u32 lhs, u32 rhs, FPCR fpcr, FPSR& fpsr);
template bool FPCompareEQ<u64>(u64 lhs, u64 rhs, FPCR fpcr, FPSR& fpsr);

}  // namespace Dynarmic::FP
