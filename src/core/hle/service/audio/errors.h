// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::Audio {

constexpr Result ERR_INVALID_DEVICE_NAME{ErrorModule::Audio, 1};
constexpr Result ERR_OPERATION_FAILED{ErrorModule::Audio, 2};
constexpr Result ERR_INVALID_SAMPLE_RATE{ErrorModule::Audio, 3};
constexpr Result ERR_INSUFFICIENT_BUFFER_SIZE{ErrorModule::Audio, 4};
constexpr Result ERR_MAXIMUM_SESSIONS_REACHED{ErrorModule::Audio, 5};
constexpr Result ERR_BUFFER_COUNT_EXCEEDED{ErrorModule::Audio, 8};
constexpr Result ERR_INVALID_CHANNEL_COUNT{ErrorModule::Audio, 10};
constexpr Result ERR_INVALID_UPDATE_DATA{ErrorModule::Audio, 41};
constexpr Result ERR_POOL_MAPPING_FAILED{ErrorModule::Audio, 42};
constexpr Result ERR_NOT_SUPPORTED{ErrorModule::Audio, 513};
constexpr Result ERR_INVALID_PROCESS_HANDLE{ErrorModule::Audio, 1536};
constexpr Result ERR_INVALID_REVISION{ErrorModule::Audio, 1537};

} // namespace Service::Audio
