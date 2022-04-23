// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::AM {

class OMM final : public ServiceFramework<OMM> {
public:
    explicit OMM(Core::System& system_);
    ~OMM() override;
};

} // namespace Service::AM
