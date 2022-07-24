// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"

namespace FileSys {

enum class Mode : u32 {
    Read = 1 << 0,
    Write = 1 << 1,
    ReadWrite = Read | Write,
    Append = 1 << 2,
    ReadAppend = Read | Append,
    WriteAppend = Write | Append,
    All = ReadWrite | Append,
};

DECLARE_ENUM_FLAG_OPERATORS(Mode)

} // namespace FileSys
