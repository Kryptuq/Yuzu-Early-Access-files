/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

static bool RegSharedDecodeAndOperation(TranslatorVisitor& v, size_t scale, u8 shift, Imm<2> size, Imm<1> opc_1, Imm<1> opc_0, Reg Rm, Imm<3> option, Reg Rn, Reg Rt) {
    // Shared Decode

    const auto acctype = IR::AccType::NORMAL;
    IR::MemOp memop;
    size_t regsize = 64;
    bool signed_ = false;

    if (opc_1 == 0) {
        memop = opc_0 == 1 ? IR::MemOp::LOAD : IR::MemOp::STORE;
        regsize = size == 0b11 ? 64 : 32;
        signed_ = false;
    } else if (size == 0b11) {
        memop = IR::MemOp::PREFETCH;
        if (opc_0 == 1) {
            return v.UnallocatedEncoding();
        }
    } else {
        memop = IR::MemOp::LOAD;
        if (size == 0b10 && opc_0 == 1) {
            return v.UnallocatedEncoding();
        }
        regsize = opc_0 == 1 ? 32 : 64;
        signed_ = true;
    }

    const size_t datasize = 8 << scale;

    // Operation

    const IR::U64 offset = v.ExtendReg(64, Rm, option, shift);

    IR::U64 address;
    if (Rn == Reg::SP) {
        // TODO: Check SP alignment
        address = v.SP(64);
    } else {
        address = v.X(64, Rn);
    }
    address = v.ir.Add(address, offset);

    switch (memop) {
    case IR::MemOp::STORE: {
        const IR::UAny data = v.X(datasize, Rt);
        v.Mem(address, datasize / 8, acctype, data);
        break;
    }
    case IR::MemOp::LOAD: {
        const IR::UAny data = v.Mem(address, datasize / 8, acctype);
        if (signed_) {
            v.X(regsize, Rt, v.SignExtend(data, regsize));
        } else {
            v.X(regsize, Rt, v.ZeroExtend(data, regsize));
        }
        break;
    }
    case IR::MemOp::PREFETCH:
        // TODO: Prefetch
        break;
    default:
        UNREACHABLE();
    }

    return true;
}

bool TranslatorVisitor::STRx_reg(Imm<2> size, Imm<1> opc_1, Reg Rm, Imm<3> option, bool S, Reg Rn, Reg Rt) {
    const Imm<1> opc_0{0};
    const size_t scale = size.ZeroExtend<size_t>();
    const u8 shift = S ? static_cast<u8>(scale) : 0;
    if (!option.Bit<1>()) {
        return UnallocatedEncoding();
    }
    return RegSharedDecodeAndOperation(*this, scale, shift, size, opc_1, opc_0, Rm, option, Rn, Rt);
}

bool TranslatorVisitor::LDRx_reg(Imm<2> size, Imm<1> opc_1, Reg Rm, Imm<3> option, bool S, Reg Rn, Reg Rt) {
    const Imm<1> opc_0{1};
    const size_t scale = size.ZeroExtend<size_t>();
    const u8 shift = S ? static_cast<u8>(scale) : 0;
    if (!option.Bit<1>()) {
        return UnallocatedEncoding();
    }
    return RegSharedDecodeAndOperation(*this, scale, shift, size, opc_1, opc_0, Rm, option, Rn, Rt);
}

static bool VecSharedDecodeAndOperation(TranslatorVisitor& v, size_t scale, u8 shift, Imm<1> opc_0, Reg Rm, Imm<3> option, Reg Rn, Vec Vt) {
    // Shared Decode

    const auto acctype = IR::AccType::VEC;
    const auto memop = opc_0 == 1 ? IR::MemOp::LOAD : IR::MemOp::STORE;
    const size_t datasize = 8 << scale;

    // Operation

    const IR::U64 offset = v.ExtendReg(64, Rm, option, shift);

    IR::U64 address;
    if (Rn == Reg::SP) {
        // TODO: Check SP alignment
        address = v.SP(64);
    } else {
        address = v.X(64, Rn);
    }
    address = v.ir.Add(address, offset);

    switch (memop) {
    case IR::MemOp::STORE: {
        const IR::UAnyU128 data = v.V_scalar(datasize, Vt);
        v.Mem(address, datasize / 8, acctype, data);
        break;
    }
    case IR::MemOp::LOAD: {
        const IR::UAnyU128 data = v.Mem(address, datasize / 8, acctype);
        v.V_scalar(datasize, Vt, data);
        break;
    }
    default:
        UNREACHABLE();
    }

    return true;
}

bool TranslatorVisitor::STR_reg_fpsimd(Imm<2> size, Imm<1> opc_1, Reg Rm, Imm<3> option, bool S, Reg Rn, Vec Vt) {
    const Imm<1> opc_0{0};
    const size_t scale = concatenate(opc_1, size).ZeroExtend<size_t>();
    if (scale > 4) {
        return UnallocatedEncoding();
    }
    const u8 shift = S ? static_cast<u8>(scale) : 0;
    if (!option.Bit<1>()) {
        return UnallocatedEncoding();
    }
    return VecSharedDecodeAndOperation(*this, scale, shift, opc_0, Rm, option, Rn, Vt);
}

bool TranslatorVisitor::LDR_reg_fpsimd(Imm<2> size, Imm<1> opc_1, Reg Rm, Imm<3> option, bool S, Reg Rn, Vec Vt) {
    const Imm<1> opc_0{1};
    const size_t scale = concatenate(opc_1, size).ZeroExtend<size_t>();
    if (scale > 4) {
        return UnallocatedEncoding();
    }
    const u8 shift = S ? static_cast<u8>(scale) : 0;
    if (!option.Bit<1>()) {
        return UnallocatedEncoding();
    }
    return VecSharedDecodeAndOperation(*this, scale, shift, opc_0, Rm, option, Rn, Vt);
}

}  // namespace Dynarmic::A64
