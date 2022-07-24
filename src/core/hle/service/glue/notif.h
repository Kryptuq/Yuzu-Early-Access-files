// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>

#include "common/uuid.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Glue {

class NOTIF_A final : public ServiceFramework<NOTIF_A> {
public:
    explicit NOTIF_A(Core::System& system_);
    ~NOTIF_A() override;

private:
    static constexpr std::size_t max_alarms = 8;

    // This is nn::notification::AlarmSettingId
    using AlarmSettingId = u16;
    static_assert(sizeof(AlarmSettingId) == 0x2, "AlarmSettingId is an invalid size");

    using ApplicationParameter = std::array<u8, 0x400>;
    static_assert(sizeof(ApplicationParameter) == 0x400, "ApplicationParameter is an invalid size");

    struct DailyAlarmSetting {
        s8 hour;
        s8 minute;
    };
    static_assert(sizeof(DailyAlarmSetting) == 0x2, "DailyAlarmSetting is an invalid size");

    struct WeeklyScheduleAlarmSetting {
        INSERT_PADDING_BYTES(0xA);
        std::array<DailyAlarmSetting, 0x7> day_of_week;
    };
    static_assert(sizeof(WeeklyScheduleAlarmSetting) == 0x18,
                  "WeeklyScheduleAlarmSetting is an invalid size");

    // This is nn::notification::AlarmSetting
    struct AlarmSetting {
        AlarmSettingId alarm_setting_id;
        u8 kind;
        u8 muted;
        INSERT_PADDING_BYTES(0x4);
        Common::UUID account_id;
        u64 application_id;
        INSERT_PADDING_BYTES(0x8);
        WeeklyScheduleAlarmSetting schedule;
    };
    static_assert(sizeof(AlarmSetting) == 0x40, "AlarmSetting is an invalid size");

    void RegisterAlarmSetting(Kernel::HLERequestContext& ctx);
    void UpdateAlarmSetting(Kernel::HLERequestContext& ctx);
    void ListAlarmSettings(Kernel::HLERequestContext& ctx);
    void LoadApplicationParameter(Kernel::HLERequestContext& ctx);
    void DeleteAlarmSetting(Kernel::HLERequestContext& ctx);
    void Initialize(Kernel::HLERequestContext& ctx);

    std::vector<AlarmSetting>::iterator GetAlarmFromId(AlarmSettingId alarm_setting_id);

    std::vector<AlarmSetting> alarms{};
    AlarmSettingId last_alarm_setting_id{};
};

} // namespace Service::Glue
