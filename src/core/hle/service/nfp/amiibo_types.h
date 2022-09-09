// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "core/hle/service/mii/types.h"

namespace Service::NFP {
static constexpr std::size_t amiibo_name_length = 0xA;

enum class ServiceType : u32 {
    User,
    Debug,
    System,
};

enum class State : u32 {
    NonInitialized,
    Initialized,
};

enum class DeviceState : u32 {
    Initialized,
    SearchingForTag,
    TagFound,
    TagRemoved,
    TagMounted,
    Unaviable,
    Finalized,
};

enum class ModelType : u32 {
    Amiibo,
};

enum class MountTarget : u32 {
    Rom,
    Ram,
    All,
};

enum class AmiiboType : u8 {
    Figure,
    Card,
    Yarn,
};

enum class AmiiboSeries : u8 {
    SuperSmashBros,
    SuperMario,
    ChibiRobo,
    YoshiWoollyWorld,
    Splatoon,
    AnimalCrossing,
    EightBitMario,
    Skylanders,
    Unknown8,
    TheLegendOfZelda,
    ShovelKnight,
    Unknown11,
    Kiby,
    Pokemon,
    MarioSportsSuperstars,
    MonsterHunter,
    BoxBoy,
    Pikmin,
    FireEmblem,
    Metroid,
    Others,
    MegaMan,
    Diablo,
};

using TagUuid = std::array<u8, 10>;
using HashData = std::array<u8, 0x20>;
using ApplicationArea = std::array<u8, 0xD8>;

struct AmiiboDate {
    u16 raw_date{};

    u16 GetYear() const {
        return static_cast<u16>(((raw_date & 0xFE00) >> 9) + 2000);
    }
    u8 GetMonth() const {
        return static_cast<u8>(((raw_date & 0x01E0) >> 5) - 1);
    }
    u8 GetDay() const {
        return static_cast<u8>(raw_date & 0x001F);
    }
};
static_assert(sizeof(AmiiboDate) == 2, "AmiiboDate is an invalid size");

struct Settings {
    union {
        u8 raw{};

        BitField<4, 1, u8> amiibo_initialized;
        BitField<5, 1, u8> appdata_initialized;
    };
};
static_assert(sizeof(Settings) == 1, "AmiiboDate is an invalid size");

struct AmiiboSettings {
    Settings settings;
    u8 country_code_id;
    u16_be crc_counter; // Incremented each time crc is changed
    AmiiboDate init_date;
    AmiiboDate write_date;
    u32_be crc;
    std::array<u16_be, amiibo_name_length> amiibo_name; // UTF-16 text
};
static_assert(sizeof(AmiiboSettings) == 0x20, "AmiiboSettings is an invalid size");

struct AmiiboModelInfo {
    u16 character_id;
    u8 character_variant;
    AmiiboType amiibo_type;
    u16 model_number;
    AmiiboSeries series;
    u8 constant_value;         // Must be 02
    INSERT_PADDING_BYTES(0x4); // Unknown
};
static_assert(sizeof(AmiiboModelInfo) == 0xC, "AmiiboModelInfo is an invalid size");

struct NTAG215Password {
    u32 PWD;  // Password to allow write access
    u16 PACK; // Password acknowledge reply
    u16 RFUI; // Reserved for future use
};
static_assert(sizeof(NTAG215Password) == 0x8, "NTAG215Password is an invalid size");

#pragma pack(1)
struct EncryptedAmiiboFile {
    u8 constant_value;                     // Must be A5
    u16 write_counter;                     // Number of times the amiibo has been written?
    INSERT_PADDING_BYTES(0x1);             // Unknown 1
    AmiiboSettings settings;               // Encrypted amiibo settings
    HashData hmac_tag;                     // Hash
    AmiiboModelInfo model_info;            // Encrypted amiibo model info
    HashData keygen_salt;                  // Salt
    HashData hmac_data;                    // Hash
    Service::Mii::Ver3StoreData owner_mii; // Encrypted Mii data
    u64_be title_id;                       // Encrypted Game id
    u16_be applicaton_write_counter;       // Encrypted Counter
    u32_be application_area_id;            // Encrypted Game id
    std::array<u8, 0x2> unknown;
    HashData hash;                    // Probably a SHA256-HMAC hash?
    ApplicationArea application_area; // Encrypted Game data
};
static_assert(sizeof(EncryptedAmiiboFile) == 0x1F8, "AmiiboFile is an invalid size");

struct NTAG215File {
    std::array<u8, 0x2> uuid2;
    u16 static_lock;           // Set defined pages as read only
    u32 compability_container; // Defines available memory
    HashData hmac_data;        // Hash
    u8 constant_value;         // Must be A5
    u16 write_counter;         // Number of times the amiibo has been written?
    INSERT_PADDING_BYTES(0x1); // Unknown 1
    AmiiboSettings settings;
    Service::Mii::Ver3StoreData owner_mii; // Encrypted Mii data
    u64_be title_id;
    u16_be applicaton_write_counter; // Encrypted Counter
    u32_be application_area_id;
    std::array<u8, 0x2> unknown;
    HashData hash;                    // Probably a SHA256-HMAC hash?
    ApplicationArea application_area; // Encrypted Game data
    HashData hmac_tag;                // Hash
    std::array<u8, 0x8> uuid;
    AmiiboModelInfo model_info;
    HashData keygen_salt;     // Salt
    u32 dynamic_lock;         // Dynamic lock
    u32 CFG0;                 // Defines memory protected by password
    u32 CFG1;                 // Defines number of verification attempts
    NTAG215Password password; // Password data
};
static_assert(sizeof(NTAG215File) == 0x21C, "NTAG215File is an invalid size");
static_assert(std::is_trivially_copyable_v<NTAG215File>, "NTAG215File must be trivially copyable.");
#pragma pack()

struct EncryptedNTAG215File {
    TagUuid uuid;                    // Unique serial number
    u16 static_lock;                 // Set defined pages as read only
    u32 compability_container;       // Defines available memory
    EncryptedAmiiboFile user_memory; // Writable data
    u32 dynamic_lock;                // Dynamic lock
    u32 CFG0;                        // Defines memory protected by password
    u32 CFG1;                        // Defines number of verification attempts
    NTAG215Password password;        // Password data
};
static_assert(sizeof(EncryptedNTAG215File) == 0x21C, "EncryptedNTAG215File is an invalid size");
static_assert(std::is_trivially_copyable_v<EncryptedNTAG215File>,
              "EncryptedNTAG215File must be trivially copyable.");

} // namespace Service::NFP
