// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

constexpr char STARTUP_CHECK_ENV_VAR[] = "YUZU_DO_STARTUP_CHECKS";

void CheckVulkan();
bool StartupChecks(const char* arg0, bool* has_broken_vulkan);

#ifdef _WIN32
bool SpawnChild(const char* arg0, PROCESS_INFORMATION* pi);
#endif
