// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <fmt/format.h>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "yuzu/configuration/config.h"
#include "yuzu/configuration/input_profiles.h"

namespace FS = Common::FS;

namespace {

bool ProfileExistsInFilesystem(std::string_view profile_name) {
    return FS::Exists(FS::GetYuzuPath(FS::YuzuPath::ConfigDir) / "input" /
                      fmt::format("{}.ini", profile_name));
}

bool IsINI(const std::filesystem::path& filename) {
    return filename.extension() == ".ini";
}

std::filesystem::path GetNameWithoutExtension(std::filesystem::path filename) {
    return filename.replace_extension();
}

} // namespace

InputProfiles::InputProfiles(Core::System& system_) : system{system_} {
    const auto input_profile_loc = FS::GetYuzuPath(FS::YuzuPath::ConfigDir) / "input";

    if (!FS::IsDir(input_profile_loc)) {
        return;
    }

    FS::IterateDirEntries(
        input_profile_loc,
        [this](const std::filesystem::path& full_path) {
            const auto filename = full_path.filename();
            const auto name_without_ext =
                Common::FS::PathToUTF8String(GetNameWithoutExtension(filename));

            if (IsINI(filename) && IsProfileNameValid(name_without_ext)) {
                map_profiles.insert_or_assign(
                    name_without_ext, std::make_unique<Config>(system, name_without_ext,
                                                               Config::ConfigType::InputProfile));
            }

            return true;
        },
        FS::DirEntryFilter::File);
}

InputProfiles::~InputProfiles() = default;

std::vector<std::string> InputProfiles::GetInputProfileNames() {
    std::vector<std::string> profile_names;
    profile_names.reserve(map_profiles.size());

    for (const auto& [profile_name, config] : map_profiles) {
        if (!ProfileExistsInFilesystem(profile_name)) {
            DeleteProfile(profile_name);
            continue;
        }

        profile_names.push_back(profile_name);
    }

    return profile_names;
}

bool InputProfiles::IsProfileNameValid(std::string_view profile_name) {
    return profile_name.find_first_of("<>:;\"/\\|,.!?*") == std::string::npos;
}

bool InputProfiles::CreateProfile(const std::string& profile_name, std::size_t player_index) {
    if (ProfileExistsInMap(profile_name)) {
        return false;
    }

    map_profiles.insert_or_assign(
        profile_name,
        std::make_unique<Config>(system, profile_name, Config::ConfigType::InputProfile));

    return SaveProfile(profile_name, player_index);
}

bool InputProfiles::DeleteProfile(const std::string& profile_name) {
    if (!ProfileExistsInMap(profile_name)) {
        return false;
    }

    if (!ProfileExistsInFilesystem(profile_name) ||
        FS::RemoveFile(map_profiles[profile_name]->GetConfigFilePath())) {
        map_profiles.erase(profile_name);
    }

    return !ProfileExistsInMap(profile_name) && !ProfileExistsInFilesystem(profile_name);
}

bool InputProfiles::LoadProfile(const std::string& profile_name, std::size_t player_index) {
    if (!ProfileExistsInMap(profile_name)) {
        return false;
    }

    if (!ProfileExistsInFilesystem(profile_name)) {
        map_profiles.erase(profile_name);
        return false;
    }

    map_profiles[profile_name]->ReadControlPlayerValue(player_index);
    return true;
}

bool InputProfiles::SaveProfile(const std::string& profile_name, std::size_t player_index) {
    if (!ProfileExistsInMap(profile_name)) {
        return false;
    }

    map_profiles[profile_name]->SaveControlPlayerValue(player_index);
    return true;
}

bool InputProfiles::ProfileExistsInMap(const std::string& profile_name) const {
    return map_profiles.find(profile_name) != map_profiles.end();
}
