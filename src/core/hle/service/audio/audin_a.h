// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Audio {

class AudInA final : public ServiceFramework<AudInA> {
public:
    explicit AudInA(Core::System& system_);
    ~AudInA() override;
};

} // namespace Service::Audio
