/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "sirit/sirit.h"

#include "stream.h"

namespace Sirit {

Id Module::OpFunction(Id result_type, spv::FunctionControlMask function_control, Id function_type) {
    code->Reserve(5);
    return *code << OpId{spv::Op::OpFunction, result_type} << function_control << function_type
                 << EndOp{};
}

void Module::OpFunctionEnd() {
    code->Reserve(1);
    *code << spv::Op::OpFunctionEnd << EndOp{};
}

Id Module::OpFunctionCall(Id result_type, Id function, std::span<const Id> arguments) {
    code->Reserve(4 + arguments.size());
    return *code << OpId{spv::Op::OpFunctionCall, result_type} << function << arguments << EndOp{};
}

Id Module::OpFunctionParameter(Id result_type) {
    code->Reserve(3);
    return *code << OpId{spv::Op::OpFunctionParameter, result_type} << EndOp{};
}

} // namespace Sirit
