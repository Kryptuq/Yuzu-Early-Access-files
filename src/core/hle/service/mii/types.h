// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <type_traits>

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/uuid.h"

namespace Service::Mii {

enum class Age : u32 {
    Young,
    Normal,
    Old,
    All,
};

enum class BeardType : u32 {
    None,
    Beard1,
    Beard2,
    Beard3,
    Beard4,
    Beard5,
};

enum class BeardAndMustacheFlag : u32 {
    Beard = 1,
    Mustache,
    All = Beard | Mustache,
};
DECLARE_ENUM_FLAG_OPERATORS(BeardAndMustacheFlag);

enum class FontRegion : u32 {
    Standard,
    China,
    Korea,
    Taiwan,
};

enum class Gender : u32 {
    Male,
    Female,
    All,
    Maximum = Female,
};

enum class HairFlip : u32 {
    Left,
    Right,
    Maximum = Right,
};

enum class MustacheType : u32 {
    None,
    Mustache1,
    Mustache2,
    Mustache3,
    Mustache4,
    Mustache5,
};

enum class Race : u32 {
    Black,
    White,
    Asian,
    All,
};

enum class Source : u32 {
    Database = 0,
    Default = 1,
    Account = 2,
    Friend = 3,
};

enum class SourceFlag : u32 {
    None = 0,
    Database = 1 << 0,
    Default = 1 << 1,
};
DECLARE_ENUM_FLAG_OPERATORS(SourceFlag);

// nn::mii::CharInfo
struct CharInfo {
    Common::UUID uuid;
    std::array<char16_t, 11> name;
    u8 font_region;
    u8 favorite_color;
    u8 gender;
    u8 height;
    u8 build;
    u8 type;
    u8 region_move;
    u8 faceline_type;
    u8 faceline_color;
    u8 faceline_wrinkle;
    u8 faceline_make;
    u8 hair_type;
    u8 hair_color;
    u8 hair_flip;
    u8 eye_type;
    u8 eye_color;
    u8 eye_scale;
    u8 eye_aspect;
    u8 eye_rotate;
    u8 eye_x;
    u8 eye_y;
    u8 eyebrow_type;
    u8 eyebrow_color;
    u8 eyebrow_scale;
    u8 eyebrow_aspect;
    u8 eyebrow_rotate;
    u8 eyebrow_x;
    u8 eyebrow_y;
    u8 nose_type;
    u8 nose_scale;
    u8 nose_y;
    u8 mouth_type;
    u8 mouth_color;
    u8 mouth_scale;
    u8 mouth_aspect;
    u8 mouth_y;
    u8 beard_color;
    u8 beard_type;
    u8 mustache_type;
    u8 mustache_scale;
    u8 mustache_y;
    u8 glasses_type;
    u8 glasses_color;
    u8 glasses_scale;
    u8 glasses_y;
    u8 mole_type;
    u8 mole_scale;
    u8 mole_x;
    u8 mole_y;
    u8 padding;
};
static_assert(sizeof(CharInfo) == 0x58, "CharInfo has incorrect size.");
static_assert(std::has_unique_object_representations_v<CharInfo>,
              "All bits of CharInfo must contribute to its value.");

#pragma pack(push, 4)

struct MiiInfoElement {
    MiiInfoElement(const CharInfo& info_, Source source_) : info{info_}, source{source_} {}

    CharInfo info{};
    Source source{};
};
static_assert(sizeof(MiiInfoElement) == 0x5c, "MiiInfoElement has incorrect size.");

struct MiiStoreBitFields {
    union {
        u32 word_0{};

        BitField<0, 8, u32> hair_type;
        BitField<8, 7, u32> height;
        BitField<15, 1, u32> mole_type;
        BitField<16, 7, u32> build;
        BitField<23, 1, HairFlip> hair_flip;
        BitField<24, 7, u32> hair_color;
        BitField<31, 1, u32> type;
    };

    union {
        u32 word_1{};

        BitField<0, 7, u32> eye_color;
        BitField<7, 1, Gender> gender;
        BitField<8, 7, u32> eyebrow_color;
        BitField<16, 7, u32> mouth_color;
        BitField<24, 7, u32> beard_color;
    };

    union {
        u32 word_2{};

        BitField<0, 7, u32> glasses_color;
        BitField<8, 6, u32> eye_type;
        BitField<14, 2, u32> region_move;
        BitField<16, 6, u32> mouth_type;
        BitField<22, 2, FontRegion> font_region;
        BitField<24, 5, u32> eye_y;
        BitField<29, 3, u32> glasses_scale;
    };

    union {
        u32 word_3{};

        BitField<0, 5, u32> eyebrow_type;
        BitField<5, 3, MustacheType> mustache_type;
        BitField<8, 5, u32> nose_type;
        BitField<13, 3, BeardType> beard_type;
        BitField<16, 5, u32> nose_y;
        BitField<21, 3, u32> mouth_aspect;
        BitField<24, 5, u32> mouth_y;
        BitField<29, 3, u32> eyebrow_aspect;
    };

    union {
        u32 word_4{};

        BitField<0, 5, u32> mustache_y;
        BitField<5, 3, u32> eye_rotate;
        BitField<8, 5, u32> glasses_y;
        BitField<13, 3, u32> eye_aspect;
        BitField<16, 5, u32> mole_x;
        BitField<21, 3, u32> eye_scale;
        BitField<24, 5, u32> mole_y;
    };

    union {
        u32 word_5{};

        BitField<0, 5, u32> glasses_type;
        BitField<8, 4, u32> favorite_color;
        BitField<12, 4, u32> faceline_type;
        BitField<16, 4, u32> faceline_color;
        BitField<20, 4, u32> faceline_wrinkle;
        BitField<24, 4, u32> faceline_makeup;
        BitField<28, 4, u32> eye_x;
    };

    union {
        u32 word_6{};

        BitField<0, 4, u32> eyebrow_scale;
        BitField<4, 4, u32> eyebrow_rotate;
        BitField<8, 4, u32> eyebrow_x;
        BitField<12, 4, u32> eyebrow_y;
        BitField<16, 4, u32> nose_scale;
        BitField<20, 4, u32> mouth_scale;
        BitField<24, 4, u32> mustache_scale;
        BitField<28, 4, u32> mole_scale;
    };
};
static_assert(sizeof(MiiStoreBitFields) == 0x1c, "MiiStoreBitFields has incorrect size.");
static_assert(std::is_trivially_copyable_v<MiiStoreBitFields>,
              "MiiStoreBitFields is not trivially copyable.");

// This is nn::mii::Ver3StoreData
// Based on citra HLE::Applets::MiiData and PretendoNetwork.
// https://github.com/citra-emu/citra/blob/master/src/core/hle/applets/mii_selector.h#L48
// https://github.com/PretendoNetwork/mii-js/blob/master/mii.js#L299
struct Ver3StoreData {
    u8 version;
    union {
        u8 raw;

        BitField<0, 1, u8> allow_copying;
        BitField<1, 1, u8> profanity_flag;
        BitField<2, 2, u8> region_lock;
        BitField<4, 2, u8> character_set;
    } region_information;
    u16_be mii_id;
    u64_be system_id;
    u32_be specialness_and_creation_date;
    std::array<u8, 0x6> creator_mac;
    u16_be padding;
    union {
        u16 raw;

        BitField<0, 1, u16> gender;
        BitField<1, 4, u16> birth_month;
        BitField<5, 5, u16> birth_day;
        BitField<10, 4, u16> favorite_color;
        BitField<14, 1, u16> favorite;
    } mii_information;
    std::array<char16_t, 0xA> mii_name;
    u8 height;
    u8 build;
    union {
        u8 raw;

        BitField<0, 1, u8> disable_sharing;
        BitField<1, 4, u8> face_shape;
        BitField<5, 3, u8> skin_color;
    } appearance_bits1;
    union {
        u8 raw;

        BitField<0, 4, u8> wrinkles;
        BitField<4, 4, u8> makeup;
    } appearance_bits2;
    u8 hair_style;
    union {
        u8 raw;

        BitField<0, 3, u8> hair_color;
        BitField<3, 1, u8> flip_hair;
    } appearance_bits3;
    union {
        u32 raw;

        BitField<0, 6, u32> eye_type;
        BitField<6, 3, u32> eye_color;
        BitField<9, 4, u32> eye_scale;
        BitField<13, 3, u32> eye_vertical_stretch;
        BitField<16, 5, u32> eye_rotation;
        BitField<21, 4, u32> eye_spacing;
        BitField<25, 5, u32> eye_y_position;
    } appearance_bits4;
    union {
        u32 raw;

        BitField<0, 5, u32> eyebrow_style;
        BitField<5, 3, u32> eyebrow_color;
        BitField<8, 4, u32> eyebrow_scale;
        BitField<12, 3, u32> eyebrow_yscale;
        BitField<16, 4, u32> eyebrow_rotation;
        BitField<21, 4, u32> eyebrow_spacing;
        BitField<25, 5, u32> eyebrow_y_position;
    } appearance_bits5;
    union {
        u16 raw;

        BitField<0, 5, u16> nose_type;
        BitField<5, 4, u16> nose_scale;
        BitField<9, 5, u16> nose_y_position;
    } appearance_bits6;
    union {
        u16 raw;

        BitField<0, 6, u16> mouth_type;
        BitField<6, 3, u16> mouth_color;
        BitField<9, 4, u16> mouth_scale;
        BitField<13, 3, u16> mouth_horizontal_stretch;
    } appearance_bits7;
    union {
        u8 raw;

        BitField<0, 5, u8> mouth_y_position;
        BitField<5, 3, u8> mustache_type;
    } appearance_bits8;
    u8 allow_copying;
    union {
        u16 raw;

        BitField<0, 3, u16> bear_type;
        BitField<3, 3, u16> facial_hair_color;
        BitField<6, 4, u16> mustache_scale;
        BitField<10, 5, u16> mustache_y_position;
    } appearance_bits9;
    union {
        u16 raw;

        BitField<0, 4, u16> glasses_type;
        BitField<4, 3, u16> glasses_color;
        BitField<7, 4, u16> glasses_scale;
        BitField<11, 5, u16> glasses_y_position;
    } appearance_bits10;
    union {
        u16 raw;

        BitField<0, 1, u16> mole_enabled;
        BitField<1, 4, u16> mole_scale;
        BitField<5, 5, u16> mole_x_position;
        BitField<10, 5, u16> mole_y_position;
    } appearance_bits11;

    std::array<u16_le, 0xA> author_name;
    INSERT_PADDING_BYTES(0x2);
    u16_be crc;
};
static_assert(sizeof(Ver3StoreData) == 0x60, "Ver3StoreData is an invalid size");

struct NfpStoreDataExtension {
    u8 faceline_color;
    u8 hair_color;
    u8 eye_color;
    u8 eyebrow_color;
    u8 mouth_color;
    u8 beard_color;
    u8 glass_color;
    u8 glass_type;
};
static_assert(sizeof(NfpStoreDataExtension) == 0x8, "NfpStoreDataExtension is an invalid size");

constexpr std::array<u8, 0x10> Ver3FacelineColorTable{
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x0, 0x1, 0x5, 0x5,
};

constexpr std::array<u8, 100> Ver3HairColorTable{
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x0, 0x4, 0x3, 0x5, 0x4, 0x4, 0x6, 0x2, 0x0,
    0x6, 0x4, 0x3, 0x2, 0x2, 0x7, 0x3, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2,
    0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x0, 0x0, 0x4,
    0x4, 0x4, 0x4, 0x4, 0x4, 0x0, 0x0, 0x0, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x5, 0x5, 0x5,
    0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x5, 0x7, 0x5, 0x7, 0x7, 0x7, 0x7, 0x7, 0x6, 0x7,
    0x7, 0x7, 0x7, 0x7, 0x3, 0x7, 0x7, 0x7, 0x7, 0x7, 0x0, 0x4, 0x4, 0x4, 0x4,
};

constexpr std::array<u8, 100> Ver3EyeColorTable{
    0x0, 0x2, 0x2, 0x2, 0x1, 0x3, 0x2, 0x3, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x2, 0x2, 0x4,
    0x2, 0x1, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2,
    0x2, 0x2, 0x2, 0x2, 0x0, 0x0, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x1, 0x0, 0x4, 0x4,
    0x4, 0x4, 0x4, 0x4, 0x4, 0x0, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5,
    0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x2, 0x2,
    0x3, 0x3, 0x3, 0x3, 0x2, 0x2, 0x2, 0x2, 0x2, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
};

constexpr std::array<u8, 100> Ver3MouthlineColorTable{
    0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x3, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x1, 0x4,
    0x4, 0x4, 0x0, 0x1, 0x2, 0x3, 0x4, 0x4, 0x2, 0x3, 0x3, 0x4, 0x4, 0x4, 0x4, 0x1, 0x4,
    0x4, 0x2, 0x3, 0x3, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x3, 0x3, 0x3, 0x4, 0x4, 0x4,
    0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x4, 0x4, 0x4, 0x4, 0x3, 0x3, 0x3, 0x3, 0x4, 0x4, 0x4,
    0x4, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x4, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3,
    0x3, 0x3, 0x3, 0x3, 0x4, 0x0, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3, 0x3, 0x3,
};

constexpr std::array<u8, 100> Ver3GlassColorTable{
    0x0, 0x1, 0x1, 0x1, 0x5, 0x1, 0x1, 0x4, 0x0, 0x5, 0x1, 0x1, 0x3, 0x5, 0x1, 0x2, 0x3,
    0x4, 0x5, 0x4, 0x2, 0x2, 0x4, 0x4, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2,
    0x2, 0x2, 0x2, 0x2, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x3, 0x3, 0x3, 0x3, 0x3, 0x0, 0x0, 0x0, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x0, 0x5, 0x5,
    0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x1, 0x4,
    0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5,
};

constexpr std::array<u8, 20> Ver3GlassTypeTable{
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x1,
    0x2, 0x1, 0x3, 0x7, 0x7, 0x6, 0x7, 0x8, 0x7, 0x7,
};

struct MiiStoreData {
    using Name = std::array<char16_t, 10>;

    MiiStoreData();
    MiiStoreData(const Name& name, const MiiStoreBitFields& bit_fields,
                 const Common::UUID& user_id);

    // This corresponds to the above structure MiiStoreBitFields. I did it like this because the
    // BitField<> type makes this (and any thing that contains it) not trivially copyable, which is
    // not suitable for our uses.
    struct {
        std::array<u8, 0x1C> data{};
        static_assert(sizeof(MiiStoreBitFields) == sizeof(data), "data field has incorrect size.");

        Name name{};
        Common::UUID uuid{};
    } data;

    u16 data_crc{};
    u16 device_crc{};
};
static_assert(sizeof(MiiStoreData) == 0x44, "MiiStoreData has incorrect size.");

struct MiiStoreDataElement {
    MiiStoreData data{};
    Source source{};
};
static_assert(sizeof(MiiStoreDataElement) == 0x48, "MiiStoreDataElement has incorrect size.");

struct MiiDatabase {
    u32 magic{}; // 'NFDB'
    std::array<MiiStoreData, 0x64> miis{};
    INSERT_PADDING_BYTES(1);
    u8 count{};
    u16 crc{};
};
static_assert(sizeof(MiiDatabase) == 0x1A98, "MiiDatabase has incorrect size.");

struct RandomMiiValues {
    std::array<u8, 0xbc> values{};
};
static_assert(sizeof(RandomMiiValues) == 0xbc, "RandomMiiValues has incorrect size.");

struct RandomMiiData4 {
    Gender gender{};
    Age age{};
    Race race{};
    u32 values_count{};
    std::array<u32, 47> values{};
};
static_assert(sizeof(RandomMiiData4) == 0xcc, "RandomMiiData4 has incorrect size.");

struct RandomMiiData3 {
    u32 arg_1;
    u32 arg_2;
    u32 values_count;
    std::array<u32, 47> values{};
};
static_assert(sizeof(RandomMiiData3) == 0xc8, "RandomMiiData3 has incorrect size.");

struct RandomMiiData2 {
    u32 arg_1;
    u32 values_count;
    std::array<u32, 47> values{};
};
static_assert(sizeof(RandomMiiData2) == 0xc4, "RandomMiiData2 has incorrect size.");

struct DefaultMii {
    u32 face_type{};
    u32 face_color{};
    u32 face_wrinkle{};
    u32 face_makeup{};
    u32 hair_type{};
    u32 hair_color{};
    u32 hair_flip{};
    u32 eye_type{};
    u32 eye_color{};
    u32 eye_scale{};
    u32 eye_aspect{};
    u32 eye_rotate{};
    u32 eye_x{};
    u32 eye_y{};
    u32 eyebrow_type{};
    u32 eyebrow_color{};
    u32 eyebrow_scale{};
    u32 eyebrow_aspect{};
    u32 eyebrow_rotate{};
    u32 eyebrow_x{};
    u32 eyebrow_y{};
    u32 nose_type{};
    u32 nose_scale{};
    u32 nose_y{};
    u32 mouth_type{};
    u32 mouth_color{};
    u32 mouth_scale{};
    u32 mouth_aspect{};
    u32 mouth_y{};
    u32 mustache_type{};
    u32 beard_type{};
    u32 beard_color{};
    u32 mustache_scale{};
    u32 mustache_y{};
    u32 glasses_type{};
    u32 glasses_color{};
    u32 glasses_scale{};
    u32 glasses_y{};
    u32 mole_type{};
    u32 mole_scale{};
    u32 mole_x{};
    u32 mole_y{};
    u32 height{};
    u32 weight{};
    Gender gender{};
    u32 favorite_color{};
    u32 region{};
    FontRegion font_region{};
    u32 type{};
    INSERT_PADDING_WORDS(5);
};
static_assert(sizeof(DefaultMii) == 0xd8, "MiiStoreData has incorrect size.");

#pragma pack(pop)

} // namespace Service::Mii
