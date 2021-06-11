/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::CRC32(bool sf, Reg Rm, Imm<2> sz, Reg Rn, Reg Rd) {
    const u32 integral_size = sz.ZeroExtend();

    if (sf && integral_size != 0b11) {
        return UnallocatedEncoding();
    }

    if (!sf && integral_size == 0b11) {
        return UnallocatedEncoding();
    }

    const IR::U32 result = [&] {
        const size_t datasize = sf ? 64 : 32;
        const IR::U32 accumulator = ir.GetW(Rn);
        const IR::U32U64 data = X(datasize, Rm);

        switch (integral_size) {
        case 0b00:
            return ir.CRC32ISO8(accumulator, data);
        case 0b01:
            return ir.CRC32ISO16(accumulator, data);
        case 0b10:
            return ir.CRC32ISO32(accumulator, data);
        case 0b11:
        default:
            return ir.CRC32ISO64(accumulator, data);
        }
    }();

    X(32, Rd, result);
    return true;
}

bool TranslatorVisitor::CRC32C(bool sf, Reg Rm, Imm<2> sz, Reg Rn, Reg Rd) {
    const u32 integral_size = sz.ZeroExtend();

    if (sf && integral_size != 0b11) {
        return UnallocatedEncoding();
    }

    if (!sf && integral_size == 0b11) {
        return UnallocatedEncoding();
    }

    const IR::U32 result = [&] {
        const size_t datasize = sf ? 64 : 32;
        const IR::U32 accumulator = ir.GetW(Rn);
        const IR::U32U64 data = X(datasize, Rm);

        switch (integral_size) {
        case 0b00:
            return ir.CRC32Castagnoli8(accumulator, data);
        case 0b01:
            return ir.CRC32Castagnoli16(accumulator, data);
        case 0b10:
            return ir.CRC32Castagnoli32(accumulator, data);
        case 0b11:
        default:
            return ir.CRC32Castagnoli64(accumulator, data);
        }
    }();

    X(32, Rd, result);
    return true;
}

}  // namespace Dynarmic::A64
