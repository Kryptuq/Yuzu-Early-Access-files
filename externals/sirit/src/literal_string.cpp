/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <string>
#include "common_types.h"
#include "literal_string.h"

namespace Sirit {

LiteralString::LiteralString(std::string string_)
    : Operand{OperandType::String}, string{std::move(string_)} {}

LiteralString::~LiteralString() = default;

void LiteralString::Fetch(Stream& stream) const {
    stream.Write(string);
}

std::size_t LiteralString::GetWordCount() const noexcept {
    return string.size() / 4 + 1;
}

bool LiteralString::operator==(const Operand& other) const noexcept {
    if (!EqualType(other)) {
        return false;
    }
    return static_cast<const LiteralString&>(other).string == string;
}

} // namespace Sirit
