/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/x64_disassemble.h"

#include <Zydis/Zydis.h>
#include <fmt/printf.h>
#include <mcl/stdint.hpp>

namespace Dynarmic::Common {

void DumpDisassembledX64(const void* ptr, size_t size) {
    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);

    ZydisFormatter formatter;
    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

    size_t offset = 0;
    ZydisDecodedInstruction instruction;
    while (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, static_cast<const char*>(ptr) + offset, size - offset, &instruction))) {
        fmt::print("{:016x}  ", (u64)ptr + offset);

        char buffer[256];
        ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), reinterpret_cast<u64>(ptr) + offset);
        puts(buffer);

        offset += instruction.length;
    }
}

std::vector<std::string> DisassembleX64(const void* ptr, size_t size) {
    std::vector<std::string> result;
    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);

    ZydisFormatter formatter;
    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

    size_t offset = 0;
    ZydisDecodedInstruction instruction;
    while (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, static_cast<const char*>(ptr) + offset, size - offset, &instruction))) {
        char buffer[256];
        ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), reinterpret_cast<u64>(ptr) + offset);

        result.push_back(fmt::format("{:016x}  {}", (u64)ptr + offset, buffer));

        offset += instruction.length;
    }

    return result;
}
}  // namespace Dynarmic::Common
