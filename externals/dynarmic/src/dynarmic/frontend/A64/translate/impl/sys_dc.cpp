/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

static bool DataCacheInstruction(TranslatorVisitor& v, DataCacheOperation op, const Reg Rt) {
    v.ir.DataCacheOperationRaised(op, v.X(64, Rt));
    return true;
}

bool TranslatorVisitor::DC_IVAC(Reg Rt) {
    return DataCacheInstruction(*this, DataCacheOperation::InvalidateByVAToPoC, Rt);
}

bool TranslatorVisitor::DC_ISW(Reg Rt) {
    return DataCacheInstruction(*this, DataCacheOperation::InvalidateBySetWay, Rt);
}

bool TranslatorVisitor::DC_CSW(Reg Rt) {
    return DataCacheInstruction(*this, DataCacheOperation::CleanBySetWay, Rt);
}

bool TranslatorVisitor::DC_CISW(Reg Rt) {
    return DataCacheInstruction(*this, DataCacheOperation::CleanAndInvalidateBySetWay, Rt);
}

bool TranslatorVisitor::DC_ZVA(Reg Rt) {
    return DataCacheInstruction(*this, DataCacheOperation::ZeroByVA, Rt);
}

bool TranslatorVisitor::DC_CVAC(Reg Rt) {
    return DataCacheInstruction(*this, DataCacheOperation::CleanByVAToPoC, Rt);
}

bool TranslatorVisitor::DC_CVAU(Reg Rt) {
    return DataCacheInstruction(*this, DataCacheOperation::CleanByVAToPoU, Rt);
}

bool TranslatorVisitor::DC_CVAP(Reg Rt) {
    return DataCacheInstruction(*this, DataCacheOperation::CleanByVAToPoP, Rt);
}

bool TranslatorVisitor::DC_CIVAC(Reg Rt) {
    return DataCacheInstruction(*this, DataCacheOperation::CleanAndInvalidateByVAToPoC, Rt);
}

}  // namespace Dynarmic::A64
