// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <utility>
#include <variant>

#include "common/common_types.h"
#include "video_core/shader/node.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

namespace {

std::pair<Node, s64> FindOperation(const NodeBlock& code, s64 cursor,
                                   OperationCode operation_code) {
    for (; cursor >= 0; --cursor) {
        Node node = code.at(cursor);

        if (const auto operation = std::get_if<OperationNode>(&*node)) {
            if (operation->GetCode() == operation_code) {
                return {std::move(node), cursor};
            }
        }

        if (const auto conditional = std::get_if<ConditionalNode>(&*node)) {
            const auto& conditional_code = conditional->GetCode();
            auto result = FindOperation(
                conditional_code, static_cast<s64>(conditional_code.size() - 1), operation_code);
            auto& found = result.first;
            if (found) {
                return {std::move(found), cursor};
            }
        }
    }
    return {};
}

std::optional<std::pair<Node, Node>> DecoupleIndirectRead(const OperationNode& operation) {
    if (operation.GetCode() != OperationCode::UAdd) {
        return std::nullopt;
    }
    Node gpr;
    Node offset;
    ASSERT(operation.GetOperandsCount() == 2);
    for (std::size_t i = 0; i < operation.GetOperandsCount(); i++) {
        Node operand = operation[i];
        if (std::holds_alternative<ImmediateNode>(*operand)) {
            offset = operation[i];
        } else if (std::holds_alternative<GprNode>(*operand)) {
            gpr = operation[i];
        }
    }
    if (offset && gpr) {
        return std::make_pair(gpr, offset);
    }
    return std::nullopt;
}

bool AmendNodeCv(std::size_t amend_index, Node node) {
    if (const auto operation = std::get_if<OperationNode>(&*node)) {
        operation->SetAmendIndex(amend_index);
        return true;
    }
    if (const auto conditional = std::get_if<ConditionalNode>(&*node)) {
        conditional->SetAmendIndex(amend_index);
        return true;
    }
    return false;
}

} // Anonymous namespace

std::pair<Node, TrackSampler> ShaderIR::TrackBindlessSampler(Node tracked, const NodeBlock& code,
                                                             s64 cursor) {
    if (const auto cbuf = std::get_if<CbufNode>(&*tracked)) {
        const u32 cbuf_index = cbuf->GetIndex();

        // Constant buffer found, test if it's an immediate
        const auto& offset = cbuf->GetOffset();
        if (const auto immediate = std::get_if<ImmediateNode>(&*offset)) {
            auto track = MakeTrackSampler<BindlessSamplerNode>(cbuf_index, immediate->GetValue());
            return {tracked, track};
        }
        if (const auto operation = std::get_if<OperationNode>(&*offset)) {
            const u32 bound_buffer = registry.GetBoundBuffer();
            if (bound_buffer != cbuf_index) {
                return {};
            }
            if (const std::optional pair = DecoupleIndirectRead(*operation)) {
                auto [gpr, base_offset] = *pair;
                return HandleBindlessIndirectRead(*cbuf, *operation, gpr, base_offset, tracked,
                                                  code, cursor);
            }
        }
        return {};
    }
    if (const auto gpr = std::get_if<GprNode>(&*tracked)) {
        if (gpr->GetIndex() == Tegra::Shader::Register::ZeroIndex) {
            return {};
        }
        // Reduce the cursor in one to avoid infinite loops when the instruction sets the same
        // register that it uses as operand
        const auto [source, new_cursor] = TrackRegister(gpr, code, cursor - 1);
        if (!source) {
            return {};
        }
        return TrackBindlessSampler(source, code, new_cursor);
    }
    if (const auto operation = std::get_if<OperationNode>(&*tracked)) {
        const OperationNode& op = *operation;

        const OperationCode opcode = operation->GetCode();
        if (opcode == OperationCode::IBitwiseOr || opcode == OperationCode::UBitwiseOr) {
            ASSERT(op.GetOperandsCount() == 2);
            auto [node_a, index_a, offset_a] = TrackCbuf(op[0], code, cursor);
            auto [node_b, index_b, offset_b] = TrackCbuf(op[1], code, cursor);
            if (node_a && node_b) {
                auto track = MakeTrackSampler<SeparateSamplerNode>(std::pair{index_a, index_b},
                                                                   std::pair{offset_a, offset_b});
                return {tracked, std::move(track)};
            }
        }
        std::size_t i = op.GetOperandsCount();
        while (i--) {
            if (auto found = TrackBindlessSampler(op[i - 1], code, cursor); std::get<0>(found)) {
                // Constant buffer found in operand.
                return found;
            }
        }
        return {};
    }
    if (const auto conditional = std::get_if<ConditionalNode>(&*tracked)) {
        const auto& conditional_code = conditional->GetCode();
        return TrackBindlessSampler(tracked, conditional_code,
                                    static_cast<s64>(conditional_code.size()));
    }
    return {};
}

std::pair<Node, TrackSampler> ShaderIR::HandleBindlessIndirectRead(
    const CbufNode& cbuf, const OperationNode& operation, Node gpr, Node base_offset, Node tracked,
    const NodeBlock& code, s64 cursor) {
    const auto offset_imm = std::get<ImmediateNode>(*base_offset);
    const auto& gpu_driver = registry.AccessGuestDriverProfile();
    const u32 bindless_cv = NewCustomVariable();
    const u32 texture_handler_size = gpu_driver.GetTextureHandlerSize();
    Node op = Operation(OperationCode::UDiv, gpr, Immediate(texture_handler_size));

    Node cv_node = GetCustomVariable(bindless_cv);
    Node amend_op = Operation(OperationCode::Assign, std::move(cv_node), std::move(op));
    const std::size_t amend_index = DeclareAmend(std::move(amend_op));
    AmendNodeCv(amend_index, code[cursor]);

    // TODO: Implement bindless index custom variable
    auto track =
        MakeTrackSampler<ArraySamplerNode>(cbuf.GetIndex(), offset_imm.GetValue(), bindless_cv);
    return {tracked, track};
}

std::tuple<Node, u32, u32> ShaderIR::TrackCbuf(Node tracked, const NodeBlock& code,
                                               s64 cursor) const {
    if (const auto cbuf = std::get_if<CbufNode>(&*tracked)) {
        // Constant buffer found, test if it's an immediate
        const auto& offset = cbuf->GetOffset();
        if (const auto immediate = std::get_if<ImmediateNode>(&*offset)) {
            return {tracked, cbuf->GetIndex(), immediate->GetValue()};
        }
        return {};
    }
    if (const auto gpr = std::get_if<GprNode>(&*tracked)) {
        if (gpr->GetIndex() == Tegra::Shader::Register::ZeroIndex) {
            return {};
        }
        // Reduce the cursor in one to avoid infinite loops when the instruction sets the same
        // register that it uses as operand
        const auto [source, new_cursor] = TrackRegister(gpr, code, cursor - 1);
        if (!source) {
            return {};
        }
        return TrackCbuf(source, code, new_cursor);
    }
    if (const auto operation = std::get_if<OperationNode>(&*tracked)) {
        for (std::size_t i = operation->GetOperandsCount(); i > 0; --i) {
            if (auto found = TrackCbuf((*operation)[i - 1], code, cursor); std::get<0>(found)) {
                // Cbuf found in operand.
                return found;
            }
        }
        return {};
    }
    if (const auto conditional = std::get_if<ConditionalNode>(&*tracked)) {
        const auto& conditional_code = conditional->GetCode();
        return TrackCbuf(tracked, conditional_code, static_cast<s64>(conditional_code.size()));
    }
    return {};
}

std::optional<u32> ShaderIR::TrackImmediate(Node tracked, const NodeBlock& code, s64 cursor) const {
    // Reduce the cursor in one to avoid infinite loops when the instruction sets the same register
    // that it uses as operand
    const auto result = TrackRegister(&std::get<GprNode>(*tracked), code, cursor - 1);
    const auto& found = result.first;
    if (!found) {
        return std::nullopt;
    }
    if (const auto immediate = std::get_if<ImmediateNode>(&*found)) {
        return immediate->GetValue();
    }
    return std::nullopt;
}

std::pair<Node, s64> ShaderIR::TrackRegister(const GprNode* tracked, const NodeBlock& code,
                                             s64 cursor) const {
    for (; cursor >= 0; --cursor) {
        const auto [found_node, new_cursor] = FindOperation(code, cursor, OperationCode::Assign);
        if (!found_node) {
            return {};
        }
        const auto operation = std::get_if<OperationNode>(&*found_node);
        ASSERT(operation);

        const auto& target = (*operation)[0];
        if (const auto gpr_target = std::get_if<GprNode>(&*target)) {
            if (gpr_target->GetIndex() == tracked->GetIndex()) {
                return {(*operation)[1], new_cursor};
            }
        }
    }
    return {};
}

} // namespace VideoCommon::Shader
