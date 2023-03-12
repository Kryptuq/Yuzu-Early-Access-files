// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/overflow.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {
constexpr s64 DefaultTimeout = 10000000000; // 10 seconds

KResourceLimit::KResourceLimit(KernelCore& kernel)
    : KAutoObjectWithSlabHeapAndContainer{kernel}, m_lock{m_kernel}, m_cond_var{m_kernel} {}
KResourceLimit::~KResourceLimit() = default;

void KResourceLimit::Initialize(const Core::Timing::CoreTiming* core_timing) {
    m_core_timing = core_timing;
}

void KResourceLimit::Finalize() {}

s64 KResourceLimit::GetLimitValue(LimitableResource which) const {
    const auto index = static_cast<std::size_t>(which);
    s64 value{};
    {
        KScopedLightLock lk{m_lock};
        value = m_limit_values[index];
        ASSERT(value >= 0);
        ASSERT(m_current_values[index] <= m_limit_values[index]);
        ASSERT(m_current_hints[index] <= m_current_values[index]);
    }
    return value;
}

s64 KResourceLimit::GetCurrentValue(LimitableResource which) const {
    const auto index = static_cast<std::size_t>(which);
    s64 value{};
    {
        KScopedLightLock lk{m_lock};
        value = m_current_values[index];
        ASSERT(value >= 0);
        ASSERT(m_current_values[index] <= m_limit_values[index]);
        ASSERT(m_current_hints[index] <= m_current_values[index]);
    }
    return value;
}

s64 KResourceLimit::GetPeakValue(LimitableResource which) const {
    const auto index = static_cast<std::size_t>(which);
    s64 value{};
    {
        KScopedLightLock lk{m_lock};
        value = m_peak_values[index];
        ASSERT(value >= 0);
        ASSERT(m_current_values[index] <= m_limit_values[index]);
        ASSERT(m_current_hints[index] <= m_current_values[index]);
    }
    return value;
}

s64 KResourceLimit::GetFreeValue(LimitableResource which) const {
    const auto index = static_cast<std::size_t>(which);
    s64 value{};
    {
        KScopedLightLock lk(m_lock);
        ASSERT(m_current_values[index] >= 0);
        ASSERT(m_current_values[index] <= m_limit_values[index]);
        ASSERT(m_current_hints[index] <= m_current_values[index]);
        value = m_limit_values[index] - m_current_values[index];
    }

    return value;
}

Result KResourceLimit::SetLimitValue(LimitableResource which, s64 value) {
    const auto index = static_cast<std::size_t>(which);
    KScopedLightLock lk(m_lock);
    R_UNLESS(m_current_values[index] <= value, ResultInvalidState);

    m_limit_values[index] = value;
    m_peak_values[index] = m_current_values[index];

    R_SUCCEED();
}

bool KResourceLimit::Reserve(LimitableResource which, s64 value) {
    return Reserve(which, value, m_core_timing->GetGlobalTimeNs().count() + DefaultTimeout);
}

bool KResourceLimit::Reserve(LimitableResource which, s64 value, s64 timeout) {
    ASSERT(value >= 0);
    const auto index = static_cast<std::size_t>(which);
    KScopedLightLock lk(m_lock);

    ASSERT(m_current_hints[index] <= m_current_values[index]);
    if (m_current_hints[index] >= m_limit_values[index]) {
        return false;
    }

    // Loop until we reserve or run out of time.
    while (true) {
        ASSERT(m_current_values[index] <= m_limit_values[index]);
        ASSERT(m_current_hints[index] <= m_current_values[index]);

        // If we would overflow, don't allow to succeed.
        if (Common::WrappingAdd(m_current_values[index], value) <= m_current_values[index]) {
            break;
        }

        if (m_current_values[index] + value <= m_limit_values[index]) {
            m_current_values[index] += value;
            m_current_hints[index] += value;
            m_peak_values[index] = std::max(m_peak_values[index], m_current_values[index]);
            return true;
        }

        if (m_current_hints[index] + value <= m_limit_values[index] &&
            (timeout < 0 || m_core_timing->GetGlobalTimeNs().count() < timeout)) {
            m_waiter_count++;
            m_cond_var.Wait(std::addressof(m_lock), timeout, false);
            m_waiter_count--;
        } else {
            break;
        }
    }

    return false;
}

void KResourceLimit::Release(LimitableResource which, s64 value) {
    Release(which, value, value);
}

void KResourceLimit::Release(LimitableResource which, s64 value, s64 hint) {
    ASSERT(value >= 0);
    ASSERT(hint >= 0);

    const auto index = static_cast<std::size_t>(which);
    KScopedLightLock lk(m_lock);
    ASSERT(m_current_values[index] <= m_limit_values[index]);
    ASSERT(m_current_hints[index] <= m_current_values[index]);
    ASSERT(value <= m_current_values[index]);
    ASSERT(hint <= m_current_hints[index]);

    m_current_values[index] -= value;
    m_current_hints[index] -= hint;

    if (m_waiter_count != 0) {
        m_cond_var.Broadcast();
    }
}

KResourceLimit* CreateResourceLimitForProcess(Core::System& system, s64 physical_memory_size) {
    auto* resource_limit = KResourceLimit::Create(system.Kernel());
    resource_limit->Initialize(std::addressof(system.CoreTiming()));

    // Initialize default resource limit values.
    // TODO(bunnei): These values are the system defaults, the limits for service processes are
    // lower. These should use the correct limit values.

    ASSERT(resource_limit->SetLimitValue(LimitableResource::PhysicalMemoryMax, physical_memory_size)
               .IsSuccess());
    ASSERT(resource_limit->SetLimitValue(LimitableResource::ThreadCountMax, 800).IsSuccess());
    ASSERT(resource_limit->SetLimitValue(LimitableResource::EventCountMax, 900).IsSuccess());
    ASSERT(
        resource_limit->SetLimitValue(LimitableResource::TransferMemoryCountMax, 200).IsSuccess());
    ASSERT(resource_limit->SetLimitValue(LimitableResource::SessionCountMax, 1133).IsSuccess());

    return resource_limit;
}

} // namespace Kernel
