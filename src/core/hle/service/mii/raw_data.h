// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "core/hle/service/mii/types.h"

namespace Service::Mii::RawData {

extern const std::array<Service::Mii::DefaultMii, 8> DefaultMii;
extern const std::array<Service::Mii::RandomMiiData4, 18> RandomMiiFaceline;
extern const std::array<Service::Mii::RandomMiiData3, 6> RandomMiiFacelineColor;
extern const std::array<Service::Mii::RandomMiiData4, 18> RandomMiiFacelineWrinkle;
extern const std::array<Service::Mii::RandomMiiData4, 18> RandomMiiFacelineMakeup;
extern const std::array<Service::Mii::RandomMiiData4, 18> RandomMiiHairType;
extern const std::array<Service::Mii::RandomMiiData3, 9> RandomMiiHairColor;
extern const std::array<Service::Mii::RandomMiiData4, 18> RandomMiiEyeType;
extern const std::array<Service::Mii::RandomMiiData2, 3> RandomMiiEyeColor;
extern const std::array<Service::Mii::RandomMiiData4, 18> RandomMiiEyebrowType;
extern const std::array<Service::Mii::RandomMiiData4, 18> RandomMiiNoseType;
extern const std::array<Service::Mii::RandomMiiData4, 18> RandomMiiMouthType;
extern const std::array<Service::Mii::RandomMiiData2, 3> RandomMiiGlassType;

} // namespace Service::Mii::RawData
