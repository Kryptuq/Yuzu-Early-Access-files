/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include <memory>
#include "common_types.h"
#include "op.h"
#include "sirit/sirit.h"

namespace Sirit {

#define DEFINE_UNARY(opcode)                                                                       \
    Id Module::opcode(Id result_type, Id operand) {                                                \
        auto op{std::make_unique<Op>(spv::Op::opcode, bound++, result_type)};                      \
        op->Add(operand);                                                                          \
        return AddCode(std::move(op));                                                             \
    }

DEFINE_UNARY(OpConvertFToU)
DEFINE_UNARY(OpConvertFToS)
DEFINE_UNARY(OpConvertSToF)
DEFINE_UNARY(OpConvertUToF)
DEFINE_UNARY(OpUConvert)
DEFINE_UNARY(OpSConvert)
DEFINE_UNARY(OpFConvert)
DEFINE_UNARY(OpQuantizeToF16)
DEFINE_UNARY(OpBitcast)

} // namespace Sirit
