// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/arm/arm_interface.h"
#include "core/core.h"
#include "core/hle/kernel/svc.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel::Svc {

Result SendSyncRequestLight(Core::System& system, Handle session_handle, u32* args) {
    UNIMPLEMENTED();
    R_THROW(ResultNotImplemented);
}

Result ReplyAndReceiveLight(Core::System& system, Handle session_handle, u32* args) {
    UNIMPLEMENTED();
    R_THROW(ResultNotImplemented);
}

Result SendSyncRequestLight64(Core::System& system, Handle session_handle, u32* args) {
    R_RETURN(SendSyncRequestLight(system, session_handle, args));
}

Result ReplyAndReceiveLight64(Core::System& system, Handle session_handle, u32* args) {
    R_RETURN(ReplyAndReceiveLight(system, session_handle, args));
}

Result SendSyncRequestLight64From32(Core::System& system, Handle session_handle, u32* args) {
    R_RETURN(SendSyncRequestLight(system, session_handle, args));
}

Result ReplyAndReceiveLight64From32(Core::System& system, Handle session_handle, u32* args) {
    R_RETURN(ReplyAndReceiveLight(system, session_handle, args));
}

// Custom ABI implementation for light IPC.

template <typename F>
static void SvcWrap_LightIpc(Core::System& system, F&& cb) {
    auto& core = system.CurrentArmInterface();
    std::array<u32, 7> arguments{};

    Handle session_handle = static_cast<Handle>(core.GetReg(0));
    for (int i = 0; i < 7; i++) {
        arguments[i] = static_cast<u32>(core.GetReg(i + 1));
    }

    Result ret = cb(system, session_handle, arguments.data());

    core.SetReg(0, ret.raw);
    for (int i = 0; i < 7; i++) {
        core.SetReg(i + 1, arguments[i]);
    }
}

void SvcWrap_SendSyncRequestLight64(Core::System& system) {
    SvcWrap_LightIpc(system, SendSyncRequestLight64);
}

void SvcWrap_ReplyAndReceiveLight64(Core::System& system) {
    SvcWrap_LightIpc(system, ReplyAndReceiveLight64);
}

void SvcWrap_SendSyncRequestLight64From32(Core::System& system) {
    SvcWrap_LightIpc(system, SendSyncRequestLight64From32);
}

void SvcWrap_ReplyAndReceiveLight64From32(Core::System& system) {
    SvcWrap_LightIpc(system, ReplyAndReceiveLight64From32);
}

} // namespace Kernel::Svc
