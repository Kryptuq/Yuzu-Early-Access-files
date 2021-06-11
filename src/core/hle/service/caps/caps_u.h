// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Kernel {
class HLERequestContext;
}

namespace Service::Capture {

class CAPS_U final : public ServiceFramework<CAPS_U> {
public:
    explicit CAPS_U(Core::System& system_);
    ~CAPS_U() override;

private:
    void SetShimLibraryVersion(Kernel::HLERequestContext& ctx);
    void GetAlbumContentsFileListForApplication(Kernel::HLERequestContext& ctx);
    void GetAlbumFileList3AaeAruid(Kernel::HLERequestContext& ctx);
};

} // namespace Service::Capture
