// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <vector>
#include "common/common_types.h"
#include "common/swap.h"
#include "core/crypto/key_manager.h"
#include "core/file_sys/vfs.h"

namespace Loader {
enum class ResultStatus : u16;
}

namespace FileSys {

class NCA;

struct NAXHeader {
    std::array<u8, 0x20> hmac;
    u64_le magic;
    std::array<Core::Crypto::Key128, 2> key_area;
    u64_le file_size;
    INSERT_PADDING_BYTES(0x30);
};
static_assert(sizeof(NAXHeader) == 0x80, "NAXHeader has incorrect size.");

enum class NAXContentType : u8 {
    Save = 0,
    NCA = 1,
};

class NAX : public ReadOnlyVfsDirectory {
public:
    explicit NAX(VirtualFile file);
    explicit NAX(VirtualFile file, std::array<u8, 0x10> nca_id);
    ~NAX() override;

    Loader::ResultStatus GetStatus() const;

    VirtualFile GetDecrypted() const;

    std::unique_ptr<NCA> AsNCA() const;

    NAXContentType GetContentType() const;

    std::vector<VirtualFile> GetFiles() const override;

    std::vector<VirtualDir> GetSubdirectories() const override;

    std::string GetName() const override;

    VirtualDir GetParentDirectory() const override;

private:
    Loader::ResultStatus Parse(std::string_view path);

    std::unique_ptr<NAXHeader> header;

    VirtualFile file;
    Loader::ResultStatus status;
    NAXContentType type{};

    VirtualFile dec_file;

    Core::Crypto::KeyManager& keys;
};
} // namespace FileSys
