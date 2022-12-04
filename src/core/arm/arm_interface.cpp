// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _MSC_VER
#include <cxxabi.h>
#endif

#include <map>
#include <optional>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/arm/arm_interface.h"
#include "core/arm/symbols.h"
#include "core/core.h"
#include "core/debugger/debugger.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/svc.h"
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
#ifdef _MSC_VER
                // TODO(DarkLordZach): Add demangling of symbol names.
                entry.name = *symbol;
#else
                int status{-1};
                char* demangled{abi::__cxa_demangle(symbol->c_str(), nullptr, nullptr, &status)};
                if (status == 0 && demangled != nullptr) {
                    entry.name = demangled;
                    std::free(demangled);
                } else {
                    entry.name = *symbol;
                }
#endif
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

void ARM_Interface::Run() {
    using Kernel::StepState;
    using Kernel::SuspendType;

    while (true) {
        Kernel::KThread* current_thread{Kernel::GetCurrentThreadPointer(system.Kernel())};
        Dynarmic::HaltReason hr{};

        // Notify the debugger and go to sleep if a step was performed
        // and this thread has been scheduled again.
        if (current_thread->GetStepState() == StepState::StepPerformed) {
            system.GetDebugger().NotifyThreadStopped(current_thread);
            current_thread->RequestSuspend(SuspendType::Debug);
            break;
        }

        // Otherwise, run the thread.
        system.EnterDynarmicProfile();
        if (current_thread->GetStepState() == StepState::StepPending) {
            hr = StepJit();

            if (Has(hr, step_thread)) {
                current_thread->SetStepState(StepState::StepPerformed);
            }
        } else {
            hr = RunJit();
        }
        system.ExitDynarmicProfile();

        // If the thread is scheduled for termination, exit the thread.
        if (current_thread->HasDpc()) {
            if (current_thread->IsTerminationRequested()) {
                current_thread->Exit();
                UNREACHABLE();
            }
        }

        // Notify the debugger and go to sleep if a breakpoint was hit,
        // or if the thread is unable to continue for any reason.
        if (Has(hr, breakpoint) || Has(hr, no_execute)) {
            if (!Has(hr, no_execute)) {
                RewindBreakpointInstruction();
            }
            if (system.DebuggerEnabled()) {
                system.GetDebugger().NotifyThreadStopped(current_thread);
            } else {
                LogBacktrace();
            }
            current_thread->RequestSuspend(SuspendType::Debug);
            break;
        }

        // Notify the debugger and go to sleep if a watchpoint was hit.
        if (Has(hr, watchpoint)) {
            if (system.DebuggerEnabled()) {
                system.GetDebugger().NotifyThreadWatchpoint(current_thread, *HaltedWatchpoint());
            }
            current_thread->RequestSuspend(SuspendType::Debug);
            break;
        }

        // Handle syscalls and scheduling (this may change the current thread/core)
        if (Has(hr, svc_call)) {
            Kernel::Svc::Call(system, GetSvcNumber());
            break;
        }
        if (Has(hr, break_loop) || !uses_wall_clock) {
            break;
        }
    }
}

void ARM_Interface::LoadWatchpointArray(const WatchpointArray& wp) {
    watchpoints = &wp;
}

const Kernel::DebugWatchpoint* ARM_Interface::MatchingWatchpoint(
    VAddr addr, u64 size, Kernel::DebugWatchpointType access_type) const {
    if (!watchpoints) {
        return nullptr;
    }

    const VAddr start_address{addr};
    const VAddr end_address{addr + size};

    for (size_t i = 0; i < Core::Hardware::NUM_WATCHPOINTS; i++) {
        const auto& watch{(*watchpoints)[i]};

        if (end_address <= watch.start_address) {
            continue;
        }
        if (start_address >= watch.end_address) {
            continue;
        }
        if ((access_type & watch.type) == Kernel::DebugWatchpointType::None) {
            continue;
        }

        return &watch;
    }

    return nullptr;
}

} // namespace Core
