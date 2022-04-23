// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
