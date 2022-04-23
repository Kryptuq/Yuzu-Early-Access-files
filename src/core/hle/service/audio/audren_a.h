// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Audio {

class AudRenA final : public ServiceFramework<AudRenA> {
public:
    explicit AudRenA(Core::System& system_);
    ~AudRenA() override;
};

} // namespace Service::Audio
