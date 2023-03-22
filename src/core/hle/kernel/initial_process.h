// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "common/literals.h"
#include "core/hle/kernel/board/nintendo/nx/k_memory_layout.h"
#include "core/hle/kernel/board/nintendo/nx/k_system_control.h"

namespace Kernel {

using namespace Common::Literals;

constexpr std::size_t InitialProcessBinarySizeMax = 12_MiB;

static inline KPhysicalAddress GetInitialProcessBinaryPhysicalAddress() {
    return Kernel::Board::Nintendo::Nx::KSystemControl::Init::GetKernelPhysicalBaseAddress(
        MainMemoryAddress);
}

} // namespace Kernel
