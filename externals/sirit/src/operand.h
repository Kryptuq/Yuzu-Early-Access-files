/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#pragma once

#include <cstddef>

#include "stream.h"

namespace Sirit {

enum class OperandType { Invalid, Op, Number, String };

class Operand {
public:
    explicit Operand(OperandType operand_type);
    virtual ~Operand();

    virtual void Fetch(Stream& stream) const = 0;

    virtual std::size_t GetWordCount() const noexcept = 0;

    virtual bool operator==(const Operand& other) const noexcept = 0;

    bool operator!=(const Operand& other) const noexcept {
        return !operator==(other);
    }

    bool EqualType(const Operand& other) const noexcept {
        return operand_type == other.operand_type;
    }

private:
    OperandType operand_type;
};

} // namespace Sirit
