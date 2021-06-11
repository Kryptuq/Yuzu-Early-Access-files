/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/rounding_mode.h"
#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::SCVTF_float_int(bool sf, Imm<2> type, Reg Rn, Vec Vd) {
    const size_t intsize = sf ? 64 : 32;
    const auto fltsize = FPGetDataSize(type);
    if (!fltsize || *fltsize == 16) {
        return UnallocatedEncoding();
    }

    const IR::U32U64 intval = X(intsize, Rn);
    IR::U32U64 fltval;

    if (*fltsize == 32) {
        fltval = ir.FPSignedFixedToSingle(intval, 0, ir.current_location->FPCR().RMode());
    } else if (*fltsize == 64) {
        fltval = ir.FPSignedFixedToDouble(intval, 0, ir.current_location->FPCR().RMode());
    } else {
        UNREACHABLE();
    }

    V_scalar(*fltsize, Vd, fltval);

    return true;
}

bool TranslatorVisitor::UCVTF_float_int(bool sf, Imm<2> type, Reg Rn, Vec Vd) {
    const size_t intsize = sf ? 64 : 32;
    const auto fltsize = FPGetDataSize(type);
    if (!fltsize || *fltsize == 16) {
        return UnallocatedEncoding();
    }

    const IR::U32U64 intval = X(intsize, Rn);
    IR::U32U64 fltval;

    if (*fltsize == 32) {
        fltval = ir.FPUnsignedFixedToSingle(intval, 0, ir.current_location->FPCR().RMode());
    } else if (*fltsize == 64) {
        fltval = ir.FPUnsignedFixedToDouble(intval, 0, ir.current_location->FPCR().RMode());
    } else {
        UNREACHABLE();
    }

    V_scalar(*fltsize, Vd, fltval);

    return true;
}

bool TranslatorVisitor::FMOV_float_gen(bool sf, Imm<2> type, Imm<1> rmode_0, Imm<1> opc_0, size_t n, size_t d) {
    // NOTE:
    // opcode<2:1> == 0b11
    // rmode<1> == 0b0

    if (type == 0b10 && rmode_0 != 1) {
        return UnallocatedEncoding();
    }

    const size_t intsize = sf ? 64 : 32;
    size_t fltsize = [type] {
        switch (type.ZeroExtend()) {
        case 0b00:
            return 32;
        case 0b01:
            return 64;
        case 0b10:
            return 128;
        case 0b11:
            return 16;
        default:
            UNREACHABLE();
        }
    }();

    bool integer_to_float;
    size_t part;
    switch (rmode_0.ZeroExtend()) {
    case 0b0:
        if (fltsize != 16 && fltsize != intsize) {
            return UnallocatedEncoding();
        }
        integer_to_float = opc_0 == 0b1;
        part = 0;
        break;
    default:
    case 0b1:
        if (intsize != 64 || fltsize != 128) {
            return UnallocatedEncoding();
        }
        integer_to_float = opc_0 == 0b1;
        part = 1;
        fltsize = 64;
        break;
    }

    if (integer_to_float) {
        const IR::U16U32U64 intval = X(fltsize, static_cast<Reg>(n));
        Vpart_scalar(fltsize, static_cast<Vec>(d), part, intval);
    } else {
        const IR::UAny fltval = Vpart_scalar(fltsize, static_cast<Vec>(n), part);
        const IR::U32U64 intval = ZeroExtend(fltval, intsize);
        X(intsize, static_cast<Reg>(d), intval);
    }

    return true;
}

static bool FloaingPointConvertSignedInteger(TranslatorVisitor& v, bool sf, Imm<2> type, Vec Vn, Reg Rd, FP::RoundingMode rounding_mode) {
    const size_t intsize = sf ? 64 : 32;
    const auto fltsize = FPGetDataSize(type);
    if (!fltsize) {
        return v.UnallocatedEncoding();
    }

    const IR::U16U32U64 fltval = v.V_scalar(*fltsize, Vn);
    IR::U32U64 intval;

    if (intsize == 32) {
        intval = v.ir.FPToFixedS32(fltval, 0, rounding_mode);
    } else if (intsize == 64) {
        intval = v.ir.FPToFixedS64(fltval, 0, rounding_mode);
    } else {
        UNREACHABLE();
    }

    v.X(intsize, Rd, intval);
    return true;
}

static bool FloaingPointConvertUnsignedInteger(TranslatorVisitor& v, bool sf, Imm<2> type, Vec Vn, Reg Rd, FP::RoundingMode rounding_mode) {
    const size_t intsize = sf ? 64 : 32;
    const auto fltsize = FPGetDataSize(type);
    if (!fltsize) {
        return v.UnallocatedEncoding();
    }

    const IR::U16U32U64 fltval = v.V_scalar(*fltsize, Vn);
    IR::U32U64 intval;

    if (intsize == 32) {
        intval = v.ir.FPToFixedU32(fltval, 0, rounding_mode);
    } else if (intsize == 64) {
        intval = v.ir.FPToFixedU64(fltval, 0, rounding_mode);
    } else {
        UNREACHABLE();
    }

    v.X(intsize, Rd, intval);
    return true;
}

bool TranslatorVisitor::FCVTNS_float(bool sf, Imm<2> type, Vec Vn, Reg Rd) {
    return FloaingPointConvertSignedInteger(*this, sf, type, Vn, Rd, FP::RoundingMode::ToNearest_TieEven);
}

bool TranslatorVisitor::FCVTNU_float(bool sf, Imm<2> type, Vec Vn, Reg Rd) {
    return FloaingPointConvertUnsignedInteger(*this, sf, type, Vn, Rd, FP::RoundingMode::ToNearest_TieEven);
}

bool TranslatorVisitor::FCVTZS_float_int(bool sf, Imm<2> type, Vec Vn, Reg Rd) {
    return FloaingPointConvertSignedInteger(*this, sf, type, Vn, Rd, FP::RoundingMode::TowardsZero);
}

bool TranslatorVisitor::FCVTZU_float_int(bool sf, Imm<2> type, Vec Vn, Reg Rd) {
    return FloaingPointConvertUnsignedInteger(*this, sf, type, Vn, Rd, FP::RoundingMode::TowardsZero);
}

bool TranslatorVisitor::FCVTAS_float(bool sf, Imm<2> type, Vec Vn, Reg Rd) {
    return FloaingPointConvertSignedInteger(*this, sf, type, Vn, Rd, FP::RoundingMode::ToNearest_TieAwayFromZero);
}

bool TranslatorVisitor::FCVTAU_float(bool sf, Imm<2> type, Vec Vn, Reg Rd) {
    return FloaingPointConvertUnsignedInteger(*this, sf, type, Vn, Rd, FP::RoundingMode::ToNearest_TieAwayFromZero);
}

bool TranslatorVisitor::FCVTPS_float(bool sf, Imm<2> type, Vec Vn, Reg Rd) {
    return FloaingPointConvertSignedInteger(*this, sf, type, Vn, Rd, FP::RoundingMode::TowardsPlusInfinity);
}

bool TranslatorVisitor::FCVTPU_float(bool sf, Imm<2> type, Vec Vn, Reg Rd) {
    return FloaingPointConvertUnsignedInteger(*this, sf, type, Vn, Rd, FP::RoundingMode::TowardsPlusInfinity);
}

bool TranslatorVisitor::FCVTMS_float(bool sf, Imm<2> type, Vec Vn, Reg Rd) {
    return FloaingPointConvertSignedInteger(*this, sf, type, Vn, Rd, FP::RoundingMode::TowardsMinusInfinity);
}

bool TranslatorVisitor::FCVTMU_float(bool sf, Imm<2> type, Vec Vn, Reg Rd) {
    return FloaingPointConvertUnsignedInteger(*this, sf, type, Vn, Rd, FP::RoundingMode::TowardsMinusInfinity);
}

}  // namespace Dynarmic::A64
