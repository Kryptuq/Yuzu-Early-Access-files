// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cinttypes>
#include <memory>
#include <dynarmic/interface/A32/a32.h>
#include <dynarmic/interface/A32/config.h>
#include <dynarmic/interface/A32/context.h>
#include "common/assert.h"
#include "common/literals.h"
#include "common/logging/log.h"
#include "common/page_table.h"
#include "common/settings.h"
#include "core/arm/dynarmic/arm_dynarmic_32.h"
#include "core/arm/dynarmic/arm_dynarmic_cp15.h"
#include "core/arm/dynarmic/arm_exclusive_monitor.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/debugger/debugger.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/svc.h"
#include "core/memory.h"

namespace Core {

using namespace Common::Literals;

class DynarmicCallbacks32 : public Dynarmic::A32::UserCallbacks {
public:
    explicit DynarmicCallbacks32(ARM_Dynarmic_32& parent_)
        : parent{parent_},
          memory(parent.system.Memory()), debugger_enabled{parent.system.DebuggerEnabled()},
          check_memory_access{debugger_enabled ||
                              !Settings::values.cpuopt_ignore_memory_aborts.GetValue()} {}

    u8 MemoryRead8(u32 vaddr) override {
        CheckMemoryAccess(vaddr, 1, Kernel::DebugWatchpointType::Read);
        return memory.Read8(vaddr);
    }
    u16 MemoryRead16(u32 vaddr) override {
        CheckMemoryAccess(vaddr, 2, Kernel::DebugWatchpointType::Read);
        return memory.Read16(vaddr);
    }
    u32 MemoryRead32(u32 vaddr) override {
        CheckMemoryAccess(vaddr, 4, Kernel::DebugWatchpointType::Read);
        return memory.Read32(vaddr);
    }
    u64 MemoryRead64(u32 vaddr) override {
        CheckMemoryAccess(vaddr, 8, Kernel::DebugWatchpointType::Read);
        return memory.Read64(vaddr);
    }
    std::optional<u32> MemoryReadCode(u32 vaddr) override {
        if (!memory.IsValidVirtualAddressRange(vaddr, sizeof(u32))) {
            return std::nullopt;
        }
        return memory.Read32(vaddr);
    }

    void MemoryWrite8(u32 vaddr, u8 value) override {
        if (CheckMemoryAccess(vaddr, 1, Kernel::DebugWatchpointType::Write)) {
            memory.Write8(vaddr, value);
        }
    }
    void MemoryWrite16(u32 vaddr, u16 value) override {
        if (CheckMemoryAccess(vaddr, 2, Kernel::DebugWatchpointType::Write)) {
            memory.Write16(vaddr, value);
        }
    }
    void MemoryWrite32(u32 vaddr, u32 value) override {
        if (CheckMemoryAccess(vaddr, 4, Kernel::DebugWatchpointType::Write)) {
            memory.Write32(vaddr, value);
        }
    }
    void MemoryWrite64(u32 vaddr, u64 value) override {
        if (CheckMemoryAccess(vaddr, 8, Kernel::DebugWatchpointType::Write)) {
            memory.Write64(vaddr, value);
        }
    }

    bool MemoryWriteExclusive8(u32 vaddr, u8 value, u8 expected) override {
        return CheckMemoryAccess(vaddr, 1, Kernel::DebugWatchpointType::Write) &&
               memory.WriteExclusive8(vaddr, value, expected);
    }
    bool MemoryWriteExclusive16(u32 vaddr, u16 value, u16 expected) override {
        return CheckMemoryAccess(vaddr, 2, Kernel::DebugWatchpointType::Write) &&
               memory.WriteExclusive16(vaddr, value, expected);
    }
    bool MemoryWriteExclusive32(u32 vaddr, u32 value, u32 expected) override {
        return CheckMemoryAccess(vaddr, 4, Kernel::DebugWatchpointType::Write) &&
               memory.WriteExclusive32(vaddr, value, expected);
    }
    bool MemoryWriteExclusive64(u32 vaddr, u64 value, u64 expected) override {
        return CheckMemoryAccess(vaddr, 8, Kernel::DebugWatchpointType::Write) &&
               memory.WriteExclusive64(vaddr, value, expected);
    }

    void InterpreterFallback(u32 pc, std::size_t num_instructions) override {
        parent.LogBacktrace();
        LOG_ERROR(Core_ARM,
                  "Unimplemented instruction @ 0x{:X} for {} instructions (instr = {:08X})", pc,
                  num_instructions, memory.Read32(pc));
    }

    void ExceptionRaised(u32 pc, Dynarmic::A32::Exception exception) override {
        switch (exception) {
        case Dynarmic::A32::Exception::NoExecuteFault:
            LOG_CRITICAL(Core_ARM, "Cannot execute instruction at unmapped address {:#08x}", pc);
            ReturnException(pc, ARM_Interface::no_execute);
            return;
        default:
            if (debugger_enabled) {
                ReturnException(pc, ARM_Interface::breakpoint);
                return;
            }

            parent.LogBacktrace();
            LOG_CRITICAL(Core_ARM,
                         "ExceptionRaised(exception = {}, pc = {:08X}, code = {:08X}, thumb = {})",
                         exception, pc, memory.Read32(pc), parent.IsInThumbMode());
        }
    }

    void CallSVC(u32 swi) override {
        parent.svc_swi = swi;
        parent.jit.load()->HaltExecution(ARM_Interface::svc_call);
    }

    void AddTicks(u64 ticks) override {
        if (parent.uses_wall_clock) {
            return;
        }

        // Divide the number of ticks by the amount of CPU cores. TODO(Subv): This yields only a
        // rough approximation of the amount of executed ticks in the system, it may be thrown off
        // if not all cores are doing a similar amount of work. Instead of doing this, we should
        // device a way so that timing is consistent across all cores without increasing the ticks 4
        // times.
        u64 amortized_ticks =
            (ticks - num_interpreted_instructions) / Core::Hardware::NUM_CPU_CORES;
        // Always execute at least one tick.
        amortized_ticks = std::max<u64>(amortized_ticks, 1);

        parent.system.CoreTiming().AddTicks(amortized_ticks);
        num_interpreted_instructions = 0;
    }

    u64 GetTicksRemaining() override {
        if (parent.uses_wall_clock) {
            if (!IsInterrupted()) {
                return minimum_run_cycles;
            }
            return 0U;
        }

        return std::max<s64>(parent.system.CoreTiming().GetDowncount(), 0);
    }

    bool CheckMemoryAccess(VAddr addr, u64 size, Kernel::DebugWatchpointType type) {
        if (!check_memory_access) {
            return true;
        }

        if (!memory.IsValidVirtualAddressRange(addr, size)) {
            LOG_CRITICAL(Core_ARM, "Stopping execution due to unmapped memory access at {:#x}",
                         addr);
            parent.jit.load()->HaltExecution(ARM_Interface::no_execute);
            return false;
        }

        if (!debugger_enabled) {
            return true;
        }

        const auto match{parent.MatchingWatchpoint(addr, size, type)};
        if (match) {
            parent.halted_watchpoint = match;
            parent.jit.load()->HaltExecution(ARM_Interface::watchpoint);
            return false;
        }

        return true;
    }

    void ReturnException(u32 pc, Dynarmic::HaltReason hr) {
        parent.SaveContext(parent.breakpoint_context);
        parent.breakpoint_context.cpu_registers[15] = pc;
        parent.jit.load()->HaltExecution(hr);
    }

    bool IsInterrupted() {
        return parent.system.Kernel().PhysicalCore(parent.core_index).IsInterrupted();
    }

    ARM_Dynarmic_32& parent;
    Core::Memory::Memory& memory;
    std::size_t num_interpreted_instructions{};
    const bool debugger_enabled{};
    const bool check_memory_access{};
    static constexpr u64 minimum_run_cycles = 10000U;
};

std::shared_ptr<Dynarmic::A32::Jit> ARM_Dynarmic_32::MakeJit(Common::PageTable* page_table) const {
    Dynarmic::A32::UserConfig config;
    config.callbacks = cb.get();
    config.coprocessors[15] = cp15;
    config.define_unpredictable_behaviour = true;
    static constexpr std::size_t YUZU_PAGEBITS = 12;
    static constexpr std::size_t NUM_PAGE_TABLE_ENTRIES = 1 << (32 - YUZU_PAGEBITS);
    if (page_table) {
        config.page_table = reinterpret_cast<std::array<std::uint8_t*, NUM_PAGE_TABLE_ENTRIES>*>(
            page_table->pointers.data());
        config.absolute_offset_page_table = true;
        config.page_table_pointer_mask_bits = Common::PageTable::ATTRIBUTE_BITS;
        config.detect_misaligned_access_via_page_table = 16 | 32 | 64 | 128;
        config.only_detect_misalignment_via_page_table_on_page_boundary = true;

        config.fastmem_pointer = page_table->fastmem_arena;

        config.fastmem_exclusive_access = config.fastmem_pointer != nullptr;
        config.recompile_on_exclusive_fastmem_failure = true;
    }

    // Multi-process state
    config.processor_id = core_index;
    config.global_monitor = &exclusive_monitor.monitor;

    // Timing
    config.wall_clock_cntpct = uses_wall_clock;
    config.enable_cycle_counting = true;

    // Code cache size
#ifdef ARCHITECTURE_arm64
    config.code_cache_size = 128_MiB;
#else
    config.code_cache_size = 512_MiB;
#endif

    // Allow memory fault handling to work
    if (system.DebuggerEnabled()) {
        config.check_halt_on_memory_access = true;
    }

    // null_jit
    if (!page_table) {
        // Don't waste too much memory on null_jit
        config.code_cache_size = 8_MiB;
    }

    // Safe optimizations
    if (Settings::values.cpu_debug_mode) {
        if (!Settings::values.cpuopt_page_tables) {
            config.page_table = nullptr;
        }
        if (!Settings::values.cpuopt_block_linking) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::BlockLinking;
        }
        if (!Settings::values.cpuopt_return_stack_buffer) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::ReturnStackBuffer;
        }
        if (!Settings::values.cpuopt_fast_dispatcher) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::FastDispatch;
        }
        if (!Settings::values.cpuopt_context_elimination) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::GetSetElimination;
        }
        if (!Settings::values.cpuopt_const_prop) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::ConstProp;
        }
        if (!Settings::values.cpuopt_misc_ir) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::MiscIROpt;
        }
        if (!Settings::values.cpuopt_reduce_misalign_checks) {
            config.only_detect_misalignment_via_page_table_on_page_boundary = false;
        }
        if (!Settings::values.cpuopt_fastmem) {
            config.fastmem_pointer = nullptr;
            config.fastmem_exclusive_access = false;
        }
        if (!Settings::values.cpuopt_fastmem_exclusives) {
            config.fastmem_exclusive_access = false;
        }
        if (!Settings::values.cpuopt_recompile_exclusives) {
            config.recompile_on_exclusive_fastmem_failure = false;
        }
        if (!Settings::values.cpuopt_ignore_memory_aborts) {
            config.check_halt_on_memory_access = true;
        }
    } else {
        // Unsafe optimizations
        if (Settings::values.cpu_accuracy.GetValue() == Settings::CPUAccuracy::Unsafe) {
            config.unsafe_optimizations = true;
            if (Settings::values.cpuopt_unsafe_unfuse_fma) {
                config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_UnfuseFMA;
            }
            if (Settings::values.cpuopt_unsafe_reduce_fp_error) {
                config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_ReducedErrorFP;
            }
            if (Settings::values.cpuopt_unsafe_ignore_standard_fpcr) {
                config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_IgnoreStandardFPCRValue;
            }
            if (Settings::values.cpuopt_unsafe_inaccurate_nan) {
                config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_InaccurateNaN;
            }
            if (Settings::values.cpuopt_unsafe_ignore_global_monitor) {
                config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_IgnoreGlobalMonitor;
            }
        }

        // Curated optimizations
        if (Settings::values.cpu_accuracy.GetValue() == Settings::CPUAccuracy::Auto) {
            config.unsafe_optimizations = true;
            config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_UnfuseFMA;
            config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_IgnoreStandardFPCRValue;
            config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_InaccurateNaN;
            config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_IgnoreGlobalMonitor;
        }

        // Paranoia mode for debugging optimizations
        if (Settings::values.cpu_accuracy.GetValue() == Settings::CPUAccuracy::Paranoid) {
            config.unsafe_optimizations = false;
            config.optimizations = Dynarmic::no_optimizations;
        }
    }

#ifdef ARCHITECTURE_arm64
    // TODO: remove when fixed in dynarmic
    config.optimizations &= ~Dynarmic::OptimizationFlag::BlockLinking;
#endif

    return std::make_unique<Dynarmic::A32::Jit>(config);
}

Dynarmic::HaltReason ARM_Dynarmic_32::RunJit() {
    return jit.load()->Run();
}

Dynarmic::HaltReason ARM_Dynarmic_32::StepJit() {
    return jit.load()->Step();
}

u32 ARM_Dynarmic_32::GetSvcNumber() const {
    return svc_swi;
}

const Kernel::DebugWatchpoint* ARM_Dynarmic_32::HaltedWatchpoint() const {
    return halted_watchpoint;
}

void ARM_Dynarmic_32::RewindBreakpointInstruction() {
    LoadContext(breakpoint_context);
}

ARM_Dynarmic_32::ARM_Dynarmic_32(System& system_, bool uses_wall_clock_,
                                 ExclusiveMonitor& exclusive_monitor_, std::size_t core_index_)
    : ARM_Interface{system_, uses_wall_clock_}, cb(std::make_unique<DynarmicCallbacks32>(*this)),
      cp15(std::make_shared<DynarmicCP15>(*this)), core_index{core_index_},
      exclusive_monitor{dynamic_cast<DynarmicExclusiveMonitor&>(exclusive_monitor_)},
      null_jit{MakeJit(nullptr)}, jit{null_jit.get()} {}

ARM_Dynarmic_32::~ARM_Dynarmic_32() = default;

void ARM_Dynarmic_32::SetPC(u64 pc) {
    jit.load()->Regs()[15] = static_cast<u32>(pc);
}

u64 ARM_Dynarmic_32::GetPC() const {
    return jit.load()->Regs()[15];
}

u64 ARM_Dynarmic_32::GetSP() const {
    return jit.load()->Regs()[13];
}

u64 ARM_Dynarmic_32::GetReg(int index) const {
    return jit.load()->Regs()[index];
}

void ARM_Dynarmic_32::SetReg(int index, u64 value) {
    jit.load()->Regs()[index] = static_cast<u32>(value);
}

u128 ARM_Dynarmic_32::GetVectorReg(int index) const {
    return {};
}

void ARM_Dynarmic_32::SetVectorReg(int index, u128 value) {}

u32 ARM_Dynarmic_32::GetPSTATE() const {
    return jit.load()->Cpsr();
}

void ARM_Dynarmic_32::SetPSTATE(u32 cpsr) {
    jit.load()->SetCpsr(cpsr);
}

u64 ARM_Dynarmic_32::GetTlsAddress() const {
    return cp15->uro;
}

void ARM_Dynarmic_32::SetTlsAddress(VAddr address) {
    cp15->uro = static_cast<u32>(address);
}

u64 ARM_Dynarmic_32::GetTPIDR_EL0() const {
    return cp15->uprw;
}

void ARM_Dynarmic_32::SetTPIDR_EL0(u64 value) {
    cp15->uprw = static_cast<u32>(value);
}

void ARM_Dynarmic_32::SaveContext(ThreadContext32& ctx) {
    Dynarmic::A32::Context context;
    jit.load()->SaveContext(context);
    ctx.cpu_registers = context.Regs();
    ctx.extension_registers = context.ExtRegs();
    ctx.cpsr = context.Cpsr();
    ctx.fpscr = context.Fpscr();
}

void ARM_Dynarmic_32::LoadContext(const ThreadContext32& ctx) {
    Dynarmic::A32::Context context;
    context.Regs() = ctx.cpu_registers;
    context.ExtRegs() = ctx.extension_registers;
    context.SetCpsr(ctx.cpsr);
    context.SetFpscr(ctx.fpscr);
    jit.load()->LoadContext(context);
}

void ARM_Dynarmic_32::SignalInterrupt() {
    jit.load()->HaltExecution(break_loop);
}

void ARM_Dynarmic_32::ClearInterrupt() {
    jit.load()->ClearHalt(break_loop);
}

void ARM_Dynarmic_32::ClearInstructionCache() {
    jit.load()->ClearCache();
}

void ARM_Dynarmic_32::InvalidateCacheRange(VAddr addr, std::size_t size) {
    jit.load()->InvalidateCacheRange(static_cast<u32>(addr), size);
}

void ARM_Dynarmic_32::ClearExclusiveState() {
    jit.load()->ClearExclusiveState();
}

void ARM_Dynarmic_32::PageTableChanged(Common::PageTable& page_table,
                                       std::size_t new_address_space_size_in_bits) {
    ThreadContext32 ctx{};
    SaveContext(ctx);

    auto key = std::make_pair(&page_table, new_address_space_size_in_bits);
    auto iter = jit_cache.find(key);
    if (iter != jit_cache.end()) {
        jit.store(iter->second.get());
        LoadContext(ctx);
        return;
    }
    std::shared_ptr new_jit = MakeJit(&page_table);
    jit.store(new_jit.get());
    LoadContext(ctx);
    jit_cache.emplace(key, std::move(new_jit));
}

std::vector<ARM_Interface::BacktraceEntry> ARM_Dynarmic_32::GetBacktrace(Core::System& system,
                                                                         u64 fp, u64 lr, u64 pc) {
    std::vector<BacktraceEntry> out;
    auto& memory = system.Memory();

    out.push_back({"", 0, pc, 0, ""});

    // fp (= r11) points to the last frame record.
    // Frame records are two words long:
    // fp+0 : pointer to previous frame record
    // fp+4 : value of lr for frame
    for (size_t i = 0; i < 256; i++) {
        out.push_back({"", 0, lr, 0, ""});
        if (!fp || (fp % 4 != 0) || !memory.IsValidVirtualAddressRange(fp, 8)) {
            break;
        }
        lr = memory.Read32(fp + 4);
        fp = memory.Read32(fp);
    }

    SymbolicateBacktrace(system, out);

    return out;
}

std::vector<ARM_Interface::BacktraceEntry> ARM_Dynarmic_32::GetBacktraceFromContext(
    System& system, const ThreadContext32& ctx) {
    const auto& reg = ctx.cpu_registers;
    return GetBacktrace(system, reg[11], reg[14], reg[15]);
}

std::vector<ARM_Interface::BacktraceEntry> ARM_Dynarmic_32::GetBacktrace() const {
    return GetBacktrace(system, GetReg(11), GetReg(14), GetReg(15));
}

} // namespace Core
