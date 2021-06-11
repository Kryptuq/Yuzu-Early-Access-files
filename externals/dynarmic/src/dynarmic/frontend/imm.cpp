/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/imm.h"

#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"

namespace Dynarmic {

u64 AdvSIMDExpandImm(bool op, Imm<4> cmode, Imm<8> imm8) {
    switch (cmode.Bits<1, 3>()) {
    case 0b000:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>(), 32);
    case 0b001:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>() << 8, 32);
    case 0b010:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>() << 16, 32);
    case 0b011:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>() << 24, 32);
    case 0b100:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>(), 16);
    case 0b101:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>() << 8, 16);
    case 0b110:
        if (!cmode.Bit<0>()) {
            return Common::Replicate<u64>((imm8.ZeroExtend<u64>() << 8) | Common::Ones<u64>(8), 32);
        }
        return Common::Replicate<u64>((imm8.ZeroExtend<u64>() << 16) | Common::Ones<u64>(16), 32);
    case 0b111:
        if (!cmode.Bit<0>() && !op) {
            return Common::Replicate<u64>(imm8.ZeroExtend<u64>(), 8);
        }
        if (!cmode.Bit<0>() && op) {
            u64 result = 0;
            result |= imm8.Bit<0>() ? Common::Ones<u64>(8) << (0 * 8) : 0;
            result |= imm8.Bit<1>() ? Common::Ones<u64>(8) << (1 * 8) : 0;
            result |= imm8.Bit<2>() ? Common::Ones<u64>(8) << (2 * 8) : 0;
            result |= imm8.Bit<3>() ? Common::Ones<u64>(8) << (3 * 8) : 0;
            result |= imm8.Bit<4>() ? Common::Ones<u64>(8) << (4 * 8) : 0;
            result |= imm8.Bit<5>() ? Common::Ones<u64>(8) << (5 * 8) : 0;
            result |= imm8.Bit<6>() ? Common::Ones<u64>(8) << (6 * 8) : 0;
            result |= imm8.Bit<7>() ? Common::Ones<u64>(8) << (7 * 8) : 0;
            return result;
        }
        if (cmode.Bit<0>() && !op) {
            u64 result = 0;
            result |= imm8.Bit<7>() ? 0x80000000 : 0;
            result |= imm8.Bit<6>() ? 0x3E000000 : 0x40000000;
            result |= imm8.Bits<0, 5, u64>() << 19;
            return Common::Replicate<u64>(result, 32);
        }
        if (cmode.Bit<0>() && op) {
            u64 result = 0;
            result |= imm8.Bit<7>() ? 0x80000000'00000000 : 0;
            result |= imm8.Bit<6>() ? 0x3FC00000'00000000 : 0x40000000'00000000;
            result |= imm8.Bits<0, 5, u64>() << 48;
            return result;
        }
    }
    UNREACHABLE();
}

}  // namespace Dynarmic
