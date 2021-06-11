/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#pragma once

#include <string_view>
#include <vector>
#include "common_types.h"

namespace Sirit {

class Stream {
public:
    explicit Stream(std::vector<u32>& words);
    ~Stream();

    void Write(std::string_view string);

    void Write(u64 value);

    void Write(u32 value);

    void Write(u16 first, u16 second);

    void Write(u8 first, u8 second, u8 third, u8 fourth);

private:
    std::vector<u32>& words;
};

} // namespace Sirit
