/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <optional>

namespace Dynarmic::FP {

class FPCR;
class FPSR;
enum class FPType;

template<typename FPT>
FPT FPProcessNaN(FPType type, FPT op, FPCR fpcr, FPSR& fpsr);

template<typename FPT>
std::optional<FPT> FPProcessNaNs(FPType type1, FPType type2, FPT op1, FPT op2, FPCR fpcr, FPSR& fpsr);

template<typename FPT>
std::optional<FPT> FPProcessNaNs3(FPType type1, FPType type2, FPType type3, FPT op1, FPT op2, FPT op3, FPCR fpcr, FPSR& fpsr);

}  // namespace Dynarmic::FP
