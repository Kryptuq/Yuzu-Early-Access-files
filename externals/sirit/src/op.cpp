/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <algorithm>
#include <cassert>
#include <limits>

#include "common_types.h"
#include "literal_number.h"
#include "literal_string.h"
#include "op.h"
#include "operand.h"

namespace Sirit {

Op::Op(spv::Op opcode_, std::optional<u32> id_, Id result_type_)
    : Operand{OperandType::Op}, opcode{opcode_}, result_type{result_type_}, id{id_} {}

Op::~Op() = default;

void Op::Fetch(Stream& stream) const {
    assert(id.has_value());
    stream.Write(id.value());
}

std::size_t Op::GetWordCount() const noexcept {
    return 1;
}

bool Op::operator==(const Operand& other) const noexcept {
    if (!EqualType(other)) {
        return false;
    }
    const auto& op = static_cast<const Op&>(other);
    if (op.opcode == opcode && result_type == op.result_type &&
        operands.size() == op.operands.size()) {
        for (std::size_t i = 0; i < operands.size(); i++) {
            if (*operands[i] != *op.operands[i]) {
                return false;
            }
        }
        return true;
    }
    return false;
}

void Op::Write(Stream& stream) const {
    stream.Write(static_cast<u16>(opcode), CalculateTotalWords());

    if (result_type) {
        result_type->Fetch(stream);
    }
    if (id.has_value()) {
        stream.Write(id.value());
    }
    for (const auto* operand : operands) {
        operand->Fetch(stream);
    }
}

void Op::Sink(std::unique_ptr<Operand> operand) {
    Add(static_cast<const Operand*>(operand.get()));
    operand_store.push_back(std::move(operand));
}

void Op::Add(const Literal& literal) {
    Sink([&] {
        switch (literal.index()) {
        case 0:
            return LiteralNumber::Create(std::get<0>(literal));
        case 1:
            return LiteralNumber::Create(std::get<1>(literal));
        case 2:
            return LiteralNumber::Create(std::get<2>(literal));
        case 3:
            return LiteralNumber::Create(std::get<3>(literal));
        case 4:
            return LiteralNumber::Create(std::get<4>(literal));
        case 5:
            return LiteralNumber::Create(std::get<5>(literal));
        default:
            // Invalid literal type
            assert(0);
            abort();
        }
    }());
}

void Op::Add(const std::vector<Literal>& literals) {
    for (const auto& literal : literals) {
        Add(literal);
    }
}

void Op::Add(const Operand* operand) {
    assert(operand);
    operands.push_back(operand);
}

void Op::Add(u32 integer) {
    Sink(LiteralNumber::Create(integer));
}

void Op::Add(s32 integer) {
    Sink(LiteralNumber::Create(integer));
}

void Op::Add(std::string string) {
    Sink(std::make_unique<LiteralString>(std::move(string)));
}

void Op::Add(const std::vector<Id>& ids) {
    assert(std::all_of(ids.begin(), ids.end(), [](auto id_) { return id_; }));
    operands.insert(operands.end(), ids.begin(), ids.end());
}

u16 Op::CalculateTotalWords() const noexcept {
    std::size_t count = 1;
    if (result_type) {
        ++count;
    }
    if (id.has_value()) {
        ++count;
    }
    for (const Operand* operand : operands) {
        count += operand->GetWordCount();
    }
    assert(count < std::numeric_limits<u16>::max());
    return static_cast<u16>(count);
}

} // namespace Sirit
