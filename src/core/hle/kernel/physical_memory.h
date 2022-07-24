// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

#include "common/alignment.h"

namespace Kernel {

// This encapsulation serves 2 purposes:
// - First, to encapsulate host physical memory under a single type and set an
// standard for managing it.
// - Second to ensure all host backing memory used is aligned to 256 bytes due
// to strict alignment restrictions on GPU memory.

using PhysicalMemoryVector = std::vector<u8, Common::AlignmentAllocator<u8, 256>>;
class PhysicalMemory final : public PhysicalMemoryVector {
    using PhysicalMemoryVector::PhysicalMemoryVector;
};

} // namespace Kernel
