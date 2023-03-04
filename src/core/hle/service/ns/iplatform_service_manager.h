// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <vector>
#include "core/hle/service/service.h"

namespace Service {

namespace FileSystem {
class FileSystemController;
} // namespace FileSystem

namespace NS {

enum class FontArchives : u64 {
    Extension = 0x0100000000000810,
    Standard = 0x0100000000000811,
    Korean = 0x0100000000000812,
    ChineseTraditional = 0x0100000000000813,
    ChineseSimple = 0x0100000000000814,
};

constexpr std::array<std::pair<FontArchives, const char*>, 7> SHARED_FONTS{
    std::make_pair(FontArchives::Standard, "nintendo_udsg-r_std_003.bfttf"),
    std::make_pair(FontArchives::ChineseSimple, "nintendo_udsg-r_org_zh-cn_003.bfttf"),
    std::make_pair(FontArchives::ChineseSimple, "nintendo_udsg-r_ext_zh-cn_003.bfttf"),
    std::make_pair(FontArchives::ChineseTraditional, "nintendo_udjxh-db_zh-tw_003.bfttf"),
    std::make_pair(FontArchives::Korean, "nintendo_udsg-r_ko_003.bfttf"),
    std::make_pair(FontArchives::Extension, "nintendo_ext_003.bfttf"),
    std::make_pair(FontArchives::Extension, "nintendo_ext2_003.bfttf"),
};

void DecryptSharedFontToTTF(const std::vector<u32>& input, std::vector<u8>& output);
void EncryptSharedFont(const std::vector<u32>& input, std::vector<u8>& output, std::size_t& offset);

class IPlatformServiceManager final : public ServiceFramework<IPlatformServiceManager> {
public:
    explicit IPlatformServiceManager(Core::System& system_, const char* service_name_);
    ~IPlatformServiceManager() override;

private:
    void RequestLoad(HLERequestContext& ctx);
    void GetLoadState(HLERequestContext& ctx);
    void GetSize(HLERequestContext& ctx);
    void GetSharedMemoryAddressOffset(HLERequestContext& ctx);
    void GetSharedMemoryNativeHandle(HLERequestContext& ctx);
    void GetSharedFontInOrderOfPriority(HLERequestContext& ctx);

    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace NS

} // namespace Service
