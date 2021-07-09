/* This file is part of the sirit project.
 * Copyright (c) 2021 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

#define DEFINE_UNARY(funcname, opcode)                                                             \
    Id Module::funcname(Id result_type, Id operand) {                                              \
        code->Reserve(4);                                                                          \
        return *code << OpId{opcode, result_type} << operand << EndOp{};                           \
    }

DEFINE_UNARY(OpDPdx, spv::Op::OpDPdx)
DEFINE_UNARY(OpDPdy, spv::Op::OpDPdy)
DEFINE_UNARY(OpFwidth, spv::Op::OpFwidth)
DEFINE_UNARY(OpDPdxFine, spv::Op::OpDPdxFine)
DEFINE_UNARY(OpDPdyFine, spv::Op::OpDPdyFine)
DEFINE_UNARY(OpFwidthFine, spv::Op::OpFwidthFine)
DEFINE_UNARY(OpDPdxCoarse, spv::Op::OpDPdxCoarse)
DEFINE_UNARY(OpDPdyCoarse, spv::Op::OpDPdyCoarse)
DEFINE_UNARY(OpFwidthCoarse, spv::Op::OpFwidthCoarse)

} // namespace Sirit
