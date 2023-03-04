// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "core/hle/service/service.h"

namespace Service {
namespace FileSystem {
class FileSystemController;
}

namespace Nvnflinger {
class Nvnflinger;
}

namespace AM {

class AppletMessageQueue;

class AppletAE final : public ServiceFramework<AppletAE> {
public:
    explicit AppletAE(Nvnflinger::Nvnflinger& nvnflinger_,
                      std::shared_ptr<AppletMessageQueue> msg_queue_, Core::System& system_);
    ~AppletAE() override;

    const std::shared_ptr<AppletMessageQueue>& GetMessageQueue() const;

private:
    void OpenSystemAppletProxy(HLERequestContext& ctx);
    void OpenLibraryAppletProxy(HLERequestContext& ctx);
    void OpenLibraryAppletProxyOld(HLERequestContext& ctx);

    Nvnflinger::Nvnflinger& nvnflinger;
    std::shared_ptr<AppletMessageQueue> msg_queue;
};

} // namespace AM
} // namespace Service
