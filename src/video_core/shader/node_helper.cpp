// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <vector>

#include "common/common_types.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

Node Conditional(Node condition, std::vector<Node> code) {
    return MakeNode<ConditionalNode>(std::move(condition), std::move(code));
}

Node Comment(std::string text) {
    return MakeNode<CommentNode>(std::move(text));
}

Node Immediate(u32 value) {
    return MakeNode<ImmediateNode>(value);
}

Node Immediate(s32 value) {
    return Immediate(static_cast<u32>(value));
}

Node Immediate(f32 value) {
    u32 integral;
    std::memcpy(&integral, &value, sizeof(u32));
    return Immediate(integral);
}

OperationCode SignedToUnsignedCode(OperationCode operation_code, bool is_signed) {
    if (is_signed) {
        return operation_code;
    }
    switch (operation_code) {
    case OperationCode::FCastInteger:
        return OperationCode::FCastUInteger;
    case OperationCode::IAdd:
        return OperationCode::UAdd;
    case OperationCode::IMul:
        return OperationCode::UMul;
    case OperationCode::IDiv:
        return OperationCode::UDiv;
    case OperationCode::IMin:
        return OperationCode::UMin;
    case OperationCode::IMax:
        return OperationCode::UMax;
    case OperationCode::ICastFloat:
        return OperationCode::UCastFloat;
    case OperationCode::ICastUnsigned:
        return OperationCode::UCastSigned;
    case OperationCode::ILogicalShiftLeft:
        return OperationCode::ULogicalShiftLeft;
    case OperationCode::ILogicalShiftRight:
        return OperationCode::ULogicalShiftRight;
    case OperationCode::IArithmeticShiftRight:
        return OperationCode::UArithmeticShiftRight;
    case OperationCode::IBitwiseAnd:
        return OperationCode::UBitwiseAnd;
    case OperationCode::IBitwiseOr:
        return OperationCode::UBitwiseOr;
    case OperationCode::IBitwiseXor:
        return OperationCode::UBitwiseXor;
    case OperationCode::IBitwiseNot:
        return OperationCode::UBitwiseNot;
    case OperationCode::IBitfieldExtract:
        return OperationCode::UBitfieldExtract;
    case OperationCode::IBitfieldInsert:
        return OperationCode::UBitfieldInsert;
    case OperationCode::IBitCount:
        return OperationCode::UBitCount;
    case OperationCode::LogicalILessThan:
        return OperationCode::LogicalULessThan;
    case OperationCode::LogicalIEqual:
        return OperationCode::LogicalUEqual;
    case OperationCode::LogicalILessEqual:
        return OperationCode::LogicalULessEqual;
    case OperationCode::LogicalIGreaterThan:
        return OperationCode::LogicalUGreaterThan;
    case OperationCode::LogicalINotEqual:
        return OperationCode::LogicalUNotEqual;
    case OperationCode::LogicalIGreaterEqual:
        return OperationCode::LogicalUGreaterEqual;
    case OperationCode::AtomicIExchange:
        return OperationCode::AtomicUExchange;
    case OperationCode::AtomicIAdd:
        return OperationCode::AtomicUAdd;
    case OperationCode::AtomicIMin:
        return OperationCode::AtomicUMin;
    case OperationCode::AtomicIMax:
        return OperationCode::AtomicUMax;
    case OperationCode::AtomicIAnd:
        return OperationCode::AtomicUAnd;
    case OperationCode::AtomicIOr:
        return OperationCode::AtomicUOr;
    case OperationCode::AtomicIXor:
        return OperationCode::AtomicUXor;
    case OperationCode::INegate:
        UNREACHABLE_MSG("Can't negate an unsigned integer");
        return {};
    case OperationCode::IAbsolute:
        UNREACHABLE_MSG("Can't apply absolute to an unsigned integer");
        return {};
    default:
        UNREACHABLE_MSG("Unknown signed operation with code={}", operation_code);
        return {};
    }
}

} // namespace VideoCommon::Shader
