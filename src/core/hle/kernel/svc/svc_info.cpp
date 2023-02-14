// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/svc.h"

namespace Kernel::Svc {

/// Gets system/memory information for the current process
Result GetInfo(Core::System& system, u64* result, InfoType info_id_type, Handle handle,
               u64 info_sub_id) {
    LOG_TRACE(Kernel_SVC, "called info_id=0x{:X}, info_sub_id=0x{:X}, handle=0x{:08X}",
              info_id_type, info_sub_id, handle);

    u32 info_id = static_cast<u32>(info_id_type);

    switch (info_id_type) {
    case InfoType::CoreMask:
    case InfoType::PriorityMask:
    case InfoType::AliasRegionAddress:
    case InfoType::AliasRegionSize:
    case InfoType::HeapRegionAddress:
    case InfoType::HeapRegionSize:
    case InfoType::AslrRegionAddress:
    case InfoType::AslrRegionSize:
    case InfoType::StackRegionAddress:
    case InfoType::StackRegionSize:
    case InfoType::TotalMemorySize:
    case InfoType::UsedMemorySize:
    case InfoType::SystemResourceSizeTotal:
    case InfoType::SystemResourceSizeUsed:
    case InfoType::ProgramId:
    case InfoType::UserExceptionContextAddress:
    case InfoType::TotalNonSystemMemorySize:
    case InfoType::UsedNonSystemMemorySize:
    case InfoType::IsApplication:
    case InfoType::FreeThreadCount: {
        if (info_sub_id != 0) {
            LOG_ERROR(Kernel_SVC, "Info sub id is non zero! info_id={}, info_sub_id={}", info_id,
                      info_sub_id);
            return ResultInvalidEnumValue;
        }

        const auto& handle_table = GetCurrentProcess(system.Kernel()).GetHandleTable();
        KScopedAutoObject process = handle_table.GetObject<KProcess>(handle);
        if (process.IsNull()) {
            LOG_ERROR(Kernel_SVC, "Process is not valid! info_id={}, info_sub_id={}, handle={:08X}",
                      info_id, info_sub_id, handle);
            return ResultInvalidHandle;
        }

        switch (info_id_type) {
        case InfoType::CoreMask:
            *result = process->GetCoreMask();
            return ResultSuccess;

        case InfoType::PriorityMask:
            *result = process->GetPriorityMask();
            return ResultSuccess;

        case InfoType::AliasRegionAddress:
            *result = process->PageTable().GetAliasRegionStart();
            return ResultSuccess;

        case InfoType::AliasRegionSize:
            *result = process->PageTable().GetAliasRegionSize();
            return ResultSuccess;

        case InfoType::HeapRegionAddress:
            *result = process->PageTable().GetHeapRegionStart();
            return ResultSuccess;

        case InfoType::HeapRegionSize:
            *result = process->PageTable().GetHeapRegionSize();
            return ResultSuccess;

        case InfoType::AslrRegionAddress:
            *result = process->PageTable().GetAliasCodeRegionStart();
            return ResultSuccess;

        case InfoType::AslrRegionSize:
            *result = process->PageTable().GetAliasCodeRegionSize();
            return ResultSuccess;

        case InfoType::StackRegionAddress:
            *result = process->PageTable().GetStackRegionStart();
            return ResultSuccess;

        case InfoType::StackRegionSize:
            *result = process->PageTable().GetStackRegionSize();
            return ResultSuccess;

        case InfoType::TotalMemorySize:
            *result = process->GetTotalPhysicalMemoryAvailable();
            return ResultSuccess;

        case InfoType::UsedMemorySize:
            *result = process->GetTotalPhysicalMemoryUsed();
            return ResultSuccess;

        case InfoType::SystemResourceSizeTotal:
            *result = process->GetSystemResourceSize();
            return ResultSuccess;

        case InfoType::SystemResourceSizeUsed:
            LOG_WARNING(Kernel_SVC, "(STUBBED) Attempted to query system resource usage");
            *result = process->GetSystemResourceUsage();
            return ResultSuccess;

        case InfoType::ProgramId:
            *result = process->GetProgramID();
            return ResultSuccess;

        case InfoType::UserExceptionContextAddress:
            *result = process->GetProcessLocalRegionAddress();
            return ResultSuccess;

        case InfoType::TotalNonSystemMemorySize:
            *result = process->GetTotalPhysicalMemoryAvailableWithoutSystemResource();
            return ResultSuccess;

        case InfoType::UsedNonSystemMemorySize:
            *result = process->GetTotalPhysicalMemoryUsedWithoutSystemResource();
            return ResultSuccess;

        case InfoType::FreeThreadCount:
            *result = process->GetFreeThreadCount();
            return ResultSuccess;

        default:
            break;
        }

        LOG_ERROR(Kernel_SVC, "Unimplemented svcGetInfo id=0x{:016X}", info_id);
        return ResultInvalidEnumValue;
    }

    case InfoType::DebuggerAttached:
        *result = 0;
        return ResultSuccess;

    case InfoType::ResourceLimit: {
        if (handle != 0) {
            LOG_ERROR(Kernel, "Handle is non zero! handle={:08X}", handle);
            return ResultInvalidHandle;
        }

        if (info_sub_id != 0) {
            LOG_ERROR(Kernel, "Info sub id is non zero! info_id={}, info_sub_id={}", info_id,
                      info_sub_id);
            return ResultInvalidCombination;
        }

        KProcess* const current_process = GetCurrentProcessPointer(system.Kernel());
        KHandleTable& handle_table = current_process->GetHandleTable();
        const auto resource_limit = current_process->GetResourceLimit();
        if (!resource_limit) {
            *result = Svc::InvalidHandle;
            // Yes, the kernel considers this a successful operation.
            return ResultSuccess;
        }

        Handle resource_handle{};
        R_TRY(handle_table.Add(&resource_handle, resource_limit));

        *result = resource_handle;
        return ResultSuccess;
    }

    case InfoType::RandomEntropy:
        if (handle != 0) {
            LOG_ERROR(Kernel_SVC, "Process Handle is non zero, expected 0 result but got {:016X}",
                      handle);
            return ResultInvalidHandle;
        }

        if (info_sub_id >= KProcess::RANDOM_ENTROPY_SIZE) {
            LOG_ERROR(Kernel_SVC, "Entropy size is out of range, expected {} but got {}",
                      KProcess::RANDOM_ENTROPY_SIZE, info_sub_id);
            return ResultInvalidCombination;
        }

        *result = GetCurrentProcess(system.Kernel()).GetRandomEntropy(info_sub_id);
        return ResultSuccess;

    case InfoType::InitialProcessIdRange:
        LOG_WARNING(Kernel_SVC,
                    "(STUBBED) Attempted to query privileged process id bounds, returned 0");
        *result = 0;
        return ResultSuccess;

    case InfoType::ThreadTickCount: {
        constexpr u64 num_cpus = 4;
        if (info_sub_id != 0xFFFFFFFFFFFFFFFF && info_sub_id >= num_cpus) {
            LOG_ERROR(Kernel_SVC, "Core count is out of range, expected {} but got {}", num_cpus,
                      info_sub_id);
            return ResultInvalidCombination;
        }

        KScopedAutoObject thread = GetCurrentProcess(system.Kernel())
                                       .GetHandleTable()
                                       .GetObject<KThread>(static_cast<Handle>(handle));
        if (thread.IsNull()) {
            LOG_ERROR(Kernel_SVC, "Thread handle does not exist, handle=0x{:08X}",
                      static_cast<Handle>(handle));
            return ResultInvalidHandle;
        }

        const auto& core_timing = system.CoreTiming();
        const auto& scheduler = *system.Kernel().CurrentScheduler();
        const auto* const current_thread = GetCurrentThreadPointer(system.Kernel());
        const bool same_thread = current_thread == thread.GetPointerUnsafe();

        const u64 prev_ctx_ticks = scheduler.GetLastContextSwitchTime();
        u64 out_ticks = 0;
        if (same_thread && info_sub_id == 0xFFFFFFFFFFFFFFFF) {
            const u64 thread_ticks = current_thread->GetCpuTime();

            out_ticks = thread_ticks + (core_timing.GetCPUTicks() - prev_ctx_ticks);
        } else if (same_thread && info_sub_id == system.Kernel().CurrentPhysicalCoreIndex()) {
            out_ticks = core_timing.GetCPUTicks() - prev_ctx_ticks;
        }

        *result = out_ticks;
        return ResultSuccess;
    }
    case InfoType::IdleTickCount: {
        // Verify the input handle is invalid.
        R_UNLESS(handle == InvalidHandle, ResultInvalidHandle);

        // Verify the requested core is valid.
        const bool core_valid =
            (info_sub_id == 0xFFFFFFFFFFFFFFFF) ||
            (info_sub_id == static_cast<u64>(system.Kernel().CurrentPhysicalCoreIndex()));
        R_UNLESS(core_valid, ResultInvalidCombination);

        // Get the idle tick count.
        *result = system.Kernel().CurrentScheduler()->GetIdleThread()->GetCpuTime();
        return ResultSuccess;
    }
    case InfoType::MesosphereCurrentProcess: {
        // Verify the input handle is invalid.
        R_UNLESS(handle == InvalidHandle, ResultInvalidHandle);

        // Verify the sub-type is valid.
        R_UNLESS(info_sub_id == 0, ResultInvalidCombination);

        // Get the handle table.
        KProcess* current_process = GetCurrentProcessPointer(system.Kernel());
        KHandleTable& handle_table = current_process->GetHandleTable();

        // Get a new handle for the current process.
        Handle tmp;
        R_TRY(handle_table.Add(&tmp, current_process));

        // Set the output.
        *result = tmp;

        // We succeeded.
        return ResultSuccess;
    }
    default:
        LOG_ERROR(Kernel_SVC, "Unimplemented svcGetInfo id=0x{:016X}", info_id);
        return ResultInvalidEnumValue;
    }
}

Result GetSystemInfo(Core::System& system, uint64_t* out, SystemInfoType info_type, Handle handle,
                     uint64_t info_subtype) {
    UNIMPLEMENTED();
    R_THROW(ResultNotImplemented);
}

Result GetInfo64(Core::System& system, uint64_t* out, InfoType info_type, Handle handle,
                 uint64_t info_subtype) {
    R_RETURN(GetInfo(system, out, info_type, handle, info_subtype));
}

Result GetSystemInfo64(Core::System& system, uint64_t* out, SystemInfoType info_type, Handle handle,
                       uint64_t info_subtype) {
    R_RETURN(GetSystemInfo(system, out, info_type, handle, info_subtype));
}

Result GetInfo64From32(Core::System& system, uint64_t* out, InfoType info_type, Handle handle,
                       uint64_t info_subtype) {
    R_RETURN(GetInfo(system, out, info_type, handle, info_subtype));
}

Result GetSystemInfo64From32(Core::System& system, uint64_t* out, SystemInfoType info_type,
                             Handle handle, uint64_t info_subtype) {
    R_RETURN(GetSystemInfo(system, out, info_type, handle, info_subtype));
}

} // namespace Kernel::Svc
