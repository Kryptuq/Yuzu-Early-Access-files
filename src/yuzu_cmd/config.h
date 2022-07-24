// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include "common/settings.h"

class INIReader;

class Config {
    std::filesystem::path sdl2_config_loc;
    std::unique_ptr<INIReader> sdl2_config;

    bool LoadINI(const std::string& default_contents = "", bool retry = true);
    void ReadValues();

public:
    explicit Config(std::optional<std::filesystem::path> config_path);
    ~Config();

    void Reload();

private:
    /**
     * Applies a value read from the sdl2_config to a Setting.
     *
     * @param group The name of the INI group
     * @param setting The yuzu setting to modify
     */
    template <typename Type, bool ranged>
    void ReadSetting(const std::string& group, Settings::Setting<Type, ranged>& setting);
};
