// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>

namespace Core::Frontend {

class MiiEditApplet {
public:
    virtual ~MiiEditApplet();

    virtual void ShowMiiEdit(const std::function<void()>& callback) const = 0;
};

class DefaultMiiEditApplet final : public MiiEditApplet {
public:
    void ShowMiiEdit(const std::function<void()>& callback) const override;
};

} // namespace Core::Frontend
