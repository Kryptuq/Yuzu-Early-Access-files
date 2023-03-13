// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <filesystem>
#include <map>
#include <optional>
#include <string>

#include <variant>
#include <fmt/format.h>
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/crypto/partition_data_manager.h"

namespace Common::FS {
class IOFile;
}

namespace FileSys {
class ContentProvider;
}

namespace Loader {
enum class ResultStatus : u16;
}

namespace Core::Crypto {

constexpr u64 TICKET_FILE_TITLEKEY_OFFSET = 0x180;

using Key128 = std::array<u8, 0x10>;
using Key256 = std::array<u8, 0x20>;
using SHA256Hash = std::array<u8, 0x20>;

enum class SignatureType {
    RSA_4096_SHA1 = 0x10000,
    RSA_2048_SHA1 = 0x10001,
    ECDSA_SHA1 = 0x10002,
    RSA_4096_SHA256 = 0x10003,
    RSA_2048_SHA256 = 0x10004,
    ECDSA_SHA256 = 0x10005,
};

u64 GetSignatureTypeDataSize(SignatureType type);
u64 GetSignatureTypePaddingSize(SignatureType type);

enum class TitleKeyType : u8 {
    Common = 0,
    Personalized = 1,
};

struct TicketData {
    std::array<u8, 0x40> issuer;
    union {
        std::array<u8, 0x100> title_key_block;

        struct {
            Key128 title_key_common;
            std::array<u8, 0xF0> title_key_common_pad;
        };
    };

    INSERT_PADDING_BYTES(0x1);
    TitleKeyType type;
    INSERT_PADDING_BYTES(0x3);
    u8 revision;
    INSERT_PADDING_BYTES(0xA);
    u64 ticket_id;
    u64 device_id;
    std::array<u8, 0x10> rights_id;
    u32 account_id;
    INSERT_PADDING_BYTES(0x14C);
};
static_assert(sizeof(TicketData) == 0x2C0, "TicketData has incorrect size.");

struct RSA4096Ticket {
    SignatureType sig_type;
    std::array<u8, 0x200> sig_data;
    INSERT_PADDING_BYTES(0x3C);
    TicketData data;
};

struct RSA2048Ticket {
    SignatureType sig_type;
    std::array<u8, 0x100> sig_data;
    INSERT_PADDING_BYTES(0x3C);
    TicketData data;
};

struct ECDSATicket {
    SignatureType sig_type;
    std::array<u8, 0x3C> sig_data;
    INSERT_PADDING_BYTES(0x40);
    TicketData data;
};

struct Ticket {
    std::variant<RSA4096Ticket, RSA2048Ticket, ECDSATicket> data;

    SignatureType GetSignatureType() const;
    TicketData& GetData();
    const TicketData& GetData() const;
    u64 GetSize() const;

    static Ticket SynthesizeCommon(Key128 title_key, const std::array<u8, 0x10>& rights_id);
};

static_assert(sizeof(Key128) == 16, "Key128 must be 128 bytes big.");
static_assert(sizeof(Key256) == 32, "Key256 must be 256 bytes big.");

template <size_t bit_size, size_t byte_size = (bit_size >> 3)>
struct RSAKeyPair {
    std::array<u8, byte_size> encryption_key;
    std::array<u8, byte_size> decryption_key;
    std::array<u8, byte_size> modulus;
    std::array<u8, 4> exponent;
};

template <size_t bit_size, size_t byte_size>
bool operator==(const RSAKeyPair<bit_size, byte_size>& lhs,
                const RSAKeyPair<bit_size, byte_size>& rhs) {
    return std::tie(lhs.encryption_key, lhs.decryption_key, lhs.modulus, lhs.exponent) ==
           std::tie(rhs.encryption_key, rhs.decryption_key, rhs.modulus, rhs.exponent);
}

template <size_t bit_size, size_t byte_size>
bool operator!=(const RSAKeyPair<bit_size, byte_size>& lhs,
                const RSAKeyPair<bit_size, byte_size>& rhs) {
    return !(lhs == rhs);
}

enum class KeyCategory : u8 {
    Standard,
    Title,
    Console,
};

enum class S256KeyType : u64 {
    SDKey,        // f1=SDKeyType
    Header,       //
    SDKeySource,  // f1=SDKeyType
    HeaderSource, //
};

enum class S128KeyType : u64 {
    Master,        // f1=crypto revision
    Package1,      // f1=crypto revision
    Package2,      // f1=crypto revision
    Titlekek,      // f1=crypto revision
    ETicketRSAKek, //
    KeyArea,       // f1=crypto revision f2=type {app, ocean, system}
    SDSeed,        //
    Titlekey,      // f1=rights id LSB f2=rights id MSB
    Source,        // f1=source type, f2= sub id
    Keyblob,       // f1=crypto revision
    KeyblobMAC,    // f1=crypto revision
    TSEC,          //
    SecureBoot,    //
    BIS,           // f1=partition (0-3), f2=type {crypt, tweak}
    HeaderKek,     //
    SDKek,         //
    RSAKek,        //
};

enum class KeyAreaKeyType : u8 {
    Application,
    Ocean,
    System,
};

enum class SourceKeyType : u8 {
    SDKek,                //
    AESKekGeneration,     //
    AESKeyGeneration,     //
    RSAOaepKekGeneration, //
    Master,               //
    Keyblob,              // f2=crypto revision
    KeyAreaKey,           // f2=KeyAreaKeyType
    Titlekek,             //
    Package2,             //
    HeaderKek,            //
    KeyblobMAC,           //
    ETicketKek,           //
    ETicketKekek,         //
};

enum class SDKeyType : u8 {
    Save,
    NCA,
};

enum class BISKeyType : u8 {
    Crypto,
    Tweak,
};

enum class RSAKekType : u8 {
    Mask0,
    Seed3,
};

template <typename KeyType>
struct KeyIndex {
    KeyType type;
    u64 field1;
    u64 field2;

    std::string DebugInfo() const {
        u8 key_size = 16;
        if constexpr (std::is_same_v<KeyType, S256KeyType>)
            key_size = 32;
        return fmt::format("key_size={:02X}, key={:02X}, field1={:016X}, field2={:016X}", key_size,
                           static_cast<u8>(type), field1, field2);
    }
};

// boost flat_map requires operator< for O(log(n)) lookups.
template <typename KeyType>
bool operator<(const KeyIndex<KeyType>& lhs, const KeyIndex<KeyType>& rhs) {
    return std::tie(lhs.type, lhs.field1, lhs.field2) < std::tie(rhs.type, rhs.field1, rhs.field2);
}

class KeyManager {
public:
    static KeyManager& Instance() {
        static KeyManager instance;
        return instance;
    }

    KeyManager(const KeyManager&) = delete;
    KeyManager& operator=(const KeyManager&) = delete;

    KeyManager(KeyManager&&) = delete;
    KeyManager& operator=(KeyManager&&) = delete;

    bool HasKey(S128KeyType id, u64 field1 = 0, u64 field2 = 0) const;
    bool HasKey(S256KeyType id, u64 field1 = 0, u64 field2 = 0) const;

    Key128 GetKey(S128KeyType id, u64 field1 = 0, u64 field2 = 0) const;
    Key256 GetKey(S256KeyType id, u64 field1 = 0, u64 field2 = 0) const;

    Key256 GetBISKey(u8 partition_id) const;

    void SetKey(S128KeyType id, Key128 key, u64 field1 = 0, u64 field2 = 0);
    void SetKey(S256KeyType id, Key256 key, u64 field1 = 0, u64 field2 = 0);

    static bool KeyFileExists(bool title);

    // Call before using the sd seed to attempt to derive it if it doesn't exist. Needs system
    // save 8*43 and the private file to exist.
    void DeriveSDSeedLazy();

    bool BaseDeriveNecessary() const;
    void DeriveBase();
    void DeriveETicket(PartitionDataManager& data, const FileSys::ContentProvider& provider);
    void PopulateTickets();
    void SynthesizeTickets();

    void PopulateFromPartitionData(PartitionDataManager& data);

    const std::map<u128, Ticket>& GetCommonTickets() const;
    const std::map<u128, Ticket>& GetPersonalizedTickets() const;

    bool AddTicketCommon(Ticket raw);
    bool AddTicketPersonalized(Ticket raw);

private:
    KeyManager();

    std::map<KeyIndex<S128KeyType>, Key128> s128_keys;
    std::map<KeyIndex<S256KeyType>, Key256> s256_keys;

    // Map from rights ID to ticket
    std::map<u128, Ticket> common_tickets;
    std::map<u128, Ticket> personal_tickets;

    std::array<std::array<u8, 0xB0>, 0x20> encrypted_keyblobs{};
    std::array<std::array<u8, 0x90>, 0x20> keyblobs{};
    std::array<u8, 576> eticket_extended_kek{};

    bool dev_mode;
    void LoadFromFile(const std::filesystem::path& file_path, bool is_title_keys);

    template <size_t Size>
    void WriteKeyToFile(KeyCategory category, std::string_view keyname,
                        const std::array<u8, Size>& key);

    void DeriveGeneralPurposeKeys(std::size_t crypto_revision);

    RSAKeyPair<2048> GetETicketRSAKey() const;

    void SetKeyWrapped(S128KeyType id, Key128 key, u64 field1 = 0, u64 field2 = 0);
    void SetKeyWrapped(S256KeyType id, Key256 key, u64 field1 = 0, u64 field2 = 0);
};

Key128 GenerateKeyEncryptionKey(Key128 source, Key128 master, Key128 kek_seed, Key128 key_seed);
Key128 DeriveKeyblobKey(const Key128& sbk, const Key128& tsec, Key128 source);
Key128 DeriveKeyblobMACKey(const Key128& keyblob_key, const Key128& mac_source);
Key128 DeriveMasterKey(const std::array<u8, 0x90>& keyblob, const Key128& master_source);
std::array<u8, 0x90> DecryptKeyblob(const std::array<u8, 0xB0>& encrypted_keyblob,
                                    const Key128& key);

std::optional<Key128> DeriveSDSeed();
Loader::ResultStatus DeriveSDKeys(std::array<Key256, 2>& sd_keys, KeyManager& keys);

std::vector<Ticket> GetTicketblob(const Common::FS::IOFile& ticket_save);

// Returns a pair of {rights_id, titlekey}. Fails if the ticket has no certificate authority
// (offset 0x140-0x144 is zero)
std::optional<std::pair<Key128, Key128>> ParseTicket(const Ticket& ticket,
                                                     const RSAKeyPair<2048>& eticket_extended_key);

} // namespace Core::Crypto
