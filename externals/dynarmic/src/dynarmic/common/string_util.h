/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace Dynarmic::Common {

template<typename T>
constexpr char SignToChar(T value) {
    return value >= 0 ? '+' : '-';
}

}  // namespace Dynarmic::Common
