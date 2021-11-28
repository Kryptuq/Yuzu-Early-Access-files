// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#pragma once

#include "core/hle/service/service.h"

namespace Service::NS {

struct PlayStatistics {
    u64 application_id{};
    u32 first_entry_index{};
    u32 first_timestamp_user{};
    u32 first_timestamp_network{};
    u32 last_entry_index{};
    u32 last_timestamp_user{};
    u32 last_timestamp_network{};
    u32 play_time_in_minutes{};
    u32 total_launches{};
};
static_assert(sizeof(PlayStatistics) == 0x28, "PlayStatistics is an invalid size");

class PDM_QRY final : public ServiceFramework<PDM_QRY> {
public:
    explicit PDM_QRY(Core::System& system_);
    ~PDM_QRY() override;

private:
    void QueryPlayStatisticsByApplicationIdAndUserAccountId(Kernel::HLERequestContext& ctx);
};

} // namespace Service::NS
