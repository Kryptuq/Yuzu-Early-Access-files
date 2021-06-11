// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
