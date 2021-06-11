// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
