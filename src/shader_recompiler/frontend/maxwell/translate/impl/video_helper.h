// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "shader_recompiler/frontend/maxwell/translate/impl/impl.h"

namespace Shader::Maxwell {
enum class VideoWidth : u64 {
    Byte,
    Unknown,
    Short,
    Word,
};

[[nodiscard]] IR::U32 ExtractVideoOperandValue(IR::IREmitter& ir, const IR::U32& value,
                                               VideoWidth width, u32 selector, bool is_signed);

[[nodiscard]] VideoWidth GetVideoSourceWidth(VideoWidth width, bool is_immediate);

} // namespace Shader::Maxwell
