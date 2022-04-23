// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::SM {

class Controller final : public ServiceFramework<Controller> {
public:
    explicit Controller(Core::System& system_);
    ~Controller() override;

private:
    void ConvertCurrentObjectToDomain(Kernel::HLERequestContext& ctx);
    void CloneCurrentObject(Kernel::HLERequestContext& ctx);
    void CloneCurrentObjectEx(Kernel::HLERequestContext& ctx);
    void QueryPointerBufferSize(Kernel::HLERequestContext& ctx);
};

} // namespace Service::SM
