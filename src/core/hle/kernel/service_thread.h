// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>

namespace Kernel {

class HLERequestContext;
class KernelCore;
class KSession;
class SessionRequestManager;

class ServiceThread final {
public:
    explicit ServiceThread(KernelCore& kernel, const std::string& name);
    ~ServiceThread();

    void RegisterServerSession(KServerSession* session,
                               std::shared_ptr<SessionRequestManager> manager);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Kernel
