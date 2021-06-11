// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/common_funcs.h"

#include "common/settings.h"

void assert_handle_failure() {
    if (Settings::values.use_debug_asserts) {
        Crash();
    }
}
