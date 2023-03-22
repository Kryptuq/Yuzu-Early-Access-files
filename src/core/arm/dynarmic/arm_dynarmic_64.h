// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>

#include <dynarmic/interface/A64/a64.h>
#include "common/common_types.h"
#include "common/hash.h"
#include "core/arm/arm_interface.h"
#include "core/arm/exclusive_monitor.h"

namespace Core::Memory {
class Memory;
}

namespace Core {

class DynarmicCallbacks64;
class DynarmicExclusiveMonitor;
class System;

class ARM_Dynarmic_64 final : public ARM_Interface {
public:
    ARM_Dynarmic_64(System& system_, bool uses_wall_clock_, ExclusiveMonitor& exclusive_monitor_,
                    std::size_t core_index_);
    ~ARM_Dynarmic_64() override;

    void SetPC(u64 pc) override;
    u64 GetPC() const override;
    u64 GetSP() const override;
    u64 GetReg(int index) const override;
    void SetReg(int index, u64 value) override;
    u128 GetVectorReg(int index) const override;
    void SetVectorReg(int index, u128 value) override;
    u32 GetPSTATE() const override;
    void SetPSTATE(u32 pstate) override;
    u64 GetTlsAddress() const override;
    void SetTlsAddress(u64 address) override;
    void SetTPIDR_EL0(u64 value) override;
    u64 GetTPIDR_EL0() const override;

    void SaveContext(ThreadContext32& ctx) override {}
    void SaveContext(ThreadContext64& ctx) override;
    void LoadContext(const ThreadContext32& ctx) override {}
    void LoadContext(const ThreadContext64& ctx) override;

    void SignalInterrupt() override;
    void ClearInterrupt() override;
    void ClearExclusiveState() override;

    void ClearInstructionCache() override;
    void InvalidateCacheRange(u64 addr, std::size_t size) override;
    void PageTableChanged(Common::PageTable& new_page_table,
                          std::size_t new_address_space_size_in_bits) override;

    static std::vector<BacktraceEntry> GetBacktraceFromContext(System& system,
                                                               const ThreadContext64& ctx);

    std::vector<BacktraceEntry> GetBacktrace() const override;

protected:
    Dynarmic::HaltReason RunJit() override;
    Dynarmic::HaltReason StepJit() override;
    u32 GetSvcNumber() const override;
    const Kernel::DebugWatchpoint* HaltedWatchpoint() const override;
    void RewindBreakpointInstruction() override;

private:
    std::shared_ptr<Dynarmic::A64::Jit> MakeJit(Common::PageTable* page_table,
                                                std::size_t address_space_bits) const;

    static std::vector<BacktraceEntry> GetBacktrace(Core::System& system, u64 fp, u64 lr, u64 pc);

    using JitCacheKey = std::pair<Common::PageTable*, std::size_t>;
    using JitCacheType =
        std::unordered_map<JitCacheKey, std::shared_ptr<Dynarmic::A64::Jit>, Common::PairHash>;

    friend class DynarmicCallbacks64;
    std::unique_ptr<DynarmicCallbacks64> cb;
    JitCacheType jit_cache;

    std::size_t core_index;
    DynarmicExclusiveMonitor& exclusive_monitor;

    std::shared_ptr<Dynarmic::A64::Jit> null_jit;

    // A raw pointer here is fine; we never delete Jit instances.
    std::atomic<Dynarmic::A64::Jit*> jit;

    // SVC callback
    u32 svc_swi{};

    // Breakpoint info
    const Kernel::DebugWatchpoint* halted_watchpoint;
    ThreadContext64 breakpoint_context;
};

} // namespace Core
