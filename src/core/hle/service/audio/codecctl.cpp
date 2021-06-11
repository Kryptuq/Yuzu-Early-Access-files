// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/audio/codecctl.h"

namespace Service::Audio {

CodecCtl::CodecCtl(Core::System& system_) : ServiceFramework{system_, "codecctl"} {
    static const FunctionInfo functions[] = {
        {0, nullptr, "Initialize"},
        {1, nullptr, "Finalize"},
        {2, nullptr, "Sleep"},
        {3, nullptr, "Wake"},
        {4, nullptr, "SetVolume"},
        {5, nullptr, "GetVolumeMax"},
        {6, nullptr, "GetVolumeMin"},
        {7, nullptr, "SetActiveTarget"},
        {8, nullptr, "GetActiveTarget"},
        {9, nullptr, "BindHeadphoneMicJackInterrupt"},
        {10, nullptr, "IsHeadphoneMicJackInserted"},
        {11, nullptr, "ClearHeadphoneMicJackInterrupt"},
        {12, nullptr, "IsRequested"},
    };
    RegisterHandlers(functions);
}

CodecCtl::~CodecCtl() = default;

} // namespace Service::Audio
