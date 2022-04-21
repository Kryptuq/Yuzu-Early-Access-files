// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <map>
#include <optional>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/arm/arm_interface.h"
#include "core/arm/symbols.h"
#include "core/core.h"
#include "core/hle/kernel/k_process.h"
#include "core/loader/loader.h"
#include "core/memory.h"

#include "core/arm/dynarmic/arm_dynarmic_32.h"
#include "core/arm/dynarmic/arm_dynarmic_64.h"

namespace Core {

constexpr u64 SEGMENT_BASE = 0x7100000000ull;

std::vector<ARM_Interface::BacktraceEntry> ARM_Interface::GetBacktraceFromContext(
    Core::System& system, const ARM_Interface::ThreadContext32& ctx) {
    return ARM_Dynarmic_32::GetBacktraceFromContext(system, ctx);
}

std::vector<ARM_Interface::BacktraceEntry> ARM_Interface::GetBacktraceFromContext(
    Core::System& system, const ARM_Interface::ThreadContext64& ctx) {
    return ARM_Dynarmic_64::GetBacktraceFromContext(system, ctx);
}

void ARM_Interface::SymbolicateBacktrace(Core::System& system, std::vector<BacktraceEntry>& out) {
    std::map<VAddr, std::string> modules;
    auto& loader{system.GetAppLoader()};
    if (loader.ReadNSOModules(modules) != Loader::ResultStatus::Success) {
        return;
    }

    std::map<std::string, Symbols::Symbols> symbols;
    for (const auto& module : modules) {
        symbols.insert_or_assign(module.second,
                                 Symbols::GetSymbols(module.first, system.Memory(),
                                                     system.CurrentProcess()->Is64BitProcess()));
    }

    for (auto& entry : out) {
        VAddr base = 0;
        for (auto iter = modules.rbegin(); iter != modules.rend(); ++iter) {
            const auto& module{*iter};
            if (entry.original_address >= module.first) {
                entry.module = module.second;
                base = module.first;
                break;
            }
        }

        entry.offset = entry.original_address - base;
        entry.address = SEGMENT_BASE + entry.offset;

        if (entry.module.empty()) {
            entry.module = "unknown";
        }

        const auto symbol_set = symbols.find(entry.module);
        if (symbol_set != symbols.end()) {
            const auto symbol = Symbols::GetSymbolName(symbol_set->second, entry.offset);
            if (symbol.has_value()) {
                // TODO(DarkLordZach): Add demangling of symbol names.
                entry.name = *symbol;
            }
        }
    }
}

void ARM_Interface::LogBacktrace() const {
    const VAddr sp = GetSP();
    const VAddr pc = GetPC();
    LOG_ERROR(Core_ARM, "Backtrace, sp={:016X}, pc={:016X}", sp, pc);
    LOG_ERROR(Core_ARM, "{:20}{:20}{:20}{:20}{}", "Module Name", "Address", "Original Address",
              "Offset", "Symbol");
    LOG_ERROR(Core_ARM, "");

    const auto backtrace = GetBacktrace();
    for (const auto& entry : backtrace) {
        LOG_ERROR(Core_ARM, "{:20}{:016X}    {:016X}    {:016X}    {}", entry.module, entry.address,
                  entry.original_address, entry.offset, entry.name);
    }
}

} // namespace Core
