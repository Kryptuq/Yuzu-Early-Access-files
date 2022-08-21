// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "common/hex_util.h"
#include "common/logging/log.h"
#include "common/swap.h"
#include "core/file_sys/ips_layer.h"
#include "core/file_sys/vfs_vector.h"

namespace FileSys {

enum class IPSFileType {
    IPS,
    IPS32,
    Error,
};

constexpr std::array<std::pair<const char*, const char*>, 11> ESCAPE_CHARACTER_MAP{{
    {"\\a", "\a"},
    {"\\b", "\b"},
    {"\\f", "\f"},
    {"\\n", "\n"},
    {"\\r", "\r"},
    {"\\t", "\t"},
    {"\\v", "\v"},
    {"\\\\", "\\"},
    {"\\\'", "\'"},
    {"\\\"", "\""},
    {"\\\?", "\?"},
}};

static IPSFileType IdentifyMagic(const std::vector<u8>& magic) {
    if (magic.size() != 5) {
        return IPSFileType::Error;
    }

    constexpr std::array<u8, 5> patch_magic{{'P', 'A', 'T', 'C', 'H'}};
    if (std::equal(magic.begin(), magic.end(), patch_magic.begin())) {
        return IPSFileType::IPS;
    }

    constexpr std::array<u8, 5> ips32_magic{{'I', 'P', 'S', '3', '2'}};
    if (std::equal(magic.begin(), magic.end(), ips32_magic.begin())) {
        return IPSFileType::IPS32;
    }

    return IPSFileType::Error;
}

static bool IsEOF(IPSFileType type, const std::vector<u8>& data) {
    constexpr std::array<u8, 3> eof{{'E', 'O', 'F'}};
    if (type == IPSFileType::IPS && std::equal(data.begin(), data.end(), eof.begin())) {
        return true;
    }

    constexpr std::array<u8, 4> eeof{{'E', 'E', 'O', 'F'}};
    return type == IPSFileType::IPS32 && std::equal(data.begin(), data.end(), eeof.begin());
}

VirtualFile PatchIPS(const VirtualFile& in, const VirtualFile& ips) {
    if (in == nullptr || ips == nullptr)
        return nullptr;

    const auto type = IdentifyMagic(ips->ReadBytes(0x5));
    if (type == IPSFileType::Error)
        return nullptr;

    auto in_data = in->ReadAllBytes();

    std::vector<u8> temp(type == IPSFileType::IPS ? 3 : 4);
    u64 offset = 5; // After header
    while (ips->Read(temp.data(), temp.size(), offset) == temp.size()) {
        offset += temp.size();
        if (IsEOF(type, temp)) {
            break;
        }

        u32 real_offset{};
        if (type == IPSFileType::IPS32)
            real_offset = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];
        else
            real_offset = (temp[0] << 16) | (temp[1] << 8) | temp[2];

        u16 data_size{};
        if (ips->ReadObject(&data_size, offset) != sizeof(u16))
            return nullptr;
        data_size = Common::swap16(data_size);
        offset += sizeof(u16);

        if (data_size == 0) { // RLE
            u16 rle_size{};
            if (ips->ReadObject(&rle_size, offset) != sizeof(u16))
                return nullptr;
            rle_size = Common::swap16(rle_size);
            offset += sizeof(u16);

            const auto data = ips->ReadByte(offset++);
            if (!data)
                return nullptr;

            if (real_offset + rle_size > in_data.size())
                rle_size = static_cast<u16>(in_data.size() - real_offset);
            std::memset(in_data.data() + real_offset, *data, rle_size);
        } else { // Standard Patch
            auto read = data_size;
            if (real_offset + read > in_data.size())
                read = static_cast<u16>(in_data.size() - real_offset);
            if (ips->Read(in_data.data() + real_offset, read, offset) != data_size)
                return nullptr;
            offset += data_size;
        }
    }

    if (!IsEOF(type, temp)) {
        return nullptr;
    }

    return std::make_shared<VectorVfsFile>(std::move(in_data), in->GetName(),
                                           in->GetContainingDirectory());
}

struct IPSwitchCompiler::IPSwitchPatch {
    std::string name;
    bool enabled;
    std::map<u32, std::vector<u8>> records;
};

IPSwitchCompiler::IPSwitchCompiler(VirtualFile patch_text_) : patch_text(std::move(patch_text_)) {
    Parse();
}

IPSwitchCompiler::~IPSwitchCompiler() = default;

std::array<u8, 32> IPSwitchCompiler::GetBuildID() const {
    return nso_build_id;
}

bool IPSwitchCompiler::IsValid() const {
    return valid;
}

static bool StartsWith(std::string_view base, std::string_view check) {
    return base.size() >= check.size() && base.substr(0, check.size()) == check;
}

static std::string EscapeStringSequences(std::string in) {
    for (const auto& seq : ESCAPE_CHARACTER_MAP) {
        for (auto index = in.find(seq.first); index != std::string::npos;
             index = in.find(seq.first, index)) {
            in.replace(index, std::strlen(seq.first), seq.second);
            index += std::strlen(seq.second);
        }
    }

    return in;
}

void IPSwitchCompiler::ParseFlag(const std::string& line) {
    if (StartsWith(line, "@flag offset_shift ")) {
        // Offset Shift Flag
        offset_shift = std::stoll(line.substr(19), nullptr, 0);
    } else if (StartsWith(line, "@little-endian")) {
        // Set values to read as little endian
        is_little_endian = true;
    } else if (StartsWith(line, "@big-endian")) {
        // Set values to read as big endian
        is_little_endian = false;
    } else if (StartsWith(line, "@flag print_values")) {
        // Force printing of applied values
        print_values = true;
    }
}

void IPSwitchCompiler::Parse() {
    const auto bytes = patch_text->ReadAllBytes();
    std::stringstream s;
    s.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());

    std::vector<std::string> lines;
    std::string stream_line;
    while (std::getline(s, stream_line)) {
        // Remove a trailing \r
        if (!stream_line.empty() && stream_line.back() == '\r')
            stream_line.pop_back();
        lines.push_back(std::move(stream_line));
    }

    for (std::size_t i = 0; i < lines.size(); ++i) {
        auto line = lines[i];

        // Remove midline comments
        std::size_t comment_index = std::string::npos;
        bool within_string = false;
        for (std::size_t k = 0; k < line.size(); ++k) {
            if (line[k] == '\"' && (k > 0 && line[k - 1] != '\\')) {
                within_string = !within_string;
            } else if (line[k] == '\\' && (k < line.size() - 1 && line[k + 1] == '\\')) {
                comment_index = k;
                break;
            }
        }

        if (!StartsWith(line, "//") && comment_index != std::string::npos) {
            last_comment = line.substr(comment_index + 2);
            line = line.substr(0, comment_index);
        }

        if (StartsWith(line, "@stop")) {
            // Force stop
            break;
        } else if (StartsWith(line, "@nsobid-")) {
            // NSO Build ID Specifier
            const auto raw_build_id = fmt::format("{:0<64}", line.substr(8));
            nso_build_id = Common::HexStringToArray<0x20>(raw_build_id);
        } else if (StartsWith(line, "#")) {
            // Mandatory Comment
            LOG_INFO(Loader, "[IPSwitchCompiler ('{}')] Forced output comment: {}",
                     patch_text->GetName(), line.substr(1));
        } else if (StartsWith(line, "//")) {
            // Normal Comment
            last_comment = line.substr(2);
            if (last_comment.find_first_not_of(' ') == std::string::npos)
                continue;
            if (last_comment.find_first_not_of(' ') != 0)
                last_comment = last_comment.substr(last_comment.find_first_not_of(' '));
        } else if (StartsWith(line, "@enabled") || StartsWith(line, "@disabled")) {
            // Start of patch
            const auto enabled = StartsWith(line, "@enabled");
            if (i == 0)
                return;
            LOG_INFO(Loader, "[IPSwitchCompiler ('{}')] Parsing patch '{}' ({})",
                     patch_text->GetName(), last_comment, line.substr(1));

            IPSwitchPatch patch{last_comment, enabled, {}};

            // Read rest of patch
            while (true) {
                if (i + 1 >= lines.size()) {
                    break;
                }

                const auto& patch_line = lines[++i];

                // Start of new patch
                if (StartsWith(patch_line, "@enabled") || StartsWith(patch_line, "@disabled")) {
                    --i;
                    break;
                }

                // Check for a flag
                if (StartsWith(patch_line, "@")) {
                    ParseFlag(patch_line);
                    continue;
                }

                // 11 - 8 hex digit offset + space + minimum two digit overwrite val
                if (patch_line.length() < 11)
                    break;
                auto offset = std::stoul(patch_line.substr(0, 8), nullptr, 16);
                offset += static_cast<unsigned long>(offset_shift);

                std::vector<u8> replace;
                // 9 - first char of replacement val
                if (patch_line[9] == '\"') {
                    // string replacement
                    auto end_index = patch_line.find('\"', 10);
                    if (end_index == std::string::npos || end_index < 10)
                        return;
                    while (patch_line[end_index - 1] == '\\') {
                        end_index = patch_line.find('\"', end_index + 1);
                        if (end_index == std::string::npos || end_index < 10)
                            return;
                    }

                    auto value = patch_line.substr(10, end_index - 10);
                    value = EscapeStringSequences(value);
                    replace.reserve(value.size());
                    std::copy(value.begin(), value.end(), std::back_inserter(replace));
                } else {
                    // hex replacement
                    const auto value =
                        patch_line.substr(9, patch_line.find_first_of(" /\r\n", 9) - 9);
                    replace = Common::HexStringToVector(value, is_little_endian);
                }

                if (print_values) {
                    LOG_INFO(Loader,
                             "[IPSwitchCompiler ('{}')]     - Patching value at offset 0x{:08X} "
                             "with byte string '{}'",
                             patch_text->GetName(), offset, Common::HexToString(replace));
                }

                patch.records.insert_or_assign(static_cast<u32>(offset), std::move(replace));
            }

            patches.push_back(std::move(patch));
        } else if (StartsWith(line, "@")) {
            ParseFlag(line);
        }
    }

    valid = true;
}

VirtualFile IPSwitchCompiler::Apply(const VirtualFile& in) const {
    if (in == nullptr || !valid)
        return nullptr;

    auto in_data = in->ReadAllBytes();

    for (const auto& patch : patches) {
        if (!patch.enabled)
            continue;

        for (const auto& record : patch.records) {
            if (record.first >= in_data.size())
                continue;
            auto replace_size = record.second.size();
            if (record.first + replace_size > in_data.size())
                replace_size = in_data.size() - record.first;
            for (std::size_t i = 0; i < replace_size; ++i)
                in_data[i + record.first] = record.second[i];
        }
    }

    return std::make_shared<VectorVfsFile>(std::move(in_data), in->GetName(),
                                           in->GetContainingDirectory());
}

} // namespace FileSys
