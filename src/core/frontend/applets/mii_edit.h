// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
