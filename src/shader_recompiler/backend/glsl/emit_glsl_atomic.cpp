// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string_view>

#include "shader_recompiler/backend/glsl/emit_glsl_instructions.h"
#include "shader_recompiler/backend/glsl/glsl_emit_context.h"
#include "shader_recompiler/frontend/ir/value.h"

namespace Shader::Backend::GLSL {
namespace {
constexpr char cas_loop[]{
    "for (;;){{uint old={};{}=atomicCompSwap({},old,{}({},{}));if({}==old){{break;}}}}"};

void SharedCasFunction(EmitContext& ctx, IR::Inst& inst, std::string_view offset,
                       std::string_view value, std::string_view function) {
    const auto ret{ctx.var_alloc.Define(inst, GlslVarType::U32)};
    const std::string smem{fmt::format("smem[{}>>2]", offset)};
    ctx.Add(cas_loop, smem, ret, smem, function, smem, value, ret);
}

void SsboCasFunction(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                     const IR::Value& offset, std::string_view value, std::string_view function) {
    const auto ret{ctx.var_alloc.Define(inst, GlslVarType::U32)};
    const std::string ssbo{fmt::format("{}_ssbo{}[{}>>2]", ctx.stage_name, binding.U32(),
                                       ctx.var_alloc.Consume(offset))};
    ctx.Add(cas_loop, ssbo, ret, ssbo, function, ssbo, value, ret);
}

void SsboCasFunctionF32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                        const IR::Value& offset, std::string_view value,
                        std::string_view function) {
    const std::string ssbo{fmt::format("{}_ssbo{}[{}>>2]", ctx.stage_name, binding.U32(),
                                       ctx.var_alloc.Consume(offset))};
    const auto ret{ctx.var_alloc.Define(inst, GlslVarType::U32)};
    ctx.Add(cas_loop, ssbo, ret, ssbo, function, ssbo, value, ret);
    ctx.AddF32("{}=utof({});", inst, ret);
}
} // Anonymous namespace

void EmitSharedAtomicIAdd32(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                            std::string_view value) {
    ctx.AddU32("{}=atomicAdd(smem[{}>>2],{});", inst, pointer_offset, value);
}

void EmitSharedAtomicSMin32(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                            std::string_view value) {
    const std::string u32_value{fmt::format("uint({})", value)};
    SharedCasFunction(ctx, inst, pointer_offset, u32_value, "CasMinS32");
}

void EmitSharedAtomicUMin32(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                            std::string_view value) {
    ctx.AddU32("{}=atomicMin(smem[{}>>2],{});", inst, pointer_offset, value);
}

void EmitSharedAtomicSMax32(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                            std::string_view value) {
    const std::string u32_value{fmt::format("uint({})", value)};
    SharedCasFunction(ctx, inst, pointer_offset, u32_value, "CasMaxS32");
}

void EmitSharedAtomicUMax32(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                            std::string_view value) {
    ctx.AddU32("{}=atomicMax(smem[{}>>2],{});", inst, pointer_offset, value);
}

void EmitSharedAtomicInc32(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                           std::string_view value) {
    SharedCasFunction(ctx, inst, pointer_offset, value, "CasIncrement");
}

void EmitSharedAtomicDec32(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                           std::string_view value) {
    SharedCasFunction(ctx, inst, pointer_offset, value, "CasDecrement");
}

void EmitSharedAtomicAnd32(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                           std::string_view value) {
    ctx.AddU32("{}=atomicAnd(smem[{}>>2],{});", inst, pointer_offset, value);
}

void EmitSharedAtomicOr32(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                          std::string_view value) {
    ctx.AddU32("{}=atomicOr(smem[{}>>2],{});", inst, pointer_offset, value);
}

void EmitSharedAtomicXor32(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                           std::string_view value) {
    ctx.AddU32("{}=atomicXor(smem[{}>>2],{});", inst, pointer_offset, value);
}

void EmitSharedAtomicExchange32(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                                std::string_view value) {
    ctx.AddU32("{}=atomicExchange(smem[{}>>2],{});", inst, pointer_offset, value);
}

void EmitSharedAtomicExchange64(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                                std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU64("{}=packUint2x32(uvec2(smem[{}>>2],smem[({}+4)>>2]));", inst, pointer_offset,
               pointer_offset);
    ctx.Add("smem[{}>>2]=unpackUint2x32({}).x;smem[({}+4)>>2]=unpackUint2x32({}).y;",
            pointer_offset, value, pointer_offset, value);
}

void EmitSharedAtomicExchange32x2(EmitContext& ctx, IR::Inst& inst, std::string_view pointer_offset,
                                  std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU32x2("{}=uvec2(smem[{}>>2],smem[({}+4)>>2]);", inst, pointer_offset, pointer_offset);
    ctx.Add("smem[{}>>2]={}.x;smem[({}+4)>>2]={}.y;", pointer_offset, value, pointer_offset, value);
}

void EmitStorageAtomicIAdd32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    ctx.AddU32("{}=atomicAdd({}_ssbo{}[{}>>2],{});", inst, ctx.stage_name, binding.U32(),
               ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicSMin32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    const std::string u32_value{fmt::format("uint({})", value)};
    SsboCasFunction(ctx, inst, binding, offset, u32_value, "CasMinS32");
}

void EmitStorageAtomicUMin32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    ctx.AddU32("{}=atomicMin({}_ssbo{}[{}>>2],{});", inst, ctx.stage_name, binding.U32(),
               ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicSMax32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    const std::string u32_value{fmt::format("uint({})", value)};
    SsboCasFunction(ctx, inst, binding, offset, u32_value, "CasMaxS32");
}

void EmitStorageAtomicUMax32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    ctx.AddU32("{}=atomicMax({}_ssbo{}[{}>>2],{});", inst, ctx.stage_name, binding.U32(),
               ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicInc32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                            const IR::Value& offset, std::string_view value) {
    SsboCasFunction(ctx, inst, binding, offset, value, "CasIncrement");
}

void EmitStorageAtomicDec32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                            const IR::Value& offset, std::string_view value) {
    SsboCasFunction(ctx, inst, binding, offset, value, "CasDecrement");
}

void EmitStorageAtomicAnd32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                            const IR::Value& offset, std::string_view value) {
    ctx.AddU32("{}=atomicAnd({}_ssbo{}[{}>>2],{});", inst, ctx.stage_name, binding.U32(),
               ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicOr32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                           const IR::Value& offset, std::string_view value) {
    ctx.AddU32("{}=atomicOr({}_ssbo{}[{}>>2],{});", inst, ctx.stage_name, binding.U32(),
               ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicXor32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                            const IR::Value& offset, std::string_view value) {
    ctx.AddU32("{}=atomicXor({}_ssbo{}[{}>>2],{});", inst, ctx.stage_name, binding.U32(),
               ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicExchange32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                                 const IR::Value& offset, std::string_view value) {
    ctx.AddU32("{}=atomicExchange({}_ssbo{}[{}>>2],{});", inst, ctx.stage_name, binding.U32(),
               ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicIAdd64(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU64("{}=packUint2x32(uvec2({}_ssbo{}[{}>>2],{}_ssbo{}[({}>>2)+1]));", inst,
               ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
               binding.U32(), ctx.var_alloc.Consume(offset));
    ctx.Add("{}_ssbo{}[{}>>2]+=unpackUint2x32({}).x;{}_ssbo{}[({}>>2)+1]+=unpackUint2x32({}).y;",
            ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value, ctx.stage_name,
            binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicSMin64(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU64("{}=packInt2x32(ivec2({}_ssbo{}[{}>>2],{}_ssbo{}[({}>>2)+1]));", inst,
               ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
               binding.U32(), ctx.var_alloc.Consume(offset));
    ctx.Add("for(int i=0;i<2;++i){{ "
            "{}_ssbo{}[({}>>2)+i]=uint(min(int({}_ssbo{}[({}>>2)+i]),unpackInt2x32(int64_t({}))[i])"
            ");}}",
            ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
            binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicUMin64(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU64("{}=packUint2x32(uvec2({}_ssbo{}[{}>>2],{}_ssbo{}[({}>>2)+1]));", inst,
               ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
               binding.U32(), ctx.var_alloc.Consume(offset));
    ctx.Add("for(int i=0;i<2;++i){{ "
            "{}_ssbo{}[({}>>2)+i]=min({}_ssbo{}[({}>>2)+i],unpackUint2x32(uint64_t({}))[i]);}}",
            ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
            binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicSMax64(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU64("{}=packInt2x32(ivec2({}_ssbo{}[{}>>2],{}_ssbo{}[({}>>2)+1]));", inst,
               ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
               binding.U32(), ctx.var_alloc.Consume(offset));
    ctx.Add("for(int i=0;i<2;++i){{ "
            "{}_ssbo{}[({}>>2)+i]=uint(max(int({}_ssbo{}[({}>>2)+i]),unpackInt2x32(int64_t({}))[i])"
            ");}}",
            ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
            binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicUMax64(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU64("{}=packUint2x32(uvec2({}_ssbo{}[{}>>2],{}_ssbo{}[({}>>2)+1]));", inst,
               ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
               binding.U32(), ctx.var_alloc.Consume(offset));
    ctx.Add("for(int "
            "i=0;i<2;++i){{{}_ssbo{}[({}>>2)+i]=max({}_ssbo{}[({}>>2)+i],unpackUint2x32(uint64_t({}"
            "))[i]);}}",
            ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
            binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicAnd64(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                            const IR::Value& offset, std::string_view value) {
    ctx.AddU64(
        "{}=packUint2x32(uvec2(atomicAnd({}_ssbo{}[{}>>2],unpackUint2x32({}).x),atomicAnd({}_"
        "ssbo{}[({}>>2)+1],unpackUint2x32({}).y)));",
        inst, ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value, ctx.stage_name,
        binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicOr64(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                           const IR::Value& offset, std::string_view value) {
    ctx.AddU64("{}=packUint2x32(uvec2(atomicOr({}_ssbo{}[{}>>2],unpackUint2x32({}).x),atomicOr({}_"
               "ssbo{}[({}>>2)+1],unpackUint2x32({}).y)));",
               inst, ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value,
               ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicXor64(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                            const IR::Value& offset, std::string_view value) {
    ctx.AddU64(
        "{}=packUint2x32(uvec2(atomicXor({}_ssbo{}[{}>>2],unpackUint2x32({}).x),atomicXor({}_"
        "ssbo{}[({}>>2)+1],unpackUint2x32({}).y)));",
        inst, ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value, ctx.stage_name,
        binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicExchange64(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                                 const IR::Value& offset, std::string_view value) {
    ctx.AddU64("{}=packUint2x32(uvec2(atomicExchange({}_ssbo{}[{}>>2],unpackUint2x32({}).x),"
               "atomicExchange({}_ssbo{}[({}>>2)+1],unpackUint2x32({}).y)));",
               inst, ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value,
               ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicIAdd32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                               const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU32x2("{}=uvec2({}_ssbo{}[{}>>2],{}_ssbo{}[({}>>2)+1]);", inst, ctx.stage_name,
                 binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name, binding.U32(),
                 ctx.var_alloc.Consume(offset));
    ctx.Add("{}_ssbo{}[{}>>2]+={}.x;{}_ssbo{}[({}>>2)+1]+={}.y;", ctx.stage_name, binding.U32(),
            ctx.var_alloc.Consume(offset), value, ctx.stage_name, binding.U32(),
            ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicSMin32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                               const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU32x2("{}=ivec2({}_ssbo{}[{}>>2],{}_ssbo{}[({}>>2)+1]);", inst, ctx.stage_name,
                 binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name, binding.U32(),
                 ctx.var_alloc.Consume(offset));
    ctx.Add("for(int "
            "i=0;i<2;++i){{{}_ssbo{}[({}>>2)+i]=uint(min(int({}_ssbo{}[({}>>2)+i]),int({}[i])));}}",
            ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
            binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicUMin32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                               const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU32x2("{}=uvec2({}_ssbo{}[{}>>2],{}_ssbo{}[({}>>2)+1]);", inst, ctx.stage_name,
                 binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name, binding.U32(),
                 ctx.var_alloc.Consume(offset));
    ctx.Add("for(int i=0;i<2;++i){{ "
            "{}_ssbo{}[({}>>2)+i]=min({}_ssbo{}[({}>>2)+i],{}[i]);}}",
            ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
            binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicSMax32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                               const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU32x2("{}=ivec2({}_ssbo{}[{}>>2],{}_ssbo{}[({}>>2)+1]);", inst, ctx.stage_name,
                 binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name, binding.U32(),
                 ctx.var_alloc.Consume(offset));
    ctx.Add("for(int "
            "i=0;i<2;++i){{{}_ssbo{}[({}>>2)+i]=uint(max(int({}_ssbo{}[({}>>2)+i]),int({}[i])));}}",
            ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
            binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicUMax32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                               const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to non-atomic");
    ctx.AddU32x2("{}=uvec2({}_ssbo{}[{}>>2],{}_ssbo{}[({}>>2)+1]);", inst, ctx.stage_name,
                 binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name, binding.U32(),
                 ctx.var_alloc.Consume(offset));
    ctx.Add("for(int i=0;i<2;++i){{{}_ssbo{}[({}>>2)+i]=max({}_ssbo{}[({}>>2)+i],{}[i]);}}",
            ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), ctx.stage_name,
            binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicAnd32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                              const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to 32x2");
    ctx.AddU32x2("{}=uvec2(atomicAnd({}_ssbo{}[{}>>2],{}.x),atomicAnd({}_ssbo{}[({}>>2)+1],{}.y));",
                 inst, ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value,
                 ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicOr32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to 32x2");
    ctx.AddU32x2("{}=uvec2(atomicOr({}_ssbo{}[{}>>2],{}.x),atomicOr({}_ssbo{}[({}>>2)+1],{}.y));",
                 inst, ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value,
                 ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicXor32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                              const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to 32x2");
    ctx.AddU32x2("{}=uvec2(atomicXor({}_ssbo{}[{}>>2],{}.x),atomicXor({}_ssbo{}[({}>>2)+1],{}.y));",
                 inst, ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value,
                 ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicExchange32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                                   const IR::Value& offset, std::string_view value) {
    LOG_WARNING(Shader_GLSL, "Int64 atomics not supported, fallback to 32x2");
    ctx.AddU32x2("{}=uvec2(atomicExchange({}_ssbo{}[{}>>2],{}.x),atomicExchange({}_ssbo{}[({}>>2)+"
                 "1],{}.y));",
                 inst, ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value,
                 ctx.stage_name, binding.U32(), ctx.var_alloc.Consume(offset), value);
}

void EmitStorageAtomicAddF32(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                             const IR::Value& offset, std::string_view value) {
    SsboCasFunctionF32(ctx, inst, binding, offset, value, "CasFloatAdd");
}

void EmitStorageAtomicAddF16x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                               const IR::Value& offset, std::string_view value) {
    SsboCasFunction(ctx, inst, binding, offset, value, "CasFloatAdd16x2");
}

void EmitStorageAtomicAddF32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                               const IR::Value& offset, std::string_view value) {
    SsboCasFunction(ctx, inst, binding, offset, value, "CasFloatAdd32x2");
}

void EmitStorageAtomicMinF16x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                               const IR::Value& offset, std::string_view value) {
    SsboCasFunction(ctx, inst, binding, offset, value, "CasFloatMin16x2");
}

void EmitStorageAtomicMinF32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                               const IR::Value& offset, std::string_view value) {
    SsboCasFunction(ctx, inst, binding, offset, value, "CasFloatMin32x2");
}

void EmitStorageAtomicMaxF16x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                               const IR::Value& offset, std::string_view value) {
    SsboCasFunction(ctx, inst, binding, offset, value, "CasFloatMax16x2");
}

void EmitStorageAtomicMaxF32x2(EmitContext& ctx, IR::Inst& inst, const IR::Value& binding,
                               const IR::Value& offset, std::string_view value) {
    SsboCasFunction(ctx, inst, binding, offset, value, "CasFloatMax32x2");
}

void EmitGlobalAtomicIAdd32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicSMin32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicUMin32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicSMax32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicUMax32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicInc32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicDec32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicAnd32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicOr32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicXor32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicExchange32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicIAdd64(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicSMin64(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicUMin64(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicSMax64(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicUMax64(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicInc64(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicDec64(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicAnd64(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicOr64(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicXor64(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicExchange64(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicIAdd32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicSMin32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicUMin32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicSMax32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicUMax32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicInc32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicDec32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicAnd32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicOr32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicXor32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicExchange32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicAddF32(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicAddF16x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicAddF32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicMinF16x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicMinF32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicMaxF16x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}

void EmitGlobalAtomicMaxF32x2(EmitContext&) {
    throw NotImplementedException("GLSL Instruction");
}
} // namespace Shader::Backend::GLSL
