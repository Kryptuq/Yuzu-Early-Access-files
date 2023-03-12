// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_system_resource.h"

namespace Kernel {

Result KSecureSystemResource::Initialize(size_t size, KResourceLimit* resource_limit,
                                         KMemoryManager::Pool pool) {
    // Unimplemented
    UNREACHABLE();
}

void KSecureSystemResource::Finalize() {
    // Unimplemented
    UNREACHABLE();
}

size_t KSecureSystemResource::CalculateRequiredSecureMemorySize(size_t size,
                                                                KMemoryManager::Pool pool) {
    // Unimplemented
    UNREACHABLE();
}

} // namespace Kernel
