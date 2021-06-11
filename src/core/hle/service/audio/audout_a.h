// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
