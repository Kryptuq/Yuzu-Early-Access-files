/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <optional>

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

static bool SharedDecodeAndOperation(TranslatorVisitor& v, bool wback, IR::MemOp memop, bool Q, bool S, bool R, bool replicate, std::optional<Reg> Rm, Imm<3> opcode, Imm<2> size, Reg Rn, Vec Vt) {
    const size_t selem = (opcode.Bit<0>() << 1 | u32{R}) + 1;
    size_t scale = opcode.Bits<1, 2>();
    size_t index = 0;

    switch (scale) {
    case 0:
        index = Q << 3 | S << 2 | size.ZeroExtend();
        break;
    case 1:
        if (size.Bit<0>()) {
            return v.UnallocatedEncoding();
        }
        index = Q << 2 | S << 1 | u32{size.Bit<1>()};
        break;
    case 2:
        if (size.Bit<1>()) {
            return v.UnallocatedEncoding();
        }
        if (size.Bit<0>()) {
            if (S) {
                return v.UnallocatedEncoding();
            }
            index = Q;
            scale = 3;
        } else {
            index = Q << 1 | u32{S};
        }
        break;
    case 3:
        if (memop == IR::MemOp::STORE || S) {
            return v.UnallocatedEncoding();
        }
        scale = size.ZeroExtend();
        break;
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 8 << scale;
    const size_t ebytes = esize / 8;

    IR::U64 address;
    if (Rn == Reg::SP) {
        // TODO: Check SP Alignment
        address = v.SP(64);
    } else {
        address = v.X(64, Rn);
    }

    IR::U64 offs = v.ir.Imm64(0);
    if (replicate) {
        for (size_t s = 0; s < selem; s++) {
            const Vec tt = static_cast<Vec>((VecNumber(Vt) + s) % 32);
            const IR::UAnyU128 element = v.Mem(v.ir.Add(address, offs), ebytes, IR::AccessType::VEC);
            const IR::U128 broadcasted_element = v.ir.VectorBroadcast(esize, element);

            v.V(datasize, tt, broadcasted_element);

            offs = v.ir.Add(offs, v.ir.Imm64(ebytes));
        }
    } else {
        for (size_t s = 0; s < selem; s++) {
            const Vec tt = static_cast<Vec>((VecNumber(Vt) + s) % 32);
            const IR::U128 rval = v.V(128, tt);

            if (memop == IR::MemOp::LOAD) {
                const IR::UAny elem = v.Mem(v.ir.Add(address, offs), ebytes, IR::AccessType::VEC);
                const IR::U128 vec = v.ir.VectorSetElement(esize, rval, index, elem);
                v.V(128, tt, vec);
            } else {
                const IR::UAny elem = v.ir.VectorGetElement(esize, rval, index);
                v.Mem(v.ir.Add(address, offs), ebytes, IR::AccessType::VEC, elem);
            }
            offs = v.ir.Add(offs, v.ir.Imm64(ebytes));
        }
    }

    if (wback) {
        if (*Rm != Reg::SP) {
            offs = v.X(64, *Rm);
        }

        if (Rn == Reg::SP) {
            v.SP(64, v.ir.Add(address, offs));
        } else {
            v.X(64, Rn, v.ir.Add(address, offs));
        }
    }

    return true;
}

bool TranslatorVisitor::LD1_sngl_1(bool Q, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::LOAD, Q, S, false, false, {},
                                    Imm<3>{upper_opcode.ZeroExtend() << 1}, size, Rn, Vt);
}

bool TranslatorVisitor::LD1_sngl_2(bool Q, Reg Rm, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::LOAD, Q, S, false, false, Rm,
                                    Imm<3>{upper_opcode.ZeroExtend() << 1}, size, Rn, Vt);
}

bool TranslatorVisitor::LD1R_1(bool Q, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::LOAD, Q, false, false, true,
                                    {}, Imm<3>{0b110}, size, Rn, Vt);
}

bool TranslatorVisitor::LD1R_2(bool Q, Reg Rm, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::LOAD, Q, false, false, true,
                                    Rm, Imm<3>{0b110}, size, Rn, Vt);
}

bool TranslatorVisitor::LD2_sngl_1(bool Q, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::LOAD, Q, S, true, false, {},
                                    Imm<3>{upper_opcode.ZeroExtend() << 1}, size, Rn, Vt);
}

bool TranslatorVisitor::LD2_sngl_2(bool Q, Reg Rm, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::LOAD, Q, S, true, false, Rm,
                                    Imm<3>{upper_opcode.ZeroExtend() << 1}, size, Rn, Vt);
}

bool TranslatorVisitor::LD2R_1(bool Q, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::LOAD, Q, false, true, true,
                                    {}, Imm<3>{0b110}, size, Rn, Vt);
}

bool TranslatorVisitor::LD2R_2(bool Q, Reg Rm, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::LOAD, Q, false, true, true,
                                    Rm, Imm<3>{0b110}, size, Rn, Vt);
}

bool TranslatorVisitor::LD3_sngl_1(bool Q, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::LOAD, Q, S, false, false, {},
                                    Imm<3>{(upper_opcode.ZeroExtend() << 1) | 1}, size, Rn, Vt);
}

bool TranslatorVisitor::LD3_sngl_2(bool Q, Reg Rm, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::LOAD, Q, S, false, false, Rm,
                                    Imm<3>{(upper_opcode.ZeroExtend() << 1) | 1}, size, Rn, Vt);
}

bool TranslatorVisitor::LD3R_1(bool Q, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::LOAD, Q, false, false, true,
                                    {}, Imm<3>{0b111}, size, Rn, Vt);
}

bool TranslatorVisitor::LD3R_2(bool Q, Reg Rm, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::LOAD, Q, false, false, true,
                                    Rm, Imm<3>{0b111}, size, Rn, Vt);
}

bool TranslatorVisitor::LD4_sngl_1(bool Q, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::LOAD, Q, S, true, false, {},
                                    Imm<3>{(upper_opcode.ZeroExtend() << 1) | 1}, size, Rn, Vt);
}

bool TranslatorVisitor::LD4_sngl_2(bool Q, Reg Rm, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::LOAD, Q, S, true, false, Rm,
                                    Imm<3>{(upper_opcode.ZeroExtend() << 1) | 1}, size, Rn, Vt);
}

bool TranslatorVisitor::LD4R_1(bool Q, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::LOAD, Q, false, true, true,
                                    {}, Imm<3>{0b111}, size, Rn, Vt);
}

bool TranslatorVisitor::LD4R_2(bool Q, Reg Rm, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::LOAD, Q, false, true, true,
                                    Rm, Imm<3>{0b111}, size, Rn, Vt);
}

bool TranslatorVisitor::ST1_sngl_1(bool Q, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::STORE, Q, S, false, false, {},
                                    Imm<3>{upper_opcode.ZeroExtend() << 1}, size, Rn, Vt);
}

bool TranslatorVisitor::ST1_sngl_2(bool Q, Reg Rm, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::STORE, Q, S, false, false, Rm,
                                    Imm<3>{upper_opcode.ZeroExtend() << 1}, size, Rn, Vt);
}

bool TranslatorVisitor::ST2_sngl_1(bool Q, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::STORE, Q, S, true, false, {},
                                    Imm<3>{upper_opcode.ZeroExtend() << 1}, size, Rn, Vt);
}

bool TranslatorVisitor::ST2_sngl_2(bool Q, Reg Rm, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::STORE, Q, S, true, false, Rm,
                                    Imm<3>{upper_opcode.ZeroExtend() << 1}, size, Rn, Vt);
}

bool TranslatorVisitor::ST3_sngl_1(bool Q, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::STORE, Q, S, false, false, {},
                                    Imm<3>{(upper_opcode.ZeroExtend() << 1) | 1}, size, Rn, Vt);
}

bool TranslatorVisitor::ST3_sngl_2(bool Q, Reg Rm, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::STORE, Q, S, false, false, Rm,
                                    Imm<3>{(upper_opcode.ZeroExtend() << 1) | 1}, size, Rn, Vt);
}

bool TranslatorVisitor::ST4_sngl_1(bool Q, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, false, IR::MemOp::STORE, Q, S, true, false, {},
                                    Imm<3>{(upper_opcode.ZeroExtend() << 1) | 1}, size, Rn, Vt);
}

bool TranslatorVisitor::ST4_sngl_2(bool Q, Reg Rm, Imm<2> upper_opcode, bool S, Imm<2> size, Reg Rn, Vec Vt) {
    return SharedDecodeAndOperation(*this, true, IR::MemOp::STORE, Q, S, true, false, Rm,
                                    Imm<3>{(upper_opcode.ZeroExtend() << 1) | 1}, size, Rn, Vt);
}

}  // namespace Dynarmic::A64
