// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "core/hle/service/service.h"

namespace Service {
namespace FileSystem {
class FileSystemController;
}

namespace NVFlinger {
class NVFlinger;
}

namespace AM {

class AppletMessageQueue;

class AppletAE final : public ServiceFramework<AppletAE> {
public:
    explicit AppletAE(NVFlinger::NVFlinger& nvflinger_,
                      std::shared_ptr<AppletMessageQueue> msg_queue_, Core::System& system_);
    ~AppletAE() override;

    const std::shared_ptr<AppletMessageQueue>& GetMessageQueue() const;

private:
    void OpenSystemAppletProxy(Kernel::HLERequestContext& ctx);
    void OpenLibraryAppletProxy(Kernel::HLERequestContext& ctx);
    void OpenLibraryAppletProxyOld(Kernel::HLERequestContext& ctx);

    NVFlinger::NVFlinger& nvflinger;
    std::shared_ptr<AppletMessageQueue> msg_queue;
};

} // namespace AM
} // namespace Service
