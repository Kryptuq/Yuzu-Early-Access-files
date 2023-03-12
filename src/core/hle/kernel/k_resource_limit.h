// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include "common/common_types.h"
#include "core/hle/kernel/k_light_condition_variable.h"
#include "core/hle/kernel/k_light_lock.h"

union Result;

namespace Core::Timing {
class CoreTiming;
}

namespace Kernel {
class KernelCore;

using LimitableResource = Svc::LimitableResource;

constexpr bool IsValidResourceType(LimitableResource type) {
    return type < LimitableResource::Count;
}

class KResourceLimit final
    : public KAutoObjectWithSlabHeapAndContainer<KResourceLimit, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KResourceLimit, KAutoObject);

public:
    explicit KResourceLimit(KernelCore& kernel);
    ~KResourceLimit() override;

    void Initialize(const Core::Timing::CoreTiming* core_timing);
    void Finalize() override;

    s64 GetLimitValue(LimitableResource which) const;
    s64 GetCurrentValue(LimitableResource which) const;
    s64 GetPeakValue(LimitableResource which) const;
    s64 GetFreeValue(LimitableResource which) const;

    Result SetLimitValue(LimitableResource which, s64 value);

    bool Reserve(LimitableResource which, s64 value);
    bool Reserve(LimitableResource which, s64 value, s64 timeout);
    void Release(LimitableResource which, s64 value);
    void Release(LimitableResource which, s64 value, s64 hint);

    static void PostDestroy(uintptr_t arg) {}

private:
    using ResourceArray = std::array<s64, static_cast<std::size_t>(LimitableResource::Count)>;
    ResourceArray m_limit_values{};
    ResourceArray m_current_values{};
    ResourceArray m_current_hints{};
    ResourceArray m_peak_values{};
    mutable KLightLock m_lock;
    s32 m_waiter_count{};
    KLightConditionVariable m_cond_var;
    const Core::Timing::CoreTiming* m_core_timing{};
};

KResourceLimit* CreateResourceLimitForProcess(Core::System& system, s64 physical_memory_size);

} // namespace Kernel
