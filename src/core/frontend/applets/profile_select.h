// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <optional>

#include "common/uuid.h"
#include "core/frontend/applets/applet.h"
#include "core/hle/service/am/applets/applet_profile_select.h"

namespace Core::Frontend {

struct ProfileSelectParameters {
    Service::AM::Applets::UiMode mode;
    std::array<Common::UUID, 8> invalid_uid_list;
    Service::AM::Applets::UiSettingsDisplayOptions display_options;
    Service::AM::Applets::UserSelectionPurpose purpose;
};

class ProfileSelectApplet : public Applet {
public:
    using SelectProfileCallback = std::function<void(std::optional<Common::UUID>)>;

    virtual ~ProfileSelectApplet();

    virtual void SelectProfile(SelectProfileCallback callback,
                               const ProfileSelectParameters& parameters) const = 0;
};

class DefaultProfileSelectApplet final : public ProfileSelectApplet {
public:
    void Close() const override;
    void SelectProfile(SelectProfileCallback callback,
                       const ProfileSelectParameters& parameters) const override;
};

} // namespace Core::Frontend
