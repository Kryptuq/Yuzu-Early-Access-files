// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

constexpr char IS_CHILD_ENV_VAR[] = "YUZU_IS_CHILD";
constexpr char STARTUP_CHECK_ENV_VAR[] = "YUZU_DO_STARTUP_CHECKS";
constexpr char ENV_VAR_ENABLED_TEXT[] = "ON";

void CheckVulkan();
bool CheckEnvVars(bool* is_child);
bool StartupChecks(const char* arg0, bool* has_broken_vulkan);

#ifdef _WIN32
bool SpawnChild(const char* arg0, PROCESS_INFORMATION* pi, int flags);
#endif
