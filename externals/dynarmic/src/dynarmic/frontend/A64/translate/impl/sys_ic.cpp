/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::IC_IALLU() {
    ir.InstructionCacheOperationRaised(InstructionCacheOperation::InvalidateAllToPoU, ir.Imm64(0));
    ir.SetPC(ir.Imm64(ir.current_location->PC() + 4));
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
    return false;
}

bool TranslatorVisitor::IC_IALLUIS() {
    ir.InstructionCacheOperationRaised(InstructionCacheOperation::InvalidateAllToPoUInnerSharable, ir.Imm64(0));
    ir.SetPC(ir.Imm64(ir.current_location->PC() + 4));
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
    return false;
}

bool TranslatorVisitor::IC_IVAU(Reg Rt) {
    ir.InstructionCacheOperationRaised(InstructionCacheOperation::InvalidateByVAToPoU, X(64, Rt));
    ir.SetPC(ir.Imm64(ir.current_location->PC() + 4));
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
    return false;
}

}  // namespace Dynarmic::A64
