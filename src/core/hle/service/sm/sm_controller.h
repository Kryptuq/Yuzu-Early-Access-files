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
    void ConvertCurrentObjectToDomain(HLERequestContext& ctx);
    void CloneCurrentObject(HLERequestContext& ctx);
    void CloneCurrentObjectEx(HLERequestContext& ctx);
    void QueryPointerBufferSize(HLERequestContext& ctx);
};

} // namespace Service::SM
