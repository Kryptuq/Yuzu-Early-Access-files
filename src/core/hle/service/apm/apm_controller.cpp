// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <utility>

#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core_timing.h"
#include "core/hle/service/apm/apm_controller.h"

namespace Service::APM {

constexpr auto DEFAULT_PERFORMANCE_CONFIGURATION = PerformanceConfiguration::Config7;

Controller::Controller(Core::Timing::CoreTiming& core_timing_)
    : core_timing{core_timing_}, configs{
                                     {PerformanceMode::Normal, DEFAULT_PERFORMANCE_CONFIGURATION},
                                     {PerformanceMode::Boost, DEFAULT_PERFORMANCE_CONFIGURATION},
                                 } {}

Controller::~Controller() = default;

void Controller::SetPerformanceConfiguration(PerformanceMode mode,
                                             PerformanceConfiguration config) {
    static constexpr std::array<std::pair<PerformanceConfiguration, u32>, 16> config_to_speed{{
        {PerformanceConfiguration::Config1, 1020},
        {PerformanceConfiguration::Config2, 1020},
        {PerformanceConfiguration::Config3, 1224},
        {PerformanceConfiguration::Config4, 1020},
        {PerformanceConfiguration::Config5, 1020},
        {PerformanceConfiguration::Config6, 1224},
        {PerformanceConfiguration::Config7, 1020},
        {PerformanceConfiguration::Config8, 1020},
        {PerformanceConfiguration::Config9, 1020},
        {PerformanceConfiguration::Config10, 1020},
        {PerformanceConfiguration::Config11, 1020},
        {PerformanceConfiguration::Config12, 1020},
        {PerformanceConfiguration::Config13, 1785},
        {PerformanceConfiguration::Config14, 1785},
        {PerformanceConfiguration::Config15, 1020},
        {PerformanceConfiguration::Config16, 1020},
    }};

    const auto iter = std::find_if(config_to_speed.cbegin(), config_to_speed.cend(),
                                   [config](const auto& entry) { return entry.first == config; });

    if (iter == config_to_speed.cend()) {
        LOG_ERROR(Service_APM, "Invalid performance configuration value provided: {}", config);
        return;
    }

    SetClockSpeed(iter->second);
    configs.insert_or_assign(mode, config);
}

void Controller::SetFromCpuBoostMode(CpuBoostMode mode) {
    constexpr std::array<PerformanceConfiguration, 3> BOOST_MODE_TO_CONFIG_MAP{{
        PerformanceConfiguration::Config7,
        PerformanceConfiguration::Config13,
        PerformanceConfiguration::Config15,
    }};

    SetPerformanceConfiguration(PerformanceMode::Boost,
                                BOOST_MODE_TO_CONFIG_MAP.at(static_cast<u32>(mode)));
}

PerformanceMode Controller::GetCurrentPerformanceMode() const {
    return Settings::values.use_docked_mode.GetValue() ? PerformanceMode::Boost
                                                       : PerformanceMode::Normal;
}

PerformanceConfiguration Controller::GetCurrentPerformanceConfiguration(PerformanceMode mode) {
    if (configs.find(mode) == configs.end()) {
        configs.insert_or_assign(mode, DEFAULT_PERFORMANCE_CONFIGURATION);
    }

    return configs[mode];
}

void Controller::SetClockSpeed(u32 mhz) {
    LOG_INFO(Service_APM, "called, mhz={:08X}", mhz);
    // TODO(DarkLordZach): Actually signal core_timing to change clock speed.
    // TODO(Rodrigo): Remove [[maybe_unused]] when core_timing is used.
}

} // namespace Service::APM
