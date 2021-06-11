// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "common/common_types.h"
#include "video_core/shader/node.h"

namespace VideoCommon::Shader {

/// This arithmetic operation cannot be constraint
inline constexpr MetaArithmetic PRECISE = {true};
/// This arithmetic operation can be optimized away
inline constexpr MetaArithmetic NO_PRECISE = {false};

/// Creates a conditional node
Node Conditional(Node condition, std::vector<Node> code);

/// Creates a commentary node
Node Comment(std::string text);

/// Creates a function call
Node FunctionCall(u32 func_id);

/// Creates an u32 immediate
Node Immediate(u32 value);

/// Creates a s32 immediate
Node Immediate(s32 value);

/// Creates a f32 immediate
Node Immediate(f32 value);

/// Converts an signed operation code to an unsigned operation code
OperationCode SignedToUnsignedCode(OperationCode operation_code, bool is_signed);

template <typename T, typename... Args>
Node MakeNode(Args&&... args) {
    static_assert(std::is_convertible_v<T, NodeData>);
    return std::make_shared<NodeData>(T(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
TrackSampler MakeTrackSampler(Args&&... args) {
    static_assert(std::is_convertible_v<T, TrackSamplerData>);
    return std::make_shared<TrackSamplerData>(T{std::forward<Args>(args)...});
}

template <typename... Args>
Node Operation(OperationCode code, Args&&... args) {
    if constexpr (sizeof...(args) == 0) {
        return MakeNode<OperationNode>(code);
    } else if constexpr (std::is_convertible_v<std::tuple_element_t<0, std::tuple<Args...>>,
                                               Meta>) {
        return MakeNode<OperationNode>(code, std::forward<Args>(args)...);
    } else {
        return MakeNode<OperationNode>(code, Meta{}, std::forward<Args>(args)...);
    }
}

template <typename... Args>
Node SignedOperation(OperationCode code, bool is_signed, Args&&... args) {
    return Operation(SignedToUnsignedCode(code, is_signed), std::forward<Args>(args)...);
}

} // namespace VideoCommon::Shader
