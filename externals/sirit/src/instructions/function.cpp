/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "common_types.h"
#include "op.h"
#include "sirit/sirit.h"

namespace Sirit {

Id Module::OpFunction(Id result_type, spv::FunctionControlMask function_control, Id function_type) {
    auto op{std::make_unique<Op>(spv::Op::OpFunction, bound++, result_type)};
    op->Add(static_cast<u32>(function_control));
    op->Add(function_type);
    return AddCode(std::move(op));
}

Id Module::OpFunctionEnd() {
    return AddCode(spv::Op::OpFunctionEnd);
}

Id Module::OpFunctionCall(Id result_type, Id function, const std::vector<Id>& arguments) {
    auto op{std::make_unique<Op>(spv::Op::OpFunctionCall, bound++, result_type)};
    op->Add(function);
    op->Add(arguments);
    return AddCode(std::move(op));
}

} // namespace Sirit
