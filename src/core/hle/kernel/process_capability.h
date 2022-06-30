// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <bitset>

#include "common/common_types.h"

union Result;

namespace Kernel {

class KPageTable;

/// The possible types of programs that may be indicated
/// by the program type capability descriptor.
enum class ProgramType {
    SysModule,
    Application,
    Applet,
};

/// Handles kernel capability descriptors that are provided by
/// application metadata. These descriptors provide information
/// that alters certain parameters for kernel process instance
/// that will run said application (or applet).
///
/// Capabilities are a sequence of flag descriptors, that indicate various
/// configurations and constraints for a particular process.
///
/// Flag types are indicated by a sequence of set low bits. E.g. the
/// types are indicated with the low bits as follows (where x indicates "don't care"):
///
/// - Priority and core mask   : 0bxxxxxxxxxxxx0111
/// - Allowed service call mask: 0bxxxxxxxxxxx01111
/// - Map physical memory      : 0bxxxxxxxxx0111111
/// - Map IO memory            : 0bxxxxxxxx01111111
/// - Interrupts               : 0bxxxx011111111111
/// - Application type         : 0bxx01111111111111
/// - Kernel version           : 0bx011111111111111
/// - Handle table size        : 0b0111111111111111
/// - Debugger flags           : 0b1111111111111111
///
/// These are essentially a bit offset subtracted by 1 to create a mask.
/// e.g. The first entry in the above list is simply bit 3 (value 8 -> 0b1000)
///      subtracted by one (7 -> 0b0111)
///
/// An example of a bit layout (using the map physical layout):
/// <example>
///   The MapPhysical type indicates a sequence entry pair of:
///
///   [initial, memory_flags], where:
///
///   initial:
///     bits:
///       7-24: Starting page to map memory at.
///       25  : Indicates if the memory should be mapped as read only.
///
///   memory_flags:
///     bits:
///       7-20 : Number of pages to map
///       21-25: Seems to be reserved (still checked against though)
///       26   : Whether or not the memory being mapped is IO memory, or physical memory
/// </example>
///
class ProcessCapabilities {
public:
    using InterruptCapabilities = std::bitset<1024>;
    using SyscallCapabilities = std::bitset<192>;

    ProcessCapabilities() = default;
    ProcessCapabilities(const ProcessCapabilities&) = delete;
    ProcessCapabilities(ProcessCapabilities&&) = default;

    ProcessCapabilities& operator=(const ProcessCapabilities&) = delete;
    ProcessCapabilities& operator=(ProcessCapabilities&&) = default;

    /// Initializes this process capabilities instance for a kernel process.
    ///
    /// @param capabilities     The capabilities to parse
    /// @param num_capabilities The number of capabilities to parse.
    /// @param page_table       The memory manager to use for handling any mapping-related
    ///                         operations (such as mapping IO memory, etc).
    ///
    /// @returns ResultSuccess if this capabilities instance was able to be initialized,
    ///          otherwise, an error code upon failure.
    ///
    Result InitializeForKernelProcess(const u32* capabilities, std::size_t num_capabilities,
                                      KPageTable& page_table);

    /// Initializes this process capabilities instance for a userland process.
    ///
    /// @param capabilities     The capabilities to parse.
    /// @param num_capabilities The total number of capabilities to parse.
    /// @param page_table       The memory manager to use for handling any mapping-related
    ///                         operations (such as mapping IO memory, etc).
    ///
    /// @returns ResultSuccess if this capabilities instance was able to be initialized,
    ///          otherwise, an error code upon failure.
    ///
    Result InitializeForUserProcess(const u32* capabilities, std::size_t num_capabilities,
                                    KPageTable& page_table);

    /// Initializes this process capabilities instance for a process that does not
    /// have any metadata to parse.
    ///
    /// This is necessary, as we allow running raw executables, and the internal
    /// kernel process capabilities also determine what CPU cores the process is
    /// allowed to run on, and what priorities are allowed for  threads. It also
    /// determines the max handle table size, what the program type is, whether or
    /// not the process can be debugged, or whether it's possible for a process to
    /// forcibly debug another process.
    ///
    /// Given the above, this essentially enables all capabilities across the board
    /// for the process. It allows the process to:
    ///
    /// - Run on any core
    /// - Use any thread priority
    /// - Use the maximum amount of handles a process is allowed to.
    /// - Be debuggable
    /// - Forcibly debug other processes.
    ///
    /// Note that this is not a behavior that the kernel allows a process to do via
    /// a single function like this. This is yuzu-specific behavior to handle
    /// executables with no capability descriptors whatsoever to derive behavior from.
    /// It being yuzu-specific is why this is also not the default behavior and not
    /// done by default in the constructor.
    ///
    void InitializeForMetadatalessProcess();

    /// Gets the allowable core mask
    u64 GetCoreMask() const {
        return core_mask;
    }

    /// Gets the allowable priority mask
    u64 GetPriorityMask() const {
        return priority_mask;
    }

    /// Gets the SVC access permission bits
    const SyscallCapabilities& GetServiceCapabilities() const {
        return svc_capabilities;
    }

    /// Gets the valid interrupt bits.
    const InterruptCapabilities& GetInterruptCapabilities() const {
        return interrupt_capabilities;
    }

    /// Gets the program type for this process.
    ProgramType GetProgramType() const {
        return program_type;
    }

    /// Gets the number of total allowable handles for the process' handle table.
    s32 GetHandleTableSize() const {
        return handle_table_size;
    }

    /// Gets the kernel version value.
    u32 GetKernelVersion() const {
        return kernel_version;
    }

    /// Whether or not this process can be debugged.
    bool IsDebuggable() const {
        return is_debuggable;
    }

    /// Whether or not this process can forcibly debug another
    /// process, even if that process is not considered debuggable.
    bool CanForceDebug() const {
        return can_force_debug;
    }

private:
    /// Attempts to parse a given sequence of capability descriptors.
    ///
    /// @param capabilities     The sequence of capability descriptors to parse.
    /// @param num_capabilities The number of descriptors within the given sequence.
    /// @param page_table       The memory manager that will perform any memory
    ///                         mapping if necessary.
    ///
    /// @return ResultSuccess if no errors occur, otherwise an error code.
    ///
    Result ParseCapabilities(const u32* capabilities, std::size_t num_capabilities,
                             KPageTable& page_table);

    /// Attempts to parse a capability descriptor that is only represented by a
    /// single flag set.
    ///
    /// @param set_flags    Running set of flags that are used to catch
    ///                     flags being initialized more than once when they shouldn't be.
    /// @param set_svc_bits Running set of bits representing the allowed supervisor calls mask.
    /// @param flag         The flag to attempt to parse.
    /// @param page_table   The memory manager that will perform any memory
    ///                     mapping if necessary.
    ///
    /// @return ResultSuccess if no errors occurred, otherwise an error code.
    ///
    Result ParseSingleFlagCapability(u32& set_flags, u32& set_svc_bits, u32 flag,
                                     KPageTable& page_table);

    /// Clears the internal state of this process capability instance. Necessary,
    /// to have a sane starting point due to us allowing running executables without
    /// configuration metadata. We assume a process is not going to have metadata,
    /// and if it turns out that the process does, in fact, have metadata, then
    /// we attempt to parse it. Thus, we need this to reset data members back to
    /// a good state.
    ///
    /// DO NOT ever make this a public member function. This isn't an invariant
    /// anything external should depend upon (and if anything comes to rely on it,
    /// you should immediately be questioning the design of that thing, not this
    /// class. If the kernel itself can run without depending on behavior like that,
    /// then so can yuzu).
    ///
    void Clear();

    /// Handles flags related to the priority and core number capability flags.
    Result HandlePriorityCoreNumFlags(u32 flags);

    /// Handles flags related to determining the allowable SVC mask.
    Result HandleSyscallFlags(u32& set_svc_bits, u32 flags);

    /// Handles flags related to mapping physical memory pages.
    Result HandleMapPhysicalFlags(u32 flags, u32 size_flags, KPageTable& page_table);

    /// Handles flags related to mapping IO pages.
    Result HandleMapIOFlags(u32 flags, KPageTable& page_table);

    /// Handles flags related to mapping physical memory regions.
    Result HandleMapRegionFlags(u32 flags, KPageTable& page_table);

    /// Handles flags related to the interrupt capability flags.
    Result HandleInterruptFlags(u32 flags);

    /// Handles flags related to the program type.
    Result HandleProgramTypeFlags(u32 flags);

    /// Handles flags related to the handle table size.
    Result HandleHandleTableFlags(u32 flags);

    /// Handles flags related to the kernel version capability flags.
    Result HandleKernelVersionFlags(u32 flags);

    /// Handles flags related to debug-specific capabilities.
    Result HandleDebugFlags(u32 flags);

    SyscallCapabilities svc_capabilities;
    InterruptCapabilities interrupt_capabilities;

    u64 core_mask = 0;
    u64 priority_mask = 0;

    s32 handle_table_size = 0;
    u32 kernel_version = 0;

    ProgramType program_type = ProgramType::SysModule;

    bool is_debuggable = false;
    bool can_force_debug = false;
};

} // namespace Kernel
