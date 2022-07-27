// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/logging/filter.h"

namespace Common::Log {

class Filter;

/// Initializes the logging system. This should be the first thing called in main.
void Initialize();

void Start();

void DisableLoggingInTests();

/**
 * The global filter will prevent any messages from even being processed if they are filtered.
 */
void SetGlobalFilter(const Filter& filter);

void SetColorConsoleBackendEnabled(bool enabled);
} // namespace Common::Log
