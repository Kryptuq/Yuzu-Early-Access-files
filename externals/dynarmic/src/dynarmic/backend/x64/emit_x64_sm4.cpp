/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/emit_x64.h"
#include "dynarmic/common/crypto/sm4.h"
#include "dynarmic/ir/microinstruction.h"

namespace Dynarmic::Backend::X64 {

void EmitX64::EmitSM4AccessSubstitutionBox(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ctx.reg_alloc.HostCall(inst, args[0]);
    code.CallFunction(&Common::Crypto::SM4::AccessSubstitutionBox);
    code.movzx(code.ABI_RETURN.cvt32(), code.ABI_RETURN.cvt8());
}

}  // namespace Dynarmic::Backend::X64
