// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/elf.h"
#include "core/arm/symbols.h"
#include "core/core.h"
#include "core/memory.h"

using namespace Common::ELF;

namespace Core {
namespace Symbols {

template <typename Word, typename ELFSymbol, typename ByteReader>
static Symbols GetSymbols(ByteReader ReadBytes) {
    const auto Read8{[&](u64 index) {
        u8 ret;
        ReadBytes(&ret, index, sizeof(u8));
        return ret;
    }};

    const auto Read32{[&](u64 index) {
        u32 ret;
        ReadBytes(&ret, index, sizeof(u32));
        return ret;
    }};

    const auto ReadWord{[&](u64 index) {
        Word ret;
        ReadBytes(&ret, index, sizeof(Word));
        return ret;
    }};

    const u32 mod_offset = Read32(4);

    if (Read32(mod_offset) != Common::MakeMagic('M', 'O', 'D', '0')) {
        return {};
    }

    VAddr string_table_offset{};
    VAddr symbol_table_offset{};
    u64 symbol_entry_size{};

    const auto dynamic_offset = Read32(mod_offset + 0x4) + mod_offset;

    VAddr dynamic_index = dynamic_offset;
    while (true) {
        const Word tag = ReadWord(dynamic_index);
        const Word value = ReadWord(dynamic_index + sizeof(Word));
        dynamic_index += 2 * sizeof(Word);

        if (tag == ElfDtNull) {
            break;
        }

        if (tag == ElfDtStrtab) {
            string_table_offset = value;
        } else if (tag == ElfDtSymtab) {
            symbol_table_offset = value;
        } else if (tag == ElfDtSyment) {
            symbol_entry_size = value;
        }
    }

    if (string_table_offset == 0 || symbol_table_offset == 0 || symbol_entry_size == 0) {
        return {};
    }

    Symbols out;

    VAddr symbol_index = symbol_table_offset;
    while (symbol_index < string_table_offset) {
        ELFSymbol symbol{};
        ReadBytes(&symbol, symbol_index, sizeof(ELFSymbol));

        VAddr string_offset = string_table_offset + symbol.st_name;
        std::string name;
        for (u8 c = Read8(string_offset); c != 0; c = Read8(++string_offset)) {
            name += static_cast<char>(c);
        }

        symbol_index += symbol_entry_size;
        out[name] = std::make_pair(symbol.st_value, symbol.st_size);
    }

    return out;
}

Symbols GetSymbols(VAddr base, Core::Memory::Memory& memory, bool is_64) {
    const auto ReadBytes{
        [&](void* ptr, size_t offset, size_t size) { memory.ReadBlock(base + offset, ptr, size); }};

    if (is_64) {
        return GetSymbols<u64, Elf64_Sym>(ReadBytes);
    } else {
        return GetSymbols<u32, Elf32_Sym>(ReadBytes);
    }
}

Symbols GetSymbols(std::span<const u8> data, bool is_64) {
    const auto ReadBytes{[&](void* ptr, size_t offset, size_t size) {
        std::memcpy(ptr, data.data() + offset, size);
    }};

    if (is_64) {
        return GetSymbols<u64, Elf64_Sym>(ReadBytes);
    } else {
        return GetSymbols<u32, Elf32_Sym>(ReadBytes);
    }
}

std::optional<std::string> GetSymbolName(const Symbols& symbols, VAddr addr) {
    const auto iter = std::find_if(symbols.cbegin(), symbols.cend(), [addr](const auto& pair) {
        const auto& [name, sym_info] = pair;
        const auto& [start_address, size] = sym_info;
        const auto end_address = start_address + size;
        return addr >= start_address && addr < end_address;
    });

    if (iter == symbols.cend()) {
        return std::nullopt;
    }

    return iter->first;
}

} // namespace Symbols
} // namespace Core
