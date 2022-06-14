// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <span>
#include <vector>

#include "common/common_types.h"

namespace Kernel {
class KThread;
}

namespace Core {

enum class DebuggerAction {
    Interrupt,          ///< Stop emulation as soon as possible.
    Continue,           ///< Resume emulation.
    StepThreadLocked,   ///< Step the currently-active thread without resuming others.
    StepThreadUnlocked, ///< Step the currently-active thread and resume others.
    ShutdownEmulation,  ///< Shut down the emulator.
};

class DebuggerBackend {
public:
    virtual ~DebuggerBackend() = default;

    /**
     * Can be invoked from a callback to synchronously wait for more data.
     * Will return as soon as least one byte is received. Reads up to 4096 bytes.
     */
    virtual std::span<const u8> ReadFromClient() = 0;

    /**
     * Can be invoked from a callback to write data to the client.
     * Returns immediately after the data is sent.
     */
    virtual void WriteToClient(std::span<const u8> data) = 0;

    /**
     * Gets the currently active thread when the debugger is stopped.
     */
    virtual Kernel::KThread* GetActiveThread() = 0;

    /**
     * Sets the currently active thread when the debugger is stopped.
     */
    virtual void SetActiveThread(Kernel::KThread* thread) = 0;
};

class DebuggerFrontend {
public:
    explicit DebuggerFrontend(DebuggerBackend& backend_) : backend{backend_} {}

    virtual ~DebuggerFrontend() = default;

    /**
     * Called after the client has successfully connected to the port.
     */
    virtual void Connected() = 0;

    /**
     * Called when emulation has stopped.
     */
    virtual void Stopped(Kernel::KThread* thread) = 0;

    /**
     * Called when emulation is shutting down.
     */
    virtual void ShuttingDown() = 0;

    /**
     * Called when new data is asynchronously received on the client socket.
     * A list of actions to perform is returned.
     */
    [[nodiscard]] virtual std::vector<DebuggerAction> ClientData(std::span<const u8> data) = 0;

protected:
    DebuggerBackend& backend;
};

} // namespace Core
