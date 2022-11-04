// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_system_resource.h"

namespace Kernel {

Result KSecureSystemResource::Initialize([[maybe_unused]] size_t size,
                                         [[maybe_unused]] KResourceLimit* resource_limit,
                                         [[maybe_unused]] KMemoryManager::Pool pool) {
    // Unimplemented
    UNREACHABLE();
}

void KSecureSystemResource::Finalize() {
    // Unimplemented
    UNREACHABLE();
}

size_t KSecureSystemResource::CalculateRequiredSecureMemorySize(
    [[maybe_unused]] size_t size, [[maybe_unused]] KMemoryManager::Pool pool) {
    // Unimplemented
    UNREACHABLE();
}

} // namespace Kernel
