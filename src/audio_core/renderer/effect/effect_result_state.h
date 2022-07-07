// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "common/common_types.h"

namespace AudioCore::AudioRenderer {

struct EffectResultState {
    std::array<u8, 0x80> state;
};

} // namespace AudioCore::AudioRenderer
