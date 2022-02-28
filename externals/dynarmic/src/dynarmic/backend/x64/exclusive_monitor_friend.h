/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "dynarmic/interface/exclusive_monitor.h"

namespace Dynarmic {

inline volatile int* GetExclusiveMonitorLockPointer(ExclusiveMonitor* monitor) {
    return &monitor->lock.storage;
}

inline size_t GetExclusiveMonitorProcessorCount(ExclusiveMonitor* monitor) {
    return monitor->exclusive_addresses.size();
}

inline VAddr* GetExclusiveMonitorAddressPointer(ExclusiveMonitor* monitor, size_t index) {
    return monitor->exclusive_addresses.data() + index;
}

inline Vector* GetExclusiveMonitorValuePointer(ExclusiveMonitor* monitor, size_t index) {
    return monitor->exclusive_values.data() + index;
}

}  // namespace Dynarmic
