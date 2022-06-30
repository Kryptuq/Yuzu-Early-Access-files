// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <random>

#include <fmt/format.h>

#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/settings.h"
#include "core/hle/service/acc/profile_manager.h"

namespace Service::Account {

namespace FS = Common::FS;
using Common::UUID;

struct UserRaw {
    UUID uuid{};
    UUID uuid2{};
    u64 timestamp{};
    ProfileUsername username{};
    ProfileData extra_data{};
};
static_assert(sizeof(UserRaw) == 0xC8, "UserRaw has incorrect size.");

struct ProfileDataRaw {
    INSERT_PADDING_BYTES(0x10);
    std::array<UserRaw, MAX_USERS> users{};
};
static_assert(sizeof(ProfileDataRaw) == 0x650, "ProfileDataRaw has incorrect size.");

// TODO(ogniK): Get actual error codes
constexpr Result ERROR_TOO_MANY_USERS(ErrorModule::Account, u32(-1));
constexpr Result ERROR_USER_ALREADY_EXISTS(ErrorModule::Account, u32(-2));
constexpr Result ERROR_ARGUMENT_IS_NULL(ErrorModule::Account, 20);

constexpr char ACC_SAVE_AVATORS_BASE_PATH[] = "system/save/8000000000000010/su/avators";

ProfileManager::ProfileManager() {
    ParseUserSaveFile();

    // Create an user if none are present
    if (user_count == 0) {
        CreateNewUser(UUID::MakeRandom(), "yuzu");
    }

    auto current =
        std::clamp<int>(static_cast<s32>(Settings::values.current_user), 0, MAX_USERS - 1);

    // If user index don't exist. Load the first user and change the active user
    if (!UserExistsIndex(current)) {
        current = 0;
        Settings::values.current_user = 0;
    }

    OpenUser(*GetUser(current));
}

ProfileManager::~ProfileManager() {
    WriteUserSaveFile();
}

/// After a users creation it needs to be "registered" to the system. AddToProfiles handles the
/// internal management of the users profiles
std::optional<std::size_t> ProfileManager::AddToProfiles(const ProfileInfo& profile) {
    if (user_count >= MAX_USERS) {
        return std::nullopt;
    }
    profiles[user_count] = profile;
    return user_count++;
}

/// Deletes a specific profile based on it's profile index
bool ProfileManager::RemoveProfileAtIndex(std::size_t index) {
    if (index >= MAX_USERS || index >= user_count) {
        return false;
    }
    if (index < user_count - 1) {
        std::rotate(profiles.begin() + index, profiles.begin() + index + 1, profiles.end());
    }
    profiles.back() = {};
    user_count--;
    return true;
}

/// Helper function to register a user to the system
Result ProfileManager::AddUser(const ProfileInfo& user) {
    if (!AddToProfiles(user)) {
        return ERROR_TOO_MANY_USERS;
    }
    return ResultSuccess;
}

/// Create a new user on the system. If the uuid of the user already exists, the user is not
/// created.
Result ProfileManager::CreateNewUser(UUID uuid, const ProfileUsername& username) {
    if (user_count == MAX_USERS) {
        return ERROR_TOO_MANY_USERS;
    }
    if (uuid.IsInvalid()) {
        return ERROR_ARGUMENT_IS_NULL;
    }
    if (username[0] == 0x0) {
        return ERROR_ARGUMENT_IS_NULL;
    }
    if (std::any_of(profiles.begin(), profiles.end(),
                    [&uuid](const ProfileInfo& profile) { return uuid == profile.user_uuid; })) {
        return ERROR_USER_ALREADY_EXISTS;
    }

    return AddUser({
        .user_uuid = uuid,
        .username = username,
        .creation_time = 0,
        .data = {},
        .is_open = false,
    });
}

/// Creates a new user on the system. This function allows a much simpler method of registration
/// specifically by allowing an std::string for the username. This is required specifically since
/// we're loading a string straight from the config
Result ProfileManager::CreateNewUser(UUID uuid, const std::string& username) {
    ProfileUsername username_output{};

    if (username.size() > username_output.size()) {
        std::copy_n(username.begin(), username_output.size(), username_output.begin());
    } else {
        std::copy(username.begin(), username.end(), username_output.begin());
    }
    return CreateNewUser(uuid, username_output);
}

std::optional<UUID> ProfileManager::GetUser(std::size_t index) const {
    if (index >= MAX_USERS) {
        return std::nullopt;
    }

    return profiles[index].user_uuid;
}

/// Returns a users profile index based on their user id.
std::optional<std::size_t> ProfileManager::GetUserIndex(const UUID& uuid) const {
    if (uuid.IsInvalid()) {
        return std::nullopt;
    }

    const auto iter = std::find_if(profiles.begin(), profiles.end(),
                                   [&uuid](const ProfileInfo& p) { return p.user_uuid == uuid; });
    if (iter == profiles.end()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(profiles.begin(), iter));
}

/// Returns a users profile index based on their profile
std::optional<std::size_t> ProfileManager::GetUserIndex(const ProfileInfo& user) const {
    return GetUserIndex(user.user_uuid);
}

/// Returns the data structure used by the switch when GetProfileBase is called on acc:*
bool ProfileManager::GetProfileBase(std::optional<std::size_t> index, ProfileBase& profile) const {
    if (!index || index >= MAX_USERS) {
        return false;
    }
    const auto& prof_info = profiles[*index];
    profile.user_uuid = prof_info.user_uuid;
    profile.username = prof_info.username;
    profile.timestamp = prof_info.creation_time;
    return true;
}

/// Returns the data structure used by the switch when GetProfileBase is called on acc:*
bool ProfileManager::GetProfileBase(UUID uuid, ProfileBase& profile) const {
    const auto idx = GetUserIndex(uuid);
    return GetProfileBase(idx, profile);
}

/// Returns the data structure used by the switch when GetProfileBase is called on acc:*
bool ProfileManager::GetProfileBase(const ProfileInfo& user, ProfileBase& profile) const {
    return GetProfileBase(user.user_uuid, profile);
}

/// Returns the current user count on the system. We keep a variable which tracks the count so we
/// don't have to loop the internal profile array every call.

std::size_t ProfileManager::GetUserCount() const {
    return user_count;
}

/// Lists the current "opened" users on the system. Users are typically not open until they sign
/// into something or pick a profile. As of right now users should all be open until qlaunch is
/// booting

std::size_t ProfileManager::GetOpenUserCount() const {
    return std::count_if(profiles.begin(), profiles.end(),
                         [](const ProfileInfo& p) { return p.is_open; });
}

/// Checks if a user id exists in our profile manager
bool ProfileManager::UserExists(UUID uuid) const {
    return GetUserIndex(uuid).has_value();
}

bool ProfileManager::UserExistsIndex(std::size_t index) const {
    if (index >= MAX_USERS) {
        return false;
    }
    return profiles[index].user_uuid.IsValid();
}

/// Opens a specific user
void ProfileManager::OpenUser(UUID uuid) {
    const auto idx = GetUserIndex(uuid);
    if (!idx) {
        return;
    }

    profiles[*idx].is_open = true;
    last_opened_user = uuid;
}

/// Closes a specific user
void ProfileManager::CloseUser(UUID uuid) {
    const auto idx = GetUserIndex(uuid);
    if (!idx) {
        return;
    }

    profiles[*idx].is_open = false;
}

/// Gets all valid user ids on the system
UserIDArray ProfileManager::GetAllUsers() const {
    UserIDArray output{};
    std::ranges::transform(profiles, output.begin(),
                           [](const ProfileInfo& p) { return p.user_uuid; });
    return output;
}

/// Get all the open users on the system and zero out the rest of the data. This is specifically
/// needed for GetOpenUsers and we need to ensure the rest of the output buffer is zero'd out
UserIDArray ProfileManager::GetOpenUsers() const {
    UserIDArray output{};
    std::ranges::transform(profiles, output.begin(), [](const ProfileInfo& p) {
        if (p.is_open)
            return p.user_uuid;
        return Common::InvalidUUID;
    });
    std::stable_partition(output.begin(), output.end(),
                          [](const UUID& uuid) { return uuid.IsValid(); });
    return output;
}

/// Returns the last user which was opened
UUID ProfileManager::GetLastOpenedUser() const {
    return last_opened_user;
}

/// Return the users profile base and the unknown arbitary data.
bool ProfileManager::GetProfileBaseAndData(std::optional<std::size_t> index, ProfileBase& profile,
                                           ProfileData& data) const {
    if (GetProfileBase(index, profile)) {
        data = profiles[*index].data;
        return true;
    }
    return false;
}

/// Return the users profile base and the unknown arbitary data.
bool ProfileManager::GetProfileBaseAndData(UUID uuid, ProfileBase& profile,
                                           ProfileData& data) const {
    const auto idx = GetUserIndex(uuid);
    return GetProfileBaseAndData(idx, profile, data);
}

/// Return the users profile base and the unknown arbitary data.
bool ProfileManager::GetProfileBaseAndData(const ProfileInfo& user, ProfileBase& profile,
                                           ProfileData& data) const {
    return GetProfileBaseAndData(user.user_uuid, profile, data);
}

/// Returns if the system is allowing user registrations or not
bool ProfileManager::CanSystemRegisterUser() const {
    return false; // TODO(ogniK): Games shouldn't have
                  // access to user registration, when we
    // emulate qlaunch. Update this to dynamically change.
}

bool ProfileManager::RemoveUser(UUID uuid) {
    const auto index = GetUserIndex(uuid);
    if (!index) {
        return false;
    }

    profiles[*index] = ProfileInfo{};
    std::stable_partition(profiles.begin(), profiles.end(),
                          [](const ProfileInfo& profile) { return profile.user_uuid.IsValid(); });
    return true;
}

bool ProfileManager::SetProfileBase(UUID uuid, const ProfileBase& profile_new) {
    const auto index = GetUserIndex(uuid);
    if (!index || profile_new.user_uuid.IsInvalid()) {
        return false;
    }

    auto& profile = profiles[*index];
    profile.user_uuid = profile_new.user_uuid;
    profile.username = profile_new.username;
    profile.creation_time = profile_new.timestamp;

    return true;
}

bool ProfileManager::SetProfileBaseAndData(Common::UUID uuid, const ProfileBase& profile_new,
                                           const ProfileData& data_new) {
    const auto index = GetUserIndex(uuid);
    if (index.has_value() && SetProfileBase(uuid, profile_new)) {
        profiles[*index].data = data_new;
        return true;
    }

    return false;
}

void ProfileManager::ParseUserSaveFile() {
    const auto save_path(FS::GetYuzuPath(FS::YuzuPath::NANDDir) / ACC_SAVE_AVATORS_BASE_PATH /
                         "profiles.dat");
    const FS::IOFile save(save_path, FS::FileAccessMode::Read, FS::FileType::BinaryFile);

    if (!save.IsOpen()) {
        LOG_WARNING(Service_ACC, "Failed to load profile data from save data... Generating new "
                                 "user 'yuzu' with random UUID.");
        return;
    }

    ProfileDataRaw data;
    if (!save.ReadObject(data)) {
        LOG_WARNING(Service_ACC, "profiles.dat is smaller than expected... Generating new user "
                                 "'yuzu' with random UUID.");
        return;
    }

    for (const auto& user : data.users) {
        if (user.uuid.IsInvalid()) {
            continue;
        }

        AddUser({
            .user_uuid = user.uuid,
            .username = user.username,
            .creation_time = user.timestamp,
            .data = user.extra_data,
            .is_open = false,
        });
    }

    std::stable_partition(profiles.begin(), profiles.end(),
                          [](const ProfileInfo& profile) { return profile.user_uuid.IsValid(); });
}

void ProfileManager::WriteUserSaveFile() {
    ProfileDataRaw raw{};

    for (std::size_t i = 0; i < MAX_USERS; ++i) {
        raw.users[i] = {
            .uuid = profiles[i].user_uuid,
            .uuid2 = profiles[i].user_uuid,
            .timestamp = profiles[i].creation_time,
            .username = profiles[i].username,
            .extra_data = profiles[i].data,
        };
    }

    const auto raw_path(FS::GetYuzuPath(FS::YuzuPath::NANDDir) / "system/save/8000000000000010");
    if (FS::IsFile(raw_path) && !FS::RemoveFile(raw_path)) {
        return;
    }

    const auto save_path(FS::GetYuzuPath(FS::YuzuPath::NANDDir) / ACC_SAVE_AVATORS_BASE_PATH /
                         "profiles.dat");

    if (!FS::CreateParentDirs(save_path)) {
        LOG_WARNING(Service_ACC, "Failed to create full path of profiles.dat. Create the directory "
                                 "nand/system/save/8000000000000010/su/avators to mitigate this "
                                 "issue.");
        return;
    }

    FS::IOFile save(save_path, FS::FileAccessMode::Write, FS::FileType::BinaryFile);

    if (!save.IsOpen() || !save.SetSize(sizeof(ProfileDataRaw)) || !save.WriteObject(raw)) {
        LOG_WARNING(Service_ACC, "Failed to write save data to file... No changes to user data "
                                 "made in current session will be saved.");
    }
}

}; // namespace Service::Account
