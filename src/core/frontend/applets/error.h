// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <functional>

#include "core/hle/result.h"

namespace Core::Frontend {

class ErrorApplet {
public:
    using FinishedCallback = std::function<void()>;

    virtual ~ErrorApplet();

    virtual void ShowError(Result error, FinishedCallback finished) const = 0;

    virtual void ShowErrorWithTimestamp(Result error, std::chrono::seconds time,
                                        FinishedCallback finished) const = 0;

    virtual void ShowCustomErrorText(Result error, std::string dialog_text,
                                     std::string fullscreen_text,
                                     FinishedCallback finished) const = 0;
};

class DefaultErrorApplet final : public ErrorApplet {
public:
    void ShowError(Result error, FinishedCallback finished) const override;
    void ShowErrorWithTimestamp(Result error, std::chrono::seconds time,
                                FinishedCallback finished) const override;
    void ShowCustomErrorText(Result error, std::string main_text, std::string detail_text,
                             FinishedCallback finished) const override;
};

} // namespace Core::Frontend
