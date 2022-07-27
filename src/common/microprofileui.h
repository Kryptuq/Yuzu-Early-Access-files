// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/microprofile.h"

// Customized Citra settings.
// This file wraps the MicroProfile header so that these are consistent everywhere.
#define MICROPROFILE_TEXT_WIDTH 6
#define MICROPROFILE_TEXT_HEIGHT 12
#define MICROPROFILE_HELP_ALT "Right-Click"
#define MICROPROFILE_HELP_MOD "Ctrl"

// This isn't included by microprofileui.h :(
#include <cstdlib> // For std::abs

#include <microprofileui.h>
