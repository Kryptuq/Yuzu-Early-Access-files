// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "common/string_util.h"
#include "core/file_sys/kernel_executable.h"
#include "core/file_sys/vfs_offset.h"
#include "core/loader/loader.h"

namespace FileSys {

constexpr u32 INI_MAX_KIPS = 0x50;

namespace {
bool DecompressBLZ(std::vector<u8>& data) {
    if (data.size() < 0xC)
        return {};

    const auto data_size = data.size() - 0xC;

    u32 compressed_size{};
    u32 init_index{};
    u32 additional_size{};
    std::memcpy(&compressed_size, data.data() + data_size, sizeof(u32));
    std::memcpy(&init_index, data.data() + data_size + 0x4, sizeof(u32));
    std::memcpy(&additional_size, data.data() + data_size + 0x8, sizeof(u32));

    const auto start_offset = data.size() - compressed_size;
    data.resize(compressed_size + additional_size + start_offset);

    std::size_t index = compressed_size - init_index;
    std::size_t out_index = compressed_size + additional_size;

    while (out_index > 0) {
        --index;
        auto control = data[index + start_offset];
        for (size_t i = 0; i < 8; ++i) {
            if (((control << i) & 0x80) > 0) {
                if (index < 2) {
                    return false;
                }
                index -= 2;
                std::size_t segment_offset =
                    data[index + start_offset] | data[index + start_offset + 1] << 8;
                std::size_t segment_size = ((segment_offset >> 12) & 0xF) + 3;
                segment_offset &= 0xFFF;
                segment_offset += 3;

                if (out_index < segment_size)
                    segment_size = out_index;

                if (out_index < segment_size) {
                    return false;
                }

                out_index -= segment_size;

                for (size_t j = 0; j < segment_size; ++j) {
                    if (out_index + j + segment_offset + start_offset >= data.size()) {
                        return false;
                    }
                    data[out_index + j + start_offset] =
                        data[out_index + j + segment_offset + start_offset];
                }
            } else {
                if (out_index < 1) {
                    return false;
                }
                --out_index;
                --index;
                data[out_index + start_offset] = data[index + start_offset];
            }

            if (out_index == 0)
                break;
        }
    }

    return true;
}
} // Anonymous namespace

KIP::KIP(const VirtualFile& file) : status(Loader::ResultStatus::Success) {
    if (file == nullptr) {
        status = Loader::ResultStatus::ErrorNullFile;
        return;
    }

    if (file->GetSize() < sizeof(KIPHeader) || file->ReadObject(&header) != sizeof(KIPHeader)) {
        status = Loader::ResultStatus::ErrorBadKIPHeader;
        return;
    }

    if (header.magic != Common::MakeMagic('K', 'I', 'P', '1')) {
        status = Loader::ResultStatus::ErrorBadKIPHeader;
        return;
    }

    u64 offset = sizeof(KIPHeader);
    for (std::size_t i = 0; i < header.sections.size(); ++i) {
        auto compressed = file->ReadBytes(header.sections[i].compressed_size, offset);
        offset += header.sections[i].compressed_size;

        if (header.sections[i].compressed_size == 0 && header.sections[i].decompressed_size != 0) {
            decompressed_sections[i] = std::vector<u8>(header.sections[i].decompressed_size);
        } else if (header.sections[i].compressed_size == header.sections[i].decompressed_size) {
            decompressed_sections[i] = std::move(compressed);
        } else {
            decompressed_sections[i] = compressed;
            if (!DecompressBLZ(decompressed_sections[i])) {
                status = Loader::ResultStatus::ErrorBLZDecompressionFailed;
                return;
            }
        }
    }
}

Loader::ResultStatus KIP::GetStatus() const {
    return status;
}

std::string KIP::GetName() const {
    return Common::StringFromFixedZeroTerminatedBuffer(header.name.data(), header.name.size());
}

u64 KIP::GetTitleID() const {
    return header.title_id;
}

std::vector<u8> KIP::GetSectionDecompressed(u8 index) const {
    return decompressed_sections[index];
}

bool KIP::Is64Bit() const {
    return (header.flags & 0x8) != 0;
}

bool KIP::Is39BitAddressSpace() const {
    return (header.flags & 0x10) != 0;
}

bool KIP::IsService() const {
    return (header.flags & 0x20) != 0;
}

std::vector<u32> KIP::GetKernelCapabilities() const {
    return std::vector<u32>(header.capabilities.begin(), header.capabilities.end());
}

s32 KIP::GetMainThreadPriority() const {
    return static_cast<s32>(header.main_thread_priority);
}

u32 KIP::GetMainThreadStackSize() const {
    return header.sections[1].attribute;
}

u32 KIP::GetMainThreadCpuCore() const {
    return header.default_core;
}

const std::vector<u8>& KIP::GetTextSection() const {
    return decompressed_sections[0];
}

const std::vector<u8>& KIP::GetRODataSection() const {
    return decompressed_sections[1];
}

const std::vector<u8>& KIP::GetDataSection() const {
    return decompressed_sections[2];
}

u32 KIP::GetTextOffset() const {
    return header.sections[0].offset;
}

u32 KIP::GetRODataOffset() const {
    return header.sections[1].offset;
}

u32 KIP::GetDataOffset() const {
    return header.sections[2].offset;
}

u32 KIP::GetBSSSize() const {
    return header.sections[3].decompressed_size;
}

u32 KIP::GetBSSOffset() const {
    return header.sections[3].offset;
}

INI::INI(const VirtualFile& file) : status(Loader::ResultStatus::Success) {
    if (file->GetSize() < sizeof(INIHeader) || file->ReadObject(&header) != sizeof(INIHeader)) {
        status = Loader::ResultStatus::ErrorBadINIHeader;
        return;
    }

    if (header.magic != Common::MakeMagic('I', 'N', 'I', '1')) {
        status = Loader::ResultStatus::ErrorBadINIHeader;
        return;
    }

    if (header.kip_count > INI_MAX_KIPS) {
        status = Loader::ResultStatus::ErrorINITooManyKIPs;
        return;
    }

    u64 offset = sizeof(INIHeader);
    for (std::size_t i = 0; i < header.kip_count; ++i) {
        const auto kip_file =
            std::make_shared<OffsetVfsFile>(file, file->GetSize() - offset, offset);
        KIP kip(kip_file);
        if (kip.GetStatus() == Loader::ResultStatus::Success) {
            kips.push_back(std::move(kip));
        }
    }
}

Loader::ResultStatus INI::GetStatus() const {
    return status;
}

const std::vector<KIP>& INI::GetKIPs() const {
    return kips;
}

} // namespace FileSys
