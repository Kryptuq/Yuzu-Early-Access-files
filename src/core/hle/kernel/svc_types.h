// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"

namespace Kernel::Svc {

enum class MemoryState : u32 {
    Free = 0x00,
    Io = 0x01,
    Static = 0x02,
    Code = 0x03,
    CodeData = 0x04,
    Normal = 0x05,
    Shared = 0x06,
    Alias = 0x07,
    AliasCode = 0x08,
    AliasCodeData = 0x09,
    Ipc = 0x0A,
    Stack = 0x0B,
    ThreadLocal = 0x0C,
    Transfered = 0x0D,
    SharedTransfered = 0x0E,
    SharedCode = 0x0F,
    Inaccessible = 0x10,
    NonSecureIpc = 0x11,
    NonDeviceIpc = 0x12,
    Kernel = 0x13,
    GeneratedCode = 0x14,
    CodeOut = 0x15,
    Coverage = 0x16,
    Insecure = 0x17,
};
DECLARE_ENUM_FLAG_OPERATORS(MemoryState);

enum class MemoryAttribute : u32 {
    Locked = (1 << 0),
    IpcLocked = (1 << 1),
    DeviceShared = (1 << 2),
    Uncached = (1 << 3),
};
DECLARE_ENUM_FLAG_OPERATORS(MemoryAttribute);

enum class MemoryPermission : u32 {
    None = (0 << 0),
    Read = (1 << 0),
    Write = (1 << 1),
    Execute = (1 << 2),
    ReadWrite = Read | Write,
    ReadExecute = Read | Execute,
    DontCare = (1 << 28),
};
DECLARE_ENUM_FLAG_OPERATORS(MemoryPermission);

struct MemoryInfo {
    u64 addr{};
    u64 size{};
    MemoryState state{};
    MemoryAttribute attr{};
    MemoryPermission perm{};
    u32 ipc_refcount{};
    u32 device_refcount{};
    u32 padding{};
};

enum class SignalType : u32 {
    Signal = 0,
    SignalAndIncrementIfEqual = 1,
    SignalAndModifyByWaitingCountIfEqual = 2,
};

enum class ArbitrationType : u32 {
    WaitIfLessThan = 0,
    DecrementAndWaitIfLessThan = 1,
    WaitIfEqual = 2,
};

enum class YieldType : s64 {
    WithoutCoreMigration = 0,
    WithCoreMigration = -1,
    ToAnyThread = -2,
};

enum class ThreadExitReason : u32 {
    ExitThread = 0,
    TerminateThread = 1,
    ExitProcess = 2,
    TerminateProcess = 3,
};

enum class ThreadActivity : u32 {
    Runnable = 0,
    Paused = 1,
};

constexpr inline s32 IdealCoreDontCare = -1;
constexpr inline s32 IdealCoreUseProcessValue = -2;
constexpr inline s32 IdealCoreNoUpdate = -3;

constexpr inline s32 LowestThreadPriority = 63;
constexpr inline s32 HighestThreadPriority = 0;

constexpr inline s32 SystemThreadPriorityHighest = 16;

enum class ProcessState : u32 {
    Created = 0,
    CreatedAttached = 1,
    Running = 2,
    Crashed = 3,
    RunningAttached = 4,
    Terminating = 5,
    Terminated = 6,
    DebugBreak = 7,
};

enum class ProcessExitReason : u32 {
    ExitProcess = 0,
    TerminateProcess = 1,
    Exception = 2,
};

constexpr inline size_t ThreadLocalRegionSize = 0x200;

// Debug types.
enum class DebugEvent : u32 {
    CreateProcess = 0,
    CreateThread = 1,
    ExitProcess = 2,
    ExitThread = 3,
    Exception = 4,
};

enum class DebugException : u32 {
    UndefinedInstruction = 0,
    InstructionAbort = 1,
    DataAbort = 2,
    AlignmentFault = 3,
    DebuggerAttached = 4,
    BreakPoint = 5,
    UserBreak = 6,
    DebuggerBreak = 7,
    UndefinedSystemCall = 8,
    MemorySystemError = 9,
};

} // namespace Kernel::Svc
