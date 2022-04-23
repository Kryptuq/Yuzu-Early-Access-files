// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Audio {

class AudOutA final : public ServiceFramework<AudOutA> {
public:
    explicit AudOutA(Core::System& system_);
    ~AudOutA() override;
};

} // namespace Service::Audio
