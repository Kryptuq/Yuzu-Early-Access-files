// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "common/literals.h"
#include "core/hle/kernel/board/nintendo/nx/k_memory_layout.h"
#include "core/hle/kernel/board/nintendo/nx/k_system_control.h"

namespace Kernel {

using namespace Common::Literals;

constexpr std::size_t InitialProcessBinarySizeMax = 12_MiB;

static inline PAddr GetInitialProcessBinaryPhysicalAddress() {
    return Kernel::Board::Nintendo::Nx::KSystemControl::Init::GetKernelPhysicalBaseAddress(
        MainMemoryAddress);
}

} // namespace Kernel
