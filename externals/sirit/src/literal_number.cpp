/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <cassert>
#include "literal_number.h"

namespace Sirit {

LiteralNumber::LiteralNumber(u64 raw_, bool is_32_)
    : Operand{OperandType::Number}, raw{raw_}, is_32{is_32_} {}

LiteralNumber::~LiteralNumber() = default;

void LiteralNumber::Fetch(Stream& stream) const {
    if (is_32) {
        stream.Write(static_cast<u32>(raw));
    } else {
        stream.Write(raw);
    }
}

std::size_t LiteralNumber::GetWordCount() const noexcept {
    return is_32 ? 1 : 2;
}

bool LiteralNumber::operator==(const Operand& other) const noexcept {
    if (!EqualType(other)) {
        return false;
    }
    const auto& o{static_cast<const LiteralNumber&>(other)};
    return o.raw == raw && o.is_32 == is_32;
}

} // namespace Sirit
