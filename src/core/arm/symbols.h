// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>
#include <utility>

#include "common/common_types.h"

namespace Core::Memory {
class Memory;
} // namespace Core::Memory

namespace Core::Symbols {

using Symbols = std::map<std::string, std::pair<VAddr, std::size_t>, std::less<>>;

Symbols GetSymbols(VAddr base, Core::Memory::Memory& memory, bool is_64 = true);
Symbols GetSymbols(std::span<const u8> data, bool is_64 = true);
std::optional<std::string> GetSymbolName(const Symbols& symbols, VAddr addr);

} // namespace Core::Symbols
