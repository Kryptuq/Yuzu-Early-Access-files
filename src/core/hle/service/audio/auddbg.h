// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Audio {

class AudDbg final : public ServiceFramework<AudDbg> {
public:
    explicit AudDbg(Core::System& system_, const char* name);
    ~AudDbg() override;
};

} // namespace Service::Audio
