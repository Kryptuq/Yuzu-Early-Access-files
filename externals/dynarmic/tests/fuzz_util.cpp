/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "./fuzz_util.h"

#include <cstring>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "./rand_int.h"
#include "dynarmic/common/assert.h"
#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/rounding_mode.h"

using namespace Dynarmic;

std::ostream& operator<<(std::ostream& o, Vector vec) {
    return o << fmt::format("{:016x}'{:016x}", vec[1], vec[0]);
}

Vector RandomVector() {
    return {RandInt<u64>(0, ~u64(0)), RandInt<u64>(0, ~u64(0))};
}

u32 RandomFpcr() {
    FP::FPCR fpcr;
    fpcr.AHP(RandInt(0, 1) == 0);
    fpcr.DN(RandInt(0, 1) == 0);
    fpcr.FZ(RandInt(0, 1) == 0);
    fpcr.RMode(static_cast<FP::RoundingMode>(RandInt(0, 3)));
    fpcr.FZ16(RandInt(0, 1) == 0);
    return fpcr.Value();
}

InstructionGenerator::InstructionGenerator(const char* format) {
    const size_t format_len = std::strlen(format);
    ASSERT(format_len == 16 || format_len == 32);

    if (format_len == 16) {
        // Begin with 16 zeros
        mask |= 0xFFFF0000;
    }

    for (size_t i = 0; i < format_len; i++) {
        const u32 bit = 1u << (format_len - i - 1);
        switch (format[i]) {
        case '0':
            mask |= bit;
            break;
        case '1':
            bits |= bit;
            mask |= bit;
            break;
        default:
            // Do nothing
            break;
        }
    }
}

u32 InstructionGenerator::Generate() const {
    const u32 random = RandInt<u32>(0, 0xFFFFFFFF);
    return bits | (random & ~mask);
}
