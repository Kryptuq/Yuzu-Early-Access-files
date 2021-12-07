// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "shader_recompiler/backend/glasm/emit_glasm_instructions.h"
#include "shader_recompiler/backend/glasm/glasm_emit_context.h"
#include "shader_recompiler/frontend/ir/value.h"

namespace Shader::Backend::GLASM {

static void DefinePhi(EmitContext& ctx, IR::Inst& phi) {
    switch (phi.Type()) {
    case IR::Type::U1:
    case IR::Type::U32:
    case IR::Type::F32:
        ctx.reg_alloc.Define(phi);
        break;
    case IR::Type::U64:
    case IR::Type::F64:
        ctx.reg_alloc.LongDefine(phi);
        break;
    default:
        throw NotImplementedException("Phi node type {}", phi.Type());
    }
}

void EmitPhi(EmitContext& ctx, IR::Inst& phi) {
    const size_t num_args{phi.NumArgs()};
    for (size_t i = 0; i < num_args; ++i) {
        ctx.reg_alloc.Consume(phi.Arg(i));
    }
    if (!phi.Definition<Id>().is_valid) {
        // The phi node wasn't forward defined
        DefinePhi(ctx, phi);
    }
}

void EmitVoid(EmitContext&) {}

void EmitReference(EmitContext& ctx, const IR::Value& value) {
    ctx.reg_alloc.Consume(value);
}

void EmitPhiMove(EmitContext& ctx, const IR::Value& phi_value, const IR::Value& value) {
    IR::Inst& phi{RegAlloc::AliasInst(*phi_value.Inst())};
    if (!phi.Definition<Id>().is_valid) {
        // The phi node wasn't forward defined
        DefinePhi(ctx, phi);
    }
    const Register phi_reg{ctx.reg_alloc.Consume(IR::Value{&phi})};
    const Value eval_value{ctx.reg_alloc.Consume(value)};

    if (phi_reg == eval_value) {
        return;
    }
    switch (phi.Flags<IR::Type>()) {
    case IR::Type::U1:
    case IR::Type::U32:
    case IR::Type::F32:
        ctx.Add("MOV.S {}.x,{};", phi_reg, ScalarS32{eval_value});
        break;
    case IR::Type::U64:
    case IR::Type::F64:
        ctx.Add("MOV.U64 {}.x,{};", phi_reg, ScalarRegister{eval_value});
        break;
    default:
        throw NotImplementedException("Phi node type {}", phi.Type());
    }
}

void EmitPrologue(EmitContext&) {
    // TODO
}

void EmitEpilogue(EmitContext&) {
    // TODO
}

void EmitEmitVertex(EmitContext& ctx, ScalarS32 stream) {
    if (stream.type == Type::U32 && stream.imm_u32 == 0) {
        ctx.Add("EMIT;");
    } else {
        ctx.Add("EMITS {};", stream);
    }
}

void EmitEndPrimitive(EmitContext& ctx, const IR::Value& stream) {
    if (!stream.IsImmediate()) {
        LOG_WARNING(Shader_GLASM, "Stream is not immediate");
    }
    ctx.reg_alloc.Consume(stream);
    ctx.Add("ENDPRIM;");
}

} // namespace Shader::Backend::GLASM
