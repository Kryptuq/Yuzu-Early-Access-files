// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Audio {

class AudRecA final : public ServiceFramework<AudRecA> {
public:
    explicit AudRecA(Core::System& system_);
    ~AudRecA() override;
};

} // namespace Service::Audio
