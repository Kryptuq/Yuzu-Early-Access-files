// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <string_view>

#include "shader_recompiler/backend/glsl/emit_glsl_instructions.h"
#include "shader_recompiler/backend/glsl/glsl_emit_context.h"
#include "shader_recompiler/frontend/ir/value.h"
#include "shader_recompiler/profile.h"

namespace Shader::Backend::GLSL {
namespace {
constexpr char THREAD_ID[]{"gl_SubGroupInvocationARB"};

void SetInBoundsFlag(EmitContext& ctx, IR::Inst& inst) {
    IR::Inst* const in_bounds{inst.GetAssociatedPseudoOperation(IR::Opcode::GetInBoundsFromOp)};
    if (!in_bounds) {
        return;
    }
    ctx.AddU1("{}=shfl_in_bounds;", *in_bounds);
    in_bounds->Invalidate();
}

std::string ComputeMinThreadId(std::string_view thread_id, std::string_view segmentation_mask) {
    return fmt::format("({}&{})", thread_id, segmentation_mask);
}

std::string ComputeMaxThreadId(std::string_view min_thread_id, std::string_view clamp,
                               std::string_view not_seg_mask) {
    return fmt::format("({})|({}&{})", min_thread_id, clamp, not_seg_mask);
}

std::string GetMaxThreadId(std::string_view thread_id, std::string_view clamp,
                           std::string_view segmentation_mask) {
    const auto not_seg_mask{fmt::format("(~{})", segmentation_mask)};
    const auto min_thread_id{ComputeMinThreadId(thread_id, segmentation_mask)};
    return ComputeMaxThreadId(min_thread_id, clamp, not_seg_mask);
}

void UseShuffleNv(EmitContext& ctx, IR::Inst& inst, std::string_view shfl_op,
                  std::string_view value, std::string_view index,
                  [[maybe_unused]] std::string_view clamp, std::string_view segmentation_mask) {
    const auto width{fmt::format("32u>>(bitCount({}&31u))", segmentation_mask)};
    ctx.AddU32("{}={}({},{},{},shfl_in_bounds);", inst, shfl_op, value, index, width);
    SetInBoundsFlag(ctx, inst);
}

std::string_view BallotIndex(EmitContext& ctx) {
    if (!ctx.profile.warp_size_potentially_larger_than_guest) {
        return ".x";
    }
    return "[gl_SubGroupInvocationARB>>5]";
}

std::string GetMask(EmitContext& ctx, std::string_view mask) {
    const auto ballot_index{BallotIndex(ctx)};
    return fmt::format("uint(uvec2({}){})", mask, ballot_index);
}
} // Anonymous namespace

void EmitLaneId(EmitContext& ctx, IR::Inst& inst) {
    ctx.AddU32("{}={}&31u;", inst, THREAD_ID);
}

void EmitVoteAll(EmitContext& ctx, IR::Inst& inst, std::string_view pred) {
    if (!ctx.profile.warp_size_potentially_larger_than_guest) {
        ctx.AddU1("{}=allInvocationsEqualARB({});", inst, pred);
        return;
    }
    const auto ballot_index{BallotIndex(ctx)};
    const auto active_mask{fmt::format("uvec2(ballotARB(true)){}", ballot_index)};
    const auto ballot{fmt::format("uvec2(ballotARB({})){}", pred, ballot_index)};
    ctx.AddU1("{}=({}&{})=={};", inst, ballot, active_mask, active_mask);
}

void EmitVoteAny(EmitContext& ctx, IR::Inst& inst, std::string_view pred) {
    if (!ctx.profile.warp_size_potentially_larger_than_guest) {
        ctx.AddU1("{}=anyInvocationARB({});", inst, pred);
        return;
    }
    const auto ballot_index{BallotIndex(ctx)};
    const auto active_mask{fmt::format("uvec2(ballotARB(true)){}", ballot_index)};
    const auto ballot{fmt::format("uvec2(ballotARB({})){}", pred, ballot_index)};
    ctx.AddU1("{}=({}&{})!=0u;", inst, ballot, active_mask, active_mask);
}

void EmitVoteEqual(EmitContext& ctx, IR::Inst& inst, std::string_view pred) {
    if (!ctx.profile.warp_size_potentially_larger_than_guest) {
        ctx.AddU1("{}=allInvocationsEqualARB({});", inst, pred);
        return;
    }
    const auto ballot_index{BallotIndex(ctx)};
    const auto active_mask{fmt::format("uvec2(ballotARB(true)){}", ballot_index)};
    const auto ballot{fmt::format("uvec2(ballotARB({})){}", pred, ballot_index)};
    const auto value{fmt::format("({}^{})", ballot, active_mask)};
    ctx.AddU1("{}=({}==0)||({}=={});", inst, value, value, active_mask);
}

void EmitSubgroupBallot(EmitContext& ctx, IR::Inst& inst, std::string_view pred) {
    const auto ballot_index{BallotIndex(ctx)};
    ctx.AddU32("{}=uvec2(ballotARB({})){};", inst, pred, ballot_index);
}

void EmitSubgroupEqMask(EmitContext& ctx, IR::Inst& inst) {
    ctx.AddU32("{}={};", inst, GetMask(ctx, "gl_SubGroupEqMaskARB"));
}

void EmitSubgroupLtMask(EmitContext& ctx, IR::Inst& inst) {
    ctx.AddU32("{}={};", inst, GetMask(ctx, "gl_SubGroupLtMaskARB"));
}

void EmitSubgroupLeMask(EmitContext& ctx, IR::Inst& inst) {
    ctx.AddU32("{}={};", inst, GetMask(ctx, "gl_SubGroupLeMaskARB"));
}

void EmitSubgroupGtMask(EmitContext& ctx, IR::Inst& inst) {
    ctx.AddU32("{}={};", inst, GetMask(ctx, "gl_SubGroupGtMaskARB"));
}

void EmitSubgroupGeMask(EmitContext& ctx, IR::Inst& inst) {
    ctx.AddU32("{}={};", inst, GetMask(ctx, "gl_SubGroupGeMaskARB"));
}

void EmitShuffleIndex(EmitContext& ctx, IR::Inst& inst, std::string_view value,
                      std::string_view index, std::string_view clamp, std::string_view seg_mask) {
    if (ctx.profile.support_gl_warp_intrinsics) {
        UseShuffleNv(ctx, inst, "shuffleNV", value, index, clamp, seg_mask);
        return;
    }
    const bool big_warp{ctx.profile.warp_size_potentially_larger_than_guest};
    const auto is_upper_partition{"int(gl_SubGroupInvocationARB)>=32"};
    const auto upper_index{fmt::format("{}?{}+32:{}", is_upper_partition, index, index)};
    const auto upper_clamp{fmt::format("{}?{}+32:{}", is_upper_partition, clamp, clamp)};

    const auto not_seg_mask{fmt::format("(~{})", seg_mask)};
    const auto min_thread_id{ComputeMinThreadId(THREAD_ID, seg_mask)};
    const auto max_thread_id{
        ComputeMaxThreadId(min_thread_id, big_warp ? upper_clamp : clamp, not_seg_mask)};

    const auto lhs{fmt::format("({}&{})", big_warp ? upper_index : index, not_seg_mask)};
    const auto src_thread_id{fmt::format("({})|({})", lhs, min_thread_id)};
    ctx.Add("shfl_in_bounds=int({})<=int({});", src_thread_id, max_thread_id);
    SetInBoundsFlag(ctx, inst);
    ctx.AddU32("{}=shfl_in_bounds?readInvocationARB({},{}):{};", inst, value, src_thread_id, value);
}

void EmitShuffleUp(EmitContext& ctx, IR::Inst& inst, std::string_view value, std::string_view index,
                   std::string_view clamp, std::string_view seg_mask) {
    if (ctx.profile.support_gl_warp_intrinsics) {
        UseShuffleNv(ctx, inst, "shuffleUpNV", value, index, clamp, seg_mask);
        return;
    }
    const bool big_warp{ctx.profile.warp_size_potentially_larger_than_guest};
    const auto is_upper_partition{"int(gl_SubGroupInvocationARB)>=32"};
    const auto upper_clamp{fmt::format("{}?{}+32:{}", is_upper_partition, clamp, clamp)};

    const auto max_thread_id{GetMaxThreadId(THREAD_ID, big_warp ? upper_clamp : clamp, seg_mask)};
    const auto src_thread_id{fmt::format("({}-{})", THREAD_ID, index)};
    ctx.Add("shfl_in_bounds=int({})>=int({});", src_thread_id, max_thread_id);
    SetInBoundsFlag(ctx, inst);
    ctx.AddU32("{}=shfl_in_bounds?readInvocationARB({},{}):{};", inst, value, src_thread_id, value);
}

void EmitShuffleDown(EmitContext& ctx, IR::Inst& inst, std::string_view value,
                     std::string_view index, std::string_view clamp, std::string_view seg_mask) {
    if (ctx.profile.support_gl_warp_intrinsics) {
        UseShuffleNv(ctx, inst, "shuffleDownNV", value, index, clamp, seg_mask);
        return;
    }
    const bool big_warp{ctx.profile.warp_size_potentially_larger_than_guest};
    const auto is_upper_partition{"int(gl_SubGroupInvocationARB)>=32"};
    const auto upper_clamp{fmt::format("{}?{}+32:{}", is_upper_partition, clamp, clamp)};

    const auto max_thread_id{GetMaxThreadId(THREAD_ID, big_warp ? upper_clamp : clamp, seg_mask)};
    const auto src_thread_id{fmt::format("({}+{})", THREAD_ID, index)};
    ctx.Add("shfl_in_bounds=int({})<=int({});", src_thread_id, max_thread_id);
    SetInBoundsFlag(ctx, inst);
    ctx.AddU32("{}=shfl_in_bounds?readInvocationARB({},{}):{};", inst, value, src_thread_id, value);
}

void EmitShuffleButterfly(EmitContext& ctx, IR::Inst& inst, std::string_view value,
                          std::string_view index, std::string_view clamp,
                          std::string_view seg_mask) {
    if (ctx.profile.support_gl_warp_intrinsics) {
        UseShuffleNv(ctx, inst, "shuffleXorNV", value, index, clamp, seg_mask);
        return;
    }
    const bool big_warp{ctx.profile.warp_size_potentially_larger_than_guest};
    const auto is_upper_partition{"int(gl_SubGroupInvocationARB)>=32"};
    const auto upper_clamp{fmt::format("{}?{}+32:{}", is_upper_partition, clamp, clamp)};

    const auto max_thread_id{GetMaxThreadId(THREAD_ID, big_warp ? upper_clamp : clamp, seg_mask)};
    const auto src_thread_id{fmt::format("({}^{})", THREAD_ID, index)};
    ctx.Add("shfl_in_bounds=int({})<=int({});", src_thread_id, max_thread_id);
    SetInBoundsFlag(ctx, inst);
    ctx.AddU32("{}=shfl_in_bounds?readInvocationARB({},{}):{};", inst, value, src_thread_id, value);
}

void EmitFSwizzleAdd(EmitContext& ctx, IR::Inst& inst, std::string_view op_a, std::string_view op_b,
                     std::string_view swizzle) {
    const auto mask{fmt::format("({}>>((gl_SubGroupInvocationARB&3)<<1))&3", swizzle)};
    const std::string modifier_a = fmt::format("FSWZ_A[{}]", mask);
    const std::string modifier_b = fmt::format("FSWZ_B[{}]", mask);
    ctx.AddF32("{}=({}*{})+({}*{});", inst, op_a, modifier_a, op_b, modifier_b);
}

void EmitDPdxFine(EmitContext& ctx, IR::Inst& inst, std::string_view op_a) {
    if (ctx.profile.support_gl_derivative_control) {
        ctx.AddF32("{}=dFdxFine({});", inst, op_a);
    } else {
        LOG_WARNING(Shader_GLSL, "Device does not support dFdxFine, fallback to dFdx");
        ctx.AddF32("{}=dFdx({});", inst, op_a);
    }
}

void EmitDPdyFine(EmitContext& ctx, IR::Inst& inst, std::string_view op_a) {
    if (ctx.profile.support_gl_derivative_control) {
        ctx.AddF32("{}=dFdyFine({});", inst, op_a);
    } else {
        LOG_WARNING(Shader_GLSL, "Device does not support dFdyFine, fallback to dFdy");
        ctx.AddF32("{}=dFdy({});", inst, op_a);
    }
}

void EmitDPdxCoarse(EmitContext& ctx, IR::Inst& inst, std::string_view op_a) {
    if (ctx.profile.support_gl_derivative_control) {
        ctx.AddF32("{}=dFdxCoarse({});", inst, op_a);
    } else {
        LOG_WARNING(Shader_GLSL, "Device does not support dFdxCoarse, fallback to dFdx");
        ctx.AddF32("{}=dFdx({});", inst, op_a);
    }
}

void EmitDPdyCoarse(EmitContext& ctx, IR::Inst& inst, std::string_view op_a) {
    if (ctx.profile.support_gl_derivative_control) {
        ctx.AddF32("{}=dFdyCoarse({});", inst, op_a);
    } else {
        LOG_WARNING(Shader_GLSL, "Device does not support dFdyCoarse, fallback to dFdy");
        ctx.AddF32("{}=dFdy({});", inst, op_a);
    }
}
} // namespace Shader::Backend::GLSL
