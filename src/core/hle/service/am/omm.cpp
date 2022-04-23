// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/am/omm.h"

namespace Service::AM {

OMM::OMM(Core::System& system_) : ServiceFramework{system_, "omm"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetOperationMode"},
        {1, nullptr, "GetOperationModeChangeEvent"},
        {2, nullptr, "EnableAudioVisual"},
        {3, nullptr, "DisableAudioVisual"},
        {4, nullptr, "EnterSleepAndWait"},
        {5, nullptr, "GetCradleStatus"},
        {6, nullptr, "FadeInDisplay"},
        {7, nullptr, "FadeOutDisplay"},
        {8, nullptr, "GetCradleFwVersion"},
        {9, nullptr, "NotifyCecSettingsChanged"},
        {10, nullptr, "SetOperationModePolicy"},
        {11, nullptr, "GetDefaultDisplayResolution"},
        {12, nullptr, "GetDefaultDisplayResolutionChangeEvent"},
        {13, nullptr, "UpdateDefaultDisplayResolution"},
        {14, nullptr, "ShouldSleepOnBoot"},
        {15, nullptr, "NotifyHdcpApplicationExecutionStarted"},
        {16, nullptr, "NotifyHdcpApplicationExecutionFinished"},
        {17, nullptr, "NotifyHdcpApplicationDrawingStarted"},
        {18, nullptr, "NotifyHdcpApplicationDrawingFinished"},
        {19, nullptr, "GetHdcpAuthenticationFailedEvent"},
        {20, nullptr, "GetHdcpAuthenticationFailedEmulationEnabled"},
        {21, nullptr, "SetHdcpAuthenticationFailedEmulation"},
        {22, nullptr, "GetHdcpStateChangeEvent"},
        {23, nullptr, "GetHdcpState"},
        {24, nullptr, "ShowCardUpdateProcessing"},
        {25, nullptr, "SetApplicationCecSettingsAndNotifyChanged"},
        {26, nullptr, "GetOperationModeSystemInfo"},
        {27, nullptr, "GetAppletFullAwakingSystemEvent"},
        {28, nullptr, "CreateCradleFirmwareUpdater"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

OMM::~OMM() = default;

} // namespace Service::AM
