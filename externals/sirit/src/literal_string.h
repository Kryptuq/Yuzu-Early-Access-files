/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#pragma once

#include <string>
#include "operand.h"
#include "stream.h"

namespace Sirit {

class LiteralString final : public Operand {
public:
    LiteralString(std::string string);
    ~LiteralString() override;

    void Fetch(Stream& stream) const override;

    std::size_t GetWordCount() const noexcept override;

    bool operator==(const Operand& other) const noexcept override;

private:
    std::string string;
};

} // namespace Sirit
