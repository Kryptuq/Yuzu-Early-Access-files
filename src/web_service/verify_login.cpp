// SPDX-FileCopyrightText: 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <nlohmann/json.hpp>
#include "web_service/verify_login.h"
#include "web_service/web_backend.h"
#include "web_service/web_result.h"

namespace WebService {

bool VerifyLogin(const std::string& host, const std::string& username, const std::string& token) {
    Client client(host, username, token);
    auto reply = client.GetJson("/profile", false).returned_data;
    if (reply.empty()) {
        return false;
    }
    nlohmann::json json = nlohmann::json::parse(reply);
    const auto iter = json.find("username");

    if (iter == json.end()) {
        return username.empty();
    }

    return username == *iter;
}

} // namespace WebService
