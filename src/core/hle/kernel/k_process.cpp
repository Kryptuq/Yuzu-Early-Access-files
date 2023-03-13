// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bitset>
#include <ctime>
#include <memory>
#include <random>
#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/file_sys/program_metadata.h"
#include "core/hle/kernel/code_set.h"
#include "core/hle/kernel/k_memory_block_manager.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/k_shared_memory_info.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"
#include "core/memory.h"

namespace Kernel {
namespace {
/**
 * Sets up the primary application thread
 *
 * @param system The system instance to create the main thread under.
 * @param owner_process The parent process for the main thread
 * @param priority The priority to give the main thread
 */
void SetupMainThread(Core::System& system, KProcess& owner_process, u32 priority, VAddr stack_top) {
    const VAddr entry_point = owner_process.PageTable().GetCodeRegionStart();
    ASSERT(owner_process.GetResourceLimit()->Reserve(LimitableResource::ThreadCountMax, 1));

    KThread* thread = KThread::Create(system.Kernel());
    SCOPE_EXIT({ thread->Close(); });

    ASSERT(KThread::InitializeUserThread(system, thread, entry_point, 0, stack_top, priority,
                                         owner_process.GetIdealCoreId(),
                                         std::addressof(owner_process))
               .IsSuccess());

    // Register 1 must be a handle to the main thread
    Handle thread_handle{};
    owner_process.GetHandleTable().Add(std::addressof(thread_handle), thread);

    thread->GetContext32().cpu_registers[0] = 0;
    thread->GetContext64().cpu_registers[0] = 0;
    thread->GetContext32().cpu_registers[1] = thread_handle;
    thread->GetContext64().cpu_registers[1] = thread_handle;

    if (system.DebuggerEnabled()) {
        thread->RequestSuspend(SuspendType::Debug);
    }

    // Run our thread.
    void(thread->Run());
}
} // Anonymous namespace

Result KProcess::Initialize(KProcess* process, Core::System& system, std::string process_name,
                            ProcessType type, KResourceLimit* res_limit) {
    auto& kernel = system.Kernel();

    process->name = std::move(process_name);
    process->m_resource_limit = res_limit;
    process->m_system_resource_address = 0;
    process->m_state = State::Created;
    process->m_program_id = 0;
    process->m_process_id = type == ProcessType::KernelInternal ? kernel.CreateNewKernelProcessID()
                                                                : kernel.CreateNewUserProcessID();
    process->m_capabilities.InitializeForMetadatalessProcess();
    process->m_is_initialized = true;

    std::mt19937 rng(Settings::values.rng_seed.GetValue().value_or(std::time(nullptr)));
    std::uniform_int_distribution<u64> distribution;
    std::generate(process->m_random_entropy.begin(), process->m_random_entropy.end(),
                  [&] { return distribution(rng); });

    kernel.AppendNewProcess(process);

    // Clear remaining fields.
    process->m_num_running_threads = 0;
    process->m_is_signaled = false;
    process->m_exception_thread = nullptr;
    process->m_is_suspended = false;
    process->m_schedule_count = 0;
    process->m_is_handle_table_initialized = false;

    // Open a reference to the resource limit.
    process->m_resource_limit->Open();

    R_SUCCEED();
}

void KProcess::DoWorkerTaskImpl() {
    UNIMPLEMENTED();
}

KResourceLimit* KProcess::GetResourceLimit() const {
    return m_resource_limit;
}

void KProcess::IncrementRunningThreadCount() {
    ASSERT(m_num_running_threads.load() >= 0);
    ++m_num_running_threads;
}

void KProcess::DecrementRunningThreadCount() {
    ASSERT(m_num_running_threads.load() > 0);

    if (const auto prev = m_num_running_threads--; prev == 1) {
        // TODO(bunnei): Process termination to be implemented when multiprocess is supported.
    }
}

u64 KProcess::GetTotalPhysicalMemoryAvailable() {
    const u64 capacity{m_resource_limit->GetFreeValue(LimitableResource::PhysicalMemoryMax) +
                       m_page_table.GetNormalMemorySize() + GetSystemResourceSize() + m_image_size +
                       m_main_thread_stack_size};
    if (const auto pool_size = m_kernel.MemoryManager().GetSize(KMemoryManager::Pool::Application);
        capacity != pool_size) {
        LOG_WARNING(Kernel, "capacity {} != application pool size {}", capacity, pool_size);
    }
    if (capacity < m_memory_usage_capacity) {
        return capacity;
    }
    return m_memory_usage_capacity;
}

u64 KProcess::GetTotalPhysicalMemoryAvailableWithoutSystemResource() {
    return this->GetTotalPhysicalMemoryAvailable() - this->GetSystemResourceSize();
}

u64 KProcess::GetTotalPhysicalMemoryUsed() {
    return m_image_size + m_main_thread_stack_size + m_page_table.GetNormalMemorySize() +
           this->GetSystemResourceSize();
}

u64 KProcess::GetTotalPhysicalMemoryUsedWithoutSystemResource() {
    return this->GetTotalPhysicalMemoryUsed() - this->GetSystemResourceUsage();
}

bool KProcess::ReleaseUserException(KThread* thread) {
    KScopedSchedulerLock sl{m_kernel};

    if (m_exception_thread == thread) {
        m_exception_thread = nullptr;

        // Remove waiter thread.
        bool has_waiters{};
        if (KThread* next = thread->RemoveKernelWaiterByKey(
                std::addressof(has_waiters),
                reinterpret_cast<uintptr_t>(std::addressof(m_exception_thread)));
            next != nullptr) {
            next->EndWait(ResultSuccess);
        }

        KScheduler::SetSchedulerUpdateNeeded(m_kernel);

        return true;
    } else {
        return false;
    }
}

void KProcess::PinCurrentThread(s32 core_id) {
    ASSERT(KScheduler::IsSchedulerLockedByCurrentThread(m_kernel));

    // Get the current thread.
    KThread* cur_thread =
        m_kernel.Scheduler(static_cast<std::size_t>(core_id)).GetSchedulerCurrentThread();

    // If the thread isn't terminated, pin it.
    if (!cur_thread->IsTerminationRequested()) {
        // Pin it.
        this->PinThread(core_id, cur_thread);
        cur_thread->Pin(core_id);

        // An update is needed.
        KScheduler::SetSchedulerUpdateNeeded(m_kernel);
    }
}

void KProcess::UnpinCurrentThread(s32 core_id) {
    ASSERT(KScheduler::IsSchedulerLockedByCurrentThread(m_kernel));

    // Get the current thread.
    KThread* cur_thread =
        m_kernel.Scheduler(static_cast<std::size_t>(core_id)).GetSchedulerCurrentThread();

    // Unpin it.
    cur_thread->Unpin();
    this->UnpinThread(core_id, cur_thread);

    // An update is needed.
    KScheduler::SetSchedulerUpdateNeeded(m_kernel);
}

void KProcess::UnpinThread(KThread* thread) {
    ASSERT(KScheduler::IsSchedulerLockedByCurrentThread(m_kernel));

    // Get the thread's core id.
    const auto core_id = thread->GetActiveCore();

    // Unpin it.
    this->UnpinThread(core_id, thread);
    thread->Unpin();

    // An update is needed.
    KScheduler::SetSchedulerUpdateNeeded(m_kernel);
}

Result KProcess::AddSharedMemory(KSharedMemory* shmem, [[maybe_unused]] VAddr address,
                                 [[maybe_unused]] size_t size) {
    // Lock ourselves, to prevent concurrent access.
    KScopedLightLock lk(m_state_lock);

    // Try to find an existing info for the memory.
    KSharedMemoryInfo* shemen_info = nullptr;
    const auto iter = std::find_if(
        m_shared_memory_list.begin(), m_shared_memory_list.end(),
        [shmem](const KSharedMemoryInfo* info) { return info->GetSharedMemory() == shmem; });
    if (iter != m_shared_memory_list.end()) {
        shemen_info = *iter;
    }

    if (shemen_info == nullptr) {
        shemen_info = KSharedMemoryInfo::Allocate(m_kernel);
        R_UNLESS(shemen_info != nullptr, ResultOutOfMemory);

        shemen_info->Initialize(shmem);
        m_shared_memory_list.push_back(shemen_info);
    }

    // Open a reference to the shared memory and its info.
    shmem->Open();
    shemen_info->Open();

    R_SUCCEED();
}

void KProcess::RemoveSharedMemory(KSharedMemory* shmem, [[maybe_unused]] VAddr address,
                                  [[maybe_unused]] size_t size) {
    // Lock ourselves, to prevent concurrent access.
    KScopedLightLock lk(m_state_lock);

    KSharedMemoryInfo* shemen_info = nullptr;
    const auto iter = std::find_if(
        m_shared_memory_list.begin(), m_shared_memory_list.end(),
        [shmem](const KSharedMemoryInfo* info) { return info->GetSharedMemory() == shmem; });
    if (iter != m_shared_memory_list.end()) {
        shemen_info = *iter;
    }

    ASSERT(shemen_info != nullptr);

    if (shemen_info->Close()) {
        m_shared_memory_list.erase(iter);
        KSharedMemoryInfo::Free(m_kernel, shemen_info);
    }

    // Close a reference to the shared memory.
    shmem->Close();
}

void KProcess::RegisterThread(KThread* thread) {
    KScopedLightLock lk{m_list_lock};

    m_thread_list.push_back(thread);
}

void KProcess::UnregisterThread(KThread* thread) {
    KScopedLightLock lk{m_list_lock};

    m_thread_list.remove(thread);
}

u64 KProcess::GetFreeThreadCount() const {
    if (m_resource_limit != nullptr) {
        const auto current_value =
            m_resource_limit->GetCurrentValue(LimitableResource::ThreadCountMax);
        const auto limit_value = m_resource_limit->GetLimitValue(LimitableResource::ThreadCountMax);
        return limit_value - current_value;
    } else {
        return 0;
    }
}

Result KProcess::Reset() {
    // Lock the process and the scheduler.
    KScopedLightLock lk(m_state_lock);
    KScopedSchedulerLock sl{m_kernel};

    // Validate that we're in a state that we can reset.
    R_UNLESS(m_state != State::Terminated, ResultInvalidState);
    R_UNLESS(m_is_signaled, ResultInvalidState);

    // Clear signaled.
    m_is_signaled = false;
    R_SUCCEED();
}

Result KProcess::SetActivity(ProcessActivity activity) {
    // Lock ourselves and the scheduler.
    KScopedLightLock lk{m_state_lock};
    KScopedLightLock list_lk{m_list_lock};
    KScopedSchedulerLock sl{m_kernel};

    // Validate our state.
    R_UNLESS(m_state != State::Terminating, ResultInvalidState);
    R_UNLESS(m_state != State::Terminated, ResultInvalidState);

    // Either pause or resume.
    if (activity == ProcessActivity::Paused) {
        // Verify that we're not suspended.
        R_UNLESS(!m_is_suspended, ResultInvalidState);

        // Suspend all threads.
        for (auto* thread : this->GetThreadList()) {
            thread->RequestSuspend(SuspendType::Process);
        }

        // Set ourselves as suspended.
        this->SetSuspended(true);
    } else {
        ASSERT(activity == ProcessActivity::Runnable);

        // Verify that we're suspended.
        R_UNLESS(m_is_suspended, ResultInvalidState);

        // Resume all threads.
        for (auto* thread : this->GetThreadList()) {
            thread->Resume(SuspendType::Process);
        }

        // Set ourselves as resumed.
        this->SetSuspended(false);
    }

    R_SUCCEED();
}

Result KProcess::LoadFromMetadata(const FileSys::ProgramMetadata& metadata, std::size_t code_size) {
    m_program_id = metadata.GetTitleID();
    m_ideal_core = metadata.GetMainThreadCore();
    m_is_64bit_process = metadata.Is64BitProgram();
    m_system_resource_size = metadata.GetSystemResourceSize();
    m_image_size = code_size;

    KScopedResourceReservation memory_reservation(
        m_resource_limit, LimitableResource::PhysicalMemoryMax, code_size + m_system_resource_size);
    if (!memory_reservation.Succeeded()) {
        LOG_ERROR(Kernel, "Could not reserve process memory requirements of size {:X} bytes",
                  code_size + m_system_resource_size);
        R_RETURN(ResultLimitReached);
    }
    // Initialize process address space
    if (const Result result{m_page_table.InitializeForProcess(
            metadata.GetAddressSpaceType(), false, false, false, KMemoryManager::Pool::Application,
            0x8000000, code_size, std::addressof(m_kernel.GetAppSystemResource()),
            m_resource_limit)};
        result.IsError()) {
        R_RETURN(result);
    }

    // Map process code region
    if (const Result result{m_page_table.MapProcessCode(m_page_table.GetCodeRegionStart(),
                                                        code_size / PageSize, KMemoryState::Code,
                                                        KMemoryPermission::None)};
        result.IsError()) {
        R_RETURN(result);
    }

    // Initialize process capabilities
    const auto& caps{metadata.GetKernelCapabilities()};
    if (const Result result{
            m_capabilities.InitializeForUserProcess(caps.data(), caps.size(), m_page_table)};
        result.IsError()) {
        R_RETURN(result);
    }

    // Set memory usage capacity
    switch (metadata.GetAddressSpaceType()) {
    case FileSys::ProgramAddressSpaceType::Is32Bit:
    case FileSys::ProgramAddressSpaceType::Is36Bit:
    case FileSys::ProgramAddressSpaceType::Is39Bit:
        m_memory_usage_capacity =
            m_page_table.GetHeapRegionEnd() - m_page_table.GetHeapRegionStart();
        break;

    case FileSys::ProgramAddressSpaceType::Is32BitNoMap:
        m_memory_usage_capacity =
            m_page_table.GetHeapRegionEnd() - m_page_table.GetHeapRegionStart() +
            m_page_table.GetAliasRegionEnd() - m_page_table.GetAliasRegionStart();
        break;

    default:
        ASSERT(false);
        break;
    }

    // Create TLS region
    R_TRY(this->CreateThreadLocalRegion(std::addressof(m_plr_address)));
    memory_reservation.Commit();

    R_RETURN(m_handle_table.Initialize(m_capabilities.GetHandleTableSize()));
}

void KProcess::Run(s32 main_thread_priority, u64 stack_size) {
    ASSERT(this->AllocateMainThreadStack(stack_size) == ResultSuccess);
    m_resource_limit->Reserve(LimitableResource::ThreadCountMax, 1);

    const std::size_t heap_capacity{m_memory_usage_capacity -
                                    (m_main_thread_stack_size + m_image_size)};
    ASSERT(!m_page_table.SetMaxHeapSize(heap_capacity).IsError());

    this->ChangeState(State::Running);

    SetupMainThread(m_kernel.System(), *this, main_thread_priority, m_main_thread_stack_top);
}

void KProcess::PrepareForTermination() {
    this->ChangeState(State::Terminating);

    const auto stop_threads = [this](const std::vector<KThread*>& in_thread_list) {
        for (auto* thread : in_thread_list) {
            if (thread->GetOwnerProcess() != this)
                continue;

            if (thread == GetCurrentThreadPointer(m_kernel))
                continue;

            // TODO(Subv): When are the other running/ready threads terminated?
            ASSERT_MSG(thread->GetState() == ThreadState::Waiting,
                       "Exiting processes with non-waiting threads is currently unimplemented");

            thread->Exit();
        }
    };

    stop_threads(m_kernel.System().GlobalSchedulerContext().GetThreadList());

    this->DeleteThreadLocalRegion(m_plr_address);
    m_plr_address = 0;

    if (m_resource_limit) {
        m_resource_limit->Release(LimitableResource::PhysicalMemoryMax,
                                  m_main_thread_stack_size + m_image_size);
    }

    this->ChangeState(State::Terminated);
}

void KProcess::Finalize() {
    // Free all shared memory infos.
    {
        auto it = m_shared_memory_list.begin();
        while (it != m_shared_memory_list.end()) {
            KSharedMemoryInfo* info = *it;
            KSharedMemory* shmem = info->GetSharedMemory();

            while (!info->Close()) {
                shmem->Close();
            }

            shmem->Close();

            it = m_shared_memory_list.erase(it);
            KSharedMemoryInfo::Free(m_kernel, info);
        }
    }

    // Release memory to the resource limit.
    if (m_resource_limit != nullptr) {
        m_resource_limit->Close();
        m_resource_limit = nullptr;
    }

    // Finalize the page table.
    m_page_table.Finalize();

    // Perform inherited finalization.
    KSynchronizationObject::Finalize();
}

Result KProcess::CreateThreadLocalRegion(VAddr* out) {
    KThreadLocalPage* tlp = nullptr;
    VAddr tlr = 0;

    // See if we can get a region from a partially used TLP.
    {
        KScopedSchedulerLock sl{m_kernel};

        if (auto it = m_partially_used_tlp_tree.begin(); it != m_partially_used_tlp_tree.end()) {
            tlr = it->Reserve();
            ASSERT(tlr != 0);

            if (it->IsAllUsed()) {
                tlp = std::addressof(*it);
                m_partially_used_tlp_tree.erase(it);
                m_fully_used_tlp_tree.insert(*tlp);
            }

            *out = tlr;
            R_SUCCEED();
        }
    }

    // Allocate a new page.
    tlp = KThreadLocalPage::Allocate(m_kernel);
    R_UNLESS(tlp != nullptr, ResultOutOfMemory);
    auto tlp_guard = SCOPE_GUARD({ KThreadLocalPage::Free(m_kernel, tlp); });

    // Initialize the new page.
    R_TRY(tlp->Initialize(m_kernel, this));

    // Reserve a TLR.
    tlr = tlp->Reserve();
    ASSERT(tlr != 0);

    // Insert into our tree.
    {
        KScopedSchedulerLock sl{m_kernel};
        if (tlp->IsAllUsed()) {
            m_fully_used_tlp_tree.insert(*tlp);
        } else {
            m_partially_used_tlp_tree.insert(*tlp);
        }
    }

    // We succeeded!
    tlp_guard.Cancel();
    *out = tlr;
    R_SUCCEED();
}

Result KProcess::DeleteThreadLocalRegion(VAddr addr) {
    KThreadLocalPage* page_to_free = nullptr;

    // Release the region.
    {
        KScopedSchedulerLock sl{m_kernel};

        // Try to find the page in the partially used list.
        auto it = m_partially_used_tlp_tree.find_key(Common::AlignDown(addr, PageSize));
        if (it == m_partially_used_tlp_tree.end()) {
            // If we don't find it, it has to be in the fully used list.
            it = m_fully_used_tlp_tree.find_key(Common::AlignDown(addr, PageSize));
            R_UNLESS(it != m_fully_used_tlp_tree.end(), ResultInvalidAddress);

            // Release the region.
            it->Release(addr);

            // Move the page out of the fully used list.
            KThreadLocalPage* tlp = std::addressof(*it);
            m_fully_used_tlp_tree.erase(it);
            if (tlp->IsAllFree()) {
                page_to_free = tlp;
            } else {
                m_partially_used_tlp_tree.insert(*tlp);
            }
        } else {
            // Release the region.
            it->Release(addr);

            // Handle the all-free case.
            KThreadLocalPage* tlp = std::addressof(*it);
            if (tlp->IsAllFree()) {
                m_partially_used_tlp_tree.erase(it);
                page_to_free = tlp;
            }
        }
    }

    // If we should free the page it was in, do so.
    if (page_to_free != nullptr) {
        page_to_free->Finalize();

        KThreadLocalPage::Free(m_kernel, page_to_free);
    }

    R_SUCCEED();
}

bool KProcess::InsertWatchpoint(Core::System& system, VAddr addr, u64 size,
                                DebugWatchpointType type) {
    const auto watch{std::find_if(m_watchpoints.begin(), m_watchpoints.end(), [&](const auto& wp) {
        return wp.type == DebugWatchpointType::None;
    })};

    if (watch == m_watchpoints.end()) {
        return false;
    }

    watch->start_address = addr;
    watch->end_address = addr + size;
    watch->type = type;

    for (VAddr page = Common::AlignDown(addr, PageSize); page < addr + size; page += PageSize) {
        m_debug_page_refcounts[page]++;
        system.Memory().MarkRegionDebug(page, PageSize, true);
    }

    return true;
}

bool KProcess::RemoveWatchpoint(Core::System& system, VAddr addr, u64 size,
                                DebugWatchpointType type) {
    const auto watch{std::find_if(m_watchpoints.begin(), m_watchpoints.end(), [&](const auto& wp) {
        return wp.start_address == addr && wp.end_address == addr + size && wp.type == type;
    })};

    if (watch == m_watchpoints.end()) {
        return false;
    }

    watch->start_address = 0;
    watch->end_address = 0;
    watch->type = DebugWatchpointType::None;

    for (VAddr page = Common::AlignDown(addr, PageSize); page < addr + size; page += PageSize) {
        m_debug_page_refcounts[page]--;
        if (!m_debug_page_refcounts[page]) {
            system.Memory().MarkRegionDebug(page, PageSize, false);
        }
    }

    return true;
}

void KProcess::LoadModule(CodeSet code_set, VAddr base_addr) {
    const auto ReprotectSegment = [&](const CodeSet::Segment& segment,
                                      Svc::MemoryPermission permission) {
        m_page_table.SetProcessMemoryPermission(segment.addr + base_addr, segment.size, permission);
    };

    m_kernel.System().Memory().WriteBlock(*this, base_addr, code_set.memory.data(),
                                          code_set.memory.size());

    ReprotectSegment(code_set.CodeSegment(), Svc::MemoryPermission::ReadExecute);
    ReprotectSegment(code_set.RODataSegment(), Svc::MemoryPermission::Read);
    ReprotectSegment(code_set.DataSegment(), Svc::MemoryPermission::ReadWrite);
}

bool KProcess::IsSignaled() const {
    ASSERT(KScheduler::IsSchedulerLockedByCurrentThread(m_kernel));
    return m_is_signaled;
}

KProcess::KProcess(KernelCore& kernel)
    : KAutoObjectWithSlabHeapAndContainer{kernel}, m_page_table{m_kernel.System()},
      m_handle_table{m_kernel}, m_address_arbiter{m_kernel.System()},
      m_condition_var{m_kernel.System()}, m_state_lock{m_kernel}, m_list_lock{m_kernel} {}

KProcess::~KProcess() = default;

void KProcess::ChangeState(State new_state) {
    if (m_state == new_state) {
        return;
    }

    m_state = new_state;
    m_is_signaled = true;
    this->NotifyAvailable();
}

Result KProcess::AllocateMainThreadStack(std::size_t stack_size) {
    // Ensure that we haven't already allocated stack.
    ASSERT(m_main_thread_stack_size == 0);

    // Ensure that we're allocating a valid stack.
    stack_size = Common::AlignUp(stack_size, PageSize);
    // R_UNLESS(stack_size + image_size <= m_max_process_memory, ResultOutOfMemory);
    R_UNLESS(stack_size + m_image_size >= m_image_size, ResultOutOfMemory);

    // Place a tentative reservation of memory for our new stack.
    KScopedResourceReservation mem_reservation(this, Svc::LimitableResource::PhysicalMemoryMax,
                                               stack_size);
    R_UNLESS(mem_reservation.Succeeded(), ResultLimitReached);

    // Allocate and map our stack.
    if (stack_size) {
        KProcessAddress stack_bottom;
        R_TRY(m_page_table.MapPages(std::addressof(stack_bottom), stack_size / PageSize,
                                    KMemoryState::Stack, KMemoryPermission::UserReadWrite));

        m_main_thread_stack_top = stack_bottom + stack_size;
        m_main_thread_stack_size = stack_size;
    }

    // We succeeded! Commit our memory reservation.
    mem_reservation.Commit();

    R_SUCCEED();
}

} // namespace Kernel
