/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

static bool StoreRegister(TranslatorVisitor& v, const size_t datasize, const Imm<9> imm9, const Reg Rn, const Reg Rt) {
    const u64 offset = imm9.SignExtend<u64>();
    const auto acctype = IR::AccType::UNPRIV;

    IR::U64 address;
    if (Rn == Reg::SP) {
        // TODO: Check Stack Alignment
        address = v.SP(64);
    } else {
        address = v.X(64, Rn);
    }
    address = v.ir.Add(address, v.ir.Imm64(offset));

    const IR::UAny data = v.X(datasize, Rt);
    v.Mem(address, datasize / 8, acctype, data);
    return v.MemoryInstructionContinues();
}

static bool LoadRegister(TranslatorVisitor& v, const size_t datasize, const Imm<9> imm9, const Reg Rn, const Reg Rt) {
    const u64 offset = imm9.SignExtend<u64>();
    const auto acctype = IR::AccType::UNPRIV;

    IR::U64 address;
    if (Rn == Reg::SP) {
        // TODO: Check Stack Alignment
        address = v.SP(64);
    } else {
        address = v.X(64, Rn);
    }
    address = v.ir.Add(address, v.ir.Imm64(offset));

    const IR::UAny data = v.Mem(address, datasize / 8, acctype);
    // max is used to zeroextend < 32 to 32, and > 32 to 64
    const size_t extended_size = std::max<size_t>(32, datasize);
    v.X(extended_size, Rt, v.ZeroExtend(data, extended_size));
    return v.MemoryInstructionContinues();
}

static bool LoadRegisterSigned(TranslatorVisitor& v, const size_t datasize, const Imm<2> opc, const Imm<9> imm9, const Reg Rn, const Reg Rt) {
    const u64 offset = imm9.SignExtend<u64>();
    const auto acctype = IR::AccType::UNPRIV;

    IR::MemOp memop;
    bool is_signed;
    size_t regsize;
    if (opc.Bit<1>() == 0) {
        // store or zero-extending load
        memop = opc.Bit<0>() ? IR::MemOp::LOAD : IR::MemOp::STORE;
        regsize = 32;
        is_signed = false;
    } else {
        // sign-extending load
        memop = IR::MemOp::LOAD;
        regsize = opc.Bit<0>() ? 32 : 64;
        is_signed = true;
    }

    IR::U64 address;
    if (Rn == Reg::SP) {
        // TODO: Check Stack Alignment
        address = v.SP(64);
    } else {
        address = v.X(64, Rn);
    }
    address = v.ir.Add(address, v.ir.Imm64(offset));

    switch (memop) {
    case IR::MemOp::STORE:
        v.Mem(address, datasize / 8, acctype, v.X(datasize, Rt));
        break;
    case IR::MemOp::LOAD: {
        const IR::UAny data = v.Mem(address, datasize / 8, acctype);
        if (is_signed) {
            v.X(regsize, Rt, v.SignExtend(data, regsize));
        } else {
            v.X(regsize, Rt, v.ZeroExtend(data, regsize));
        }
        break;
    }
    case IR::MemOp::PREFETCH:
        // Prefetch(address, Rt);
        break;
    }
    return v.MemoryInstructionContinues();
}

bool TranslatorVisitor::STTRB(Imm<9> imm9, Reg Rn, Reg Rt) {
    return StoreRegister(*this, 8, imm9, Rn, Rt);
}

bool TranslatorVisitor::STTRH(Imm<9> imm9, Reg Rn, Reg Rt) {
    return StoreRegister(*this, 16, imm9, Rn, Rt);
}

bool TranslatorVisitor::STTR(Imm<2> size, Imm<9> imm9, Reg Rn, Reg Rt) {
    const size_t scale = size.ZeroExtend<size_t>();
    const size_t datasize = 8 << scale;
    return StoreRegister(*this, datasize, imm9, Rn, Rt);
}

bool TranslatorVisitor::LDTRB(Imm<9> imm9, Reg Rn, Reg Rt) {
    return LoadRegister(*this, 8, imm9, Rn, Rt);
}

bool TranslatorVisitor::LDTRH(Imm<9> imm9, Reg Rn, Reg Rt) {
    return LoadRegister(*this, 16, imm9, Rn, Rt);
}

bool TranslatorVisitor::LDTR(Imm<2> size, Imm<9> imm9, Reg Rn, Reg Rt) {
    const size_t scale = size.ZeroExtend<size_t>();
    const size_t datasize = 8 << scale;
    return LoadRegister(*this, datasize, imm9, Rn, Rt);
}

bool TranslatorVisitor::LDTRSB(Imm<2> opc, Imm<9> imm9, Reg Rn, Reg Rt) {
    return LoadRegisterSigned(*this, 8, opc, imm9, Rn, Rt);
}

bool TranslatorVisitor::LDTRSH(Imm<2> opc, Imm<9> imm9, Reg Rn, Reg Rt) {
    return LoadRegisterSigned(*this, 16, opc, imm9, Rn, Rt);
}

bool TranslatorVisitor::LDTRSW(Imm<9> imm9, Reg Rn, Reg Rt) {
    const u64 offset = imm9.SignExtend<u64>();
    const auto acctype = IR::AccType::UNPRIV;

    IR::U64 address;
    if (Rn == Reg::SP) {
        // TODO: Check Stack Alignment
        address = SP(64);
    } else {
        address = X(64, Rn);
    }
    address = ir.Add(address, ir.Imm64(offset));

    const IR::UAny data = Mem(address, 4, acctype);
    X(64, Rt, SignExtend(data, 64));
    return MemoryInstructionContinues();
}
}  // namespace Dynarmic::A64
