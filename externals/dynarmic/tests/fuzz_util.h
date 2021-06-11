/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <iosfwd>

#include "dynarmic/common/common_types.h"

using Vector = std::array<u64, 2>;

std::ostream& operator<<(std::ostream& o, Vector vec);
Vector RandomVector();
u32 RandomFpcr();

struct InstructionGenerator final {
public:
    explicit InstructionGenerator(const char* format);

    u32 Generate() const;
    u32 Bits() const { return bits; }
    u32 Mask() const { return mask; }
    bool Match(u32 inst) const { return (inst & mask) == bits; }

private:
    u32 bits = 0;
    u32 mask = 0;
};
