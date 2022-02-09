// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>

#include "common/common_funcs.h"
#include "common/uuid.h"
#include "core/hle/result.h"
#include "core/hle/service/am/applets/applets.h"

namespace Core {
class System;
}

namespace Service::AM::Applets {

struct UserSelectionConfig {
    // TODO(DarkLordZach): RE this structure
    // It seems to be flags and the like that determine the UI of the applet on the switch... from
    // my research this is safe to ignore for now.
    INSERT_PADDING_BYTES(0xA0);
};
static_assert(sizeof(UserSelectionConfig) == 0xA0, "UserSelectionConfig has incorrect size.");

struct UserSelectionOutput {
    u64 result;
    Common::UUID uuid_selected;
};
static_assert(sizeof(UserSelectionOutput) == 0x18, "UserSelectionOutput has incorrect size.");

class ProfileSelect final : public Applet {
public:
    explicit ProfileSelect(Core::System& system_, LibraryAppletMode applet_mode_,
                           const Core::Frontend::ProfileSelectApplet& frontend_);
    ~ProfileSelect() override;

    void Initialize() override;

    bool TransactionComplete() const override;
    ResultCode GetStatus() const override;
    void ExecuteInteractive() override;
    void Execute() override;

    void SelectionComplete(std::optional<Common::UUID> uuid);

private:
    const Core::Frontend::ProfileSelectApplet& frontend;

    UserSelectionConfig config;
    bool complete = false;
    ResultCode status = ResultSuccess;
    std::vector<u8> final_data;
    Core::System& system;
};

} // namespace Service::AM::Applets
