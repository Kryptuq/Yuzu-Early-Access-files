// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::Audio {

constexpr Result ERR_OPERATION_FAILED{ErrorModule::Audio, 2};
constexpr Result ERR_BUFFER_COUNT_EXCEEDED{ErrorModule::Audio, 8};
constexpr Result ERR_NOT_SUPPORTED{ErrorModule::Audio, 513};

} // namespace Service::Audio
