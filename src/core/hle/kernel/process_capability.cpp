// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>

#include "common/bit_util.h"
#include "common/logging/log.h"
#include "core/hle/kernel/k_handle_table.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/process_capability.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {
namespace {

// clang-format off

// Shift offsets for kernel capability types.
enum : u32 {
    CapabilityOffset_PriorityAndCoreNum = 3,
    CapabilityOffset_Syscall            = 4,
    CapabilityOffset_MapPhysical        = 6,
    CapabilityOffset_MapIO              = 7,
    CapabilityOffset_MapRegion          = 10,
    CapabilityOffset_Interrupt          = 11,
    CapabilityOffset_ProgramType        = 13,
    CapabilityOffset_KernelVersion      = 14,
    CapabilityOffset_HandleTableSize    = 15,
    CapabilityOffset_Debug              = 16,
};

// Combined mask of all parameters that may be initialized only once.
constexpr u32 InitializeOnceMask = (1U << CapabilityOffset_PriorityAndCoreNum) |
                                   (1U << CapabilityOffset_ProgramType) |
                                   (1U << CapabilityOffset_KernelVersion) |
                                   (1U << CapabilityOffset_HandleTableSize) |
                                   (1U << CapabilityOffset_Debug);

// Packed kernel version indicating 10.4.0
constexpr u32 PackedKernelVersion = 0x520000;

// Indicates possible types of capabilities that can be specified.
enum class CapabilityType : u32 {
    Unset              = 0U,
    PriorityAndCoreNum = (1U << CapabilityOffset_PriorityAndCoreNum) - 1,
    Syscall            = (1U << CapabilityOffset_Syscall) - 1,
    MapPhysical        = (1U << CapabilityOffset_MapPhysical) - 1,
    MapIO              = (1U << CapabilityOffset_MapIO) - 1,
    MapRegion          = (1U << CapabilityOffset_MapRegion) - 1,
    Interrupt          = (1U << CapabilityOffset_Interrupt) - 1,
    ProgramType        = (1U << CapabilityOffset_ProgramType) - 1,
    KernelVersion      = (1U << CapabilityOffset_KernelVersion) - 1,
    HandleTableSize    = (1U << CapabilityOffset_HandleTableSize) - 1,
    Debug              = (1U << CapabilityOffset_Debug) - 1,
    Ignorable          = 0xFFFFFFFFU,
};

// clang-format on

constexpr CapabilityType GetCapabilityType(u32 value) {
    return static_cast<CapabilityType>((~value & (value + 1)) - 1);
}

u32 GetFlagBitOffset(CapabilityType type) {
    const auto value = static_cast<u32>(type);
    return static_cast<u32>(Common::BitSize<u32>() - static_cast<u32>(std::countl_zero(value)));
}

} // Anonymous namespace

Result ProcessCapabilities::InitializeForKernelProcess(const u32* capabilities,
                                                       std::size_t num_capabilities,
                                                       KPageTable& page_table) {
    Clear();

    // Allow all cores and priorities.
    core_mask = 0xF;
    priority_mask = 0xFFFFFFFFFFFFFFFF;
    kernel_version = PackedKernelVersion;

    return ParseCapabilities(capabilities, num_capabilities, page_table);
}

Result ProcessCapabilities::InitializeForUserProcess(const u32* capabilities,
                                                     std::size_t num_capabilities,
                                                     KPageTable& page_table) {
    Clear();

    return ParseCapabilities(capabilities, num_capabilities, page_table);
}

void ProcessCapabilities::InitializeForMetadatalessProcess() {
    // Allow all cores and priorities
    core_mask = 0xF;
    priority_mask = 0xFFFFFFFFFFFFFFFF;
    kernel_version = PackedKernelVersion;

    // Allow all system calls and interrupts.
    svc_capabilities.set();
    interrupt_capabilities.set();

    // Allow using the maximum possible amount of handles
    handle_table_size = static_cast<s32>(KHandleTable::MaxTableSize);

    // Allow all debugging capabilities.
    is_debuggable = true;
    can_force_debug = true;
}

Result ProcessCapabilities::ParseCapabilities(const u32* capabilities, std::size_t num_capabilities,
                                              KPageTable& page_table) {
    u32 set_flags = 0;
    u32 set_svc_bits = 0;

    for (std::size_t i = 0; i < num_capabilities; ++i) {
        const u32 descriptor = capabilities[i];
        const auto type = GetCapabilityType(descriptor);

        if (type == CapabilityType::MapPhysical) {
            i++;

            // The MapPhysical type uses two descriptor flags for its parameters.
            // If there's only one, then there's a problem.
            if (i >= num_capabilities) {
                LOG_ERROR(Kernel, "Invalid combination! i={}", i);
                return ResultInvalidCombination;
            }

            const auto size_flags = capabilities[i];
            if (GetCapabilityType(size_flags) != CapabilityType::MapPhysical) {
                LOG_ERROR(Kernel, "Invalid capability type! size_flags={}", size_flags);
                return ResultInvalidCombination;
            }

            const auto result = HandleMapPhysicalFlags(descriptor, size_flags, page_table);
            if (result.IsError()) {
                LOG_ERROR(Kernel, "Failed to map physical flags! descriptor={}, size_flags={}",
                          descriptor, size_flags);
                return result;
            }
        } else {
            const auto result =
                ParseSingleFlagCapability(set_flags, set_svc_bits, descriptor, page_table);
            if (result.IsError()) {
                LOG_ERROR(
                    Kernel,
                    "Failed to parse capability flag! set_flags={}, set_svc_bits={}, descriptor={}",
                    set_flags, set_svc_bits, descriptor);
                return result;
            }
        }
    }

    return ResultSuccess;
}

Result ProcessCapabilities::ParseSingleFlagCapability(u32& set_flags, u32& set_svc_bits, u32 flag,
                                                      KPageTable& page_table) {
    const auto type = GetCapabilityType(flag);

    if (type == CapabilityType::Unset) {
        return ResultInvalidArgument;
    }

    // Bail early on ignorable entries, as one would expect,
    // ignorable descriptors can be ignored.
    if (type == CapabilityType::Ignorable) {
        return ResultSuccess;
    }

    // Ensure that the give flag hasn't already been initialized before.
    // If it has been, then bail.
    const u32 flag_length = GetFlagBitOffset(type);
    const u32 set_flag = 1U << flag_length;
    if ((set_flag & set_flags & InitializeOnceMask) != 0) {
        LOG_ERROR(Kernel,
                  "Attempted to initialize flags that may only be initialized once. set_flags={}",
                  set_flags);
        return ResultInvalidCombination;
    }
    set_flags |= set_flag;

    switch (type) {
    case CapabilityType::PriorityAndCoreNum:
        return HandlePriorityCoreNumFlags(flag);
    case CapabilityType::Syscall:
        return HandleSyscallFlags(set_svc_bits, flag);
    case CapabilityType::MapIO:
        return HandleMapIOFlags(flag, page_table);
    case CapabilityType::MapRegion:
        return HandleMapRegionFlags(flag, page_table);
    case CapabilityType::Interrupt:
        return HandleInterruptFlags(flag);
    case CapabilityType::ProgramType:
        return HandleProgramTypeFlags(flag);
    case CapabilityType::KernelVersion:
        return HandleKernelVersionFlags(flag);
    case CapabilityType::HandleTableSize:
        return HandleHandleTableFlags(flag);
    case CapabilityType::Debug:
        return HandleDebugFlags(flag);
    default:
        break;
    }

    LOG_ERROR(Kernel, "Invalid capability type! type={}", type);
    return ResultInvalidArgument;
}

void ProcessCapabilities::Clear() {
    svc_capabilities.reset();
    interrupt_capabilities.reset();

    core_mask = 0;
    priority_mask = 0;

    handle_table_size = 0;
    kernel_version = 0;

    program_type = ProgramType::SysModule;

    is_debuggable = false;
    can_force_debug = false;
}

Result ProcessCapabilities::HandlePriorityCoreNumFlags(u32 flags) {
    if (priority_mask != 0 || core_mask != 0) {
        LOG_ERROR(Kernel, "Core or priority mask are not zero! priority_mask={}, core_mask={}",
                  priority_mask, core_mask);
        return ResultInvalidArgument;
    }

    const u32 core_num_min = (flags >> 16) & 0xFF;
    const u32 core_num_max = (flags >> 24) & 0xFF;
    if (core_num_min > core_num_max) {
        LOG_ERROR(Kernel, "Core min is greater than core max! core_num_min={}, core_num_max={}",
                  core_num_min, core_num_max);
        return ResultInvalidCombination;
    }

    const u32 priority_min = (flags >> 10) & 0x3F;
    const u32 priority_max = (flags >> 4) & 0x3F;
    if (priority_min > priority_max) {
        LOG_ERROR(Kernel,
                  "Priority min is greater than priority max! priority_min={}, priority_max={}",
                  core_num_min, priority_max);
        return ResultInvalidCombination;
    }

    // The switch only has 4 usable cores.
    if (core_num_max >= 4) {
        LOG_ERROR(Kernel, "Invalid max cores specified! core_num_max={}", core_num_max);
        return ResultInvalidCoreId;
    }

    const auto make_mask = [](u64 min, u64 max) {
        const u64 range = max - min + 1;
        const u64 mask = (1ULL << range) - 1;

        return mask << min;
    };

    core_mask = make_mask(core_num_min, core_num_max);
    priority_mask = make_mask(priority_min, priority_max);
    return ResultSuccess;
}

Result ProcessCapabilities::HandleSyscallFlags(u32& set_svc_bits, u32 flags) {
    const u32 index = flags >> 29;
    const u32 svc_bit = 1U << index;

    // If we've already set this svc before, bail.
    if ((set_svc_bits & svc_bit) != 0) {
        return ResultInvalidCombination;
    }
    set_svc_bits |= svc_bit;

    const u32 svc_mask = (flags >> 5) & 0xFFFFFF;
    for (u32 i = 0; i < 24; ++i) {
        const u32 svc_number = index * 24 + i;

        if ((svc_mask & (1U << i)) == 0) {
            continue;
        }

        svc_capabilities[svc_number] = true;
    }

    return ResultSuccess;
}

Result ProcessCapabilities::HandleMapPhysicalFlags(u32 flags, u32 size_flags,
                                                   KPageTable& page_table) {
    // TODO(Lioncache): Implement once the memory manager can handle this.
    return ResultSuccess;
}

Result ProcessCapabilities::HandleMapIOFlags(u32 flags, KPageTable& page_table) {
    // TODO(Lioncache): Implement once the memory manager can handle this.
    return ResultSuccess;
}

Result ProcessCapabilities::HandleMapRegionFlags(u32 flags, KPageTable& page_table) {
    // TODO(Lioncache): Implement once the memory manager can handle this.
    return ResultSuccess;
}

Result ProcessCapabilities::HandleInterruptFlags(u32 flags) {
    constexpr u32 interrupt_ignore_value = 0x3FF;
    const u32 interrupt0 = (flags >> 12) & 0x3FF;
    const u32 interrupt1 = (flags >> 22) & 0x3FF;

    for (u32 interrupt : {interrupt0, interrupt1}) {
        if (interrupt == interrupt_ignore_value) {
            continue;
        }

        // NOTE:
        // This should be checking a generic interrupt controller value
        // as part of the calculation, however, given we don't currently
        // emulate that, it's sufficient to mark every interrupt as defined.

        if (interrupt >= interrupt_capabilities.size()) {
            LOG_ERROR(Kernel, "Process interrupt capability is out of range! svc_number={}",
                      interrupt);
            return ResultOutOfRange;
        }

        interrupt_capabilities[interrupt] = true;
    }

    return ResultSuccess;
}

Result ProcessCapabilities::HandleProgramTypeFlags(u32 flags) {
    const u32 reserved = flags >> 17;
    if (reserved != 0) {
        LOG_ERROR(Kernel, "Reserved value is non-zero! reserved={}", reserved);
        return ResultReservedUsed;
    }

    program_type = static_cast<ProgramType>((flags >> 14) & 0b111);
    return ResultSuccess;
}

Result ProcessCapabilities::HandleKernelVersionFlags(u32 flags) {
    // Yes, the internal member variable is checked in the actual kernel here.
    // This might look odd for options that are only allowed to be initialized
    // just once, however the kernel has a separate initialization function for
    // kernel processes and userland processes. The kernel variant sets this
    // member variable ahead of time.

    const u32 major_version = kernel_version >> 19;

    if (major_version != 0 || flags < 0x80000) {
        LOG_ERROR(Kernel,
                  "Kernel version is non zero or flags are too small! major_version={}, flags={}",
                  major_version, flags);
        return ResultInvalidArgument;
    }

    kernel_version = flags;
    return ResultSuccess;
}

Result ProcessCapabilities::HandleHandleTableFlags(u32 flags) {
    const u32 reserved = flags >> 26;
    if (reserved != 0) {
        LOG_ERROR(Kernel, "Reserved value is non-zero! reserved={}", reserved);
        return ResultReservedUsed;
    }

    handle_table_size = static_cast<s32>((flags >> 16) & 0x3FF);
    return ResultSuccess;
}

Result ProcessCapabilities::HandleDebugFlags(u32 flags) {
    const u32 reserved = flags >> 19;
    if (reserved != 0) {
        LOG_ERROR(Kernel, "Reserved value is non-zero! reserved={}", reserved);
        return ResultReservedUsed;
    }

    is_debuggable = (flags & 0x20000) != 0;
    can_force_debug = (flags & 0x40000) != 0;
    return ResultSuccess;
}

} // namespace Kernel
