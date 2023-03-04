// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Capture {

class CAPS_U final : public ServiceFramework<CAPS_U> {
public:
    explicit CAPS_U(Core::System& system_);
    ~CAPS_U() override;

private:
    void SetShimLibraryVersion(HLERequestContext& ctx);
    void GetAlbumContentsFileListForApplication(HLERequestContext& ctx);
    void GetAlbumFileList3AaeAruid(HLERequestContext& ctx);
};

} // namespace Service::Capture
