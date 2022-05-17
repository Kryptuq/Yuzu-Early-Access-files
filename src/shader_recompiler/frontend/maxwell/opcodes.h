// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <fmt/format.h>

namespace Shader::Maxwell {

enum class Opcode {
#define INST(name, cute, encode) name,
#include "maxwell.inc"
#undef INST
};

const char* NameOf(Opcode opcode);

} // namespace Shader::Maxwell

template <>
struct fmt::formatter<Shader::Maxwell::Opcode> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(const Shader::Maxwell::Opcode& opcode, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", NameOf(opcode));
    }
};
