// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"

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

enum class BeardAndMustacheFlag : u32 { Beard = 1, Mustache, All = Beard | Mustache };
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

} // namespace Service::Mii
