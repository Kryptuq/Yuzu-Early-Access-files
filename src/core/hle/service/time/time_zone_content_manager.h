// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>

#include "core/hle/service/time/time_zone_manager.h"

namespace Core {
class System;
}

namespace Service::Time {
class TimeManager;
}

namespace Service::Time::TimeZone {

class TimeZoneContentManager final {
public:
    explicit TimeZoneContentManager(Core::System& system_);

    void Initialize(TimeManager& time_manager);

    TimeZoneManager& GetTimeZoneManager() {
        return time_zone_manager;
    }

    const TimeZoneManager& GetTimeZoneManager() const {
        return time_zone_manager;
    }

    ResultCode LoadTimeZoneRule(TimeZoneRule& rules, const std::string& location_name) const;

private:
    bool IsLocationNameValid(const std::string& location_name) const;
    ResultCode GetTimeZoneInfoFile(const std::string& location_name,
                                   FileSys::VirtualFile& vfs_file) const;

    Core::System& system;
    TimeZoneManager time_zone_manager;
    const std::vector<std::string> location_name_cache;
};

} // namespace Service::Time::TimeZone
