// SPDX-FileCopyrightText: 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

namespace WebService {

/**
 * Checks if username and token is valid
 * @param host the web API URL
 * @param username yuzu username to use for authentication.
 * @param token yuzu token to use for authentication.
 * @returns a bool indicating whether the verification succeeded
 */
bool VerifyLogin(const std::string& host, const std::string& username, const std::string& token);

} // namespace WebService
