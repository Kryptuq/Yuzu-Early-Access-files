// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/common_types.h"
#include "core/hle/kernel/k_light_condition_variable.h"
#include "core/hle/kernel/k_light_lock.h"

union ResultCode;

namespace Core::Timing {
class CoreTiming;
}

namespace Kernel {
class KernelCore;
enum class LimitableResource : u32 {
    PhysicalMemory = 0,
    Threads = 1,
    Events = 2,
    TransferMemory = 3,
    Sessions = 4,

    Count,
};

constexpr bool IsValidResourceType(LimitableResource type) {
    return type < LimitableResource::Count;
}

class KResourceLimit final
    : public KAutoObjectWithSlabHeapAndContainer<KResourceLimit, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KResourceLimit, KAutoObject);

public:
    explicit KResourceLimit(KernelCore& kernel_);
    ~KResourceLimit() override;

    void Initialize(const Core::Timing::CoreTiming* core_timing_);
    void Finalize() override;

    s64 GetLimitValue(LimitableResource which) const;
    s64 GetCurrentValue(LimitableResource which) const;
    s64 GetPeakValue(LimitableResource which) const;
    s64 GetFreeValue(LimitableResource which) const;

    ResultCode SetLimitValue(LimitableResource which, s64 value);

    bool Reserve(LimitableResource which, s64 value);
    bool Reserve(LimitableResource which, s64 value, s64 timeout);
    void Release(LimitableResource which, s64 value);
    void Release(LimitableResource which, s64 value, s64 hint);

    static void PostDestroy([[maybe_unused]] uintptr_t arg) {}

private:
    using ResourceArray = std::array<s64, static_cast<std::size_t>(LimitableResource::Count)>;
    ResourceArray limit_values{};
    ResourceArray current_values{};
    ResourceArray current_hints{};
    ResourceArray peak_values{};
    mutable KLightLock lock;
    s32 waiter_count{};
    KLightConditionVariable cond_var;
    const Core::Timing::CoreTiming* core_timing{};
};

KResourceLimit* CreateResourceLimitForProcess(Core::System& system, s64 physical_memory_size);

} // namespace Kernel
