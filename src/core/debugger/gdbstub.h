// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "core/debugger/debugger_interface.h"
#include "core/debugger/gdbstub_arch.h"

namespace Core {

class System;

class GDBStub : public DebuggerFrontend {
public:
    explicit GDBStub(DebuggerBackend& backend, Core::System& system);
    ~GDBStub() override;

    void Connected() override;
    void Stopped(Kernel::KThread* thread) override;
    void ShuttingDown() override;
    void Watchpoint(Kernel::KThread* thread, const Kernel::DebugWatchpoint& watch) override;
    std::vector<DebuggerAction> ClientData(std::span<const u8> data) override;

private:
    void ProcessData(std::vector<DebuggerAction>& actions);
    void ExecuteCommand(std::string_view packet, std::vector<DebuggerAction>& actions);
    void HandleVCont(std::string_view command, std::vector<DebuggerAction>& actions);
    void HandleQuery(std::string_view command);
    void HandleBreakpointInsert(std::string_view command);
    void HandleBreakpointRemove(std::string_view command);
    std::vector<char>::const_iterator CommandEnd() const;
    std::optional<std::string> DetachCommand();
    Kernel::KThread* GetThreadByID(u64 thread_id);

    void SendReply(std::string_view data);
    void SendStatus(char status);

private:
    Core::System& system;
    std::unique_ptr<GDBStubArch> arch;
    std::vector<char> current_command;
    std::map<VAddr, u32> replaced_instructions;
    bool no_ack{};
};

} // namespace Core
