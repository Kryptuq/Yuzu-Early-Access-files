// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>
#include "common/common_types.h"

union Result;

namespace Kernel {
class HLERequestContext;
} // namespace Kernel

namespace Service::LM {
struct LogMessage;
} // namespace Service::LM

namespace Core {

class System;

class Reporter {
public:
    explicit Reporter(System& system_);
    ~Reporter();

    // Used by fatal services
    void SaveCrashReport(u64 title_id, Result result, u64 set_flags, u64 entry_point, u64 sp,
                         u64 pc, u64 pstate, u64 afsr0, u64 afsr1, u64 esr, u64 far,
                         const std::array<u64, 31>& registers, const std::array<u64, 32>& backtrace,
                         u32 backtrace_size, const std::string& arch, u32 unk10) const;

    // Used by syscall svcBreak
    void SaveSvcBreakReport(u32 type, bool signal_debugger, u64 info1, u64 info2,
                            std::optional<std::vector<u8>> resolved_buffer = {}) const;

    // Used by HLE service handler
    void SaveUnimplementedFunctionReport(Kernel::HLERequestContext& ctx, u32 command_id,
                                         const std::string& name,
                                         const std::string& service_name) const;

    // Used by stub applet implementation
    void SaveUnimplementedAppletReport(u32 applet_id, u32 common_args_version, u32 library_version,
                                       u32 theme_color, bool startup_sound, u64 system_tick,
                                       std::vector<std::vector<u8>> normal_channel,
                                       std::vector<std::vector<u8>> interactive_channel) const;

    enum class PlayReportType {
        Old,
        Old2,
        New,
        System,
    };

    void SavePlayReport(PlayReportType type, u64 title_id, std::vector<std::vector<u8>> data,
                        std::optional<u64> process_id = {}, std::optional<u128> user_id = {}) const;

    // Used by error applet
    void SaveErrorReport(u64 title_id, Result result,
                         std::optional<std::string> custom_text_main = {},
                         std::optional<std::string> custom_text_detail = {}) const;

    void SaveFSAccessLog(std::string_view log_message) const;

    // Can be used anywhere to generate a backtrace and general info report at any point during
    // execution. Not intended to be used for anything other than debugging or testing.
    void SaveUserReport() const;

private:
    void ClearFSAccessLog() const;

    bool IsReportingEnabled() const;

    System& system;
};

} // namespace Core
