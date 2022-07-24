// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include <fmt/format.h>

#include "yuzu/compatibility_list.h"

CompatibilityList::const_iterator FindMatchingCompatibilityEntry(
    const CompatibilityList& compatibility_list, u64 program_id) {
    return std::find_if(compatibility_list.begin(), compatibility_list.end(),
                        [program_id](const auto& element) {
                            std::string pid = fmt::format("{:016X}", program_id);
                            return element.first == pid;
                        });
}
