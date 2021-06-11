// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <cstring>
#include <regex>
#include <string>

#include <mbedtls/md.h>
#include <mbedtls/sha256.h>

#include "common/fs/path_util.h"
#include "common/hex_util.h"
#include "common/string_util.h"
#include "core/crypto/aes_util.h"
#include "core/crypto/key_manager.h"
#include "core/crypto/xts_encryption_layer.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/vfs_offset.h"
#include "core/file_sys/xts_archive.h"
#include "core/loader/loader.h"

namespace FileSys {

constexpr u64 NAX_HEADER_PADDING_SIZE = 0x4000;

template <typename SourceData, typename SourceKey, typename Destination>
static bool CalculateHMAC256(Destination* out, const SourceKey* key, std::size_t key_length,
                             const SourceData* data, std::size_t data_length) {
    mbedtls_md_context_t context;
    mbedtls_md_init(&context);

    if (mbedtls_md_setup(&context, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1) ||
        mbedtls_md_hmac_starts(&context, reinterpret_cast<const u8*>(key), key_length) ||
        mbedtls_md_hmac_update(&context, reinterpret_cast<const u8*>(data), data_length) ||
        mbedtls_md_hmac_finish(&context, reinterpret_cast<u8*>(out))) {
        mbedtls_md_free(&context);
        return false;
    }

    mbedtls_md_free(&context);
    return true;
}

NAX::NAX(VirtualFile file_)
    : header(std::make_unique<NAXHeader>()),
      file(std::move(file_)), keys{Core::Crypto::KeyManager::Instance()} {
    std::string path = Common::FS::SanitizePath(file->GetFullPath());
    static const std::regex nax_path_regex("/registered/(000000[0-9A-F]{2})/([0-9A-F]{32})\\.nca",
                                           std::regex_constants::ECMAScript |
                                               std::regex_constants::icase);
    std::smatch match;
    if (!std::regex_search(path, match, nax_path_regex)) {
        status = Loader::ResultStatus::ErrorBadNAXFilePath;
        return;
    }

    const std::string two_dir = Common::ToUpper(match[1]);
    const std::string nca_id = Common::ToLower(match[2]);
    status = Parse(fmt::format("/registered/{}/{}.nca", two_dir, nca_id));
}

NAX::NAX(VirtualFile file_, std::array<u8, 0x10> nca_id)
    : header(std::make_unique<NAXHeader>()),
      file(std::move(file_)), keys{Core::Crypto::KeyManager::Instance()} {
    Core::Crypto::SHA256Hash hash{};
    mbedtls_sha256_ret(nca_id.data(), nca_id.size(), hash.data(), 0);
    status = Parse(fmt::format("/registered/000000{:02X}/{}.nca", hash[0],
                               Common::HexToString(nca_id, false)));
}

NAX::~NAX() = default;

Loader::ResultStatus NAX::Parse(std::string_view path) {
    if (file == nullptr) {
        return Loader::ResultStatus::ErrorNullFile;
    }
    if (file->ReadObject(header.get()) != sizeof(NAXHeader)) {
        return Loader::ResultStatus::ErrorBadNAXHeader;
    }
    if (header->magic != Common::MakeMagic('N', 'A', 'X', '0')) {
        return Loader::ResultStatus::ErrorBadNAXHeader;
    }
    if (file->GetSize() < NAX_HEADER_PADDING_SIZE + header->file_size) {
        return Loader::ResultStatus::ErrorIncorrectNAXFileSize;
    }

    keys.DeriveSDSeedLazy();
    std::array<Core::Crypto::Key256, 2> sd_keys{};
    const auto sd_keys_res = Core::Crypto::DeriveSDKeys(sd_keys, keys);
    if (sd_keys_res != Loader::ResultStatus::Success) {
        return sd_keys_res;
    }

    const auto enc_keys = header->key_area;

    std::size_t i = 0;
    for (; i < sd_keys.size(); ++i) {
        std::array<Core::Crypto::Key128, 2> nax_keys{};
        if (!CalculateHMAC256(nax_keys.data(), sd_keys[i].data(), 0x10, path.data(), path.size())) {
            return Loader::ResultStatus::ErrorNAXKeyHMACFailed;
        }

        for (std::size_t j = 0; j < nax_keys.size(); ++j) {
            Core::Crypto::AESCipher<Core::Crypto::Key128> cipher(nax_keys[j],
                                                                 Core::Crypto::Mode::ECB);
            cipher.Transcode(enc_keys[j].data(), 0x10, header->key_area[j].data(),
                             Core::Crypto::Op::Decrypt);
        }

        Core::Crypto::SHA256Hash validation{};
        if (!CalculateHMAC256(validation.data(), &header->magic, 0x60, sd_keys[i].data() + 0x10,
                              0x10)) {
            return Loader::ResultStatus::ErrorNAXValidationHMACFailed;
        }
        if (header->hmac == validation)
            break;
    }

    if (i == 2) {
        return Loader::ResultStatus::ErrorNAXKeyDerivationFailed;
    }

    type = static_cast<NAXContentType>(i);

    Core::Crypto::Key256 final_key{};
    std::memcpy(final_key.data(), &header->key_area, final_key.size());
    const auto enc_file =
        std::make_shared<OffsetVfsFile>(file, header->file_size, NAX_HEADER_PADDING_SIZE);
    dec_file = std::make_shared<Core::Crypto::XTSEncryptionLayer>(enc_file, final_key);

    return Loader::ResultStatus::Success;
}

Loader::ResultStatus NAX::GetStatus() const {
    return status;
}

VirtualFile NAX::GetDecrypted() const {
    return dec_file;
}

std::unique_ptr<NCA> NAX::AsNCA() const {
    if (type == NAXContentType::NCA)
        return std::make_unique<NCA>(GetDecrypted());
    return nullptr;
}

NAXContentType NAX::GetContentType() const {
    return type;
}

std::vector<VirtualFile> NAX::GetFiles() const {
    return {dec_file};
}

std::vector<VirtualDir> NAX::GetSubdirectories() const {
    return {};
}

std::string NAX::GetName() const {
    return file->GetName();
}

VirtualDir NAX::GetParentDirectory() const {
    return file->GetContainingDirectory();
}

} // namespace FileSys
