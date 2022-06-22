/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "dynarmic/interface/optimization_flags.h"

namespace Dynarmic {
class ExclusiveMonitor;
}  // namespace Dynarmic

namespace Dynarmic {
namespace A64 {

using VAddr = std::uint64_t;

using Vector = std::array<std::uint64_t, 2>;
static_assert(sizeof(Vector) == sizeof(std::uint64_t) * 2, "Vector must be 128 bits in size");

enum class Exception {
    /// An UndefinedFault occured due to executing instruction with an unallocated encoding
    UnallocatedEncoding,
    /// An UndefinedFault occured due to executing instruction containing a reserved value
    ReservedValue,
    /// An unpredictable instruction is to be executed. Implementation-defined behaviour should now happen.
    /// This behaviour is up to the user of this library to define.
    /// Note: Constraints on unpredictable behaviour are specified in the ARMv8 ARM.
    UnpredictableInstruction,
    /// A WFI instruction was executed. You may now enter a low-power state. (Hint instruction.)
    WaitForInterrupt,
    /// A WFE instruction was executed. You may now enter a low-power state if the event register is clear. (Hint instruction.)
    WaitForEvent,
    /// A SEV instruction was executed. The event register of all PEs should be set. (Hint instruction.)
    SendEvent,
    /// A SEVL instruction was executed. The event register of the current PE should be set. (Hint instruction.)
    SendEventLocal,
    /// A YIELD instruction was executed. (Hint instruction.)
    Yield,
    /// A BRK instruction was executed. (Hint instruction.)
    Breakpoint,
};

enum class DataCacheOperation {
    /// DC CISW
    CleanAndInvalidateBySetWay,
    /// DC CIVAC
    CleanAndInvalidateByVAToPoC,
    /// DC CSW
    CleanBySetWay,
    /// DC CVAC
    CleanByVAToPoC,
    /// DC CVAU
    CleanByVAToPoU,
    /// DC CVAP
    CleanByVAToPoP,
    /// DC ISW
    InvalidateBySetWay,
    /// DC IVAC
    InvalidateByVAToPoC,
    /// DC ZVA
    ZeroByVA,
};

enum class InstructionCacheOperation {
    /// IC IVAU
    InvalidateByVAToPoU,
    /// IC IALLU
    InvalidateAllToPoU,
    /// IC IALLUIS
    InvalidateAllToPoUInnerSharable
};

struct UserCallbacks {
    virtual ~UserCallbacks() = default;

    // All reads through this callback are 4-byte aligned.
    // Memory must be interpreted as little endian.
    virtual std::uint32_t MemoryReadCode(VAddr vaddr) { return MemoryRead32(vaddr); }

    // Reads through these callbacks may not be aligned.
    virtual std::uint8_t MemoryRead8(VAddr vaddr) = 0;
    virtual std::uint16_t MemoryRead16(VAddr vaddr) = 0;
    virtual std::uint32_t MemoryRead32(VAddr vaddr) = 0;
    virtual std::uint64_t MemoryRead64(VAddr vaddr) = 0;
    virtual Vector MemoryRead128(VAddr vaddr) = 0;

    // Writes through these callbacks may not be aligned.
    virtual void MemoryWrite8(VAddr vaddr, std::uint8_t value) = 0;
    virtual void MemoryWrite16(VAddr vaddr, std::uint16_t value) = 0;
    virtual void MemoryWrite32(VAddr vaddr, std::uint32_t value) = 0;
    virtual void MemoryWrite64(VAddr vaddr, std::uint64_t value) = 0;
    virtual void MemoryWrite128(VAddr vaddr, Vector value) = 0;

    // Writes through these callbacks may not be aligned.
    virtual bool MemoryWriteExclusive8(VAddr /*vaddr*/, std::uint8_t /*value*/, std::uint8_t /*expected*/) { return false; }
    virtual bool MemoryWriteExclusive16(VAddr /*vaddr*/, std::uint16_t /*value*/, std::uint16_t /*expected*/) { return false; }
    virtual bool MemoryWriteExclusive32(VAddr /*vaddr*/, std::uint32_t /*value*/, std::uint32_t /*expected*/) { return false; }
    virtual bool MemoryWriteExclusive64(VAddr /*vaddr*/, std::uint64_t /*value*/, std::uint64_t /*expected*/) { return false; }
    virtual bool MemoryWriteExclusive128(VAddr /*vaddr*/, Vector /*value*/, Vector /*expected*/) { return false; }

    // If this callback returns true, the JIT will assume MemoryRead* callbacks will always
    // return the same value at any point in time for this vaddr. The JIT may use this information
    // in optimizations.
    // A conservative implementation that always returns false is safe.
    virtual bool IsReadOnlyMemory(VAddr /*vaddr*/) { return false; }

    /// The interpreter must execute exactly num_instructions starting from PC.
    virtual void InterpreterFallback(VAddr pc, size_t num_instructions) = 0;

    // This callback is called whenever a SVC instruction is executed.
    virtual void CallSVC(std::uint32_t swi) = 0;

    virtual void ExceptionRaised(VAddr pc, Exception exception) = 0;
    virtual void DataCacheOperationRaised(DataCacheOperation /*op*/, VAddr /*value*/) {}
    virtual void InstructionCacheOperationRaised(InstructionCacheOperation /*op*/, VAddr /*value*/) {}
    virtual void InstructionSynchronizationBarrierRaised() {}

    // Timing-related callbacks
    // ticks ticks have passed
    virtual void AddTicks(std::uint64_t ticks) = 0;
    // How many more ticks am I allowed to execute?
    virtual std::uint64_t GetTicksRemaining() = 0;
    // Get value in the emulated counter-timer physical count register.
    virtual std::uint64_t GetCNTPCT() = 0;
};

struct UserConfig {
    UserCallbacks* callbacks;

    size_t processor_id = 0;
    ExclusiveMonitor* global_monitor = nullptr;

    /// This selects other optimizations than can't otherwise be disabled by setting other
    /// configuration options. This includes:
    /// - IR optimizations
    /// - Block linking optimizations
    /// - RSB optimizations
    /// This is intended to be used for debugging.
    OptimizationFlag optimizations = all_safe_optimizations;

    bool HasOptimization(OptimizationFlag f) const {
        if (!unsafe_optimizations) {
            f &= all_safe_optimizations;
        }
        return (f & optimizations) != no_optimizations;
    }

    /// This enables unsafe optimizations that reduce emulation accuracy in favour of speed.
    /// For safety, in order to enable unsafe optimizations you have to set BOTH this flag
    /// AND the appropriate flag bits above.
    /// The prefered and tested mode for this library is with unsafe optimizations disabled.
    bool unsafe_optimizations = false;

    /// When set to true, UserCallbacks::DataCacheOperationRaised will be called when any
    /// data cache instruction is executed. Notably DC ZVA will not implicitly do anything.
    /// When set to false, UserCallbacks::DataCacheOperationRaised will never be called.
    /// Executing DC ZVA in this mode will result in zeros being written to memory.
    bool hook_data_cache_operations = false;

    /// When set to true, UserCallbacks::InstructionSynchronizationBarrierRaised will be
    /// called when an ISB instruction is executed.
    /// When set to false, ISB will be treated as a NOP instruction.
    bool hook_isb = false;

    /// When set to true, UserCallbacks::ExceptionRaised will be called when any hint
    /// instruction is executed.
    bool hook_hint_instructions = false;

    /// Counter-timer frequency register. The value of the register is not interpreted by
    /// dynarmic.
    std::uint32_t cntfrq_el0 = 600000000;

    /// CTR_EL0<27:24> is log2 of the cache writeback granule in words.
    /// CTR_EL0<23:20> is log2 of the exclusives reservation granule in words.
    /// CTR_EL0<19:16> is log2 of the smallest data/unified cacheline in words.
    /// CTR_EL0<15:14> is the level 1 instruction cache policy.
    /// CTR_EL0<3:0> is log2 of the smallest instruction cacheline in words.
    std::uint32_t ctr_el0 = 0x8444c004;

    /// DCZID_EL0<3:0> is log2 of the block size in words
    /// DCZID_EL0<4> is 0 if the DC ZVA instruction is permitted.
    std::uint32_t dczid_el0 = 4;

    /// Pointer to where TPIDRRO_EL0 is stored. This pointer will be inserted into
    /// emitted code.
    const std::uint64_t* tpidrro_el0 = nullptr;

    /// Pointer to where TPIDR_EL0 is stored. This pointer will be inserted into
    /// emitted code.
    const std::uint64_t* tpidr_el0 = nullptr;

    /// Pointer to the page table which we can use for direct page table access.
    /// If an entry in page_table is null, the relevant memory callback will be called.
    /// If page_table is nullptr, all memory accesses hit the memory callbacks.
    void** page_table = nullptr;
    /// Declares how many valid address bits are there in virtual addresses.
    /// Determines the size of page_table. Valid values are between 12 and 64 inclusive.
    /// This is only used if page_table is not nullptr.
    size_t page_table_address_space_bits = 36;
    /// Masks out the first N bits in host pointers from the page table.
    /// The intention behind this is to allow users of Dynarmic to pack attributes in the
    /// same integer and update the pointer attribute pair atomically.
    /// If the configured value is 3, all pointers will be forcefully aligned to 8 bytes.
    int page_table_pointer_mask_bits = 0;
    /// Determines what happens if the guest accesses an entry that is off the end of the
    /// page table. If true, Dynarmic will silently mirror page_table's address space. If
    /// false, accessing memory outside of page_table bounds will result in a call to the
    /// relevant memory callback.
    /// This is only used if page_table is not nullptr.
    bool silently_mirror_page_table = true;
    /// Determines if the pointer in the page_table shall be offseted locally or globally.
    /// 'false' will access page_table[addr >> bits][addr & mask]
    /// 'true'  will access page_table[addr >> bits][addr]
    /// Note: page_table[addr >> bits] will still be checked to verify active pages.
    ///       So there might be wrongly faulted pages which maps to nullptr.
    ///       This can be avoided by carefully allocating the memory region.
    bool absolute_offset_page_table = false;
    /// Determines if we should detect memory accesses via page_table that straddle are
    /// misaligned. Accesses that straddle page boundaries will fallback to the relevant
    /// memory callback.
    /// This value should be the required access sizes this applies to ORed together.
    /// To detect any access, use: 8 | 16 | 32 | 64 | 128.
    std::uint8_t detect_misaligned_access_via_page_table = 0;
    /// Determines if the above option only triggers when the misalignment straddles a
    /// page boundary.
    bool only_detect_misalignment_via_page_table_on_page_boundary = false;

    /// Fastmem Pointer
    /// This should point to the beginning of a 2^page_table_address_space_bits bytes
    /// address space which is in arranged just like what you wish for emulated memory to
    /// be. If the host page faults on an address, the JIT will fallback to calling the
    /// MemoryRead*/MemoryWrite* callbacks.
    void* fastmem_pointer = nullptr;
    /// Determines if instructions that pagefault should cause recompilation of that block
    /// with fastmem disabled.
    /// Recompiled code will use the page_table if this is available, otherwise memory
    /// accesses will hit the memory callbacks.
    bool recompile_on_fastmem_failure = true;
    /// Declares how many valid address bits are there in virtual addresses.
    /// Determines the size of fastmem arena. Valid values are between 12 and 64 inclusive.
    /// This is only used if fastmem_pointer is not nullptr.
    size_t fastmem_address_space_bits = 36;
    /// Determines what happens if the guest accesses an entry that is off the end of the
    /// fastmem arena. If true, Dynarmic will silently mirror fastmem's address space. If
    /// false, accessing memory outside of fastmem bounds will result in a call to the
    /// relevant memory callback.
    /// This is only used if fastmem_pointer is not nullptr.
    bool silently_mirror_fastmem = true;

    /// Determines if we should use the above fastmem_pointer for exclusive reads and
    /// writes. On x64, dynarmic currently relies on x64 cmpxchg semantics which may not
    /// provide fully accurate emulation.
    bool fastmem_exclusive_access = false;
    /// Determines if exclusive access instructions that pagefault should cause
    /// recompilation of that block with fastmem disabled. Recompiled code will use memory
    /// callbacks.
    bool recompile_on_exclusive_fastmem_failure = true;

    /// This option relates to translation. Generally when we run into an unpredictable
    /// instruction the ExceptionRaised callback is called. If this is true, we define
    /// definite behaviour for some unpredictable instructions.
    bool define_unpredictable_behaviour = false;

    /// HACK:
    /// This tells the translator a wall clock will be used, thus allowing it
    /// to avoid writting certain unnecessary code only needed for cycle timers.
    bool wall_clock_cntpct = false;

    /// This allows accurately emulating protection fault handlers. If true, we check
    /// for exit after every data memory access by the emulated program.
    bool check_halt_on_memory_access = false;

    /// This option allows you to disable cycle counting. If this is set to false,
    /// AddTicks and GetTicksRemaining are never called, and no cycle counting is done.
    bool enable_cycle_counting = true;

    // Minimum size is about 8MiB. Maximum size is about 2GiB. Maximum size is limited by
    // the maximum length of a x64 jump.
    size_t code_cache_size = 256 * 1024 * 1024;  // bytes
    // Determines the relative size of the near and far code caches. Must be smaller than
    // code_cache_size.
    size_t far_code_offset = 200 * 1024 * 1024;  // bytes
};

}  // namespace A64
}  // namespace Dynarmic
