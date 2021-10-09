// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace Core {
class System;
}

class Config;

class InputProfiles {

public:
    explicit InputProfiles(Core::System& system_);
    virtual ~InputProfiles();

    std::vector<std::string> GetInputProfileNames();

    static bool IsProfileNameValid(std::string_view profile_name);

    bool CreateProfile(const std::string& profile_name, std::size_t player_index);
    bool DeleteProfile(const std::string& profile_name);
    bool LoadProfile(const std::string& profile_name, std::size_t player_index);
    bool SaveProfile(const std::string& profile_name, std::size_t player_index);

private:
    bool ProfileExistsInMap(const std::string& profile_name) const;

    std::unordered_map<std::string, std::unique_ptr<Config>> map_profiles;

    Core::System& system;
};
