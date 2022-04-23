// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/caps/caps_su.h"

namespace Service::Capture {

CAPS_SU::CAPS_SU(Core::System& system_) : ServiceFramework{system_, "caps:su"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {32, &CAPS_SU::SetShimLibraryVersion, "SetShimLibraryVersion"},
        {201, nullptr, "SaveScreenShot"},
        {203, nullptr, "SaveScreenShotEx0"},
        {205, nullptr, "SaveScreenShotEx1"},
        {210, nullptr, "SaveScreenShotEx2"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

CAPS_SU::~CAPS_SU() = default;

void CAPS_SU::SetShimLibraryVersion(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto library_version{rp.Pop<u64>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_Capture, "(STUBBED) called. library_version={}, applet_resource_user_id={}",
                library_version, applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

} // namespace Service::Capture
