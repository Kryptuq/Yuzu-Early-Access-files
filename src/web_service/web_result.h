// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include "common/common_types.h"

namespace WebService {
struct WebResult {
    enum class Code : u32 {
        Success,
        InvalidURL,
        CredentialsMissing,
        LibError,
        HttpError,
        WrongContent,
        NoWebservice,
    };
    Code result_code;
    std::string result_string;
    std::string returned_data;
};
} // namespace WebService
