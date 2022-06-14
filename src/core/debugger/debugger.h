// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/common_types.h"

namespace Kernel {
class KThread;
}

namespace Core {
class System;

class DebuggerImpl;

class Debugger {
public:
    /**
     * Blocks and waits for a connection on localhost, port `server_port`.
     * Does not create the debugger if the port is already in use.
     */
    explicit Debugger(Core::System& system, u16 server_port);
    ~Debugger();

    /**
     * Notify the debugger that the given thread is stopped
     * (due to a breakpoint, or due to stopping after a successful step).
     *
     * The debugger will asynchronously halt emulation after the notification has
     * occurred. If another thread attempts to notify before emulation has stopped,
     * it is ignored and this method will return false. Otherwise it will return true.
     */
    bool NotifyThreadStopped(Kernel::KThread* thread);

    /**
     * Notify the debugger that a shutdown is being performed now and disconnect.
     */
    void NotifyShutdown();

private:
    std::unique_ptr<DebuggerImpl> impl;
};
} // namespace Core
