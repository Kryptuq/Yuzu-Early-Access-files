// SPDX-FileCopyrightText: Copyright 2018 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include "common/logging/log.h"

namespace Network::VerifyUser {

struct UserData {
    std::string username;
    std::string display_name;
    std::string avatar_url;
    bool moderator = false; ///< Whether the user is a yuzu Moderator.
};

/**
 * A backend used for verifying users and loading user data.
 */
class Backend {
public:
    virtual ~Backend();

    /**
     * Verifies the given token and loads the information into a UserData struct.
     * @param verify_uid A GUID that may be used for verification.
     * @param token A token that contains user data and verification data. The format and content is
     * decided by backends.
     */
    virtual UserData LoadUserData(const std::string& verify_uid, const std::string& token) = 0;
};

/**
 * A null backend where the token is ignored.
 * No verification is performed here and the function returns an empty UserData.
 */
class NullBackend final : public Backend {
public:
    ~NullBackend();

    UserData LoadUserData(const std::string& verify_uid, const std::string& token) override;
};

} // namespace Network::VerifyUser
