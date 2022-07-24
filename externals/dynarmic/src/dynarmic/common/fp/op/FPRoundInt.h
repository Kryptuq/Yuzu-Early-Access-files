/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mcl/stdint.hpp>

namespace Dynarmic::FP {

class FPCR;
class FPSR;
enum class RoundingMode;

template<typename FPT>
u64 FPRoundInt(FPT op, FPCR fpcr, RoundingMode rounding, bool exact, FPSR& fpsr);

}  // namespace Dynarmic::FP
