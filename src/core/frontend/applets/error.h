// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <functional>

#include "core/hle/result.h"

namespace Core::Frontend {

class ErrorApplet {
public:
    virtual ~ErrorApplet();

    virtual void ShowError(Result error, std::function<void()> finished) const = 0;

    virtual void ShowErrorWithTimestamp(Result error, std::chrono::seconds time,
                                        std::function<void()> finished) const = 0;

    virtual void ShowCustomErrorText(Result error, std::string dialog_text,
                                     std::string fullscreen_text,
                                     std::function<void()> finished) const = 0;
};

class DefaultErrorApplet final : public ErrorApplet {
public:
    void ShowError(Result error, std::function<void()> finished) const override;
    void ShowErrorWithTimestamp(Result error, std::chrono::seconds time,
                                std::function<void()> finished) const override;
    void ShowCustomErrorText(Result error, std::string main_text, std::string detail_text,
                             std::function<void()> finished) const override;
};

} // namespace Core::Frontend
