// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <optional>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "common/uuid.h"
#include "core/hle/result.h"

namespace Service::Account {

constexpr std::size_t MAX_USERS{8};
constexpr std::size_t profile_username_size{32};

using ProfileUsername = std::array<u8, profile_username_size>;
using UserIDArray = std::array<Common::UUID, MAX_USERS>;

/// Contains extra data related to a user.
/// TODO: RE this structure
struct UserData {
    INSERT_PADDING_WORDS_NOINIT(1);
    u32 icon_id;
    u8 bg_color_id;
    INSERT_PADDING_BYTES_NOINIT(0x7);
    INSERT_PADDING_BYTES_NOINIT(0x10);
    INSERT_PADDING_BYTES_NOINIT(0x60);
};
static_assert(sizeof(UserData) == 0x80, "UserData structure has incorrect size");

/// This holds general information about a users profile. This is where we store all the information
/// based on a specific user
struct ProfileInfo {
    Common::UUID user_uuid{};
    ProfileUsername username{};
    u64 creation_time{};
    UserData data{}; // TODO(ognik): Work out what this is
    bool is_open{};
};

struct ProfileBase {
    Common::UUID user_uuid;
    u64_le timestamp;
    ProfileUsername username;

    // Zero out all the fields to make the profile slot considered "Empty"
    void Invalidate() {
        user_uuid = {};
        timestamp = 0;
        username.fill(0);
    }
};
static_assert(sizeof(ProfileBase) == 0x38, "ProfileBase is an invalid size");

/// The profile manager is used for handling multiple user profiles at once. It keeps track of open
/// users, all the accounts registered on the "system" as well as fetching individual "ProfileInfo"
/// objects
class ProfileManager {
public:
    ProfileManager();
    ~ProfileManager();

    Result AddUser(const ProfileInfo& user);
    Result CreateNewUser(Common::UUID uuid, const ProfileUsername& username);
    Result CreateNewUser(Common::UUID uuid, const std::string& username);
    std::optional<Common::UUID> GetUser(std::size_t index) const;
    std::optional<std::size_t> GetUserIndex(const Common::UUID& uuid) const;
    std::optional<std::size_t> GetUserIndex(const ProfileInfo& user) const;
    bool GetProfileBase(std::optional<std::size_t> index, ProfileBase& profile) const;
    bool GetProfileBase(Common::UUID uuid, ProfileBase& profile) const;
    bool GetProfileBase(const ProfileInfo& user, ProfileBase& profile) const;
    bool GetProfileBaseAndData(std::optional<std::size_t> index, ProfileBase& profile,
                               UserData& data) const;
    bool GetProfileBaseAndData(Common::UUID uuid, ProfileBase& profile, UserData& data) const;
    bool GetProfileBaseAndData(const ProfileInfo& user, ProfileBase& profile, UserData& data) const;
    std::size_t GetUserCount() const;
    std::size_t GetOpenUserCount() const;
    bool UserExists(Common::UUID uuid) const;
    bool UserExistsIndex(std::size_t index) const;
    void OpenUser(Common::UUID uuid);
    void CloseUser(Common::UUID uuid);
    UserIDArray GetOpenUsers() const;
    UserIDArray GetAllUsers() const;
    Common::UUID GetLastOpenedUser() const;
    UserIDArray GetStoredOpenedUsers() const;
    void StoreOpenedUsers();

    bool CanSystemRegisterUser() const;

    bool RemoveUser(Common::UUID uuid);
    bool SetProfileBase(Common::UUID uuid, const ProfileBase& profile_new);
    bool SetProfileBaseAndData(Common::UUID uuid, const ProfileBase& profile_new,
                               const UserData& data_new);

private:
    void ParseUserSaveFile();
    void WriteUserSaveFile();
    std::optional<std::size_t> AddToProfiles(const ProfileInfo& profile);
    bool RemoveProfileAtIndex(std::size_t index);

    std::array<ProfileInfo, MAX_USERS> profiles{};
    std::array<ProfileInfo, MAX_USERS> stored_opened_profiles{};
    std::size_t user_count{};
    Common::UUID last_opened_user{};
};

}; // namespace Service::Account
