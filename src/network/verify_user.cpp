// SPDX-FileCopyrightText: Copyright 2018 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "network/verify_user.h"

namespace Network::VerifyUser {

Backend::~Backend() = default;

NullBackend::~NullBackend() = default;

UserData NullBackend::LoadUserData([[maybe_unused]] const std::string& verify_uid,
                                   [[maybe_unused]] const std::string& token) {
    return {};
}

} // namespace Network::VerifyUser
