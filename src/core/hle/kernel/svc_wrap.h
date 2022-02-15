// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/arm/arm_interface.h"
#include "core/core.h"
#include "core/hle/kernel/svc_types.h"
#include "core/hle/result.h"

namespace Kernel {

static inline u64 Param(const Core::System& system, int n) {
    return system.CurrentArmInterface().GetReg(n);
}

static inline u32 Param32(const Core::System& system, int n) {
    return static_cast<u32>(system.CurrentArmInterface().GetReg(n));
}

/**
 * HLE a function return from the current ARM userland process
 * @param system System context
 * @param result Result to return
 */
static inline void FuncReturn(Core::System& system, u64 result) {
    system.CurrentArmInterface().SetReg(0, result);
}

static inline void FuncReturn32(Core::System& system, u32 result) {
    system.CurrentArmInterface().SetReg(0, (u64)result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function wrappers that return type ResultCode

template <ResultCode func(Core::System&, u64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, Param(system, 0)).raw);
}

template <ResultCode func(Core::System&, u64, u64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, Param(system, 0), Param(system, 1)).raw);
}

template <ResultCode func(Core::System&, u32)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, static_cast<u32>(Param(system, 0))).raw);
}

template <ResultCode func(Core::System&, u32, u32)>
void SvcWrap64(Core::System& system) {
    FuncReturn(
        system,
        func(system, static_cast<u32>(Param(system, 0)), static_cast<u32>(Param(system, 1))).raw);
}

// Used by SetThreadActivity
template <ResultCode func(Core::System&, Handle, Svc::ThreadActivity)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, static_cast<u32>(Param(system, 0)),
                            static_cast<Svc::ThreadActivity>(Param(system, 1)))
                           .raw);
}

template <ResultCode func(Core::System&, u32, u64, u64, u64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, static_cast<u32>(Param(system, 0)), Param(system, 1),
                            Param(system, 2), Param(system, 3))
                           .raw);
}

// Used by MapProcessMemory and UnmapProcessMemory
template <ResultCode func(Core::System&, u64, u32, u64, u64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, Param(system, 0), static_cast<u32>(Param(system, 1)),
                            Param(system, 2), Param(system, 3))
                           .raw);
}

// Used by ControlCodeMemory
template <ResultCode func(Core::System&, Handle, u32, u64, u64, Svc::MemoryPermission)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, static_cast<Handle>(Param(system, 0)),
                            static_cast<u32>(Param(system, 1)), Param(system, 2), Param(system, 3),
                            static_cast<Svc::MemoryPermission>(Param(system, 4)))
                           .raw);
}

template <ResultCode func(Core::System&, u32*)>
void SvcWrap64(Core::System& system) {
    u32 param = 0;
    const u32 retval = func(system, &param).raw;
    system.CurrentArmInterface().SetReg(1, param);
    FuncReturn(system, retval);
}

template <ResultCode func(Core::System&, u32*, u32)>
void SvcWrap64(Core::System& system) {
    u32 param_1 = 0;
    const u32 retval = func(system, &param_1, static_cast<u32>(Param(system, 1))).raw;
    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

template <ResultCode func(Core::System&, u32*, u32*)>
void SvcWrap64(Core::System& system) {
    u32 param_1 = 0;
    u32 param_2 = 0;
    const u32 retval = func(system, &param_1, &param_2).raw;

    auto& arm_interface = system.CurrentArmInterface();
    arm_interface.SetReg(1, param_1);
    arm_interface.SetReg(2, param_2);

    FuncReturn(system, retval);
}

template <ResultCode func(Core::System&, u32*, u64)>
void SvcWrap64(Core::System& system) {
    u32 param_1 = 0;
    const u32 retval = func(system, &param_1, Param(system, 1)).raw;
    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

template <ResultCode func(Core::System&, u32*, u64, u32)>
void SvcWrap64(Core::System& system) {
    u32 param_1 = 0;
    const u32 retval =
        func(system, &param_1, Param(system, 1), static_cast<u32>(Param(system, 2))).raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

template <ResultCode func(Core::System&, u64*, u32)>
void SvcWrap64(Core::System& system) {
    u64 param_1 = 0;
    const u32 retval = func(system, &param_1, static_cast<u32>(Param(system, 1))).raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

template <ResultCode func(Core::System&, u64, u32)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, Param(system, 0), static_cast<u32>(Param(system, 1))).raw);
}

template <ResultCode func(Core::System&, u64*, u64)>
void SvcWrap64(Core::System& system) {
    u64 param_1 = 0;
    const u32 retval = func(system, &param_1, Param(system, 1)).raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

template <ResultCode func(Core::System&, u64*, u32, u32)>
void SvcWrap64(Core::System& system) {
    u64 param_1 = 0;
    const u32 retval = func(system, &param_1, static_cast<u32>(Param(system, 1)),
                            static_cast<u32>(Param(system, 2)))
                           .raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

// Used by GetResourceLimitLimitValue.
template <ResultCode func(Core::System&, u64*, Handle, LimitableResource)>
void SvcWrap64(Core::System& system) {
    u64 param_1 = 0;
    const u32 retval = func(system, &param_1, static_cast<Handle>(Param(system, 1)),
                            static_cast<LimitableResource>(Param(system, 2)))
                           .raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

template <ResultCode func(Core::System&, u32, u64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, static_cast<u32>(Param(system, 0)), Param(system, 1)).raw);
}

// Used by SetResourceLimitLimitValue
template <ResultCode func(Core::System&, Handle, LimitableResource, u64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, static_cast<Handle>(Param(system, 0)),
                            static_cast<LimitableResource>(Param(system, 1)), Param(system, 2))
                           .raw);
}

// Used by SetThreadCoreMask
template <ResultCode func(Core::System&, Handle, s32, u64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, static_cast<u32>(Param(system, 0)),
                            static_cast<s32>(Param(system, 1)), Param(system, 2))
                           .raw);
}

// Used by GetThreadCoreMask
template <ResultCode func(Core::System&, Handle, s32*, u64*)>
void SvcWrap64(Core::System& system) {
    s32 param_1 = 0;
    u64 param_2 = 0;
    const ResultCode retval = func(system, static_cast<u32>(Param(system, 2)), &param_1, &param_2);

    system.CurrentArmInterface().SetReg(1, param_1);
    system.CurrentArmInterface().SetReg(2, param_2);
    FuncReturn(system, retval.raw);
}

template <ResultCode func(Core::System&, u64, u64, u32, u32)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, Param(system, 0), Param(system, 1),
                            static_cast<u32>(Param(system, 2)), static_cast<u32>(Param(system, 3)))
                           .raw);
}

template <ResultCode func(Core::System&, u64, u64, u32, u64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, Param(system, 0), Param(system, 1),
                            static_cast<u32>(Param(system, 2)), Param(system, 3))
                           .raw);
}

template <ResultCode func(Core::System&, u32, u64, u32)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, static_cast<u32>(Param(system, 0)), Param(system, 1),
                            static_cast<u32>(Param(system, 2)))
                           .raw);
}

template <ResultCode func(Core::System&, u64, u64, u64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, Param(system, 0), Param(system, 1), Param(system, 2)).raw);
}

template <ResultCode func(Core::System&, u64, u64, u32)>
void SvcWrap64(Core::System& system) {
    FuncReturn(
        system,
        func(system, Param(system, 0), Param(system, 1), static_cast<u32>(Param(system, 2))).raw);
}

// Used by SetMemoryPermission
template <ResultCode func(Core::System&, u64, u64, Svc::MemoryPermission)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, Param(system, 0), Param(system, 1),
                            static_cast<Svc::MemoryPermission>(Param(system, 2)))
                           .raw);
}

// Used by MapSharedMemory
template <ResultCode func(Core::System&, Handle, u64, u64, Svc::MemoryPermission)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, static_cast<Handle>(Param(system, 0)), Param(system, 1),
                            Param(system, 2), static_cast<Svc::MemoryPermission>(Param(system, 3)))
                           .raw);
}

template <ResultCode func(Core::System&, u32, u64, u64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(
        system,
        func(system, static_cast<u32>(Param(system, 0)), Param(system, 1), Param(system, 2)).raw);
}

// Used by WaitSynchronization
template <ResultCode func(Core::System&, s32*, u64, s32, s64)>
void SvcWrap64(Core::System& system) {
    s32 param_1 = 0;
    const u32 retval = func(system, &param_1, Param(system, 1), static_cast<s32>(Param(system, 2)),
                            static_cast<s64>(Param(system, 3)))
                           .raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

template <ResultCode func(Core::System&, u64, u64, u32, s64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system, Param(system, 0), Param(system, 1),
                            static_cast<u32>(Param(system, 2)), static_cast<s64>(Param(system, 3)))
                           .raw);
}

// Used by GetInfo
template <ResultCode func(Core::System&, u64*, u64, Handle, u64)>
void SvcWrap64(Core::System& system) {
    u64 param_1 = 0;
    const u32 retval = func(system, &param_1, Param(system, 1),
                            static_cast<Handle>(Param(system, 2)), Param(system, 3))
                           .raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

template <ResultCode func(Core::System&, u32*, u64, u64, u64, u32, s32)>
void SvcWrap64(Core::System& system) {
    u32 param_1 = 0;
    const u32 retval = func(system, &param_1, Param(system, 1), Param(system, 2), Param(system, 3),
                            static_cast<u32>(Param(system, 4)), static_cast<s32>(Param(system, 5)))
                           .raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

// Used by CreateTransferMemory
template <ResultCode func(Core::System&, Handle*, u64, u64, Svc::MemoryPermission)>
void SvcWrap64(Core::System& system) {
    u32 param_1 = 0;
    const u32 retval = func(system, &param_1, Param(system, 1), Param(system, 2),
                            static_cast<Svc::MemoryPermission>(Param(system, 3)))
                           .raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

// Used by CreateCodeMemory
template <ResultCode func(Core::System&, Handle*, u64, u64)>
void SvcWrap64(Core::System& system) {
    u32 param_1 = 0;
    const u32 retval = func(system, &param_1, Param(system, 1), Param(system, 2)).raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

template <ResultCode func(Core::System&, Handle*, u64, u32, u32)>
void SvcWrap64(Core::System& system) {
    u32 param_1 = 0;
    const u32 retval = func(system, &param_1, Param(system, 1), static_cast<u32>(Param(system, 2)),
                            static_cast<u32>(Param(system, 3)))
                           .raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

// Used by WaitForAddress
template <ResultCode func(Core::System&, u64, Svc::ArbitrationType, s32, s64)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system,
               func(system, Param(system, 0), static_cast<Svc::ArbitrationType>(Param(system, 1)),
                    static_cast<s32>(Param(system, 2)), static_cast<s64>(Param(system, 3)))
                   .raw);
}

// Used by SignalToAddress
template <ResultCode func(Core::System&, u64, Svc::SignalType, s32, s32)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system,
               func(system, Param(system, 0), static_cast<Svc::SignalType>(Param(system, 1)),
                    static_cast<s32>(Param(system, 2)), static_cast<s32>(Param(system, 3)))
                   .raw);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function wrappers that return type u32

template <u32 func(Core::System&)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function wrappers that return type u64

template <u64 func(Core::System&)>
void SvcWrap64(Core::System& system) {
    FuncReturn(system, func(system));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Function wrappers that return type void

template <void func(Core::System&)>
void SvcWrap64(Core::System& system) {
    func(system);
}

template <void func(Core::System&, u32)>
void SvcWrap64(Core::System& system) {
    func(system, static_cast<u32>(Param(system, 0)));
}

template <void func(Core::System&, u32, u64, u64, u64)>
void SvcWrap64(Core::System& system) {
    func(system, static_cast<u32>(Param(system, 0)), Param(system, 1), Param(system, 2),
         Param(system, 3));
}

template <void func(Core::System&, s64)>
void SvcWrap64(Core::System& system) {
    func(system, static_cast<s64>(Param(system, 0)));
}

template <void func(Core::System&, u64, s32)>
void SvcWrap64(Core::System& system) {
    func(system, Param(system, 0), static_cast<s32>(Param(system, 1)));
}

template <void func(Core::System&, u64, u64)>
void SvcWrap64(Core::System& system) {
    func(system, Param(system, 0), Param(system, 1));
}

template <void func(Core::System&, u64, u64, u64)>
void SvcWrap64(Core::System& system) {
    func(system, Param(system, 0), Param(system, 1), Param(system, 2));
}

template <void func(Core::System&, u32, u64, u64)>
void SvcWrap64(Core::System& system) {
    func(system, static_cast<u32>(Param(system, 0)), Param(system, 1), Param(system, 2));
}

// Used by QueryMemory32, ArbitrateLock32
template <ResultCode func(Core::System&, u32, u32, u32)>
void SvcWrap32(Core::System& system) {
    FuncReturn32(system,
                 func(system, Param32(system, 0), Param32(system, 1), Param32(system, 2)).raw);
}

// Used by Break32
template <void func(Core::System&, u32, u32, u32)>
void SvcWrap32(Core::System& system) {
    func(system, Param32(system, 0), Param32(system, 1), Param32(system, 2));
}

// Used by ExitProcess32, ExitThread32
template <void func(Core::System&)>
void SvcWrap32(Core::System& system) {
    func(system);
}

// Used by GetCurrentProcessorNumber32
template <u32 func(Core::System&)>
void SvcWrap32(Core::System& system) {
    FuncReturn32(system, func(system));
}

// Used by SleepThread32
template <void func(Core::System&, u32, u32)>
void SvcWrap32(Core::System& system) {
    func(system, Param32(system, 0), Param32(system, 1));
}

// Used by CreateThread32
template <ResultCode func(Core::System&, Handle*, u32, u32, u32, u32, s32)>
void SvcWrap32(Core::System& system) {
    Handle param_1 = 0;

    const u32 retval = func(system, &param_1, Param32(system, 0), Param32(system, 1),
                            Param32(system, 2), Param32(system, 3), Param32(system, 4))
                           .raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

// Used by GetInfo32
template <ResultCode func(Core::System&, u32*, u32*, u32, u32, u32, u32)>
void SvcWrap32(Core::System& system) {
    u32 param_1 = 0;
    u32 param_2 = 0;

    const u32 retval = func(system, &param_1, &param_2, Param32(system, 0), Param32(system, 1),
                            Param32(system, 2), Param32(system, 3))
                           .raw;

    system.CurrentArmInterface().SetReg(1, param_1);
    system.CurrentArmInterface().SetReg(2, param_2);
    FuncReturn(system, retval);
}

// Used by GetThreadPriority32, ConnectToNamedPort32
template <ResultCode func(Core::System&, u32*, u32)>
void SvcWrap32(Core::System& system) {
    u32 param_1 = 0;
    const u32 retval = func(system, &param_1, Param32(system, 1)).raw;
    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

// Used by GetThreadId32
template <ResultCode func(Core::System&, u32*, u32*, u32)>
void SvcWrap32(Core::System& system) {
    u32 param_1 = 0;
    u32 param_2 = 0;

    const u32 retval = func(system, &param_1, &param_2, Param32(system, 1)).raw;
    system.CurrentArmInterface().SetReg(1, param_1);
    system.CurrentArmInterface().SetReg(2, param_2);
    FuncReturn(system, retval);
}

// Used by GetSystemTick32
template <void func(Core::System&, u32*, u32*)>
void SvcWrap32(Core::System& system) {
    u32 param_1 = 0;
    u32 param_2 = 0;

    func(system, &param_1, &param_2);
    system.CurrentArmInterface().SetReg(0, param_1);
    system.CurrentArmInterface().SetReg(1, param_2);
}

// Used by CreateEvent32
template <ResultCode func(Core::System&, Handle*, Handle*)>
void SvcWrap32(Core::System& system) {
    Handle param_1 = 0;
    Handle param_2 = 0;

    const u32 retval = func(system, &param_1, &param_2).raw;
    system.CurrentArmInterface().SetReg(1, param_1);
    system.CurrentArmInterface().SetReg(2, param_2);
    FuncReturn(system, retval);
}

// Used by GetThreadId32
template <ResultCode func(Core::System&, Handle, u32*, u32*, u32*)>
void SvcWrap32(Core::System& system) {
    u32 param_1 = 0;
    u32 param_2 = 0;
    u32 param_3 = 0;

    const u32 retval = func(system, Param32(system, 2), &param_1, &param_2, &param_3).raw;
    system.CurrentArmInterface().SetReg(1, param_1);
    system.CurrentArmInterface().SetReg(2, param_2);
    system.CurrentArmInterface().SetReg(3, param_3);
    FuncReturn(system, retval);
}

// Used by GetThreadCoreMask32
template <ResultCode func(Core::System&, Handle, s32*, u32*, u32*)>
void SvcWrap32(Core::System& system) {
    s32 param_1 = 0;
    u32 param_2 = 0;
    u32 param_3 = 0;

    const u32 retval = func(system, Param32(system, 2), &param_1, &param_2, &param_3).raw;
    system.CurrentArmInterface().SetReg(1, param_1);
    system.CurrentArmInterface().SetReg(2, param_2);
    system.CurrentArmInterface().SetReg(3, param_3);
    FuncReturn(system, retval);
}

// Used by SignalProcessWideKey32
template <void func(Core::System&, u32, s32)>
void SvcWrap32(Core::System& system) {
    func(system, static_cast<u32>(Param(system, 0)), static_cast<s32>(Param(system, 1)));
}

// Used by SetThreadActivity32
template <ResultCode func(Core::System&, Handle, Svc::ThreadActivity)>
void SvcWrap32(Core::System& system) {
    const u32 retval = func(system, static_cast<Handle>(Param(system, 0)),
                            static_cast<Svc::ThreadActivity>(Param(system, 1)))
                           .raw;
    FuncReturn(system, retval);
}

// Used by SetThreadPriority32
template <ResultCode func(Core::System&, Handle, u32)>
void SvcWrap32(Core::System& system) {
    const u32 retval =
        func(system, static_cast<Handle>(Param(system, 0)), static_cast<u32>(Param(system, 1))).raw;
    FuncReturn(system, retval);
}

// Used by SetMemoryAttribute32
template <ResultCode func(Core::System&, Handle, u32, u32, u32)>
void SvcWrap32(Core::System& system) {
    const u32 retval =
        func(system, static_cast<Handle>(Param(system, 0)), static_cast<u32>(Param(system, 1)),
             static_cast<u32>(Param(system, 2)), static_cast<u32>(Param(system, 3)))
            .raw;
    FuncReturn(system, retval);
}

// Used by MapSharedMemory32
template <ResultCode func(Core::System&, Handle, u32, u32, Svc::MemoryPermission)>
void SvcWrap32(Core::System& system) {
    const u32 retval = func(system, static_cast<Handle>(Param(system, 0)),
                            static_cast<u32>(Param(system, 1)), static_cast<u32>(Param(system, 2)),
                            static_cast<Svc::MemoryPermission>(Param(system, 3)))
                           .raw;
    FuncReturn(system, retval);
}

// Used by SetThreadCoreMask32
template <ResultCode func(Core::System&, Handle, s32, u32, u32)>
void SvcWrap32(Core::System& system) {
    const u32 retval =
        func(system, static_cast<Handle>(Param(system, 0)), static_cast<s32>(Param(system, 1)),
             static_cast<u32>(Param(system, 2)), static_cast<u32>(Param(system, 3)))
            .raw;
    FuncReturn(system, retval);
}

// Used by WaitProcessWideKeyAtomic32
template <ResultCode func(Core::System&, u32, u32, Handle, u32, u32)>
void SvcWrap32(Core::System& system) {
    const u32 retval =
        func(system, static_cast<u32>(Param(system, 0)), static_cast<u32>(Param(system, 1)),
             static_cast<Handle>(Param(system, 2)), static_cast<u32>(Param(system, 3)),
             static_cast<u32>(Param(system, 4)))
            .raw;
    FuncReturn(system, retval);
}

// Used by WaitForAddress32
template <ResultCode func(Core::System&, u32, Svc::ArbitrationType, s32, u32, u32)>
void SvcWrap32(Core::System& system) {
    const u32 retval = func(system, static_cast<u32>(Param(system, 0)),
                            static_cast<Svc::ArbitrationType>(Param(system, 1)),
                            static_cast<s32>(Param(system, 2)), static_cast<u32>(Param(system, 3)),
                            static_cast<u32>(Param(system, 4)))
                           .raw;
    FuncReturn(system, retval);
}

// Used by SignalToAddress32
template <ResultCode func(Core::System&, u32, Svc::SignalType, s32, s32)>
void SvcWrap32(Core::System& system) {
    const u32 retval = func(system, static_cast<u32>(Param(system, 0)),
                            static_cast<Svc::SignalType>(Param(system, 1)),
                            static_cast<s32>(Param(system, 2)), static_cast<s32>(Param(system, 3)))
                           .raw;
    FuncReturn(system, retval);
}

// Used by SendSyncRequest32, ArbitrateUnlock32
template <ResultCode func(Core::System&, u32)>
void SvcWrap32(Core::System& system) {
    FuncReturn(system, func(system, static_cast<u32>(Param(system, 0))).raw);
}

// Used by CreateTransferMemory32
template <ResultCode func(Core::System&, Handle*, u32, u32, Svc::MemoryPermission)>
void SvcWrap32(Core::System& system) {
    Handle handle = 0;
    const u32 retval = func(system, &handle, Param32(system, 1), Param32(system, 2),
                            static_cast<Svc::MemoryPermission>(Param32(system, 3)))
                           .raw;
    system.CurrentArmInterface().SetReg(1, handle);
    FuncReturn(system, retval);
}

// Used by WaitSynchronization32
template <ResultCode func(Core::System&, u32, u32, s32, u32, s32*)>
void SvcWrap32(Core::System& system) {
    s32 param_1 = 0;
    const u32 retval = func(system, Param32(system, 0), Param32(system, 1), Param32(system, 2),
                            Param32(system, 3), &param_1)
                           .raw;
    system.CurrentArmInterface().SetReg(1, param_1);
    FuncReturn(system, retval);
}

} // namespace Kernel
