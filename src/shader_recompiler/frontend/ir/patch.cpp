// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/exception.h"
#include "shader_recompiler/frontend/ir/patch.h"

namespace Shader::IR {

bool IsGeneric(Patch patch) noexcept {
    return patch >= Patch::Component0 && patch <= Patch::Component119;
}

u32 GenericPatchIndex(Patch patch) {
    if (!IsGeneric(patch)) {
        throw InvalidArgument("Patch {} is not generic", patch);
    }
    return (static_cast<u32>(patch) - static_cast<u32>(Patch::Component0)) / 4;
}

u32 GenericPatchElement(Patch patch) {
    if (!IsGeneric(patch)) {
        throw InvalidArgument("Patch {} is not generic", patch);
    }
    return (static_cast<u32>(patch) - static_cast<u32>(Patch::Component0)) % 4;
}

} // namespace Shader::IR
