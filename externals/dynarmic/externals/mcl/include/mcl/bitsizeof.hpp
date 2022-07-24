// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <climits>
#include <cstddef>

namespace mcl {

template<typename T>
constexpr std::size_t bitsizeof = CHAR_BIT * sizeof(T);

}  // namespace mcl
