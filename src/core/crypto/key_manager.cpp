// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <bitset>
#include <cctype>
#include <fstream>
#include <locale>
#include <map>
#include <sstream>
#include <string_view>
#include <tuple>
#include <vector>
#include <mbedtls/bignum.h>
#include <mbedtls/cipher.h>
#include <mbedtls/cmac.h>
#include <mbedtls/sha256.h>
#include "common/common_funcs.h"
#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/hex_util.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "common/string_util.h"
#include "core/crypto/aes_util.h"
#include "core/crypto/key_manager.h"
#include "core/crypto/partition_data_manager.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/nca_metadata.h"
#include "core/file_sys/partition_filesystem.h"
#include "core/file_sys/registered_cache.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/loader/loader.h"

namespace Core::Crypto {
namespace {

constexpr u64 CURRENT_CRYPTO_REVISION = 0x5;
constexpr u64 FULL_TICKET_SIZE = 0x400;

using Common::AsArray;

// clang-format off
constexpr std::array eticket_source_hashes{
    AsArray("B71DB271DC338DF380AA2C4335EF8873B1AFD408E80B3582D8719FC81C5E511C"), // eticket_rsa_kek_source
    AsArray("E8965A187D30E57869F562D04383C996DE487BBA5761363D2D4D32391866A85C"), // eticket_rsa_kekek_source
};
// clang-format on

constexpr std::array<std::pair<std::string_view, KeyIndex<S128KeyType>>, 30> s128_file_id{{
    {"eticket_rsa_kek", {S128KeyType::ETicketRSAKek, 0, 0}},
    {"eticket_rsa_kek_source",
     {S128KeyType::Source, static_cast<u64>(SourceKeyType::ETicketKek), 0}},
    {"eticket_rsa_kekek_source",
     {S128KeyType::Source, static_cast<u64>(SourceKeyType::ETicketKekek), 0}},
    {"rsa_kek_mask_0", {S128KeyType::RSAKek, static_cast<u64>(RSAKekType::Mask0), 0}},
    {"rsa_kek_seed_3", {S128KeyType::RSAKek, static_cast<u64>(RSAKekType::Seed3), 0}},
    {"rsa_oaep_kek_generation_source",
     {S128KeyType::Source, static_cast<u64>(SourceKeyType::RSAOaepKekGeneration), 0}},
    {"sd_card_kek_source", {S128KeyType::Source, static_cast<u64>(SourceKeyType::SDKek), 0}},
    {"aes_kek_generation_source",
     {S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKekGeneration), 0}},
    {"aes_key_generation_source",
     {S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKeyGeneration), 0}},
    {"package2_key_source", {S128KeyType::Source, static_cast<u64>(SourceKeyType::Package2), 0}},
    {"master_key_source", {S128KeyType::Source, static_cast<u64>(SourceKeyType::Master), 0}},
    {"header_kek_source", {S128KeyType::Source, static_cast<u64>(SourceKeyType::HeaderKek), 0}},
    {"key_area_key_application_source",
     {S128KeyType::Source, static_cast<u64>(SourceKeyType::KeyAreaKey),
      static_cast<u64>(KeyAreaKeyType::Application)}},
    {"key_area_key_ocean_source",
     {S128KeyType::Source, static_cast<u64>(SourceKeyType::KeyAreaKey),
      static_cast<u64>(KeyAreaKeyType::Ocean)}},
    {"key_area_key_system_source",
     {S128KeyType::Source, static_cast<u64>(SourceKeyType::KeyAreaKey),
      static_cast<u64>(KeyAreaKeyType::System)}},
    {"titlekek_source", {S128KeyType::Source, static_cast<u64>(SourceKeyType::Titlekek), 0}},
    {"keyblob_mac_key_source",
     {S128KeyType::Source, static_cast<u64>(SourceKeyType::KeyblobMAC), 0}},
    {"tsec_key", {S128KeyType::TSEC, 0, 0}},
    {"secure_boot_key", {S128KeyType::SecureBoot, 0, 0}},
    {"sd_seed", {S128KeyType::SDSeed, 0, 0}},
    {"bis_key_0_crypt", {S128KeyType::BIS, 0, static_cast<u64>(BISKeyType::Crypto)}},
    {"bis_key_0_tweak", {S128KeyType::BIS, 0, static_cast<u64>(BISKeyType::Tweak)}},
    {"bis_key_1_crypt", {S128KeyType::BIS, 1, static_cast<u64>(BISKeyType::Crypto)}},
    {"bis_key_1_tweak", {S128KeyType::BIS, 1, static_cast<u64>(BISKeyType::Tweak)}},
    {"bis_key_2_crypt", {S128KeyType::BIS, 2, static_cast<u64>(BISKeyType::Crypto)}},
    {"bis_key_2_tweak", {S128KeyType::BIS, 2, static_cast<u64>(BISKeyType::Tweak)}},
    {"bis_key_3_crypt", {S128KeyType::BIS, 3, static_cast<u64>(BISKeyType::Crypto)}},
    {"bis_key_3_tweak", {S128KeyType::BIS, 3, static_cast<u64>(BISKeyType::Tweak)}},
    {"header_kek", {S128KeyType::HeaderKek, 0, 0}},
    {"sd_card_kek", {S128KeyType::SDKek, 0, 0}},
}};

auto Find128ByName(std::string_view name) {
    return std::find_if(s128_file_id.begin(), s128_file_id.end(),
                        [&name](const auto& pair) { return pair.first == name; });
}

constexpr std::array<std::pair<std::string_view, KeyIndex<S256KeyType>>, 6> s256_file_id{{
    {"header_key", {S256KeyType::Header, 0, 0}},
    {"sd_card_save_key_source", {S256KeyType::SDKeySource, static_cast<u64>(SDKeyType::Save), 0}},
    {"sd_card_nca_key_source", {S256KeyType::SDKeySource, static_cast<u64>(SDKeyType::NCA), 0}},
    {"header_key_source", {S256KeyType::HeaderSource, 0, 0}},
    {"sd_card_save_key", {S256KeyType::SDKey, static_cast<u64>(SDKeyType::Save), 0}},
    {"sd_card_nca_key", {S256KeyType::SDKey, static_cast<u64>(SDKeyType::NCA), 0}},
}};

auto Find256ByName(std::string_view name) {
    return std::find_if(s256_file_id.begin(), s256_file_id.end(),
                        [&name](const auto& pair) { return pair.first == name; });
}

using KeyArray = std::array<std::pair<std::pair<S128KeyType, u64>, std::string_view>, 7>;
constexpr KeyArray KEYS_VARIABLE_LENGTH{{
    {{S128KeyType::Master, 0}, "master_key_"},
    {{S128KeyType::Package1, 0}, "package1_key_"},
    {{S128KeyType::Package2, 0}, "package2_key_"},
    {{S128KeyType::Titlekek, 0}, "titlekek_"},
    {{S128KeyType::Source, static_cast<u64>(SourceKeyType::Keyblob)}, "keyblob_key_source_"},
    {{S128KeyType::Keyblob, 0}, "keyblob_key_"},
    {{S128KeyType::KeyblobMAC, 0}, "keyblob_mac_key_"},
}};

template <std::size_t Size>
bool IsAllZeroArray(const std::array<u8, Size>& array) {
    return std::all_of(array.begin(), array.end(), [](const auto& elem) { return elem == 0; });
}
} // Anonymous namespace

u64 GetSignatureTypeDataSize(SignatureType type) {
    switch (type) {
    case SignatureType::RSA_4096_SHA1:
    case SignatureType::RSA_4096_SHA256:
        return 0x200;
    case SignatureType::RSA_2048_SHA1:
    case SignatureType::RSA_2048_SHA256:
        return 0x100;
    case SignatureType::ECDSA_SHA1:
    case SignatureType::ECDSA_SHA256:
        return 0x3C;
    }
    UNREACHABLE();
    return 0;
}

u64 GetSignatureTypePaddingSize(SignatureType type) {
    switch (type) {
    case SignatureType::RSA_4096_SHA1:
    case SignatureType::RSA_4096_SHA256:
    case SignatureType::RSA_2048_SHA1:
    case SignatureType::RSA_2048_SHA256:
        return 0x3C;
    case SignatureType::ECDSA_SHA1:
    case SignatureType::ECDSA_SHA256:
        return 0x40;
    }
    UNREACHABLE();
    return 0;
}

SignatureType Ticket::GetSignatureType() const {
    if (const auto* ticket = std::get_if<RSA4096Ticket>(&data)) {
        return ticket->sig_type;
    }
    if (const auto* ticket = std::get_if<RSA2048Ticket>(&data)) {
        return ticket->sig_type;
    }
    if (const auto* ticket = std::get_if<ECDSATicket>(&data)) {
        return ticket->sig_type;
    }
    throw std::bad_variant_access{};
}

TicketData& Ticket::GetData() {
    if (auto* ticket = std::get_if<RSA4096Ticket>(&data)) {
        return ticket->data;
    }
    if (auto* ticket = std::get_if<RSA2048Ticket>(&data)) {
        return ticket->data;
    }
    if (auto* ticket = std::get_if<ECDSATicket>(&data)) {
        return ticket->data;
    }
    throw std::bad_variant_access{};
}

const TicketData& Ticket::GetData() const {
    if (const auto* ticket = std::get_if<RSA4096Ticket>(&data)) {
        return ticket->data;
    }
    if (const auto* ticket = std::get_if<RSA2048Ticket>(&data)) {
        return ticket->data;
    }
    if (const auto* ticket = std::get_if<ECDSATicket>(&data)) {
        return ticket->data;
    }
    throw std::bad_variant_access{};
}

u64 Ticket::GetSize() const {
    const auto sig_type = GetSignatureType();

    return sizeof(SignatureType) + GetSignatureTypeDataSize(sig_type) +
           GetSignatureTypePaddingSize(sig_type) + sizeof(TicketData);
}

Ticket Ticket::SynthesizeCommon(Key128 title_key, const std::array<u8, 16>& rights_id) {
    RSA2048Ticket out{};
    out.sig_type = SignatureType::RSA_2048_SHA256;
    out.data.rights_id = rights_id;
    out.data.title_key_common = title_key;
    return Ticket{out};
}

Key128 GenerateKeyEncryptionKey(Key128 source, Key128 master, Key128 kek_seed, Key128 key_seed) {
    Key128 out{};

    AESCipher<Key128> cipher1(master, Mode::ECB);
    cipher1.Transcode(kek_seed.data(), kek_seed.size(), out.data(), Op::Decrypt);
    AESCipher<Key128> cipher2(out, Mode::ECB);
    cipher2.Transcode(source.data(), source.size(), out.data(), Op::Decrypt);

    if (key_seed != Key128{}) {
        AESCipher<Key128> cipher3(out, Mode::ECB);
        cipher3.Transcode(key_seed.data(), key_seed.size(), out.data(), Op::Decrypt);
    }

    return out;
}

Key128 DeriveKeyblobKey(const Key128& sbk, const Key128& tsec, Key128 source) {
    AESCipher<Key128> sbk_cipher(sbk, Mode::ECB);
    AESCipher<Key128> tsec_cipher(tsec, Mode::ECB);
    tsec_cipher.Transcode(source.data(), source.size(), source.data(), Op::Decrypt);
    sbk_cipher.Transcode(source.data(), source.size(), source.data(), Op::Decrypt);
    return source;
}

Key128 DeriveMasterKey(const std::array<u8, 0x90>& keyblob, const Key128& master_source) {
    Key128 master_root;
    std::memcpy(master_root.data(), keyblob.data(), sizeof(Key128));

    AESCipher<Key128> master_cipher(master_root, Mode::ECB);

    Key128 master{};
    master_cipher.Transcode(master_source.data(), master_source.size(), master.data(), Op::Decrypt);
    return master;
}

std::array<u8, 144> DecryptKeyblob(const std::array<u8, 176>& encrypted_keyblob,
                                   const Key128& key) {
    std::array<u8, 0x90> keyblob;
    AESCipher<Key128> cipher(key, Mode::CTR);
    cipher.SetIV(std::vector<u8>(encrypted_keyblob.data() + 0x10, encrypted_keyblob.data() + 0x20));
    cipher.Transcode(encrypted_keyblob.data() + 0x20, keyblob.size(), keyblob.data(), Op::Decrypt);
    return keyblob;
}

void KeyManager::DeriveGeneralPurposeKeys(std::size_t crypto_revision) {
    const auto kek_generation_source =
        GetKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKekGeneration));
    const auto key_generation_source =
        GetKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKeyGeneration));

    if (HasKey(S128KeyType::Master, crypto_revision)) {
        for (auto kak_type :
             {KeyAreaKeyType::Application, KeyAreaKeyType::Ocean, KeyAreaKeyType::System}) {
            if (HasKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::KeyAreaKey),
                       static_cast<u64>(kak_type))) {
                const auto source =
                    GetKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::KeyAreaKey),
                           static_cast<u64>(kak_type));
                const auto kek =
                    GenerateKeyEncryptionKey(source, GetKey(S128KeyType::Master, crypto_revision),
                                             kek_generation_source, key_generation_source);
                SetKey(S128KeyType::KeyArea, kek, crypto_revision, static_cast<u64>(kak_type));
            }
        }

        AESCipher<Key128> master_cipher(GetKey(S128KeyType::Master, crypto_revision), Mode::ECB);
        for (auto key_type : {SourceKeyType::Titlekek, SourceKeyType::Package2}) {
            if (HasKey(S128KeyType::Source, static_cast<u64>(key_type))) {
                Key128 key{};
                master_cipher.Transcode(
                    GetKey(S128KeyType::Source, static_cast<u64>(key_type)).data(), key.size(),
                    key.data(), Op::Decrypt);
                SetKey(key_type == SourceKeyType::Titlekek ? S128KeyType::Titlekek
                                                           : S128KeyType::Package2,
                       key, crypto_revision);
            }
        }
    }
}

RSAKeyPair<2048> KeyManager::GetETicketRSAKey() const {
    if (IsAllZeroArray(eticket_extended_kek) || !HasKey(S128KeyType::ETicketRSAKek)) {
        return {};
    }

    const auto eticket_final = GetKey(S128KeyType::ETicketRSAKek);

    std::vector<u8> extended_iv(eticket_extended_kek.begin(), eticket_extended_kek.begin() + 0x10);
    std::array<u8, 0x230> extended_dec{};
    AESCipher<Key128> rsa_1(eticket_final, Mode::CTR);
    rsa_1.SetIV(extended_iv);
    rsa_1.Transcode(eticket_extended_kek.data() + 0x10, eticket_extended_kek.size() - 0x10,
                    extended_dec.data(), Op::Decrypt);

    RSAKeyPair<2048> rsa_key{};
    std::memcpy(rsa_key.decryption_key.data(), extended_dec.data(), rsa_key.decryption_key.size());
    std::memcpy(rsa_key.modulus.data(), extended_dec.data() + 0x100, rsa_key.modulus.size());
    std::memcpy(rsa_key.exponent.data(), extended_dec.data() + 0x200, rsa_key.exponent.size());

    return rsa_key;
}

Key128 DeriveKeyblobMACKey(const Key128& keyblob_key, const Key128& mac_source) {
    AESCipher<Key128> mac_cipher(keyblob_key, Mode::ECB);
    Key128 mac_key{};
    mac_cipher.Transcode(mac_source.data(), mac_key.size(), mac_key.data(), Op::Decrypt);
    return mac_key;
}

std::optional<Key128> DeriveSDSeed() {
    const auto system_save_43_path =
        Common::FS::GetYuzuPath(Common::FS::YuzuPath::NANDDir) / "system/save/8000000000000043";
    const Common::FS::IOFile save_43{system_save_43_path, Common::FS::FileAccessMode::Read,
                                     Common::FS::FileType::BinaryFile};

    if (!save_43.IsOpen()) {
        return std::nullopt;
    }

    const auto sd_private_path =
        Common::FS::GetYuzuPath(Common::FS::YuzuPath::SDMCDir) / "Nintendo/Contents/private";

    const Common::FS::IOFile sd_private{sd_private_path, Common::FS::FileAccessMode::Read,
                                        Common::FS::FileType::BinaryFile};

    if (!sd_private.IsOpen()) {
        return std::nullopt;
    }

    std::array<u8, 0x10> private_seed{};
    if (sd_private.Read(private_seed) != private_seed.size()) {
        return std::nullopt;
    }

    std::array<u8, 0x10> buffer{};
    s64 offset = 0;
    for (; offset + 0x10 < static_cast<s64>(save_43.GetSize()); ++offset) {
        if (!save_43.Seek(offset)) {
            return std::nullopt;
        }

        if (save_43.Read(buffer) != buffer.size()) {
            return std::nullopt;
        }

        if (buffer == private_seed) {
            break;
        }
    }

    if (!save_43.Seek(offset + 0x10)) {
        return std::nullopt;
    }

    Key128 seed{};
    if (save_43.Read(seed) != seed.size()) {
        return std::nullopt;
    }

    return seed;
}

Loader::ResultStatus DeriveSDKeys(std::array<Key256, 2>& sd_keys, KeyManager& keys) {
    if (!keys.HasKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::SDKek))) {
        return Loader::ResultStatus::ErrorMissingSDKEKSource;
    }
    if (!keys.HasKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKekGeneration))) {
        return Loader::ResultStatus::ErrorMissingAESKEKGenerationSource;
    }
    if (!keys.HasKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKeyGeneration))) {
        return Loader::ResultStatus::ErrorMissingAESKeyGenerationSource;
    }

    const auto sd_kek_source =
        keys.GetKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::SDKek));
    const auto aes_kek_gen =
        keys.GetKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKekGeneration));
    const auto aes_key_gen =
        keys.GetKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKeyGeneration));
    const auto master_00 = keys.GetKey(S128KeyType::Master);
    const auto sd_kek =
        GenerateKeyEncryptionKey(sd_kek_source, master_00, aes_kek_gen, aes_key_gen);
    keys.SetKey(S128KeyType::SDKek, sd_kek);

    if (!keys.HasKey(S128KeyType::SDSeed)) {
        return Loader::ResultStatus::ErrorMissingSDSeed;
    }
    const auto sd_seed = keys.GetKey(S128KeyType::SDSeed);

    if (!keys.HasKey(S256KeyType::SDKeySource, static_cast<u64>(SDKeyType::Save))) {
        return Loader::ResultStatus::ErrorMissingSDSaveKeySource;
    }
    if (!keys.HasKey(S256KeyType::SDKeySource, static_cast<u64>(SDKeyType::NCA))) {
        return Loader::ResultStatus::ErrorMissingSDNCAKeySource;
    }

    std::array<Key256, 2> sd_key_sources{
        keys.GetKey(S256KeyType::SDKeySource, static_cast<u64>(SDKeyType::Save)),
        keys.GetKey(S256KeyType::SDKeySource, static_cast<u64>(SDKeyType::NCA)),
    };

    // Combine sources and seed
    for (auto& source : sd_key_sources) {
        for (std::size_t i = 0; i < source.size(); ++i) {
            source[i] = static_cast<u8>(source[i] ^ sd_seed[i & 0xF]);
        }
    }

    AESCipher<Key128> cipher(sd_kek, Mode::ECB);
    // The transform manipulates sd_keys as part of the Transcode, so the return/output is
    // unnecessary. This does not alter sd_keys_sources.
    std::transform(sd_key_sources.begin(), sd_key_sources.end(), sd_keys.begin(),
                   sd_key_sources.begin(), [&cipher](const Key256& source, Key256& out) {
                       cipher.Transcode(source.data(), source.size(), out.data(), Op::Decrypt);
                       return source; ///< Return unaltered source to satisfy output requirement.
                   });

    keys.SetKey(S256KeyType::SDKey, sd_keys[0], static_cast<u64>(SDKeyType::Save));
    keys.SetKey(S256KeyType::SDKey, sd_keys[1], static_cast<u64>(SDKeyType::NCA));

    return Loader::ResultStatus::Success;
}

std::vector<Ticket> GetTicketblob(const Common::FS::IOFile& ticket_save) {
    if (!ticket_save.IsOpen()) {
        return {};
    }

    std::vector<u8> buffer(ticket_save.GetSize());
    if (ticket_save.Read(buffer) != buffer.size()) {
        return {};
    }

    std::vector<Ticket> out;
    for (std::size_t offset = 0; offset + 0x4 < buffer.size(); ++offset) {
        if (buffer[offset] == 0x4 && buffer[offset + 1] == 0x0 && buffer[offset + 2] == 0x1 &&
            buffer[offset + 3] == 0x0) {
            out.emplace_back();
            auto& next = out.back();
            std::memcpy(&next, buffer.data() + offset, sizeof(Ticket));
            offset += FULL_TICKET_SIZE;
        }
    }

    return out;
}

template <size_t size>
static std::array<u8, size> operator^(const std::array<u8, size>& lhs,
                                      const std::array<u8, size>& rhs) {
    std::array<u8, size> out;
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), out.begin(),
                   [](u8 lhs_elem, u8 rhs_elem) { return u8(lhs_elem ^ rhs_elem); });
    return out;
}

template <size_t target_size, size_t in_size>
static std::array<u8, target_size> MGF1(const std::array<u8, in_size>& seed) {
    // Avoids truncation overflow within the loop below.
    static_assert(target_size <= 0xFF);

    std::array<u8, in_size + 4> seed_exp{};
    std::memcpy(seed_exp.data(), seed.data(), in_size);

    std::vector<u8> out;
    size_t i = 0;
    while (out.size() < target_size) {
        out.resize(out.size() + 0x20);
        seed_exp[in_size + 3] = static_cast<u8>(i);
        mbedtls_sha256_ret(seed_exp.data(), seed_exp.size(), out.data() + out.size() - 0x20, 0);
        ++i;
    }

    std::array<u8, target_size> target;
    std::memcpy(target.data(), out.data(), target_size);
    return target;
}

template <size_t size>
static std::optional<u64> FindTicketOffset(const std::array<u8, size>& data) {
    u64 offset = 0;
    for (size_t i = 0x20; i < data.size() - 0x10; ++i) {
        if (data[i] == 0x1) {
            offset = i + 1;
            break;
        } else if (data[i] != 0x0) {
            return std::nullopt;
        }
    }

    return offset;
}

std::optional<std::pair<Key128, Key128>> ParseTicket(const Ticket& ticket,
                                                     const RSAKeyPair<2048>& key) {
    const auto issuer = ticket.GetData().issuer;
    if (IsAllZeroArray(issuer)) {
        return std::nullopt;
    }
    if (issuer[0] != 'R' || issuer[1] != 'o' || issuer[2] != 'o' || issuer[3] != 't') {
        LOG_INFO(Crypto, "Attempting to parse ticket with non-standard certificate authority.");
    }

    Key128 rights_id = ticket.GetData().rights_id;

    if (rights_id == Key128{}) {
        return std::nullopt;
    }

    if (!std::any_of(ticket.GetData().title_key_common_pad.begin(),
                     ticket.GetData().title_key_common_pad.end(), [](u8 b) { return b != 0; })) {
        return std::make_pair(rights_id, ticket.GetData().title_key_common);
    }

    mbedtls_mpi D; // RSA Private Exponent
    mbedtls_mpi N; // RSA Modulus
    mbedtls_mpi S; // Input
    mbedtls_mpi M; // Output

    mbedtls_mpi_init(&D);
    mbedtls_mpi_init(&N);
    mbedtls_mpi_init(&S);
    mbedtls_mpi_init(&M);

    mbedtls_mpi_read_binary(&D, key.decryption_key.data(), key.decryption_key.size());
    mbedtls_mpi_read_binary(&N, key.modulus.data(), key.modulus.size());
    mbedtls_mpi_read_binary(&S, ticket.GetData().title_key_block.data(), 0x100);

    mbedtls_mpi_exp_mod(&M, &S, &D, &N, nullptr);

    std::array<u8, 0x100> rsa_step;
    mbedtls_mpi_write_binary(&M, rsa_step.data(), rsa_step.size());

    u8 m_0 = rsa_step[0];
    std::array<u8, 0x20> m_1;
    std::memcpy(m_1.data(), rsa_step.data() + 0x01, m_1.size());
    std::array<u8, 0xDF> m_2;
    std::memcpy(m_2.data(), rsa_step.data() + 0x21, m_2.size());

    if (m_0 != 0) {
        return std::nullopt;
    }

    m_1 = m_1 ^ MGF1<0x20>(m_2);
    m_2 = m_2 ^ MGF1<0xDF>(m_1);

    const auto offset = FindTicketOffset(m_2);
    if (!offset) {
        return std::nullopt;
    }
    ASSERT(*offset > 0);

    Key128 key_temp{};
    std::memcpy(key_temp.data(), m_2.data() + *offset, key_temp.size());

    return std::make_pair(rights_id, key_temp);
}

KeyManager::KeyManager() {
    // Initialize keys
    const auto yuzu_keys_dir = Common::FS::GetYuzuPath(Common::FS::YuzuPath::KeysDir);

    if (!Common::FS::CreateDir(yuzu_keys_dir)) {
        LOG_ERROR(Core, "Failed to create the keys directory.");
    }

    if (Settings::values.use_dev_keys) {
        dev_mode = true;
        LoadFromFile(yuzu_keys_dir / "dev.keys", false);
        LoadFromFile(yuzu_keys_dir / "dev.keys_autogenerated", false);
    } else {
        dev_mode = false;
        LoadFromFile(yuzu_keys_dir / "prod.keys", false);
        LoadFromFile(yuzu_keys_dir / "prod.keys_autogenerated", false);
    }

    LoadFromFile(yuzu_keys_dir / "title.keys", true);
    LoadFromFile(yuzu_keys_dir / "title.keys_autogenerated", true);
    LoadFromFile(yuzu_keys_dir / "console.keys", false);
    LoadFromFile(yuzu_keys_dir / "console.keys_autogenerated", false);
}

static bool ValidCryptoRevisionString(std::string_view base, size_t begin, size_t length) {
    if (base.size() < begin + length) {
        return false;
    }
    return std::all_of(base.begin() + begin, base.begin() + begin + length,
                       [](u8 c) { return std::isxdigit(c); });
}

void KeyManager::LoadFromFile(const std::filesystem::path& file_path, bool is_title_keys) {
    if (!Common::FS::Exists(file_path)) {
        return;
    }

    std::ifstream file;
    Common::FS::OpenFileStream(file, file_path, std::ios_base::in);

    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> out;
        std::stringstream stream(line);
        std::string item;
        while (std::getline(stream, item, '=')) {
            out.push_back(std::move(item));
        }

        if (out.size() != 2) {
            continue;
        }

        out[0].erase(std::remove(out[0].begin(), out[0].end(), ' '), out[0].end());
        out[1].erase(std::remove(out[1].begin(), out[1].end(), ' '), out[1].end());

        if (out[0].compare(0, 1, "#") == 0) {
            continue;
        }

        if (is_title_keys) {
            auto rights_id_raw = Common::HexStringToArray<16>(out[0]);
            u128 rights_id{};
            std::memcpy(rights_id.data(), rights_id_raw.data(), rights_id_raw.size());
            Key128 key = Common::HexStringToArray<16>(out[1]);
            s128_keys[{S128KeyType::Titlekey, rights_id[1], rights_id[0]}] = key;
        } else {
            out[0] = Common::ToLower(out[0]);
            if (const auto iter128 = Find128ByName(out[0]); iter128 != s128_file_id.end()) {
                const auto& index = iter128->second;
                const Key128 key = Common::HexStringToArray<16>(out[1]);
                s128_keys[{index.type, index.field1, index.field2}] = key;
            } else if (const auto iter256 = Find256ByName(out[0]); iter256 != s256_file_id.end()) {
                const auto& index = iter256->second;
                const Key256 key = Common::HexStringToArray<32>(out[1]);
                s256_keys[{index.type, index.field1, index.field2}] = key;
            } else if (out[0].compare(0, 8, "keyblob_") == 0 &&
                       out[0].compare(0, 9, "keyblob_k") != 0) {
                if (!ValidCryptoRevisionString(out[0], 8, 2)) {
                    continue;
                }

                const auto index = std::stoul(out[0].substr(8, 2), nullptr, 16);
                keyblobs[index] = Common::HexStringToArray<0x90>(out[1]);
            } else if (out[0].compare(0, 18, "encrypted_keyblob_") == 0) {
                if (!ValidCryptoRevisionString(out[0], 18, 2)) {
                    continue;
                }

                const auto index = std::stoul(out[0].substr(18, 2), nullptr, 16);
                encrypted_keyblobs[index] = Common::HexStringToArray<0xB0>(out[1]);
            } else if (out[0].compare(0, 20, "eticket_extended_kek") == 0) {
                eticket_extended_kek = Common::HexStringToArray<576>(out[1]);
            } else {
                for (const auto& kv : KEYS_VARIABLE_LENGTH) {
                    if (!ValidCryptoRevisionString(out[0], kv.second.size(), 2)) {
                        continue;
                    }
                    if (out[0].compare(0, kv.second.size(), kv.second) == 0) {
                        const auto index =
                            std::stoul(out[0].substr(kv.second.size(), 2), nullptr, 16);
                        const auto sub = kv.first.second;
                        if (sub == 0) {
                            s128_keys[{kv.first.first, index, 0}] =
                                Common::HexStringToArray<16>(out[1]);
                        } else {
                            s128_keys[{kv.first.first, kv.first.second, index}] =
                                Common::HexStringToArray<16>(out[1]);
                        }

                        break;
                    }
                }

                static constexpr std::array<const char*, 3> kak_names = {
                    "key_area_key_application_", "key_area_key_ocean_", "key_area_key_system_"};
                for (size_t j = 0; j < kak_names.size(); ++j) {
                    const auto& match = kak_names[j];
                    if (out[0].compare(0, std::strlen(match), match) == 0) {
                        const auto index =
                            std::stoul(out[0].substr(std::strlen(match), 2), nullptr, 16);
                        s128_keys[{S128KeyType::KeyArea, index, j}] =
                            Common::HexStringToArray<16>(out[1]);
                    }
                }
            }
        }
    }
}

bool KeyManager::BaseDeriveNecessary() const {
    const auto check_key_existence = [this](auto key_type, u64 index1 = 0, u64 index2 = 0) {
        return !HasKey(key_type, index1, index2);
    };

    if (check_key_existence(S256KeyType::Header)) {
        return true;
    }

    for (size_t i = 0; i < CURRENT_CRYPTO_REVISION; ++i) {
        if (check_key_existence(S128KeyType::Master, i) ||
            check_key_existence(S128KeyType::KeyArea, i,
                                static_cast<u64>(KeyAreaKeyType::Application)) ||
            check_key_existence(S128KeyType::KeyArea, i, static_cast<u64>(KeyAreaKeyType::Ocean)) ||
            check_key_existence(S128KeyType::KeyArea, i,
                                static_cast<u64>(KeyAreaKeyType::System)) ||
            check_key_existence(S128KeyType::Titlekek, i))
            return true;
    }

    return false;
}

bool KeyManager::HasKey(S128KeyType id, u64 field1, u64 field2) const {
    return s128_keys.find({id, field1, field2}) != s128_keys.end();
}

bool KeyManager::HasKey(S256KeyType id, u64 field1, u64 field2) const {
    return s256_keys.find({id, field1, field2}) != s256_keys.end();
}

Key128 KeyManager::GetKey(S128KeyType id, u64 field1, u64 field2) const {
    if (!HasKey(id, field1, field2)) {
        return {};
    }
    return s128_keys.at({id, field1, field2});
}

Key256 KeyManager::GetKey(S256KeyType id, u64 field1, u64 field2) const {
    if (!HasKey(id, field1, field2)) {
        return {};
    }
    return s256_keys.at({id, field1, field2});
}

Key256 KeyManager::GetBISKey(u8 partition_id) const {
    Key256 out{};

    for (const auto& bis_type : {BISKeyType::Crypto, BISKeyType::Tweak}) {
        if (HasKey(S128KeyType::BIS, partition_id, static_cast<u64>(bis_type))) {
            std::memcpy(
                out.data() + sizeof(Key128) * static_cast<u64>(bis_type),
                s128_keys.at({S128KeyType::BIS, partition_id, static_cast<u64>(bis_type)}).data(),
                sizeof(Key128));
        }
    }

    return out;
}

template <size_t Size>
void KeyManager::WriteKeyToFile(KeyCategory category, std::string_view keyname,
                                const std::array<u8, Size>& key) {
    const auto yuzu_keys_dir = Common::FS::GetYuzuPath(Common::FS::YuzuPath::KeysDir);

    std::string filename = "title.keys_autogenerated";

    if (category == KeyCategory::Standard) {
        filename = dev_mode ? "dev.keys_autogenerated" : "prod.keys_autogenerated";
    } else if (category == KeyCategory::Console) {
        filename = "console.keys_autogenerated";
    }

    const auto path = yuzu_keys_dir / filename;
    const auto add_info_text = !Common::FS::Exists(path);

    Common::FS::IOFile file{path, Common::FS::FileAccessMode::Append,
                            Common::FS::FileType::TextFile};

    if (!file.IsOpen()) {
        return;
    }

    if (add_info_text) {
        void(file.WriteString(
            "# This file is autogenerated by Yuzu\n"
            "# It serves to store keys that were automatically generated from the normal keys\n"
            "# If you are experiencing issues involving keys, it may help to delete this file\n"));
    }

    void(file.WriteString(fmt::format("\n{} = {}", keyname, Common::HexToString(key))));
    LoadFromFile(path, category == KeyCategory::Title);
}

void KeyManager::SetKey(S128KeyType id, Key128 key, u64 field1, u64 field2) {
    if (s128_keys.find({id, field1, field2}) != s128_keys.end() || key == Key128{}) {
        return;
    }
    if (id == S128KeyType::Titlekey) {
        Key128 rights_id;
        std::memcpy(rights_id.data(), &field2, sizeof(u64));
        std::memcpy(rights_id.data() + sizeof(u64), &field1, sizeof(u64));
        WriteKeyToFile(KeyCategory::Title, Common::HexToString(rights_id), key);
    }

    auto category = KeyCategory::Standard;
    if (id == S128KeyType::Keyblob || id == S128KeyType::KeyblobMAC || id == S128KeyType::TSEC ||
        id == S128KeyType::SecureBoot || id == S128KeyType::SDSeed || id == S128KeyType::BIS) {
        category = KeyCategory::Console;
    }

    const auto iter2 = std::find_if(
        s128_file_id.begin(), s128_file_id.end(), [&id, &field1, &field2](const auto& elem) {
            return std::tie(elem.second.type, elem.second.field1, elem.second.field2) ==
                   std::tie(id, field1, field2);
        });
    if (iter2 != s128_file_id.end()) {
        WriteKeyToFile(category, iter2->first, key);
    }

    // Variable cases
    if (id == S128KeyType::KeyArea) {
        static constexpr std::array<const char*, 3> kak_names = {
            "key_area_key_application_{:02X}",
            "key_area_key_ocean_{:02X}",
            "key_area_key_system_{:02X}",
        };
        WriteKeyToFile(category, fmt::format(kak_names.at(field2), field1), key);
    } else if (id == S128KeyType::Master) {
        WriteKeyToFile(category, fmt::format("master_key_{:02X}", field1), key);
    } else if (id == S128KeyType::Package1) {
        WriteKeyToFile(category, fmt::format("package1_key_{:02X}", field1), key);
    } else if (id == S128KeyType::Package2) {
        WriteKeyToFile(category, fmt::format("package2_key_{:02X}", field1), key);
    } else if (id == S128KeyType::Titlekek) {
        WriteKeyToFile(category, fmt::format("titlekek_{:02X}", field1), key);
    } else if (id == S128KeyType::Keyblob) {
        WriteKeyToFile(category, fmt::format("keyblob_key_{:02X}", field1), key);
    } else if (id == S128KeyType::KeyblobMAC) {
        WriteKeyToFile(category, fmt::format("keyblob_mac_key_{:02X}", field1), key);
    } else if (id == S128KeyType::Source && field1 == static_cast<u64>(SourceKeyType::Keyblob)) {
        WriteKeyToFile(category, fmt::format("keyblob_key_source_{:02X}", field2), key);
    }

    s128_keys[{id, field1, field2}] = key;
}

void KeyManager::SetKey(S256KeyType id, Key256 key, u64 field1, u64 field2) {
    if (s256_keys.find({id, field1, field2}) != s256_keys.end() || key == Key256{}) {
        return;
    }
    const auto iter = std::find_if(
        s256_file_id.begin(), s256_file_id.end(), [&id, &field1, &field2](const auto& elem) {
            return std::tie(elem.second.type, elem.second.field1, elem.second.field2) ==
                   std::tie(id, field1, field2);
        });
    if (iter != s256_file_id.end()) {
        WriteKeyToFile(KeyCategory::Standard, iter->first, key);
    }
    s256_keys[{id, field1, field2}] = key;
}

bool KeyManager::KeyFileExists(bool title) {
    const auto yuzu_keys_dir = Common::FS::GetYuzuPath(Common::FS::YuzuPath::KeysDir);

    if (title) {
        return Common::FS::Exists(yuzu_keys_dir / "title.keys");
    }

    if (Settings::values.use_dev_keys) {
        return Common::FS::Exists(yuzu_keys_dir / "dev.keys");
    }

    return Common::FS::Exists(yuzu_keys_dir / "prod.keys");
}

void KeyManager::DeriveSDSeedLazy() {
    if (HasKey(S128KeyType::SDSeed)) {
        return;
    }

    const auto res = DeriveSDSeed();
    if (res) {
        SetKey(S128KeyType::SDSeed, *res);
    }
}

static Key128 CalculateCMAC(const u8* source, size_t size, const Key128& key) {
    Key128 out{};

    mbedtls_cipher_cmac(mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB), key.data(),
                        key.size() * 8, source, size, out.data());
    return out;
}

void KeyManager::DeriveBase() {
    if (!BaseDeriveNecessary()) {
        return;
    }

    if (!HasKey(S128KeyType::SecureBoot) || !HasKey(S128KeyType::TSEC)) {
        return;
    }

    const auto has_bis = [this](u64 id) {
        return HasKey(S128KeyType::BIS, id, static_cast<u64>(BISKeyType::Crypto)) &&
               HasKey(S128KeyType::BIS, id, static_cast<u64>(BISKeyType::Tweak));
    };

    const auto copy_bis = [this](u64 id_from, u64 id_to) {
        SetKey(S128KeyType::BIS,
               GetKey(S128KeyType::BIS, id_from, static_cast<u64>(BISKeyType::Crypto)), id_to,
               static_cast<u64>(BISKeyType::Crypto));

        SetKey(S128KeyType::BIS,
               GetKey(S128KeyType::BIS, id_from, static_cast<u64>(BISKeyType::Tweak)), id_to,
               static_cast<u64>(BISKeyType::Tweak));
    };

    if (has_bis(2) && !has_bis(3)) {
        copy_bis(2, 3);
    } else if (has_bis(3) && !has_bis(2)) {
        copy_bis(3, 2);
    }

    std::bitset<32> revisions(0xFFFFFFFF);
    for (size_t i = 0; i < revisions.size(); ++i) {
        if (!HasKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::Keyblob), i) ||
            encrypted_keyblobs[i] == std::array<u8, 0xB0>{}) {
            revisions.reset(i);
        }
    }

    if (!revisions.any()) {
        return;
    }

    const auto sbk = GetKey(S128KeyType::SecureBoot);
    const auto tsec = GetKey(S128KeyType::TSEC);

    for (size_t i = 0; i < revisions.size(); ++i) {
        if (!revisions[i]) {
            continue;
        }

        // Derive keyblob key
        const auto key = DeriveKeyblobKey(
            sbk, tsec, GetKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::Keyblob), i));

        SetKey(S128KeyType::Keyblob, key, i);

        // Derive keyblob MAC key
        if (!HasKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::KeyblobMAC))) {
            continue;
        }

        const auto mac_key = DeriveKeyblobMACKey(
            key, GetKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::KeyblobMAC)));
        SetKey(S128KeyType::KeyblobMAC, mac_key, i);

        Key128 cmac = CalculateCMAC(encrypted_keyblobs[i].data() + 0x10, 0xA0, mac_key);
        if (std::memcmp(cmac.data(), encrypted_keyblobs[i].data(), cmac.size()) != 0) {
            continue;
        }

        // Decrypt keyblob
        if (keyblobs[i] == std::array<u8, 0x90>{}) {
            keyblobs[i] = DecryptKeyblob(encrypted_keyblobs[i], key);
            WriteKeyToFile<0x90>(KeyCategory::Console, fmt::format("keyblob_{:02X}", i),
                                 keyblobs[i]);
        }

        Key128 package1;
        std::memcpy(package1.data(), keyblobs[i].data() + 0x80, sizeof(Key128));
        SetKey(S128KeyType::Package1, package1, i);

        // Derive master key
        if (HasKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::Master))) {
            SetKey(S128KeyType::Master,
                   DeriveMasterKey(keyblobs[i], GetKey(S128KeyType::Source,
                                                       static_cast<u64>(SourceKeyType::Master))),
                   i);
        }
    }

    revisions.set();
    for (size_t i = 0; i < revisions.size(); ++i) {
        if (!HasKey(S128KeyType::Master, i)) {
            revisions.reset(i);
        }
    }

    if (!revisions.any()) {
        return;
    }

    for (size_t i = 0; i < revisions.size(); ++i) {
        if (!revisions[i]) {
            continue;
        }

        // Derive general purpose keys
        DeriveGeneralPurposeKeys(i);
    }

    if (HasKey(S128KeyType::Master, 0) &&
        HasKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKeyGeneration)) &&
        HasKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKekGeneration)) &&
        HasKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::HeaderKek)) &&
        HasKey(S256KeyType::HeaderSource)) {
        const auto header_kek = GenerateKeyEncryptionKey(
            GetKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::HeaderKek)),
            GetKey(S128KeyType::Master, 0),
            GetKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKekGeneration)),
            GetKey(S128KeyType::Source, static_cast<u64>(SourceKeyType::AESKeyGeneration)));
        SetKey(S128KeyType::HeaderKek, header_kek);

        AESCipher<Key128> header_cipher(header_kek, Mode::ECB);
        Key256 out = GetKey(S256KeyType::HeaderSource);
        header_cipher.Transcode(out.data(), out.size(), out.data(), Op::Decrypt);
        SetKey(S256KeyType::Header, out);
    }
}

void KeyManager::DeriveETicket(PartitionDataManager& data,
                               const FileSys::ContentProvider& provider) {
    // ETicket keys
    const auto es = provider.GetEntry(0x0100000000000033, FileSys::ContentRecordType::Program);

    if (es == nullptr) {
        return;
    }

    const auto exefs = es->GetExeFS();
    if (exefs == nullptr) {
        return;
    }

    const auto main = exefs->GetFile("main");
    if (main == nullptr) {
        return;
    }

    const auto bytes = main->ReadAllBytes();

    const auto eticket_kek = FindKeyFromHex16(bytes, eticket_source_hashes[0]);
    const auto eticket_kekek = FindKeyFromHex16(bytes, eticket_source_hashes[1]);

    const auto seed3 = data.GetRSAKekSeed3();
    const auto mask0 = data.GetRSAKekMask0();

    if (eticket_kek != Key128{}) {
        SetKey(S128KeyType::Source, eticket_kek, static_cast<size_t>(SourceKeyType::ETicketKek));
    }
    if (eticket_kekek != Key128{}) {
        SetKey(S128KeyType::Source, eticket_kekek,
               static_cast<size_t>(SourceKeyType::ETicketKekek));
    }
    if (seed3 != Key128{}) {
        SetKey(S128KeyType::RSAKek, seed3, static_cast<size_t>(RSAKekType::Seed3));
    }
    if (mask0 != Key128{}) {
        SetKey(S128KeyType::RSAKek, mask0, static_cast<size_t>(RSAKekType::Mask0));
    }
    if (eticket_kek == Key128{} || eticket_kekek == Key128{} || seed3 == Key128{} ||
        mask0 == Key128{}) {
        return;
    }

    const Key128 rsa_oaep_kek = seed3 ^ mask0;
    if (rsa_oaep_kek == Key128{}) {
        return;
    }

    SetKey(S128KeyType::Source, rsa_oaep_kek,
           static_cast<u64>(SourceKeyType::RSAOaepKekGeneration));

    Key128 temp_kek{};
    Key128 temp_kekek{};
    Key128 eticket_final{};

    // Derive ETicket RSA Kek
    AESCipher<Key128> es_master(GetKey(S128KeyType::Master), Mode::ECB);
    es_master.Transcode(rsa_oaep_kek.data(), rsa_oaep_kek.size(), temp_kek.data(), Op::Decrypt);
    AESCipher<Key128> es_kekek(temp_kek, Mode::ECB);
    es_kekek.Transcode(eticket_kekek.data(), eticket_kekek.size(), temp_kekek.data(), Op::Decrypt);
    AESCipher<Key128> es_kek(temp_kekek, Mode::ECB);
    es_kek.Transcode(eticket_kek.data(), eticket_kek.size(), eticket_final.data(), Op::Decrypt);

    if (eticket_final == Key128{}) {
        return;
    }

    SetKey(S128KeyType::ETicketRSAKek, eticket_final);

    // Titlekeys
    data.DecryptProdInfo(GetBISKey(0));

    eticket_extended_kek = data.GetETicketExtendedKek();
    WriteKeyToFile(KeyCategory::Console, "eticket_extended_kek", eticket_extended_kek);
    PopulateTickets();
}

void KeyManager::PopulateTickets() {
    const auto rsa_key = GetETicketRSAKey();

    if (rsa_key == RSAKeyPair<2048>{}) {
        return;
    }

    if (!common_tickets.empty() && !personal_tickets.empty()) {
        return;
    }

    const auto system_save_e1_path =
        Common::FS::GetYuzuPath(Common::FS::YuzuPath::NANDDir) / "system/save/80000000000000e1";

    const Common::FS::IOFile save_e1{system_save_e1_path, Common::FS::FileAccessMode::Read,
                                     Common::FS::FileType::BinaryFile};

    const auto system_save_e2_path =
        Common::FS::GetYuzuPath(Common::FS::YuzuPath::NANDDir) / "system/save/80000000000000e2";

    const Common::FS::IOFile save_e2{system_save_e2_path, Common::FS::FileAccessMode::Read,
                                     Common::FS::FileType::BinaryFile};

    const auto blob2 = GetTicketblob(save_e2);
    auto res = GetTicketblob(save_e1);

    const auto idx = res.size();
    res.insert(res.end(), blob2.begin(), blob2.end());

    for (std::size_t i = 0; i < res.size(); ++i) {
        const auto common = i < idx;
        const auto pair = ParseTicket(res[i], rsa_key);
        if (!pair) {
            continue;
        }

        const auto& [rid, key] = *pair;
        u128 rights_id;
        std::memcpy(rights_id.data(), rid.data(), rid.size());

        if (common) {
            common_tickets[rights_id] = res[i];
        } else {
            personal_tickets[rights_id] = res[i];
        }

        SetKey(S128KeyType::Titlekey, key, rights_id[1], rights_id[0]);
    }
}

void KeyManager::SynthesizeTickets() {
    for (const auto& key : s128_keys) {
        if (key.first.type != S128KeyType::Titlekey) {
            continue;
        }
        u128 rights_id{key.first.field1, key.first.field2};
        Key128 rights_id_2;
        std::memcpy(rights_id_2.data(), rights_id.data(), rights_id_2.size());
        const auto ticket = Ticket::SynthesizeCommon(key.second, rights_id_2);
        common_tickets.insert_or_assign(rights_id, ticket);
    }
}

void KeyManager::SetKeyWrapped(S128KeyType id, Key128 key, u64 field1, u64 field2) {
    if (key == Key128{}) {
        return;
    }
    SetKey(id, key, field1, field2);
}

void KeyManager::SetKeyWrapped(S256KeyType id, Key256 key, u64 field1, u64 field2) {
    if (key == Key256{}) {
        return;
    }

    SetKey(id, key, field1, field2);
}

void KeyManager::PopulateFromPartitionData(PartitionDataManager& data) {
    if (!BaseDeriveNecessary()) {
        return;
    }

    if (!data.HasBoot0()) {
        return;
    }

    for (size_t i = 0; i < encrypted_keyblobs.size(); ++i) {
        if (encrypted_keyblobs[i] != std::array<u8, 0xB0>{}) {
            continue;
        }
        encrypted_keyblobs[i] = data.GetEncryptedKeyblob(i);
        WriteKeyToFile<0xB0>(KeyCategory::Console, fmt::format("encrypted_keyblob_{:02X}", i),
                             encrypted_keyblobs[i]);
    }

    SetKeyWrapped(S128KeyType::Source, data.GetPackage2KeySource(),
                  static_cast<u64>(SourceKeyType::Package2));
    SetKeyWrapped(S128KeyType::Source, data.GetAESKekGenerationSource(),
                  static_cast<u64>(SourceKeyType::AESKekGeneration));
    SetKeyWrapped(S128KeyType::Source, data.GetTitlekekSource(),
                  static_cast<u64>(SourceKeyType::Titlekek));
    SetKeyWrapped(S128KeyType::Source, data.GetMasterKeySource(),
                  static_cast<u64>(SourceKeyType::Master));
    SetKeyWrapped(S128KeyType::Source, data.GetKeyblobMACKeySource(),
                  static_cast<u64>(SourceKeyType::KeyblobMAC));

    for (size_t i = 0; i < PartitionDataManager::MAX_KEYBLOB_SOURCE_HASH; ++i) {
        SetKeyWrapped(S128KeyType::Source, data.GetKeyblobKeySource(i),
                      static_cast<u64>(SourceKeyType::Keyblob), i);
    }

    if (data.HasFuses()) {
        SetKeyWrapped(S128KeyType::SecureBoot, data.GetSecureBootKey());
    }

    DeriveBase();

    Key128 latest_master{};
    for (s8 i = 0x1F; i >= 0; --i) {
        if (GetKey(S128KeyType::Master, static_cast<u8>(i)) != Key128{}) {
            latest_master = GetKey(S128KeyType::Master, static_cast<u8>(i));
            break;
        }
    }

    const auto masters = data.GetTZMasterKeys(latest_master);
    for (size_t i = 0; i < masters.size(); ++i) {
        if (masters[i] != Key128{} && !HasKey(S128KeyType::Master, i)) {
            SetKey(S128KeyType::Master, masters[i], i);
        }
    }

    DeriveBase();

    if (!data.HasPackage2())
        return;

    std::array<Key128, 0x20> package2_keys{};
    for (size_t i = 0; i < package2_keys.size(); ++i) {
        if (HasKey(S128KeyType::Package2, i)) {
            package2_keys[i] = GetKey(S128KeyType::Package2, i);
        }
    }
    data.DecryptPackage2(package2_keys, Package2Type::NormalMain);

    SetKeyWrapped(S128KeyType::Source, data.GetKeyAreaKeyApplicationSource(),
                  static_cast<u64>(SourceKeyType::KeyAreaKey),
                  static_cast<u64>(KeyAreaKeyType::Application));
    SetKeyWrapped(S128KeyType::Source, data.GetKeyAreaKeyOceanSource(),
                  static_cast<u64>(SourceKeyType::KeyAreaKey),
                  static_cast<u64>(KeyAreaKeyType::Ocean));
    SetKeyWrapped(S128KeyType::Source, data.GetKeyAreaKeySystemSource(),
                  static_cast<u64>(SourceKeyType::KeyAreaKey),
                  static_cast<u64>(KeyAreaKeyType::System));
    SetKeyWrapped(S128KeyType::Source, data.GetSDKekSource(),
                  static_cast<u64>(SourceKeyType::SDKek));
    SetKeyWrapped(S256KeyType::SDKeySource, data.GetSDSaveKeySource(),
                  static_cast<u64>(SDKeyType::Save));
    SetKeyWrapped(S256KeyType::SDKeySource, data.GetSDNCAKeySource(),
                  static_cast<u64>(SDKeyType::NCA));
    SetKeyWrapped(S128KeyType::Source, data.GetHeaderKekSource(),
                  static_cast<u64>(SourceKeyType::HeaderKek));
    SetKeyWrapped(S256KeyType::HeaderSource, data.GetHeaderKeySource());
    SetKeyWrapped(S128KeyType::Source, data.GetAESKeyGenerationSource(),
                  static_cast<u64>(SourceKeyType::AESKeyGeneration));

    DeriveBase();
}

const std::map<u128, Ticket>& KeyManager::GetCommonTickets() const {
    return common_tickets;
}

const std::map<u128, Ticket>& KeyManager::GetPersonalizedTickets() const {
    return personal_tickets;
}

bool KeyManager::AddTicketCommon(Ticket raw) {
    const auto rsa_key = GetETicketRSAKey();
    if (rsa_key == RSAKeyPair<2048>{}) {
        return false;
    }

    const auto pair = ParseTicket(raw, rsa_key);
    if (!pair) {
        return false;
    }

    const auto& [rid, key] = *pair;
    u128 rights_id;
    std::memcpy(rights_id.data(), rid.data(), rid.size());
    common_tickets[rights_id] = raw;
    SetKey(S128KeyType::Titlekey, key, rights_id[1], rights_id[0]);
    return true;
}

bool KeyManager::AddTicketPersonalized(Ticket raw) {
    const auto rsa_key = GetETicketRSAKey();
    if (rsa_key == RSAKeyPair<2048>{}) {
        return false;
    }

    const auto pair = ParseTicket(raw, rsa_key);
    if (!pair) {
        return false;
    }

    const auto& [rid, key] = *pair;
    u128 rights_id;
    std::memcpy(rights_id.data(), rid.data(), rid.size());
    common_tickets[rights_id] = raw;
    SetKey(S128KeyType::Titlekey, key, rights_id[1], rights_id[0]);
    return true;
}
} // namespace Core::Crypto
