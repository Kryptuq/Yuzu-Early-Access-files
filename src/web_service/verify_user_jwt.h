// SPDX-FileCopyrightText: Copyright 2018 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <fmt/format.h>
#include "network/verify_user.h"
#include "web_service/web_backend.h"

namespace WebService {

std::string GetPublicKey(const std::string& host);

class VerifyUserJWT final : public Network::VerifyUser::Backend {
public:
    VerifyUserJWT(const std::string& host);
    ~VerifyUserJWT() = default;

    Network::VerifyUser::UserData LoadUserData(const std::string& verify_uid,
                                               const std::string& token) override;

private:
    std::string pub_key;
};

} // namespace WebService
