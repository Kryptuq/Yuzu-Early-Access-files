// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <ctime>
#include <fstream>
#include <iomanip>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/hex_util.h"
#include "common/scm_rev.h"
#include "common/settings.h"
#include "core/arm/arm_interface.h"
#include "core/core.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/result.h"
#include "core/hle/service/hle_ipc.h"
#include "core/memory.h"
#include "core/reporter.h"

namespace {

std::filesystem::path GetPath(std::string_view type, u64 title_id, std::string_view timestamp) {
    return Common::FS::GetYuzuPath(Common::FS::YuzuPath::LogDir) / type /
           fmt::format("{:016X}_{}.json", title_id, timestamp);
}

std::string GetTimestamp() {
    const auto time = std::time(nullptr);
    return fmt::format("{:%FT%H-%M-%S}", *std::localtime(&time));
}

using namespace nlohmann;

void SaveToFile(const json& json, const std::filesystem::path& filename) {
    if (!Common::FS::CreateParentDirs(filename)) {
        LOG_ERROR(Core, "Failed to create path for '{}' to save report!",
                  Common::FS::PathToUTF8String(filename));
        return;
    }

    std::ofstream file;
    Common::FS::OpenFileStream(file, filename, std::ios_base::out | std::ios_base::trunc);

    file << std::setw(4) << json << std::endl;
}

json GetYuzuVersionData() {
    return {
        {"scm_rev", std::string(Common::g_scm_rev)},
        {"scm_branch", std::string(Common::g_scm_branch)},
        {"scm_desc", std::string(Common::g_scm_desc)},
        {"build_name", std::string(Common::g_build_name)},
        {"build_date", std::string(Common::g_build_date)},
        {"build_fullname", std::string(Common::g_build_fullname)},
        {"build_version", std::string(Common::g_build_version)},
    };
}

json GetReportCommonData(u64 title_id, Result result, const std::string& timestamp,
                         std::optional<u128> user_id = {}) {
    auto out = json{
        {"title_id", fmt::format("{:016X}", title_id)},
        {"result_raw", fmt::format("{:08X}", result.raw)},
        {"result_module", fmt::format("{:08X}", static_cast<u32>(result.module.Value()))},
        {"result_description", fmt::format("{:08X}", result.description.Value())},
        {"timestamp", timestamp},
    };

    if (user_id.has_value()) {
        out["user_id"] = fmt::format("{:016X}{:016X}", (*user_id)[1], (*user_id)[0]);
    }

    return out;
}

json GetProcessorStateData(const std::string& architecture, u64 entry_point, u64 sp, u64 pc,
                           u64 pstate, const std::array<u64, 31>& registers,
                           const std::optional<std::array<u64, 32>>& backtrace = {}) {
    auto out = json{
        {"entry_point", fmt::format("{:016X}", entry_point)},
        {"sp", fmt::format("{:016X}", sp)},
        {"pc", fmt::format("{:016X}", pc)},
        {"pstate", fmt::format("{:016X}", pstate)},
        {"architecture", architecture},
    };

    auto registers_out = json::object();
    for (std::size_t i = 0; i < registers.size(); ++i) {
        registers_out[fmt::format("X{:02d}", i)] = fmt::format("{:016X}", registers[i]);
    }

    out["registers"] = std::move(registers_out);

    if (backtrace.has_value()) {
        auto backtrace_out = json::array();
        for (const auto& entry : *backtrace) {
            backtrace_out.push_back(fmt::format("{:016X}", entry));
        }
        out["backtrace"] = std::move(backtrace_out);
    }

    return out;
}

json GetProcessorStateDataAuto(Core::System& system) {
    const auto* process{system.ApplicationProcess()};
    auto& arm{system.CurrentArmInterface()};

    Core::ARM_Interface::ThreadContext64 context{};
    arm.SaveContext(context);

    return GetProcessorStateData(process->Is64BitProcess() ? "AArch64" : "AArch32",
                                 GetInteger(process->PageTable().GetCodeRegionStart()), context.sp,
                                 context.pc, context.pstate, context.cpu_registers);
}

json GetBacktraceData(Core::System& system) {
    auto out = json::array();
    const auto& backtrace{system.CurrentArmInterface().GetBacktrace()};
    for (const auto& entry : backtrace) {
        out.push_back({
            {"module", entry.module},
            {"address", fmt::format("{:016X}", entry.address)},
            {"original_address", fmt::format("{:016X}", entry.original_address)},
            {"offset", fmt::format("{:016X}", entry.offset)},
            {"symbol_name", entry.name},
        });
    }

    return out;
}

json GetFullDataAuto(const std::string& timestamp, u64 title_id, Core::System& system) {
    json out;

    out["yuzu_version"] = GetYuzuVersionData();
    out["report_common"] = GetReportCommonData(title_id, ResultSuccess, timestamp);
    out["processor_state"] = GetProcessorStateDataAuto(system);
    out["backtrace"] = GetBacktraceData(system);

    return out;
}

template <bool read_value, typename DescriptorType>
json GetHLEBufferDescriptorData(const std::vector<DescriptorType>& buffer,
                                Core::Memory::Memory& memory) {
    auto buffer_out = json::array();
    for (const auto& desc : buffer) {
        auto entry = json{
            {"address", fmt::format("{:016X}", desc.Address())},
            {"size", fmt::format("{:016X}", desc.Size())},
        };

        if constexpr (read_value) {
            std::vector<u8> data(desc.Size());
            memory.ReadBlock(desc.Address(), data.data(), desc.Size());
            entry["data"] = Common::HexToString(data);
        }

        buffer_out.push_back(std::move(entry));
    }

    return buffer_out;
}

json GetHLERequestContextData(Service::HLERequestContext& ctx, Core::Memory::Memory& memory) {
    json out;

    auto cmd_buf = json::array();
    for (std::size_t i = 0; i < IPC::COMMAND_BUFFER_LENGTH; ++i) {
        cmd_buf.push_back(fmt::format("{:08X}", ctx.CommandBuffer()[i]));
    }

    out["command_buffer"] = std::move(cmd_buf);

    out["buffer_descriptor_a"] = GetHLEBufferDescriptorData<true>(ctx.BufferDescriptorA(), memory);
    out["buffer_descriptor_b"] = GetHLEBufferDescriptorData<false>(ctx.BufferDescriptorB(), memory);
    out["buffer_descriptor_c"] = GetHLEBufferDescriptorData<false>(ctx.BufferDescriptorC(), memory);
    out["buffer_descriptor_x"] = GetHLEBufferDescriptorData<true>(ctx.BufferDescriptorX(), memory);

    return out;
}

} // Anonymous namespace

namespace Core {

Reporter::Reporter(System& system_) : system(system_) {
    ClearFSAccessLog();
}

Reporter::~Reporter() = default;

void Reporter::SaveCrashReport(u64 title_id, Result result, u64 set_flags, u64 entry_point, u64 sp,
                               u64 pc, u64 pstate, u64 afsr0, u64 afsr1, u64 esr, u64 far,
                               const std::array<u64, 31>& registers,
                               const std::array<u64, 32>& backtrace, u32 backtrace_size,
                               const std::string& arch, u32 unk10) const {
    if (!IsReportingEnabled()) {
        return;
    }

    const auto timestamp = GetTimestamp();
    json out;

    out["yuzu_version"] = GetYuzuVersionData();
    out["report_common"] = GetReportCommonData(title_id, result, timestamp);

    auto proc_out = GetProcessorStateData(arch, entry_point, sp, pc, pstate, registers, backtrace);
    proc_out["set_flags"] = fmt::format("{:016X}", set_flags);
    proc_out["afsr0"] = fmt::format("{:016X}", afsr0);
    proc_out["afsr1"] = fmt::format("{:016X}", afsr1);
    proc_out["esr"] = fmt::format("{:016X}", esr);
    proc_out["far"] = fmt::format("{:016X}", far);
    proc_out["backtrace_size"] = fmt::format("{:08X}", backtrace_size);
    proc_out["unknown_10"] = fmt::format("{:08X}", unk10);

    out["processor_state"] = std::move(proc_out);

    SaveToFile(out, GetPath("crash_report", title_id, timestamp));
}

void Reporter::SaveSvcBreakReport(u32 type, bool signal_debugger, u64 info1, u64 info2,
                                  const std::optional<std::vector<u8>>& resolved_buffer) const {
    if (!IsReportingEnabled()) {
        return;
    }

    const auto timestamp = GetTimestamp();
    const auto title_id = system.GetApplicationProcessProgramID();
    auto out = GetFullDataAuto(timestamp, title_id, system);

    auto break_out = json{
        {"type", fmt::format("{:08X}", type)},
        {"signal_debugger", fmt::format("{}", signal_debugger)},
        {"info1", fmt::format("{:016X}", info1)},
        {"info2", fmt::format("{:016X}", info2)},
    };

    if (resolved_buffer.has_value()) {
        break_out["debug_buffer"] = Common::HexToString(*resolved_buffer);
    }

    out["svc_break"] = std::move(break_out);

    SaveToFile(out, GetPath("svc_break_report", title_id, timestamp));
}

void Reporter::SaveUnimplementedFunctionReport(Service::HLERequestContext& ctx, u32 command_id,
                                               const std::string& name,
                                               const std::string& service_name) const {
    if (!IsReportingEnabled()) {
        return;
    }

    const auto timestamp = GetTimestamp();
    const auto title_id = system.GetApplicationProcessProgramID();
    auto out = GetFullDataAuto(timestamp, title_id, system);

    auto function_out = GetHLERequestContextData(ctx, system.ApplicationMemory());
    function_out["command_id"] = command_id;
    function_out["function_name"] = name;
    function_out["service_name"] = service_name;

    out["function"] = std::move(function_out);

    SaveToFile(out, GetPath("unimpl_func_report", title_id, timestamp));
}

void Reporter::SaveUnimplementedAppletReport(
    u32 applet_id, u32 common_args_version, u32 library_version, u32 theme_color,
    bool startup_sound, u64 system_tick, const std::vector<std::vector<u8>>& normal_channel,
    const std::vector<std::vector<u8>>& interactive_channel) const {
    if (!IsReportingEnabled()) {
        return;
    }

    const auto timestamp = GetTimestamp();
    const auto title_id = system.GetApplicationProcessProgramID();
    auto out = GetFullDataAuto(timestamp, title_id, system);

    out["applet_common_args"] = {
        {"applet_id", fmt::format("{:02X}", applet_id)},
        {"common_args_version", fmt::format("{:08X}", common_args_version)},
        {"library_version", fmt::format("{:08X}", library_version)},
        {"theme_color", fmt::format("{:08X}", theme_color)},
        {"startup_sound", fmt::format("{}", startup_sound)},
        {"system_tick", fmt::format("{:016X}", system_tick)},
    };

    auto normal_out = json::array();
    for (const auto& data : normal_channel) {
        normal_out.push_back(Common::HexToString(data));
    }

    auto interactive_out = json::array();
    for (const auto& data : interactive_channel) {
        interactive_out.push_back(Common::HexToString(data));
    }

    out["applet_normal_data"] = std::move(normal_out);
    out["applet_interactive_data"] = std::move(interactive_out);

    SaveToFile(out, GetPath("unimpl_applet_report", title_id, timestamp));
}

void Reporter::SavePlayReport(PlayReportType type, u64 title_id,
                              const std::vector<std::span<const u8>>& data,
                              std::optional<u64> process_id, std::optional<u128> user_id) const {
    if (!IsReportingEnabled()) {
        return;
    }

    const auto timestamp = GetTimestamp();
    json out;

    out["yuzu_version"] = GetYuzuVersionData();
    out["report_common"] = GetReportCommonData(title_id, ResultSuccess, timestamp, user_id);

    auto data_out = json::array();
    for (const auto& d : data) {
        data_out.push_back(Common::HexToString(d));
    }

    if (process_id.has_value()) {
        out["play_report_process_id"] = fmt::format("{:016X}", *process_id);
    }

    out["play_report_type"] = fmt::format("{:02}", static_cast<u8>(type));
    out["play_report_data"] = std::move(data_out);

    SaveToFile(out, GetPath("play_report", title_id, timestamp));
}

void Reporter::SaveErrorReport(u64 title_id, Result result,
                               const std::optional<std::string>& custom_text_main,
                               const std::optional<std::string>& custom_text_detail) const {
    if (!IsReportingEnabled()) {
        return;
    }

    const auto timestamp = GetTimestamp();
    json out;

    out["yuzu_version"] = GetYuzuVersionData();
    out["report_common"] = GetReportCommonData(title_id, result, timestamp);
    out["processor_state"] = GetProcessorStateDataAuto(system);
    out["backtrace"] = GetBacktraceData(system);

    out["error_custom_text"] = {
        {"main", custom_text_main.value_or("")},
        {"detail", custom_text_detail.value_or("")},
    };

    SaveToFile(out, GetPath("error_report", title_id, timestamp));
}

void Reporter::SaveFSAccessLog(std::string_view log_message) const {
    const auto access_log_path =
        Common::FS::GetYuzuPath(Common::FS::YuzuPath::SDMCDir) / "FsAccessLog.txt";

    void(Common::FS::AppendStringToFile(access_log_path, Common::FS::FileType::TextFile,
                                        log_message));
}

void Reporter::SaveUserReport() const {
    if (!IsReportingEnabled()) {
        return;
    }

    const auto timestamp = GetTimestamp();
    const auto title_id = system.GetApplicationProcessProgramID();

    SaveToFile(GetFullDataAuto(timestamp, title_id, system),
               GetPath("user_report", title_id, timestamp));
}

void Reporter::ClearFSAccessLog() const {
    const auto access_log_path =
        Common::FS::GetYuzuPath(Common::FS::YuzuPath::SDMCDir) / "FsAccessLog.txt";

    Common::FS::IOFile access_log_file{access_log_path, Common::FS::FileAccessMode::Write,
                                       Common::FS::FileType::TextFile};

    if (!access_log_file.IsOpen()) {
        LOG_ERROR(Common_Filesystem, "Failed to clear the filesystem access log.");
    }
}

bool Reporter::IsReportingEnabled() const {
    return Settings::values.reporting_services.GetValue();
}

} // namespace Core
