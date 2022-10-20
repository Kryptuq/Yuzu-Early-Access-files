// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <list>
#include <map>
#include <string>
#include "common/common_types.h"
#include "core/hle/kernel/k_address_arbiter.h"
#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/k_condition_variable.h"
#include "core/hle/kernel/k_handle_table.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_synchronization_object.h"
#include "core/hle/kernel/k_thread_local_page.h"
#include "core/hle/kernel/k_worker_task.h"
#include "core/hle/kernel/process_capability.h"
#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/result.h"

namespace Core {
class System;
}

namespace FileSys {
class ProgramMetadata;
}

namespace Kernel {

class KernelCore;
class KResourceLimit;
class KThread;
class KSharedMemoryInfo;
class TLSPage;

struct CodeSet;

enum class MemoryRegion : u16 {
    APPLICATION = 1,
    SYSTEM = 2,
    BASE = 3,
};

enum class ProcessActivity : u32 {
    Runnable,
    Paused,
};

enum class DebugWatchpointType : u8 {
    None = 0,
    Read = 1 << 0,
    Write = 1 << 1,
    ReadOrWrite = Read | Write,
};
DECLARE_ENUM_FLAG_OPERATORS(DebugWatchpointType);

struct DebugWatchpoint {
    VAddr start_address;
    VAddr end_address;
    DebugWatchpointType type;
};

class KProcess final : public KAutoObjectWithSlabHeapAndContainer<KProcess, KWorkerTask> {
    KERNEL_AUTOOBJECT_TRAITS(KProcess, KSynchronizationObject);

public:
    explicit KProcess(KernelCore& kernel_);
    ~KProcess() override;

    enum class State {
        Created = static_cast<u32>(Svc::ProcessState::Created),
        CreatedAttached = static_cast<u32>(Svc::ProcessState::CreatedAttached),
        Running = static_cast<u32>(Svc::ProcessState::Running),
        Crashed = static_cast<u32>(Svc::ProcessState::Crashed),
        RunningAttached = static_cast<u32>(Svc::ProcessState::RunningAttached),
        Terminating = static_cast<u32>(Svc::ProcessState::Terminating),
        Terminated = static_cast<u32>(Svc::ProcessState::Terminated),
        DebugBreak = static_cast<u32>(Svc::ProcessState::DebugBreak),
    };

    enum : u64 {
        /// Lowest allowed process ID for a kernel initial process.
        InitialKIPIDMin = 1,
        /// Highest allowed process ID for a kernel initial process.
        InitialKIPIDMax = 80,

        /// Lowest allowed process ID for a userland process.
        ProcessIDMin = 81,
        /// Highest allowed process ID for a userland process.
        ProcessIDMax = 0xFFFFFFFFFFFFFFFF,
    };

    // Used to determine how process IDs are assigned.
    enum class ProcessType {
        KernelInternal,
        Userland,
    };

    static constexpr std::size_t RANDOM_ENTROPY_SIZE = 4;

    static Result Initialize(KProcess* process, Core::System& system, std::string process_name,
                             ProcessType type, KResourceLimit* res_limit);

    /// Gets a reference to the process' page table.
    KPageTable& PageTable() {
        return page_table;
    }

    /// Gets const a reference to the process' page table.
    const KPageTable& PageTable() const {
        return page_table;
    }

    /// Gets a reference to the process' handle table.
    KHandleTable& GetHandleTable() {
        return handle_table;
    }

    /// Gets a const reference to the process' handle table.
    const KHandleTable& GetHandleTable() const {
        return handle_table;
    }

    Result SignalToAddress(VAddr address) {
        return condition_var.SignalToAddress(address);
    }

    Result WaitForAddress(Handle handle, VAddr address, u32 tag) {
        return condition_var.WaitForAddress(handle, address, tag);
    }

    void SignalConditionVariable(u64 cv_key, int32_t count) {
        return condition_var.Signal(cv_key, count);
    }

    Result WaitConditionVariable(VAddr address, u64 cv_key, u32 tag, s64 ns) {
        R_RETURN(condition_var.Wait(address, cv_key, tag, ns));
    }

    Result SignalAddressArbiter(VAddr address, Svc::SignalType signal_type, s32 value, s32 count) {
        R_RETURN(address_arbiter.SignalToAddress(address, signal_type, value, count));
    }

    Result WaitAddressArbiter(VAddr address, Svc::ArbitrationType arb_type, s32 value,
                              s64 timeout) {
        R_RETURN(address_arbiter.WaitForAddress(address, arb_type, value, timeout));
    }

    VAddr GetProcessLocalRegionAddress() const {
        return plr_address;
    }

    /// Gets the current status of the process
    State GetState() const {
        return state;
    }

    /// Gets the unique ID that identifies this particular process.
    u64 GetProcessID() const {
        return process_id;
    }

    /// Gets the program ID corresponding to this process.
    u64 GetProgramID() const {
        return program_id;
    }

    /// Gets the resource limit descriptor for this process
    KResourceLimit* GetResourceLimit() const;

    /// Gets the ideal CPU core ID for this process
    u8 GetIdealCoreId() const {
        return ideal_core;
    }

    /// Checks if the specified thread priority is valid.
    bool CheckThreadPriority(s32 prio) const {
        return ((1ULL << prio) & GetPriorityMask()) != 0;
    }

    /// Gets the bitmask of allowed cores that this process' threads can run on.
    u64 GetCoreMask() const {
        return capabilities.GetCoreMask();
    }

    /// Gets the bitmask of allowed thread priorities.
    u64 GetPriorityMask() const {
        return capabilities.GetPriorityMask();
    }

    /// Gets the amount of secure memory to allocate for memory management.
    u32 GetSystemResourceSize() const {
        return system_resource_size;
    }

    /// Gets the amount of secure memory currently in use for memory management.
    u32 GetSystemResourceUsage() const {
        // On hardware, this returns the amount of system resource memory that has
        // been used by the kernel. This is problematic for Yuzu to emulate, because
        // system resource memory is used for page tables -- and yuzu doesn't really
        // have a way to calculate how much memory is required for page tables for
        // the current process at any given time.
        // TODO: Is this even worth implementing? Games may retrieve this value via
        // an SDK function that gets used + available system resource size for debug
        // or diagnostic purposes. However, it seems unlikely that a game would make
        // decisions based on how much system memory is dedicated to its page tables.
        // Is returning a value other than zero wise?
        return 0;
    }

    /// Whether this process is an AArch64 or AArch32 process.
    bool Is64BitProcess() const {
        return is_64bit_process;
    }

    [[nodiscard]] bool IsSuspended() const {
        return is_suspended;
    }

    void SetSuspended(bool suspended) {
        is_suspended = suspended;
    }

    /// Gets the total running time of the process instance in ticks.
    u64 GetCPUTimeTicks() const {
        return total_process_running_time_ticks;
    }

    /// Updates the total running time, adding the given ticks to it.
    void UpdateCPUTimeTicks(u64 ticks) {
        total_process_running_time_ticks += ticks;
    }

    /// Gets the process schedule count, used for thread yelding
    s64 GetScheduledCount() const {
        return schedule_count;
    }

    /// Increments the process schedule count, used for thread yielding.
    void IncrementScheduledCount() {
        ++schedule_count;
    }

    void IncrementRunningThreadCount();
    void DecrementRunningThreadCount();

    void SetRunningThread(s32 core, KThread* thread, u64 idle_count) {
        running_threads[core] = thread;
        running_thread_idle_counts[core] = idle_count;
    }

    void ClearRunningThread(KThread* thread) {
        for (size_t i = 0; i < running_threads.size(); ++i) {
            if (running_threads[i] == thread) {
                running_threads[i] = nullptr;
            }
        }
    }

    [[nodiscard]] KThread* GetRunningThread(s32 core) const {
        return running_threads[core];
    }

    bool ReleaseUserException(KThread* thread);

    [[nodiscard]] KThread* GetPinnedThread(s32 core_id) const {
        ASSERT(0 <= core_id && core_id < static_cast<s32>(Core::Hardware::NUM_CPU_CORES));
        return pinned_threads[core_id];
    }

    /// Gets 8 bytes of random data for svcGetInfo RandomEntropy
    u64 GetRandomEntropy(std::size_t index) const {
        return random_entropy.at(index);
    }

    /// Retrieves the total physical memory available to this process in bytes.
    u64 GetTotalPhysicalMemoryAvailable();

    /// Retrieves the total physical memory available to this process in bytes,
    /// without the size of the personal system resource heap added to it.
    u64 GetTotalPhysicalMemoryAvailableWithoutSystemResource();

    /// Retrieves the total physical memory used by this process in bytes.
    u64 GetTotalPhysicalMemoryUsed();

    /// Retrieves the total physical memory used by this process in bytes,
    /// without the size of the personal system resource heap added to it.
    u64 GetTotalPhysicalMemoryUsedWithoutSystemResource();

    /// Gets the list of all threads created with this process as their owner.
    std::list<KThread*>& GetThreadList() {
        return thread_list;
    }

    /// Registers a thread as being created under this process,
    /// adding it to this process' thread list.
    void RegisterThread(KThread* thread);

    /// Unregisters a thread from this process, removing it
    /// from this process' thread list.
    void UnregisterThread(KThread* thread);

    /// Clears the signaled state of the process if and only if it's signaled.
    ///
    /// @pre The process must not be already terminated. If this is called on a
    ///      terminated process, then ERR_INVALID_STATE will be returned.
    ///
    /// @pre The process must be in a signaled state. If this is called on a
    ///      process instance that is not signaled, ERR_INVALID_STATE will be
    ///      returned.
    Result Reset();

    /**
     * Loads process-specifics configuration info with metadata provided
     * by an executable.
     *
     * @param metadata The provided metadata to load process specific info from.
     *
     * @returns ResultSuccess if all relevant metadata was able to be
     *          loaded and parsed. Otherwise, an error code is returned.
     */
    Result LoadFromMetadata(const FileSys::ProgramMetadata& metadata, std::size_t code_size);

    /**
     * Starts the main application thread for this process.
     *
     * @param main_thread_priority The priority for the main thread.
     * @param stack_size           The stack size for the main thread in bytes.
     */
    void Run(s32 main_thread_priority, u64 stack_size);

    /**
     * Prepares a process for termination by stopping all of its threads
     * and clearing any other resources.
     */
    void PrepareForTermination();

    void LoadModule(CodeSet code_set, VAddr base_addr);

    bool IsInitialized() const override {
        return is_initialized;
    }

    static void PostDestroy([[maybe_unused]] uintptr_t arg) {}

    void Finalize() override;

    u64 GetId() const override {
        return GetProcessID();
    }

    bool IsSignaled() const override;

    void DoWorkerTaskImpl();

    Result SetActivity(ProcessActivity activity);

    void PinCurrentThread(s32 core_id);
    void UnpinCurrentThread(s32 core_id);
    void UnpinThread(KThread* thread);

    KLightLock& GetStateLock() {
        return state_lock;
    }

    Result AddSharedMemory(KSharedMemory* shmem, VAddr address, size_t size);
    void RemoveSharedMemory(KSharedMemory* shmem, VAddr address, size_t size);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Thread-local storage management

    // Marks the next available region as used and returns the address of the slot.
    [[nodiscard]] Result CreateThreadLocalRegion(VAddr* out);

    // Frees a used TLS slot identified by the given address
    Result DeleteThreadLocalRegion(VAddr addr);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Debug watchpoint management

    // Attempts to insert a watchpoint into a free slot. Returns false if none are available.
    bool InsertWatchpoint(Core::System& system, VAddr addr, u64 size, DebugWatchpointType type);

    // Attempts to remove the watchpoint specified by the given parameters.
    bool RemoveWatchpoint(Core::System& system, VAddr addr, u64 size, DebugWatchpointType type);

    const std::array<DebugWatchpoint, Core::Hardware::NUM_WATCHPOINTS>& GetWatchpoints() const {
        return watchpoints;
    }

private:
    void PinThread(s32 core_id, KThread* thread) {
        ASSERT(0 <= core_id && core_id < static_cast<s32>(Core::Hardware::NUM_CPU_CORES));
        ASSERT(thread != nullptr);
        ASSERT(pinned_threads[core_id] == nullptr);
        pinned_threads[core_id] = thread;
    }

    void UnpinThread(s32 core_id, KThread* thread) {
        ASSERT(0 <= core_id && core_id < static_cast<s32>(Core::Hardware::NUM_CPU_CORES));
        ASSERT(thread != nullptr);
        ASSERT(pinned_threads[core_id] == thread);
        pinned_threads[core_id] = nullptr;
    }

    void FinalizeHandleTable() {
        // Finalize the table.
        handle_table.Finalize();

        // Note that the table is finalized.
        is_handle_table_initialized = false;
    }

    void ChangeState(State new_state);

    /// Allocates the main thread stack for the process, given the stack size in bytes.
    Result AllocateMainThreadStack(std::size_t stack_size);

    /// Memory manager for this process
    KPageTable page_table;

    /// Current status of the process
    State state{};

    /// The ID of this process
    u64 process_id = 0;

    /// Title ID corresponding to the process
    u64 program_id = 0;

    /// Specifies additional memory to be reserved for the process's memory management by the
    /// system. When this is non-zero, secure memory is allocated and used for page table allocation
    /// instead of using the normal global page tables/memory block management.
    u32 system_resource_size = 0;

    /// Resource limit descriptor for this process
    KResourceLimit* resource_limit{};

    VAddr system_resource_address{};

    /// The ideal CPU core for this process, threads are scheduled on this core by default.
    u8 ideal_core = 0;

    /// Contains the parsed process capability descriptors.
    ProcessCapabilities capabilities;

    /// Whether or not this process is AArch64, or AArch32.
    /// By default, we currently assume this is true, unless otherwise
    /// specified by metadata provided to the process during loading.
    bool is_64bit_process = true;

    /// Total running time for the process in ticks.
    std::atomic<u64> total_process_running_time_ticks = 0;

    /// Per-process handle table for storing created object handles in.
    KHandleTable handle_table;

    /// Per-process address arbiter.
    KAddressArbiter address_arbiter;

    /// The per-process mutex lock instance used for handling various
    /// forms of services, such as lock arbitration, and condition
    /// variable related facilities.
    KConditionVariable condition_var;

    /// Address indicating the location of the process' dedicated TLS region.
    VAddr plr_address = 0;

    /// Random values for svcGetInfo RandomEntropy
    std::array<u64, RANDOM_ENTROPY_SIZE> random_entropy{};

    /// List of threads that are running with this process as their owner.
    std::list<KThread*> thread_list;

    /// List of shared memory that are running with this process as their owner.
    std::list<KSharedMemoryInfo*> shared_memory_list;

    /// Address of the top of the main thread's stack
    VAddr main_thread_stack_top{};

    /// Size of the main thread's stack
    std::size_t main_thread_stack_size{};

    /// Memory usage capacity for the process
    std::size_t memory_usage_capacity{};

    /// Process total image size
    std::size_t image_size{};

    /// Schedule count of this process
    s64 schedule_count{};

    size_t memory_release_hint{};

    bool is_signaled{};
    bool is_suspended{};
    bool is_immortal{};
    bool is_handle_table_initialized{};
    bool is_initialized{};

    std::atomic<u16> num_running_threads{};

    std::array<KThread*, Core::Hardware::NUM_CPU_CORES> running_threads{};
    std::array<u64, Core::Hardware::NUM_CPU_CORES> running_thread_idle_counts{};
    std::array<KThread*, Core::Hardware::NUM_CPU_CORES> pinned_threads{};
    std::array<DebugWatchpoint, Core::Hardware::NUM_WATCHPOINTS> watchpoints{};
    std::map<VAddr, u64> debug_page_refcounts;

    KThread* exception_thread{};

    KLightLock state_lock;
    KLightLock list_lock;

    using TLPTree =
        Common::IntrusiveRedBlackTreeBaseTraits<KThreadLocalPage>::TreeType<KThreadLocalPage>;
    using TLPIterator = TLPTree::iterator;
    TLPTree fully_used_tlp_tree;
    TLPTree partially_used_tlp_tree;
};

} // namespace Kernel
