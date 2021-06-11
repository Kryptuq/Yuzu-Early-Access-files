/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/process_nan.h"

#include <optional>

#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/fp/info.h"
#include "dynarmic/common/fp/process_exception.h"
#include "dynarmic/common/fp/unpacked.h"

namespace Dynarmic::FP {

template<typename FPT>
FPT FPProcessNaN(FPType type, FPT op, FPCR fpcr, FPSR& fpsr) {
    ASSERT(type == FPType::QNaN || type == FPType::SNaN);

    constexpr size_t topfrac = FPInfo<FPT>::explicit_mantissa_width - 1;

    FPT result = op;

    if (type == FPType::SNaN) {
        result = Common::ModifyBit<topfrac>(op, true);
        FPProcessException(FPExc::InvalidOp, fpcr, fpsr);
    }

    if (fpcr.DN()) {
        result = FPInfo<FPT>::DefaultNaN();
    }

    return result;
}

template u16 FPProcessNaN<u16>(FPType type, u16 op, FPCR fpcr, FPSR& fpsr);
template u32 FPProcessNaN<u32>(FPType type, u32 op, FPCR fpcr, FPSR& fpsr);
template u64 FPProcessNaN<u64>(FPType type, u64 op, FPCR fpcr, FPSR& fpsr);

template<typename FPT>
std::optional<FPT> FPProcessNaNs(FPType type1, FPType type2, FPT op1, FPT op2, FPCR fpcr, FPSR& fpsr) {
    if (type1 == FPType::SNaN) {
        return FPProcessNaN<FPT>(type1, op1, fpcr, fpsr);
    }
    if (type2 == FPType::SNaN) {
        return FPProcessNaN<FPT>(type2, op2, fpcr, fpsr);
    }
    if (type1 == FPType::QNaN) {
        return FPProcessNaN<FPT>(type1, op1, fpcr, fpsr);
    }
    if (type2 == FPType::QNaN) {
        return FPProcessNaN<FPT>(type2, op2, fpcr, fpsr);
    }
    return std::nullopt;
}

template std::optional<u16> FPProcessNaNs<u16>(FPType type1, FPType type2, u16 op1, u16 op2, FPCR fpcr, FPSR& fpsr);
template std::optional<u32> FPProcessNaNs<u32>(FPType type1, FPType type2, u32 op1, u32 op2, FPCR fpcr, FPSR& fpsr);
template std::optional<u64> FPProcessNaNs<u64>(FPType type1, FPType type2, u64 op1, u64 op2, FPCR fpcr, FPSR& fpsr);

template<typename FPT>
std::optional<FPT> FPProcessNaNs3(FPType type1, FPType type2, FPType type3, FPT op1, FPT op2, FPT op3, FPCR fpcr, FPSR& fpsr) {
    if (type1 == FPType::SNaN) {
        return FPProcessNaN<FPT>(type1, op1, fpcr, fpsr);
    }
    if (type2 == FPType::SNaN) {
        return FPProcessNaN<FPT>(type2, op2, fpcr, fpsr);
    }
    if (type3 == FPType::SNaN) {
        return FPProcessNaN<FPT>(type3, op3, fpcr, fpsr);
    }
    if (type1 == FPType::QNaN) {
        return FPProcessNaN<FPT>(type1, op1, fpcr, fpsr);
    }
    if (type2 == FPType::QNaN) {
        return FPProcessNaN<FPT>(type2, op2, fpcr, fpsr);
    }
    if (type3 == FPType::QNaN) {
        return FPProcessNaN<FPT>(type3, op3, fpcr, fpsr);
    }
    return std::nullopt;
}

template std::optional<u16> FPProcessNaNs3<u16>(FPType type1, FPType type2, FPType type3, u16 op1, u16 op2, u16 op3, FPCR fpcr, FPSR& fpsr);
template std::optional<u32> FPProcessNaNs3<u32>(FPType type1, FPType type2, FPType type3, u32 op1, u32 op2, u32 op3, FPCR fpcr, FPSR& fpsr);
template std::optional<u64> FPProcessNaNs3<u64>(FPType type1, FPType type2, FPType type3, u64 op1, u64 op2, u64 op3, FPCR fpcr, FPSR& fpsr);

}  // namespace Dynarmic::FP
