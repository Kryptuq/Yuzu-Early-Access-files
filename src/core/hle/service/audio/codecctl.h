// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Audio {

class CodecCtl final : public ServiceFramework<CodecCtl> {
public:
    explicit CodecCtl(Core::System& system_);
    ~CodecCtl() override;
};

} // namespace Service::Audio
