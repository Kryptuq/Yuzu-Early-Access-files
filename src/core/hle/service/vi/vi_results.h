// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/result.h"

namespace Service::VI {

constexpr Result ResultOperationFailed{ErrorModule::VI, 1};
constexpr Result ResultPermissionDenied{ErrorModule::VI, 5};
constexpr Result ResultNotSupported{ErrorModule::VI, 6};
constexpr Result ResultNotFound{ErrorModule::VI, 7};

} // namespace Service::VI
