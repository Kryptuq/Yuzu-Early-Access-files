// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/frontend/applets/error.h"

namespace Core::Frontend {

ErrorApplet::~ErrorApplet() = default;

void DefaultErrorApplet::ShowError(Result error, FinishedCallback finished) const {
    LOG_CRITICAL(Service_Fatal, "Application requested error display: {:04}-{:04} (raw={:08X})",
                 error.module.Value(), error.description.Value(), error.raw);
}

void DefaultErrorApplet::ShowErrorWithTimestamp(Result error, std::chrono::seconds time,
                                                FinishedCallback finished) const {
    LOG_CRITICAL(
        Service_Fatal,
        "Application requested error display: {:04X}-{:04X} (raw={:08X}) with timestamp={:016X}",
        error.module.Value(), error.description.Value(), error.raw, time.count());
}

void DefaultErrorApplet::ShowCustomErrorText(Result error, std::string main_text,
                                             std::string detail_text,
                                             FinishedCallback finished) const {
    LOG_CRITICAL(Service_Fatal,
                 "Application requested custom error with error_code={:04X}-{:04X} (raw={:08X})",
                 error.module.Value(), error.description.Value(), error.raw);
    LOG_CRITICAL(Service_Fatal, "    Main Text: {}", main_text);
    LOG_CRITICAL(Service_Fatal, "    Detail Text: {}", detail_text);
}

} // namespace Core::Frontend
