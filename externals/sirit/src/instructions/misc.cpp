/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

Id Module::OpUndef(Id result_type) {
    code->Reserve(3);
    return *code << OpId{spv::Op::OpUndef, result_type} << EndOp{};
}

void Module::OpEmitVertex() {
    code->Reserve(1);
    *code << spv::Op::OpEmitVertex << EndOp{};
}

void Module::OpEndPrimitive() {
    code->Reserve(1);
    *code << spv::Op::OpEndPrimitive << EndOp{};
}

void Module::OpEmitStreamVertex(Id stream) {
    code->Reserve(2);
    *code << spv::Op::OpEmitStreamVertex << stream << EndOp{};
}

void Module::OpEndStreamPrimitive(Id stream) {
    code->Reserve(2);
    *code << spv::Op::OpEndStreamPrimitive << stream << EndOp{};
}

} // namespace Sirit
