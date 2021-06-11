// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

#define BOARD_NINTENDO_NX

#ifdef BOARD_NINTENDO_NX

#include "core/hle/kernel/board/nintendo/nx/k_system_control.h"

namespace Kernel {

using Kernel::Board::Nintendo::Nx::KSystemControl;

} // namespace Kernel

#else
#error "Unknown board for KSystemControl"
#endif
