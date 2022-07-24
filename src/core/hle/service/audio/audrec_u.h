// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Audio {

class AudRecU final : public ServiceFramework<AudRecU> {
public:
    explicit AudRecU(Core::System& system_);
    ~AudRecU() override;
};

} // namespace Service::Audio
