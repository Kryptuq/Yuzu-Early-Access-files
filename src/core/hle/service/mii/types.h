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

struct MiiInfo {
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
static_assert(sizeof(MiiInfo) == 0x58, "MiiInfo has incorrect size.");
static_assert(std::has_unique_object_representations_v<MiiInfo>,
              "All bits of MiiInfo must contribute to its value.");

#pragma pack(push, 4)

struct MiiInfoElement {
    MiiInfoElement(const MiiInfo& info_, Source source_) : info{info_}, source{source_} {}

    MiiInfo info{};
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
