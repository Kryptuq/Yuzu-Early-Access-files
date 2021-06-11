/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::MOVN(bool sf, Imm<2> hw, Imm<16> imm16, Reg Rd) {
    if (!sf && hw.Bit<1>()) {
        return UnallocatedEncoding();
    }

    const size_t datasize = sf ? 64 : 32;
    const size_t pos = hw.ZeroExtend<size_t>() << 4;

    u64 value = imm16.ZeroExtend<u64>() << pos;
    value = ~value;

    const auto result = I(datasize, value);
    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::MOVZ(bool sf, Imm<2> hw, Imm<16> imm16, Reg Rd) {
    if (!sf && hw.Bit<1>()) {
        return UnallocatedEncoding();
    }

    const size_t datasize = sf ? 64 : 32;
    const size_t pos = hw.ZeroExtend<size_t>() << 4;

    const u64 value = imm16.ZeroExtend<u64>() << pos;
    const auto result = I(datasize, value);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::MOVK(bool sf, Imm<2> hw, Imm<16> imm16, Reg Rd) {
    if (!sf && hw.Bit<1>()) {
        return UnallocatedEncoding();
    }

    const size_t datasize = sf ? 64 : 32;
    const size_t pos = hw.ZeroExtend<size_t>() << 4;

    const u64 mask = u64(0xFFFF) << pos;
    const u64 value = imm16.ZeroExtend<u64>() << pos;

    auto result = X(datasize, Rd);
    result = ir.And(result, I(datasize, ~mask));
    result = ir.Or(result, I(datasize, value));
    X(datasize, Rd, result);
    return true;
}

}  // namespace Dynarmic::A64
