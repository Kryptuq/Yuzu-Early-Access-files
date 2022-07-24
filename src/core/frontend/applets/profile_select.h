// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <optional>
#include "common/uuid.h"

namespace Core::Frontend {

class ProfileSelectApplet {
public:
    virtual ~ProfileSelectApplet();

    virtual void SelectProfile(std::function<void(std::optional<Common::UUID>)> callback) const = 0;
};

class DefaultProfileSelectApplet final : public ProfileSelectApplet {
public:
    void SelectProfile(std::function<void(std::optional<Common::UUID>)> callback) const override;
};

} // namespace Core::Frontend
