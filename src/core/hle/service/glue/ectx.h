// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Glue {

class ECTX_AW final : public ServiceFramework<ECTX_AW> {
public:
    explicit ECTX_AW(Core::System& system_);
    ~ECTX_AW() override;
};

} // namespace Service::Glue
