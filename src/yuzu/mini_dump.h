// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

#include <dbghelp.h>

namespace MiniDump {

void CreateMiniDump(HANDLE process_handle, DWORD process_id, MINIDUMP_EXCEPTION_INFORMATION* info,
                    EXCEPTION_POINTERS* pep);

void DumpFromDebugEvent(DEBUG_EVENT& deb_ev, PROCESS_INFORMATION& pi);
bool SpawnDebuggee(const char* arg0, PROCESS_INFORMATION& pi);
void DebugDebuggee(PROCESS_INFORMATION& pi);

} // namespace MiniDump
