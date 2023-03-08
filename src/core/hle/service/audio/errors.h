// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::Audio {

constexpr Result ResultNotFound{ErrorModule::Audio, 1};
constexpr Result ResultOperationFailed{ErrorModule::Audio, 2};
constexpr Result ResultInvalidSampleRate{ErrorModule::Audio, 3};
constexpr Result ResultInsufficientBuffer{ErrorModule::Audio, 4};
constexpr Result ResultOutOfSessions{ErrorModule::Audio, 5};
constexpr Result ResultBufferCountReached{ErrorModule::Audio, 8};
constexpr Result ResultInvalidChannelCount{ErrorModule::Audio, 10};
constexpr Result ResultInvalidUpdateInfo{ErrorModule::Audio, 41};
constexpr Result ResultInvalidAddressInfo{ErrorModule::Audio, 42};
constexpr Result ResultNotSupported{ErrorModule::Audio, 513};
constexpr Result ResultInvalidHandle{ErrorModule::Audio, 1536};
constexpr Result ResultInvalidRevision{ErrorModule::Audio, 1537};

} // namespace Service::Audio
