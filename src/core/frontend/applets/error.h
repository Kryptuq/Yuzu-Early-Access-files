// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <chrono>
#include <functional>

#include "core/hle/result.h"

namespace Core::Frontend {

class ErrorApplet {
public:
    virtual ~ErrorApplet();

    virtual void ShowError(ResultCode error, std::function<void()> finished) const = 0;

    virtual void ShowErrorWithTimestamp(ResultCode error, std::chrono::seconds time,
                                        std::function<void()> finished) const = 0;

    virtual void ShowCustomErrorText(ResultCode error, std::string dialog_text,
                                     std::string fullscreen_text,
                                     std::function<void()> finished) const = 0;
};

class DefaultErrorApplet final : public ErrorApplet {
public:
    void ShowError(ResultCode error, std::function<void()> finished) const override;
    void ShowErrorWithTimestamp(ResultCode error, std::chrono::seconds time,
                                std::function<void()> finished) const override;
    void ShowCustomErrorText(ResultCode error, std::string main_text, std::string detail_text,
                             std::function<void()> finished) const override;
};

} // namespace Core::Frontend
