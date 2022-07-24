// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>
#include <vector>

#include "common/common_types.h"
#include "core/file_sys/vfs.h"

namespace FileSys {

VirtualFile PatchIPS(const VirtualFile& in, const VirtualFile& ips);

class IPSwitchCompiler {
public:
    explicit IPSwitchCompiler(VirtualFile patch_text);
    ~IPSwitchCompiler();

    std::array<u8, 0x20> GetBuildID() const;
    bool IsValid() const;
    VirtualFile Apply(const VirtualFile& in) const;

private:
    struct IPSwitchPatch;

    void ParseFlag(const std::string& flag);
    void Parse();

    bool valid = false;

    VirtualFile patch_text;
    std::vector<IPSwitchPatch> patches;
    std::array<u8, 0x20> nso_build_id{};
    bool is_little_endian = false;
    s64 offset_shift = 0;
    bool print_values = false;
    std::string last_comment = "";
};

} // namespace FileSys
