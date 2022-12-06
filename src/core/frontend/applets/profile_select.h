// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <optional>
#include "common/uuid.h"

namespace Core::Frontend {

class ProfileSelectApplet {
public:
    using SelectProfileCallback = std::function<void(std::optional<Common::UUID>)>;

    virtual ~ProfileSelectApplet();

    virtual void SelectProfile(SelectProfileCallback callback) const = 0;
};

class DefaultProfileSelectApplet final : public ProfileSelectApplet {
public:
    void SelectProfile(SelectProfileCallback callback) const override;
};

} // namespace Core::Frontend
