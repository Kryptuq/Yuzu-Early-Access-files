// SPDX-FileCopyrightText: 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>

#include <dynarmic/interface/A32/coprocessor.h>
#include "common/common_types.h"

namespace Core {

class ARM_Dynarmic_32;

class DynarmicCP15 final : public Dynarmic::A32::Coprocessor {
public:
    using CoprocReg = Dynarmic::A32::CoprocReg;

    explicit DynarmicCP15(ARM_Dynarmic_32& parent_) : parent{parent_} {}

    std::optional<Callback> CompileInternalOperation(bool two, unsigned opc1, CoprocReg CRd,
                                                     CoprocReg CRn, CoprocReg CRm,
                                                     unsigned opc2) override;
    CallbackOrAccessOneWord CompileSendOneWord(bool two, unsigned opc1, CoprocReg CRn,
                                               CoprocReg CRm, unsigned opc2) override;
    CallbackOrAccessTwoWords CompileSendTwoWords(bool two, unsigned opc, CoprocReg CRm) override;
    CallbackOrAccessOneWord CompileGetOneWord(bool two, unsigned opc1, CoprocReg CRn, CoprocReg CRm,
                                              unsigned opc2) override;
    CallbackOrAccessTwoWords CompileGetTwoWords(bool two, unsigned opc, CoprocReg CRm) override;
    std::optional<Callback> CompileLoadWords(bool two, bool long_transfer, CoprocReg CRd,
                                             std::optional<u8> option) override;
    std::optional<Callback> CompileStoreWords(bool two, bool long_transfer, CoprocReg CRd,
                                              std::optional<u8> option) override;

    ARM_Dynarmic_32& parent;
    u32 uprw = 0;
    u32 uro = 0;

    friend class ARM_Dynarmic_32;
};

} // namespace Core
