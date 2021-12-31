/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <array>

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {

template<typename FnT>
bool TranslatorVisitor::EmitVfpVectorOperation(bool sz, ExtReg d, ExtReg n, ExtReg m, const FnT& fn) {
    if (!ir.current_location.FPSCR().Stride()) {
        return UnpredictableInstruction();
    }

    // VFP register banks are 8 single-precision registers in size.
    const size_t register_bank_size = sz ? 4 : 8;
    size_t vector_length = ir.current_location.FPSCR().Len();
    const size_t vector_stride = *ir.current_location.FPSCR().Stride();

    // Unpredictable case
    if (vector_stride * vector_length > register_bank_size) {
        return UnpredictableInstruction();
    }

    // Scalar case
    if (vector_length == 1) {
        if (vector_stride != 1) {
            return UnpredictableInstruction();
        }

        fn(d, n, m);
        return true;
    }

    // The VFP register file is divided into banks each containing:
    // * eight single-precision registers, or
    // * four double-precision registers.
    // VFP vector instructions access these registers in a circular manner.
    const auto bank_increment = [register_bank_size](ExtReg reg, size_t stride) -> ExtReg {
        const auto reg_number = static_cast<size_t>(reg);
        const auto bank_index = reg_number % register_bank_size;
        const auto bank_start = reg_number - bank_index;
        const auto next_reg_number = bank_start + ((bank_index + stride) % register_bank_size);
        return static_cast<ExtReg>(next_reg_number);
    };

    // The first and fifth banks in the register file are scalar banks.
    // All the other banks are vector banks.
    const auto belongs_to_scalar_bank = [](ExtReg reg) -> bool {
        return (reg >= ExtReg::D0 && reg <= ExtReg::D3)
            || (reg >= ExtReg::D16 && reg <= ExtReg::D19)
            || (reg >= ExtReg::S0 && reg <= ExtReg::S7);
    };

    const bool d_is_scalar = belongs_to_scalar_bank(d);
    const bool m_is_scalar = belongs_to_scalar_bank(m);

    if (d_is_scalar) {
        // If destination register is in a scalar bank, the operands and results are all scalars.
        vector_length = 1;
    }

    for (size_t i = 0; i < vector_length; i++) {
        fn(d, n, m);

        d = bank_increment(d, vector_stride);
        n = bank_increment(n, vector_stride);
        if (!m_is_scalar) {
            m = bank_increment(m, vector_stride);
        }
    }

    return true;
}

template<typename FnT>
bool TranslatorVisitor::EmitVfpVectorOperation(bool sz, ExtReg d, ExtReg m, const FnT& fn) {
    return EmitVfpVectorOperation(sz, d, ExtReg::S0, m, [fn](ExtReg d, ExtReg, ExtReg m) {
        fn(d, m);
    });
}

// VADD<c>.F64 <Dd>, <Dn>, <Dm>
// VADD<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VADD(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.FPAdd(reg_n, reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VSUB<c>.F64 <Dd>, <Dn>, <Dm>
// VSUB<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VSUB(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.FPSub(reg_n, reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VMUL<c>.F64 <Dd>, <Dn>, <Dm>
// VMUL<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VMUL(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.FPMul(reg_n, reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VMLA<c>.F64 <Dd>, <Dn>, <Dm>
// VMLA<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VMLA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto reg_d = ir.GetExtendedRegister(d);
        const auto result = ir.FPAdd(reg_d, ir.FPMul(reg_n, reg_m));
        ir.SetExtendedRegister(d, result);
    });
}

// VMLS<c>.F64 <Dd>, <Dn>, <Dm>
// VMLS<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VMLS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto reg_d = ir.GetExtendedRegister(d);
        const auto result = ir.FPAdd(reg_d, ir.FPNeg(ir.FPMul(reg_n, reg_m)));
        ir.SetExtendedRegister(d, result);
    });
}

// VNMUL<c>.F64 <Dd>, <Dn>, <Dm>
// VNMUL<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VNMUL(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.FPNeg(ir.FPMul(reg_n, reg_m));
        ir.SetExtendedRegister(d, result);
    });
}

// VNMLA<c>.F64 <Dd>, <Dn>, <Dm>
// VNMLA<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VNMLA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto reg_d = ir.GetExtendedRegister(d);
        const auto result = ir.FPAdd(ir.FPNeg(reg_d), ir.FPNeg(ir.FPMul(reg_n, reg_m)));
        ir.SetExtendedRegister(d, result);
    });
}

// VNMLS<c>.F64 <Dd>, <Dn>, <Dm>
// VNMLS<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VNMLS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto reg_d = ir.GetExtendedRegister(d);
        const auto result = ir.FPAdd(ir.FPNeg(reg_d), ir.FPMul(reg_n, reg_m));
        ir.SetExtendedRegister(d, result);
    });
}

// VDIV<c>.F64 <Dd>, <Dn>, <Dm>
// VDIV<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VDIV(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.FPDiv(reg_n, reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VFNMS<c>.F64 <Dd>, <Dn>, <Dm>
// VFNMS<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VFNMS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto reg_d = ir.GetExtendedRegister(d);
        const auto result = ir.FPMulAdd(ir.FPNeg(reg_d), reg_n, reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VFNMA<c>.F64 <Dd>, <Dn>, <Dm>
// VFNMA<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VFNMA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto reg_d = ir.GetExtendedRegister(d);
        const auto result = ir.FPMulAdd(ir.FPNeg(reg_d), ir.FPNeg(reg_n), reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VFMA<c>.F64 <Dd>, <Dn>, <Dm>
// VFMA<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VFMA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto reg_d = ir.GetExtendedRegister(d);
        const auto result = ir.FPMulAdd(reg_d, reg_n, reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VFMS<c>.F64 <Dd>, <Dn>, <Dm>
// VFMS<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VFMS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto reg_d = ir.GetExtendedRegister(d);
        const auto result = ir.FPMulAdd(reg_d, ir.FPNeg(reg_n), reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VSEL<c>.F64 <Dd>, <Dn>, <Dm>
// VSEL<c>.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VSEL(bool D, Imm<2> cc, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    const Cond cond = concatenate(cc, Imm<1>{cc.Bit<0>() != cc.Bit<1>()}, Imm<1>{0}).ZeroExtend<Cond>();

    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this, cond](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.ConditionalSelect(cond, reg_n, reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VMAXNM.F64 <Dd>, <Dn>, <Dm>
// VMAXNM.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VMAXNM(bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.FPMaxNumeric(reg_n, reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VMINNM.F64 <Dd>, <Dn>, <Dm>
// VMINNM.F32 <Sd>, <Sn>, <Sm>
bool TranslatorVisitor::vfp_VMINNM(bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    const auto d = ToExtReg(sz, Vd, D);
    const auto n = ToExtReg(sz, Vn, N);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, n, m, [this](ExtReg d, ExtReg n, ExtReg m) {
        const auto reg_n = ir.GetExtendedRegister(n);
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.FPMinNumeric(reg_n, reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VMOV<c>.32 <Dd[0]>, <Rt>
bool TranslatorVisitor::vfp_VMOV_u32_f64(Cond cond, size_t Vd, Reg t, bool D) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(true, Vd, D);
    const auto reg_d = ir.GetExtendedRegister(d);
    const auto reg_t = ir.GetRegister(t);
    const auto result = ir.Pack2x32To1x64(reg_t, ir.MostSignificantWord(reg_d).result);

    ir.SetExtendedRegister(d, result);
    return true;
}

// VMOV<c>.32 <Rt>, <Dn[0]>
bool TranslatorVisitor::vfp_VMOV_f64_u32(Cond cond, size_t Vn, Reg t, bool N) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto n = ToExtReg(true, Vn, N);
    const auto reg_n = ir.GetExtendedRegister(n);
    ir.SetRegister(t, ir.LeastSignificantWord(reg_n));
    return true;
}

// VMOV<c> <Sn>, <Rt>
bool TranslatorVisitor::vfp_VMOV_u32_f32(Cond cond, size_t Vn, Reg t, bool N) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto n = ToExtReg(false, Vn, N);
    ir.SetExtendedRegister(n, ir.GetRegister(t));
    return true;
}

// VMOV<c> <Rt>, <Sn>
bool TranslatorVisitor::vfp_VMOV_f32_u32(Cond cond, size_t Vn, Reg t, bool N) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto n = ToExtReg(false, Vn, N);
    ir.SetRegister(t, ir.GetExtendedRegister(n));
    return true;
}

// VMOV<c> <Sm>, <Sm1>, <Rt>, <Rt2>
bool TranslatorVisitor::vfp_VMOV_2u32_2f32(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
    const auto m = ToExtReg(false, Vm, M);
    if (t == Reg::PC || t2 == Reg::PC || m == ExtReg::S31) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    ir.SetExtendedRegister(m, ir.GetRegister(t));
    ir.SetExtendedRegister(m + 1, ir.GetRegister(t2));
    return true;
}

// VMOV<c> <Rt>, <Rt2>, <Sm>, <Sm1>
bool TranslatorVisitor::vfp_VMOV_2f32_2u32(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
    const auto m = ToExtReg(false, Vm, M);
    if (t == Reg::PC || t2 == Reg::PC || m == ExtReg::S31) {
        return UnpredictableInstruction();
    }

    if (t == t2) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    ir.SetRegister(t, ir.GetExtendedRegister(m));
    ir.SetRegister(t2, ir.GetExtendedRegister(m + 1));
    return true;
}

// VMOV<c> <Dm>, <Rt>, <Rt2>
bool TranslatorVisitor::vfp_VMOV_2u32_f64(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
    const auto m = ToExtReg(true, Vm, M);
    if (t == Reg::PC || t2 == Reg::PC || m == ExtReg::S31) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto value = ir.Pack2x32To1x64(ir.GetRegister(t), ir.GetRegister(t2));
    ir.SetExtendedRegister(m, value);
    return true;
}

// VMOV<c> <Rt>, <Rt2>, <Dm>
bool TranslatorVisitor::vfp_VMOV_f64_2u32(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
    const auto m = ToExtReg(true, Vm, M);
    if (t == Reg::PC || t2 == Reg::PC || m == ExtReg::S31) {
        return UnpredictableInstruction();
    }

    if (t == t2) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto value = ir.GetExtendedRegister(m);
    ir.SetRegister(t, ir.LeastSignificantWord(value));
    ir.SetRegister(t2, ir.MostSignificantWord(value).result);
    return true;
}

// VMOV<c>.32 <Dn[x]>, <Rt>
bool TranslatorVisitor::vfp_VMOV_from_i32(Cond cond, Imm<1> i, size_t Vd, Reg t, bool D) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    if (t == Reg::R15) {
        // TODO: v8 removes UPREDICTABLE for R13
        return UnpredictableInstruction();
    }

    const size_t index = i.ZeroExtend();
    const auto d = ToVector(false, Vd, D);

    const auto reg_d = ir.GetVector(d);
    const auto scalar = ir.GetRegister(t);
    const auto result = ir.VectorSetElement(32, reg_d, index, scalar);

    ir.SetVector(d, result);
    return true;
}

// VMOV<c>.16 <Dn[x]>, <Rt>
bool TranslatorVisitor::vfp_VMOV_from_i16(Cond cond, Imm<1> i1, size_t Vd, Reg t, bool D, Imm<1> i2) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    if (t == Reg::R15) {
        // TODO: v8 removes UPREDICTABLE for R13
        return UnpredictableInstruction();
    }

    const size_t index = concatenate(i1, i2).ZeroExtend();
    const auto d = ToVector(false, Vd, D);

    const auto reg_d = ir.GetVector(d);
    const auto scalar = ir.LeastSignificantHalf(ir.GetRegister(t));
    const auto result = ir.VectorSetElement(16, reg_d, index, scalar);

    ir.SetVector(d, result);
    return true;
}

// VMOV<c>.8 <Dn[x]>, <Rt>
bool TranslatorVisitor::vfp_VMOV_from_i8(Cond cond, Imm<1> i1, size_t Vd, Reg t, bool D, Imm<2> i2) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    if (t == Reg::R15) {
        // TODO: v8 removes UPREDICTABLE for R13
        return UnpredictableInstruction();
    }

    const size_t index = concatenate(i1, i2).ZeroExtend();
    const auto d = ToVector(false, Vd, D);

    const auto reg_d = ir.GetVector(d);
    const auto scalar = ir.LeastSignificantByte(ir.GetRegister(t));
    const auto result = ir.VectorSetElement(8, reg_d, index, scalar);

    ir.SetVector(d, result);
    return true;
}

// VMOV<c>.32 <Rt>, <Dn[x]>
bool TranslatorVisitor::vfp_VMOV_to_i32(Cond cond, Imm<1> i, size_t Vn, Reg t, bool N) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    if (t == Reg::R15) {
        // TODO: v8 removes UPREDICTABLE for R13
        return UnpredictableInstruction();
    }

    const size_t index = i.ZeroExtend();
    const auto n = ToVector(false, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto result = ir.VectorGetElement(32, reg_n, index);

    ir.SetRegister(t, result);
    return true;
}

// VMOV<c>.{U16,S16} <Rt>, <Dn[x]>
bool TranslatorVisitor::vfp_VMOV_to_i16(Cond cond, bool U, Imm<1> i1, size_t Vn, Reg t, bool N, Imm<1> i2) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    if (t == Reg::R15) {
        // TODO: v8 removes UPREDICTABLE for R13
        return UnpredictableInstruction();
    }

    const size_t index = concatenate(i1, i2).ZeroExtend();
    const auto n = ToVector(false, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto scalar = ir.VectorGetElement(16, reg_n, index);
    const auto result = U ? ir.ZeroExtendToWord(scalar) : ir.SignExtendToWord(scalar);

    ir.SetRegister(t, result);
    return true;
}

// VMOV<c>.{U8,S8} <Rt>, <Dn[x]>
bool TranslatorVisitor::vfp_VMOV_to_i8(Cond cond, bool U, Imm<1> i1, size_t Vn, Reg t, bool N, Imm<2> i2) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    if (t == Reg::R15) {
        // TODO: v8 removes UPREDICTABLE for R13
        return UnpredictableInstruction();
    }

    const size_t index = concatenate(i1, i2).ZeroExtend();
    const auto n = ToVector(false, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto scalar = ir.VectorGetElement(8, reg_n, index);
    const auto result = U ? ir.ZeroExtendToWord(scalar) : ir.SignExtendToWord(scalar);

    ir.SetRegister(t, result);
    return true;
}

// VDUP<c>.{8,16,32} <Qd>, <Rt>
// VDUP<c>.{8,16,32} <Dd>, <Rt>
bool TranslatorVisitor::vfp_VDUP(Cond cond, Imm<1> B, bool Q, size_t Vd, Reg t, bool D, Imm<1> E) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    if (Q && Common::Bit<0>(Vd)) {
        return UndefinedInstruction();
    }
    if (t == Reg::R15) {
        return UnpredictableInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const size_t BE = concatenate(B, E).ZeroExtend();
    const size_t esize = 32u >> BE;

    if (BE == 0b11) {
        return UndefinedInstruction();
    }

    const auto scalar = ir.LeastSignificant(esize, ir.GetRegister(t));
    const auto result = ir.VectorBroadcast(esize, scalar);
    ir.SetVector(d, result);
    return true;
}

// VMOV<c>.F64 <Dd>, #<imm>
// VMOV<c>.F32 <Sd>, #<imm>
bool TranslatorVisitor::vfp_VMOV_imm(Cond cond, bool D, Imm<4> imm4H, size_t Vd, bool sz, Imm<4> imm4L) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    if (ir.current_location.FPSCR().Stride() != 1 || ir.current_location.FPSCR().Len() != 1) {
        return UndefinedInstruction();
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto imm8 = concatenate(imm4H, imm4L);

    if (sz) {
        const u64 sign = static_cast<u64>(imm8.Bit<7>());
        const u64 exp = (imm8.Bit<6>() ? 0x3FC : 0x400) | imm8.Bits<4, 5, u64>();
        const u64 fract = imm8.Bits<0, 3, u64>() << 48;
        const u64 immediate = (sign << 63) | (exp << 52) | fract;
        ir.SetExtendedRegister(d, ir.Imm64(immediate));
    } else {
        const u32 sign = static_cast<u32>(imm8.Bit<7>());
        const u32 exp = (imm8.Bit<6>() ? 0x7C : 0x80) | imm8.Bits<4, 5>();
        const u32 fract = imm8.Bits<0, 3>() << 19;
        const u32 immediate = (sign << 31) | (exp << 23) | fract;
        ir.SetExtendedRegister(d, ir.Imm32(immediate));
    }
    return true;
}

// VMOV<c>.F64 <Dd>, <Dm>
// VMOV<c>.F32 <Sd>, <Sm>
bool TranslatorVisitor::vfp_VMOV_reg(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, m, [this](ExtReg d, ExtReg m) {
        ir.SetExtendedRegister(d, ir.GetExtendedRegister(m));
    });
}

// VABS<c>.F64 <Dd>, <Dm>
// VABS<c>.F32 <Sd>, <Sm>
bool TranslatorVisitor::vfp_VABS(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, m, [this](ExtReg d, ExtReg m) {
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.FPAbs(reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VNEG<c>.F64 <Dd>, <Dm>
// VNEG<c>.F32 <Sd>, <Sm>
bool TranslatorVisitor::vfp_VNEG(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, m, [this](ExtReg d, ExtReg m) {
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.FPNeg(reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VSQRT<c>.F64 <Dd>, <Dm>
// VSQRT<c>.F32 <Sd>, <Sm>
bool TranslatorVisitor::vfp_VSQRT(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, m, [this](ExtReg d, ExtReg m) {
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.FPSqrt(reg_m);
        ir.SetExtendedRegister(d, result);
    });
}

// VCVTB<c>.f32.f16 <Dd>, <Dm>
// VCVTB<c>.f64.f16 <Dd>, <Dm>
// VCVTB<c>.f16.f32 <Dd>, <Dm>
// VCVTB<c>.f16.f64 <Dd>, <Dm>
bool TranslatorVisitor::vfp_VCVTB(Cond cond, bool D, bool op, size_t Vd, bool sz, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const bool convert_from_half = !op;
    const auto rounding_mode = ir.current_location.FPSCR().RMode();
    if (convert_from_half) {
        const auto d = ToExtReg(sz, Vd, D);
        const auto m = ToExtReg(false, Vm, M);

        return EmitVfpVectorOperation(sz, d, m, [this, sz, rounding_mode](ExtReg d, ExtReg m) {
            const auto reg_m = ir.LeastSignificantHalf(ir.GetExtendedRegister(m));
            const auto result = sz ? IR::U32U64{ir.FPHalfToDouble(reg_m, rounding_mode)} : IR::U32U64{ir.FPHalfToSingle(reg_m, rounding_mode)};
            ir.SetExtendedRegister(d, result);
        });
    } else {
        const auto d = ToExtReg(false, Vd, D);
        const auto m = ToExtReg(sz, Vm, M);

        return EmitVfpVectorOperation(sz, d, m, [this, sz, rounding_mode](ExtReg d, ExtReg m) {
            const auto reg_m = ir.GetExtendedRegister(m);
            const auto result = sz ? ir.FPDoubleToHalf(reg_m, rounding_mode) : ir.FPSingleToHalf(reg_m, rounding_mode);
            ir.SetExtendedRegister(d, ir.Or(ir.And(ir.GetExtendedRegister(d), ir.Imm32(0xFFFF0000)), ir.ZeroExtendToWord(result)));
        });
    }
}

// VCVTT<c>.f32.f16 <Dd>, <Dm>
// VCVTT<c>.f64.f16 <Dd>, <Dm>
// VCVTT<c>.f16.f32 <Dd>, <Dm>
// VCVTT<c>.f16.f64 <Dd>, <Dm>
bool TranslatorVisitor::vfp_VCVTT(Cond cond, bool D, bool op, size_t Vd, bool sz, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const bool convert_from_half = !op;
    const auto rounding_mode = ir.current_location.FPSCR().RMode();
    if (convert_from_half) {
        const auto d = ToExtReg(sz, Vd, D);
        const auto m = ToExtReg(false, Vm, M);

        return EmitVfpVectorOperation(sz, d, m, [this, sz, rounding_mode](ExtReg d, ExtReg m) {
            const auto reg_m = ir.LeastSignificantHalf(ir.LogicalShiftRight(ir.GetExtendedRegister(m), ir.Imm8(16)));
            const auto result = sz ? IR::U32U64{ir.FPHalfToDouble(reg_m, rounding_mode)} : IR::U32U64{ir.FPHalfToSingle(reg_m, rounding_mode)};
            ir.SetExtendedRegister(d, result);
        });
    } else {
        const auto d = ToExtReg(false, Vd, D);
        const auto m = ToExtReg(sz, Vm, M);

        return EmitVfpVectorOperation(sz, d, m, [this, sz, rounding_mode](ExtReg d, ExtReg m) {
            const auto reg_m = ir.GetExtendedRegister(m);
            const auto result = sz ? ir.FPDoubleToHalf(reg_m, rounding_mode) : ir.FPSingleToHalf(reg_m, rounding_mode);
            ir.SetExtendedRegister(d, ir.Or(ir.And(ir.GetExtendedRegister(d), ir.Imm32(0x0000FFFF)), ir.LogicalShiftLeft(ir.ZeroExtendToWord(result), ir.Imm8(16))));
        });
    }
}

// VCMP{E}.F32 <Sd>, <Sm>
// VCMP{E}.F64 <Dd>, <Dm>
bool TranslatorVisitor::vfp_VCMP(Cond cond, bool D, size_t Vd, bool sz, bool E, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto m = ToExtReg(sz, Vm, M);
    const auto exc_on_qnan = E;
    const auto reg_d = ir.GetExtendedRegister(d);
    const auto reg_m = ir.GetExtendedRegister(m);
    const auto nzcv = ir.FPCompare(reg_d, reg_m, exc_on_qnan);

    ir.SetFpscrNZCV(nzcv);
    return true;
}

// VCMP{E}.F32 <Sd>, #0.0
// VCMP{E}.F64 <Dd>, #0.0
bool TranslatorVisitor::vfp_VCMP_zero(Cond cond, bool D, size_t Vd, bool sz, bool E) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto exc_on_qnan = E;
    const auto reg_d = ir.GetExtendedRegister(d);

    if (sz) {
        const auto nzcv = ir.FPCompare(reg_d, ir.Imm64(0), exc_on_qnan);
        ir.SetFpscrNZCV(nzcv);
    } else {
        const auto nzcv = ir.FPCompare(reg_d, ir.Imm32(0), exc_on_qnan);
        ir.SetFpscrNZCV(nzcv);
    }

    return true;
}

// VRINTR.{F16,F32} <Sd>, <Sm>
// VRINTR.F64 <Dd>, <Dm>
bool TranslatorVisitor::vfp_VRINTR(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto m = ToExtReg(sz, Vm, M);
    const auto reg_m = ir.GetExtendedRegister(m);
    const auto rounding_mode = ir.current_location.FPSCR().RMode();

    const auto result = ir.FPRoundInt(reg_m, rounding_mode, false);
    ir.SetExtendedRegister(d, result);
    return true;
}

// VRINTZ.{F16,F32} <Sd>, <Sm>
// VRINTZ.F64 <Dd>, <Dm>
bool TranslatorVisitor::vfp_VRINTZ(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto m = ToExtReg(sz, Vm, M);
    const auto reg_m = ir.GetExtendedRegister(m);
    const auto rounding_mode = FP::RoundingMode::TowardsZero;

    const auto result = ir.FPRoundInt(reg_m, rounding_mode, false);
    ir.SetExtendedRegister(d, result);
    return true;
}

// VRINTX.{F16,F32} <Sd>, <Sm>
// VRINTX.F64 <Dd>, <Dm>
bool TranslatorVisitor::vfp_VRINTX(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto m = ToExtReg(sz, Vm, M);
    const auto reg_m = ir.GetExtendedRegister(m);
    const auto rounding_mode = ir.current_location.FPSCR().RMode();

    const auto result = ir.FPRoundInt(reg_m, rounding_mode, true);
    ir.SetExtendedRegister(d, result);
    return true;
}

// VCVT<c>.F64.F32 <Dd>, <Sm>
// VCVT<c>.F32.F64 <Sd>, <Dm>
bool TranslatorVisitor::vfp_VCVT_f_to_f(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(!sz, Vd, D);  // Destination is of opposite size to source
    const auto m = ToExtReg(sz, Vm, M);
    const auto reg_m = ir.GetExtendedRegister(m);
    const auto rounding_mode = ir.current_location.FPSCR().RMode();

    if (sz) {
        const auto result = ir.FPDoubleToSingle(reg_m, rounding_mode);
        ir.SetExtendedRegister(d, result);
    } else {
        const auto result = ir.FPSingleToDouble(reg_m, rounding_mode);
        ir.SetExtendedRegister(d, result);
    }

    return true;
}

// VCVT.F32.{S32,U32} <Sd>, <Sm>
// VCVT.F64.{S32,U32} <Sd>, <Dm>
bool TranslatorVisitor::vfp_VCVT_from_int(Cond cond, bool D, size_t Vd, bool sz, bool is_signed, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto m = ToExtReg(false, Vm, M);
    const auto rounding_mode = ir.current_location.FPSCR().RMode();
    const auto reg_m = ir.GetExtendedRegister(m);

    if (sz) {
        const auto result = is_signed
                              ? ir.FPSignedFixedToDouble(reg_m, 0, rounding_mode)
                              : ir.FPUnsignedFixedToDouble(reg_m, 0, rounding_mode);
        ir.SetExtendedRegister(d, result);
    } else {
        const auto result = is_signed
                              ? ir.FPSignedFixedToSingle(reg_m, 0, rounding_mode)
                              : ir.FPUnsignedFixedToSingle(reg_m, 0, rounding_mode);
        ir.SetExtendedRegister(d, result);
    }

    return true;
}

// VCVT.F32.{S16,U16,S32,U32} <Sdm>, <Sdm>
// VCVT.F64.{S16,U16,S32,U32} <Ddm>, <Ddm>
bool TranslatorVisitor::vfp_VCVT_from_fixed(Cond cond, bool D, bool U, size_t Vd, bool sz, bool sx, Imm<1> i, Imm<4> imm4) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const size_t size = sx ? 32 : 16;
    const size_t fbits = size - concatenate(imm4, i).ZeroExtend();

    if (fbits > size) {
        return UnpredictableInstruction();
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto rounding_mode = FP::RoundingMode::ToNearest_TieEven;
    const auto reg_d = ir.GetExtendedRegister(d);
    const auto source = ir.LeastSignificant(size, reg_d);

    if (sz) {
        const auto result = U ? ir.FPUnsignedFixedToDouble(source, fbits, rounding_mode)
                              : ir.FPSignedFixedToDouble(source, fbits, rounding_mode);
        ir.SetExtendedRegister(d, result);
    } else {
        const auto result = U ? ir.FPUnsignedFixedToSingle(source, fbits, rounding_mode)
                              : ir.FPSignedFixedToSingle(source, fbits, rounding_mode);
        ir.SetExtendedRegister(d, result);
    }

    return true;
}

// VCVT{,R}.U32.F32 <Sd>, <Sm>
// VCVT{,R}.U32.F64 <Sd>, <Dm>
bool TranslatorVisitor::vfp_VCVT_to_u32(Cond cond, bool D, size_t Vd, bool sz, bool round_towards_zero, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const ExtReg d = ToExtReg(false, Vd, D);
    const ExtReg m = ToExtReg(sz, Vm, M);
    const auto reg_m = ir.GetExtendedRegister(m);
    const auto result = ir.FPToFixedU32(reg_m, 0, round_towards_zero ? FP::RoundingMode::TowardsZero : ir.current_location.FPSCR().RMode());

    ir.SetExtendedRegister(d, result);
    return true;
}

// VCVT{,R}.S32.F32 <Sd>, <Sm>
// VCVT{,R}.S32.F64 <Sd>, <Dm>
bool TranslatorVisitor::vfp_VCVT_to_s32(Cond cond, bool D, size_t Vd, bool sz, bool round_towards_zero, bool M, size_t Vm) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const auto d = ToExtReg(false, Vd, D);
    const auto m = ToExtReg(sz, Vm, M);
    const auto reg_m = ir.GetExtendedRegister(m);
    const auto result = ir.FPToFixedS32(reg_m, 0, round_towards_zero ? FP::RoundingMode::TowardsZero : ir.current_location.FPSCR().RMode());

    ir.SetExtendedRegister(d, result);
    return true;
}

// VCVT.{S16,U16,S32,U32}.F32 <Sdm>, <Sdm>
// VCVT.{S16,U16,S32,U32}.F64 <Ddm>, <Ddm>
bool TranslatorVisitor::vfp_VCVT_to_fixed(Cond cond, bool D, bool U, size_t Vd, bool sz, bool sx, Imm<1> i, Imm<4> imm4) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const size_t size = sx ? 32 : 16;
    const size_t fbits = size - concatenate(imm4, i).ZeroExtend();

    if (fbits > size) {
        return UnpredictableInstruction();
    }

    const auto d = ToExtReg(sz, Vd, D);
    const auto rounding_mode = FP::RoundingMode::TowardsZero;
    const auto reg_d = ir.GetExtendedRegister(d);

    const auto result = [&]() -> IR::U16U32U64 {
        if (sx) {
            return U ? ir.FPToFixedU32(reg_d, fbits, rounding_mode)
                     : ir.FPToFixedS32(reg_d, fbits, rounding_mode);
        } else {
            return U ? ir.FPToFixedU16(reg_d, fbits, rounding_mode)
                     : ir.FPToFixedS16(reg_d, fbits, rounding_mode);
        }
    }();

    if (sz) {
        ir.SetExtendedRegister(d, U ? ir.ZeroExtendToLong(result) : ir.SignExtendToLong(result));
    } else {
        ir.SetExtendedRegister(d, U ? ir.ZeroExtendToWord(result) : ir.SignExtendToWord(result));
    }
    return true;
}

// VRINT{A,N,P,M}.F32 <Sd>, <Sm>
// VRINT{A,N,P,M}.F64 <Dd>, <Dm>
bool TranslatorVisitor::vfp_VRINT_rm(bool D, size_t rm, size_t Vd, bool sz, bool M, size_t Vm) {
    const std::array rm_lookup{
        FP::RoundingMode::ToNearest_TieAwayFromZero,
        FP::RoundingMode::ToNearest_TieEven,
        FP::RoundingMode::TowardsPlusInfinity,
        FP::RoundingMode::TowardsMinusInfinity,
    };
    const FP::RoundingMode rounding_mode = rm_lookup[rm];

    const auto d = ToExtReg(sz, Vd, D);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, m, [this, rounding_mode](ExtReg d, ExtReg m) {
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = ir.FPRoundInt(reg_m, rounding_mode, false);
        ir.SetExtendedRegister(d, result);
    });
}

// VCVT{A,N,P,M}.F32 <Sd>, <Sm>
// VCVT{A,N,P,M}.F64 <Sd>, <Dm>
bool TranslatorVisitor::vfp_VCVT_rm(bool D, size_t rm, size_t Vd, bool sz, bool U, bool M, size_t Vm) {
    const std::array rm_lookup{
        FP::RoundingMode::ToNearest_TieAwayFromZero,
        FP::RoundingMode::ToNearest_TieEven,
        FP::RoundingMode::TowardsPlusInfinity,
        FP::RoundingMode::TowardsMinusInfinity,
    };
    const FP::RoundingMode rounding_mode = rm_lookup[rm];
    const bool unsigned_ = !U;

    const auto d = ToExtReg(false, Vd, D);
    const auto m = ToExtReg(sz, Vm, M);

    return EmitVfpVectorOperation(sz, d, m, [this, rounding_mode, unsigned_](ExtReg d, ExtReg m) {
        const auto reg_m = ir.GetExtendedRegister(m);
        const auto result = unsigned_ ? ir.FPToFixedU32(reg_m, 0, rounding_mode) : ir.FPToFixedS32(reg_m, 0, rounding_mode);
        ir.SetExtendedRegister(d, result);
    });
}

// VMSR FPSCR, <Rt>
bool TranslatorVisitor::vfp_VMSR(Cond cond, Reg t) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    // TODO: Replace this with a local cache.
    ir.PushRSB(ir.current_location.AdvancePC(4).AdvanceIT());

    ir.UpdateUpperLocationDescriptor();
    ir.SetFpscr(ir.GetRegister(t));
    ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 4));
    ir.SetTerm(IR::Term::PopRSBHint{});
    return false;
}

// VMRS <Rt>, FPSCR
bool TranslatorVisitor::vfp_VMRS(Cond cond, Reg t) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    if (t == Reg::R15) {
        // This encodes ASPR_nzcv access
        const auto nzcv = ir.GetFpscrNZCV();
        ir.SetCpsrNZCVRaw(nzcv);
    } else {
        ir.SetRegister(t, ir.GetFpscr());
    }

    return true;
}

// VPOP.{F32,F64} <list>
bool TranslatorVisitor::vfp_VPOP(Cond cond, bool D, size_t Vd, bool sz, Imm<8> imm8) {
    const ExtReg d = ToExtReg(sz, Vd, D);
    const size_t regs = sz ? imm8.ZeroExtend() >> 1 : imm8.ZeroExtend();

    if (regs == 0 || RegNumber(d) + regs > 32) {
        return UnpredictableInstruction();
    }

    if (sz && regs > 16) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm8.ZeroExtend() << 2;
    auto address = ir.GetRegister(Reg::SP);
    ir.SetRegister(Reg::SP, ir.Add(address, ir.Imm32(imm32)));

    for (size_t i = 0; i < regs; ++i) {
        if (sz) {
            auto lo = ir.ReadMemory32(address);
            address = ir.Add(address, ir.Imm32(4));
            auto hi = ir.ReadMemory32(address);
            address = ir.Add(address, ir.Imm32(4));
            if (ir.current_location.EFlag()) {
                std::swap(lo, hi);
            }
            ir.SetExtendedRegister(d + i, ir.Pack2x32To1x64(lo, hi));
        } else {
            const auto res = ir.ReadMemory32(address);
            ir.SetExtendedRegister(d + i, res);
            address = ir.Add(address, ir.Imm32(4));
        }
    }

    return true;
}

// VPUSH.{F32,F64} <list>
bool TranslatorVisitor::vfp_VPUSH(Cond cond, bool D, size_t Vd, bool sz, Imm<8> imm8) {
    const ExtReg d = ToExtReg(sz, Vd, D);
    const size_t regs = sz ? imm8.ZeroExtend() >> 1 : imm8.ZeroExtend();

    if (regs == 0 || RegNumber(d) + regs > 32) {
        return UnpredictableInstruction();
    }

    if (sz && regs > 16) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm8.ZeroExtend() << 2;
    auto address = ir.Sub(ir.GetRegister(Reg::SP), ir.Imm32(imm32));
    ir.SetRegister(Reg::SP, address);

    for (size_t i = 0; i < regs; ++i) {
        if (sz) {
            const auto reg_d = ir.GetExtendedRegister(d + i);
            auto lo = ir.LeastSignificantWord(reg_d);
            auto hi = ir.MostSignificantWord(reg_d).result;
            if (ir.current_location.EFlag())
                std::swap(lo, hi);
            ir.WriteMemory32(address, lo);
            address = ir.Add(address, ir.Imm32(4));
            ir.WriteMemory32(address, hi);
            address = ir.Add(address, ir.Imm32(4));
        } else {
            ir.WriteMemory32(address, ir.GetExtendedRegister(d + i));
            address = ir.Add(address, ir.Imm32(4));
        }
    }

    return true;
}

// VLDR<c> <Dd>, [<Rn>{, #+/-<imm>}]
// VLDR<c> <Sd>, [<Rn>{, #+/-<imm>}]
bool TranslatorVisitor::vfp_VLDR(Cond cond, bool U, bool D, Reg n, size_t Vd, bool sz, Imm<8> imm8) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm8.ZeroExtend() << 2;
    const auto d = ToExtReg(sz, Vd, D);
    const auto base = n == Reg::PC ? ir.Imm32(ir.AlignPC(4)) : ir.GetRegister(n);
    const auto address = U ? ir.Add(base, ir.Imm32(imm32)) : ir.Sub(base, ir.Imm32(imm32));

    if (sz) {
        auto lo = ir.ReadMemory32(address);
        auto hi = ir.ReadMemory32(ir.Add(address, ir.Imm32(4)));
        if (ir.current_location.EFlag()) {
            std::swap(lo, hi);
        }
        ir.SetExtendedRegister(d, ir.Pack2x32To1x64(lo, hi));
    } else {
        ir.SetExtendedRegister(d, ir.ReadMemory32(address));
    }

    return true;
}

// VSTR<c> <Dd>, [<Rn>{, #+/-<imm>}]
// VSTR<c> <Sd>, [<Rn>{, #+/-<imm>}]
bool TranslatorVisitor::vfp_VSTR(Cond cond, bool U, bool D, Reg n, size_t Vd, bool sz, Imm<8> imm8) {
    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm8.ZeroExtend() << 2;
    const auto d = ToExtReg(sz, Vd, D);
    const auto base = n == Reg::PC ? ir.Imm32(ir.AlignPC(4)) : ir.GetRegister(n);
    const auto address = U ? ir.Add(base, ir.Imm32(imm32)) : ir.Sub(base, ir.Imm32(imm32));
    if (sz) {
        const auto reg_d = ir.GetExtendedRegister(d);
        auto lo = ir.LeastSignificantWord(reg_d);
        auto hi = ir.MostSignificantWord(reg_d).result;
        if (ir.current_location.EFlag()) {
            std::swap(lo, hi);
        }
        ir.WriteMemory32(address, lo);
        ir.WriteMemory32(ir.Add(address, ir.Imm32(4)), hi);
    } else {
        ir.WriteMemory32(address, ir.GetExtendedRegister(d));
    }

    return true;
}

// VSTM{mode}<c> <Rn>{!}, <list of double registers>
bool TranslatorVisitor::vfp_VSTM_a1(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8) {
    if (!p && !u && !w) {
        ASSERT_MSG(false, "Decode error");
    }

    if (p && !w) {
        ASSERT_MSG(false, "Decode error");
    }

    if (p == u && w) {
        return arm_UDF();
    }

    if (n == Reg::PC && w) {
        return UnpredictableInstruction();
    }

    const auto d = ToExtReg(true, Vd, D);
    const size_t regs = imm8.ZeroExtend() / 2;

    if (regs == 0 || regs > 16 || A32::RegNumber(d) + regs > 32) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm8.ZeroExtend() << 2;
    auto address = u ? ir.GetRegister(n) : IR::U32(ir.Sub(ir.GetRegister(n), ir.Imm32(imm32)));
    if (w) {
        ir.SetRegister(n, u ? IR::U32(ir.Add(address, ir.Imm32(imm32))) : address);
    }
    for (size_t i = 0; i < regs; i++) {
        const auto value = ir.GetExtendedRegister(d + i);
        auto word1 = ir.LeastSignificantWord(value);
        auto word2 = ir.MostSignificantWord(value).result;

        if (ir.current_location.EFlag()) {
            std::swap(word1, word2);
        }

        ir.WriteMemory32(address, word1);
        address = ir.Add(address, ir.Imm32(4));
        ir.WriteMemory32(address, word2);
        address = ir.Add(address, ir.Imm32(4));
    }

    return true;
}

// VSTM{mode}<c> <Rn>{!}, <list of single registers>
bool TranslatorVisitor::vfp_VSTM_a2(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8) {
    if (!p && !u && !w) {
        ASSERT_MSG(false, "Decode error");
    }

    if (p && !w) {
        ASSERT_MSG(false, "Decode error");
    }

    if (p == u && w) {
        return arm_UDF();
    }

    if (n == Reg::PC && w) {
        return UnpredictableInstruction();
    }

    const auto d = ToExtReg(false, Vd, D);
    const size_t regs = imm8.ZeroExtend();

    if (regs == 0 || A32::RegNumber(d) + regs > 32) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm8.ZeroExtend() << 2;
    auto address = u ? ir.GetRegister(n) : IR::U32(ir.Sub(ir.GetRegister(n), ir.Imm32(imm32)));
    if (w) {
        ir.SetRegister(n, u ? IR::U32(ir.Add(address, ir.Imm32(imm32))) : address);
    }
    for (size_t i = 0; i < regs; i++) {
        const auto word = ir.GetExtendedRegister(d + i);
        ir.WriteMemory32(address, word);
        address = ir.Add(address, ir.Imm32(4));
    }

    return true;
}

// VLDM{mode}<c> <Rn>{!}, <list of double registers>
bool TranslatorVisitor::vfp_VLDM_a1(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8) {
    if (!p && !u && !w) {
        ASSERT_MSG(false, "Decode error");
    }

    if (p && !w) {
        ASSERT_MSG(false, "Decode error");
    }

    if (p == u && w) {
        return arm_UDF();
    }

    if (n == Reg::PC && w) {
        return UnpredictableInstruction();
    }

    const auto d = ToExtReg(true, Vd, D);
    const size_t regs = imm8.ZeroExtend() / 2;

    if (regs == 0 || regs > 16 || A32::RegNumber(d) + regs > 32) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm8.ZeroExtend() << 2;
    auto address = u ? ir.GetRegister(n) : IR::U32(ir.Sub(ir.GetRegister(n), ir.Imm32(imm32)));
    if (w) {
        ir.SetRegister(n, u ? IR::U32(ir.Add(address, ir.Imm32(imm32))) : address);
    }
    for (size_t i = 0; i < regs; i++) {
        auto word1 = ir.ReadMemory32(address);
        address = ir.Add(address, ir.Imm32(4));
        auto word2 = ir.ReadMemory32(address);
        address = ir.Add(address, ir.Imm32(4));

        if (ir.current_location.EFlag()) {
            std::swap(word1, word2);
        }

        ir.SetExtendedRegister(d + i, ir.Pack2x32To1x64(word1, word2));
    }

    return true;
}

// VLDM{mode}<c> <Rn>{!}, <list of single registers>
bool TranslatorVisitor::vfp_VLDM_a2(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8) {
    if (!p && !u && !w) {
        ASSERT_MSG(false, "Decode error");
    }

    if (p && !w) {
        ASSERT_MSG(false, "Decode error");
    }

    if (p == u && w) {
        return arm_UDF();
    }

    if (n == Reg::PC && w) {
        return UnpredictableInstruction();
    }

    const auto d = ToExtReg(false, Vd, D);
    const size_t regs = imm8.ZeroExtend();

    if (regs == 0 || A32::RegNumber(d) + regs > 32) {
        return UnpredictableInstruction();
    }

    if (!VFPConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm8.ZeroExtend() << 2;
    auto address = u ? ir.GetRegister(n) : IR::U32(ir.Sub(ir.GetRegister(n), ir.Imm32(imm32)));
    if (w) {
        ir.SetRegister(n, u ? IR::U32(ir.Add(address, ir.Imm32(imm32))) : address);
    }
    for (size_t i = 0; i < regs; i++) {
        const auto word = ir.ReadMemory32(address);
        address = ir.Add(address, ir.Imm32(4));
        ir.SetExtendedRegister(d + i, word);
    }

    return true;
}

}  // namespace Dynarmic::A32
