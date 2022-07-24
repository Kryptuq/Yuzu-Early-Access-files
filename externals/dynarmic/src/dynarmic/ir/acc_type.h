/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace Dynarmic::IR {

enum class AccType {
    NORMAL,
    VEC,
    STREAM,
    VECSTREAM,
    ATOMIC,
    ORDERED,
    ORDEREDRW,
    LIMITEDORDERED,
    UNPRIV,
    IFETCH,
    PTW,
    DC,
    IC,
    DCZVA,
    AT,
    SWAP,  // TODO: Remove
};

}  // namespace Dynarmic::IR
