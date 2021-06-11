/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#pragma once

#include <cstddef>
#include <cstring>
#include <memory>

#include "operand.h"
#include "stream.h"

namespace Sirit {

class LiteralNumber final : public Operand {
public:
    explicit LiteralNumber(u64 raw, bool is_32);
    ~LiteralNumber() override;

    void Fetch(Stream& stream) const override;

    std::size_t GetWordCount() const noexcept override;

    bool operator==(const Operand& other) const noexcept override;

    template <typename T>
    static std::unique_ptr<LiteralNumber> Create(T value) {
        static_assert(sizeof(T) == 4 || sizeof(T) == 8);

        u64 raw{};
        std::memcpy(&raw, &value, sizeof(T));
        return std::make_unique<LiteralNumber>(raw, sizeof(T) == 4);
    }

private:
    u64 raw{};
    bool is_32{};
};

} // namespace Sirit
