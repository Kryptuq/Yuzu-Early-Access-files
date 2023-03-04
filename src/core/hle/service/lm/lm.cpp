// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string>

#include <optional>
#include <unordered_map>
#include <boost/container_hash/hash.hpp>
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/lm/lm.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"

namespace Service::LM {
enum class LogSeverity : u8 {
    Trace = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Fatal = 4,
};

// To keep flags out of hashing as well as the payload size
struct LogPacketHeaderEntry {
    u64_le pid{};
    u64_le tid{};
    LogSeverity severity{};
    u8 verbosity{};

    auto operator<=>(const LogPacketHeaderEntry&) const = default;
};
} // namespace Service::LM

namespace std {
template <>
struct hash<Service::LM::LogPacketHeaderEntry> {
    std::size_t operator()(const Service::LM::LogPacketHeaderEntry& k) const noexcept {
        std::size_t seed{};
        boost::hash_combine(seed, k.pid);
        boost::hash_combine(seed, k.tid);
        boost::hash_combine(seed, k.severity);
        boost::hash_combine(seed, k.verbosity);
        return seed;
    }
};
} // namespace std

namespace Service::LM {
namespace {
std::string_view NameOf(LogSeverity severity) {
    switch (severity) {
    case LogSeverity::Trace:
        return "TRACE";
    case LogSeverity::Info:
        return "INFO";
    case LogSeverity::Warning:
        return "WARNING";
    case LogSeverity::Error:
        return "ERROR";
    case LogSeverity::Fatal:
        return "FATAL";
    default:
        return "UNKNOWN";
    }
}
} // Anonymous namespace

enum class LogDestination : u32 {
    TargetManager = 1 << 0,
    Uart = 1 << 1,
    UartSleep = 1 << 2,
    All = 0xffff,
};
DECLARE_ENUM_FLAG_OPERATORS(LogDestination);

enum class LogPacketFlags : u8 {
    Head = 1 << 0,
    Tail = 1 << 1,
    LittleEndian = 1 << 2,
};
DECLARE_ENUM_FLAG_OPERATORS(LogPacketFlags);

class ILogger final : public ServiceFramework<ILogger> {
public:
    explicit ILogger(Core::System& system_) : ServiceFramework{system_, "ILogger"} {
        static const FunctionInfo functions[] = {
            {0, &ILogger::Log, "Log"},
            {1, &ILogger::SetDestination, "SetDestination"},
        };
        RegisterHandlers(functions);
    }

private:
    void Log(HLERequestContext& ctx) {
        std::size_t offset{};
        const auto data = ctx.ReadBuffer();

        // This function only succeeds - Get that out of the way
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);

        if (data.size() < sizeof(LogPacketHeader)) {
            LOG_ERROR(Service_LM, "Data size is too small for header! size={}", data.size());
            return;
        }

        LogPacketHeader header{};
        std::memcpy(&header, data.data(), sizeof(LogPacketHeader));
        offset += sizeof(LogPacketHeader);

        const LogPacketHeaderEntry entry{
            .pid = header.pid,
            .tid = header.tid,
            .severity = header.severity,
            .verbosity = header.verbosity,
        };

        if (True(header.flags & LogPacketFlags::Head)) {
            std::vector<u8> tmp(data.size() - sizeof(LogPacketHeader));
            std::memcpy(tmp.data(), data.data() + offset, tmp.size());
            entries.insert_or_assign(entry, std::move(tmp));
        } else {
            const auto entry_iter = entries.find(entry);

            // Append to existing entry
            if (entry_iter == entries.cend()) {
                LOG_ERROR(Service_LM, "Log entry does not exist!");
                return;
            }

            auto& existing_entry = entry_iter->second;
            const auto base = existing_entry.size();
            existing_entry.resize(base + (data.size() - sizeof(LogPacketHeader)));
            std::memcpy(existing_entry.data() + base, data.data() + offset,
                        (data.size() - sizeof(LogPacketHeader)));
        }

        if (True(header.flags & LogPacketFlags::Tail)) {
            auto it = entries.find(entry);
            if (it == entries.end()) {
                LOG_ERROR(Service_LM, "Log entry does not exist!");
                return;
            }
            ParseLog(it->first, it->second);
            entries.erase(it);
        }
    }

    void SetDestination(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto log_destination = rp.PopEnum<LogDestination>();

        LOG_DEBUG(Service_LM, "called, destination={}", DestinationToString(log_destination));
        destination = log_destination;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    u64 ReadLeb128(const std::vector<u8>& data, std::size_t& offset) {
        u64 result{};
        u32 shift{};

        for (std::size_t i = 0; i < sizeof(u64); i++) {
            const auto v = data[offset];
            result |= (static_cast<u64>(v & 0x7f) << shift);
            shift += 7;
            offset++;
            if (offset >= data.size() || ((v & 0x80) == 0)) {
                break;
            }
        }
        return result;
    }

    std::optional<std::string> ReadString(const std::vector<u8>& data, std::size_t& offset,
                                          std::size_t length) {
        if (length == 0) {
            return std::nullopt;
        }
        const auto length_to_read = std::min(length, data.size() - offset);

        std::string output(length_to_read, '\0');
        std::memcpy(output.data(), data.data() + offset, length_to_read);
        offset += length_to_read;
        return output;
    }

    u32_le ReadAsU32(const std::vector<u8>& data, std::size_t& offset, std::size_t length) {
        ASSERT(length == sizeof(u32));
        u32_le output{};
        std::memcpy(&output, data.data() + offset, sizeof(u32));
        offset += length;
        return output;
    }

    u64_le ReadAsU64(const std::vector<u8>& data, std::size_t& offset, std::size_t length) {
        ASSERT(length == sizeof(u64));
        u64_le output{};
        std::memcpy(&output, data.data() + offset, sizeof(u64));
        offset += length;
        return output;
    }

    void ParseLog(const LogPacketHeaderEntry entry, const std::vector<u8>& log_data) {
        // Possible entries
        std::optional<std::string> text_log;
        std::optional<u32> line_number;
        std::optional<std::string> file_name;
        std::optional<std::string> function_name;
        std::optional<std::string> module_name;
        std::optional<std::string> thread_name;
        std::optional<u64> log_pack_drop_count;
        std::optional<s64> user_system_clock;
        std::optional<std::string> process_name;

        std::size_t offset{};
        while (offset < log_data.size()) {
            const auto key = static_cast<LogDataChunkKey>(ReadLeb128(log_data, offset));
            const auto chunk_size = ReadLeb128(log_data, offset);

            switch (key) {
            case LogDataChunkKey::LogSessionBegin:
            case LogDataChunkKey::LogSessionEnd:
                break;
            case LogDataChunkKey::TextLog:
                text_log = ReadString(log_data, offset, chunk_size);
                break;
            case LogDataChunkKey::LineNumber:
                line_number = ReadAsU32(log_data, offset, chunk_size);
                break;
            case LogDataChunkKey::FileName:
                file_name = ReadString(log_data, offset, chunk_size);
                break;
            case LogDataChunkKey::FunctionName:
                function_name = ReadString(log_data, offset, chunk_size);
                break;
            case LogDataChunkKey::ModuleName:
                module_name = ReadString(log_data, offset, chunk_size);
                break;
            case LogDataChunkKey::ThreadName:
                thread_name = ReadString(log_data, offset, chunk_size);
                break;
            case LogDataChunkKey::LogPacketDropCount:
                log_pack_drop_count = ReadAsU64(log_data, offset, chunk_size);
                break;
            case LogDataChunkKey::UserSystemClock:
                user_system_clock = ReadAsU64(log_data, offset, chunk_size);
                break;
            case LogDataChunkKey::ProcessName:
                process_name = ReadString(log_data, offset, chunk_size);
                break;
            }
        }

        std::string output_log{};
        if (process_name) {
            output_log += fmt::format("Process: {}\n", *process_name);
        }
        if (module_name) {
            output_log += fmt::format("Module: {}\n", *module_name);
        }
        if (file_name) {
            output_log += fmt::format("File: {}\n", *file_name);
        }
        if (function_name) {
            output_log += fmt::format("Function: {}\n", *function_name);
        }
        if (line_number && *line_number != 0) {
            output_log += fmt::format("Line: {}\n", *line_number);
        }
        output_log += fmt::format("ProcessID: {:X}\n", entry.pid);
        output_log += fmt::format("ThreadID: {:X}\n", entry.tid);

        if (text_log) {
            output_log += fmt::format("Log Text: {}\n", *text_log);
        }
        LOG_DEBUG(Service_LM, "LogManager {} ({}):\n{}", NameOf(entry.severity),
                  DestinationToString(destination), output_log);
    }

    static std::string DestinationToString(LogDestination destination) {
        if (True(destination & LogDestination::All)) {
            return "TargetManager | Uart | UartSleep";
        }
        std::string output{};
        if (True(destination & LogDestination::TargetManager)) {
            output += "| TargetManager";
        }
        if (True(destination & LogDestination::Uart)) {
            output += "| Uart";
        }
        if (True(destination & LogDestination::UartSleep)) {
            output += "| UartSleep";
        }
        if (output.length() > 0) {
            return output.substr(2);
        }
        return "No Destination";
    }

    enum class LogDataChunkKey : u32 {
        LogSessionBegin = 0,
        LogSessionEnd = 1,
        TextLog = 2,
        LineNumber = 3,
        FileName = 4,
        FunctionName = 5,
        ModuleName = 6,
        ThreadName = 7,
        LogPacketDropCount = 8,
        UserSystemClock = 9,
        ProcessName = 10,
    };

    struct LogPacketHeader {
        u64_le pid{};
        u64_le tid{};
        LogPacketFlags flags{};
        INSERT_PADDING_BYTES(1);
        LogSeverity severity{};
        u8 verbosity{};
        u32_le payload_size{};
    };
    static_assert(sizeof(LogPacketHeader) == 0x18, "LogPacketHeader is an invalid size");

    std::unordered_map<LogPacketHeaderEntry, std::vector<u8>> entries{};
    LogDestination destination{LogDestination::All};
};

class LM final : public ServiceFramework<LM> {
public:
    explicit LM(Core::System& system_) : ServiceFramework{system_, "lm"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &LM::OpenLogger, "OpenLogger"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void OpenLogger(HLERequestContext& ctx) {
        LOG_DEBUG(Service_LM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<ILogger>(system);
    }
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("lm", std::make_shared<LM>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::LM
