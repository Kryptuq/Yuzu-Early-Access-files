/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::STP_LDP_gen(Imm<2> opc, bool not_postindex, bool wback, Imm<1> L, Imm<7> imm7, Reg Rt2, Reg Rn, Reg Rt) {
    if ((L == 0 && opc.Bit<0>() == 1) || opc == 0b11) {
        return UnallocatedEncoding();
    }

    const auto memop = L == 1 ? IR::MemOp::LOAD : IR::MemOp::STORE;
    if (memop == IR::MemOp::LOAD && wback && (Rt == Rn || Rt2 == Rn) && Rn != Reg::R31) {
        return UnpredictableInstruction();
    }
    if (memop == IR::MemOp::STORE && wback && (Rt == Rn || Rt2 == Rn) && Rn != Reg::R31) {
        return UnpredictableInstruction();
    }
    if (memop == IR::MemOp::LOAD && Rt == Rt2) {
        return UnpredictableInstruction();
    }

    IR::U64 address;
    if (Rn == Reg::SP) {
        // TODO: Check SP Alignment
        address = SP(64);
    } else {
        address = X(64, Rn);
    }

    const bool postindex = !not_postindex;
    const bool signed_ = opc.Bit<0>() != 0;
    const size_t scale = 2 + opc.Bit<1>();
    const size_t datasize = 8 << scale;
    const u64 offset = imm7.SignExtend<u64>() << scale;

    if (!postindex) {
        address = ir.Add(address, ir.Imm64(offset));
    }

    const size_t dbytes = datasize / 8;
    switch (memop) {
    case IR::MemOp::STORE: {
        const IR::U32U64 data1 = X(datasize, Rt);
        const IR::U32U64 data2 = X(datasize, Rt2);
        Mem(address, dbytes, IR::AccType::NORMAL, data1);
        Mem(ir.Add(address, ir.Imm64(dbytes)), dbytes, IR::AccType::NORMAL, data2);
        break;
    }
    case IR::MemOp::LOAD: {
        const IR::U32U64 data1 = Mem(address, dbytes, IR::AccType::NORMAL);
        const IR::U32U64 data2 = Mem(ir.Add(address, ir.Imm64(dbytes)), dbytes, IR::AccType::NORMAL);
        if (signed_) {
            X(64, Rt, SignExtend(data1, 64));
            X(64, Rt2, SignExtend(data2, 64));
        } else {
            X(datasize, Rt, data1);
            X(datasize, Rt2, data2);
        }
        break;
    }
    case IR::MemOp::PREFETCH:
        UNREACHABLE();
    }

    if (wback) {
        if (postindex) {
            address = ir.Add(address, ir.Imm64(offset));
        }

        if (Rn == Reg::SP) {
            SP(64, address);
        } else {
            X(64, Rn, address);
        }
    }

    return MemoryInstructionContinues();
}

bool TranslatorVisitor::STP_LDP_fpsimd(Imm<2> opc, bool not_postindex, bool wback, Imm<1> L, Imm<7> imm7, Vec Vt2, Reg Rn, Vec Vt) {
    if (opc == 0b11) {
        return UnallocatedEncoding();
    }

    const auto memop = L == 1 ? IR::MemOp::LOAD : IR::MemOp::STORE;
    if (memop == IR::MemOp::LOAD && Vt == Vt2) {
        return UnpredictableInstruction();
    }

    IR::U64 address;
    if (Rn == Reg::SP) {
        // TODO: Check SP Alignment
        address = SP(64);
    } else {
        address = X(64, Rn);
    }

    const bool postindex = !not_postindex;
    const size_t scale = 2 + opc.ZeroExtend<size_t>();
    const size_t datasize = 8 << scale;
    const u64 offset = imm7.SignExtend<u64>() << scale;
    const size_t dbytes = datasize / 8;

    if (!postindex) {
        address = ir.Add(address, ir.Imm64(offset));
    }

    switch (memop) {
    case IR::MemOp::STORE: {
        IR::UAnyU128 data1 = V(datasize, Vt);
        IR::UAnyU128 data2 = V(datasize, Vt2);
        if (datasize != 128) {
            data1 = ir.VectorGetElement(datasize, data1, 0);
            data2 = ir.VectorGetElement(datasize, data2, 0);
        }
        Mem(address, dbytes, IR::AccType::VEC, data1);
        Mem(ir.Add(address, ir.Imm64(dbytes)), dbytes, IR::AccType::VEC, data2);
        break;
    }
    case IR::MemOp::LOAD: {
        IR::UAnyU128 data1 = Mem(address, dbytes, IR::AccType::VEC);
        IR::UAnyU128 data2 = Mem(ir.Add(address, ir.Imm64(dbytes)), dbytes, IR::AccType::VEC);
        if (datasize != 128) {
            data1 = ir.ZeroExtendToQuad(data1);
            data2 = ir.ZeroExtendToQuad(data2);
        }
        V(datasize, Vt, data1);
        V(datasize, Vt2, data2);
        break;
    }
    case IR::MemOp::PREFETCH:
        UNREACHABLE();
    }

    if (wback) {
        if (postindex) {
            address = ir.Add(address, ir.Imm64(offset));
        }

        if (Rn == Reg::SP) {
            SP(64, address);
        } else {
            X(64, Rn, address);
        }
    }

    return MemoryInstructionContinues();
}

}  // namespace Dynarmic::A64
