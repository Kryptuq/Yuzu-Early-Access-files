/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

#define DEFINE_UNARY(opcode)                                                                       \
    Id Module::opcode(Id result_type, Id operand) {                                                \
        code->Reserve(4);                                                                          \
        return *code << OpId{spv::Op::opcode, result_type} << operand << EndOp{};                  \
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
