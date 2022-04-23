// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/common_funcs.h"

#include "common/settings.h"

void assert_handle_failure() {
    if (Settings::values.use_debug_asserts) {
        Crash();
    }
}
