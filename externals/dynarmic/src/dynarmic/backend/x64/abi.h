/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */
#pragma once

#include <array>

#include <mcl/stdint.hpp>

#include "dynarmic/backend/x64/hostloc.h"

namespace Dynarmic::Backend::X64 {

class BlockOfCode;

#ifdef _WIN32

constexpr HostLoc ABI_RETURN = HostLoc::RAX;

constexpr size_t ABI_PARAM_COUNT = 4;

constexpr HostLoc ABI_PARAM1 = HostLoc::RCX;
constexpr HostLoc ABI_PARAM2 = HostLoc::RDX;
constexpr HostLoc ABI_PARAM3 = HostLoc::R8;
constexpr HostLoc ABI_PARAM4 = HostLoc::R9;

constexpr std::array<HostLoc, 13> ABI_ALL_CALLER_SAVE = {
    HostLoc::RAX,
    HostLoc::RCX,
    HostLoc::RDX,
    HostLoc::R8,
    HostLoc::R9,
    HostLoc::R10,
    HostLoc::R11,
    HostLoc::XMM0,
    HostLoc::XMM1,
    HostLoc::XMM2,
    HostLoc::XMM3,
    HostLoc::XMM4,
    HostLoc::XMM5,
};

constexpr std::array<HostLoc, 18> ABI_ALL_CALLEE_SAVE = {
    HostLoc::RBX,
    HostLoc::RSI,
    HostLoc::RDI,
    HostLoc::RBP,
    HostLoc::R12,
    HostLoc::R13,
    HostLoc::R14,
    HostLoc::R15,
    HostLoc::XMM6,
    HostLoc::XMM7,
    HostLoc::XMM8,
    HostLoc::XMM9,
    HostLoc::XMM10,
    HostLoc::XMM11,
    HostLoc::XMM12,
    HostLoc::XMM13,
    HostLoc::XMM14,
    HostLoc::XMM15,
};

constexpr size_t ABI_SHADOW_SPACE = 32;  // bytes

#else

constexpr HostLoc ABI_RETURN = HostLoc::RAX;
constexpr HostLoc ABI_RETURN2 = HostLoc::RDX;

constexpr size_t ABI_PARAM_COUNT = 6;

constexpr HostLoc ABI_PARAM1 = HostLoc::RDI;
constexpr HostLoc ABI_PARAM2 = HostLoc::RSI;
constexpr HostLoc ABI_PARAM3 = HostLoc::RDX;
constexpr HostLoc ABI_PARAM4 = HostLoc::RCX;
constexpr HostLoc ABI_PARAM5 = HostLoc::R8;
constexpr HostLoc ABI_PARAM6 = HostLoc::R9;

constexpr std::array<HostLoc, 25> ABI_ALL_CALLER_SAVE = {
    HostLoc::RAX,
    HostLoc::RCX,
    HostLoc::RDX,
    HostLoc::RDI,
    HostLoc::RSI,
    HostLoc::R8,
    HostLoc::R9,
    HostLoc::R10,
    HostLoc::R11,
    HostLoc::XMM0,
    HostLoc::XMM1,
    HostLoc::XMM2,
    HostLoc::XMM3,
    HostLoc::XMM4,
    HostLoc::XMM5,
    HostLoc::XMM6,
    HostLoc::XMM7,
    HostLoc::XMM8,
    HostLoc::XMM9,
    HostLoc::XMM10,
    HostLoc::XMM11,
    HostLoc::XMM12,
    HostLoc::XMM13,
    HostLoc::XMM14,
    HostLoc::XMM15,
};

constexpr std::array<HostLoc, 6> ABI_ALL_CALLEE_SAVE = {
    HostLoc::RBX,
    HostLoc::RBP,
    HostLoc::R12,
    HostLoc::R13,
    HostLoc::R14,
    HostLoc::R15,
};

constexpr size_t ABI_SHADOW_SPACE = 0;  // bytes

#endif

static_assert(ABI_ALL_CALLER_SAVE.size() + ABI_ALL_CALLEE_SAVE.size() == 31, "Invalid total number of registers");

void ABI_PushCalleeSaveRegistersAndAdjustStack(BlockOfCode& code, size_t frame_size = 0);
void ABI_PopCalleeSaveRegistersAndAdjustStack(BlockOfCode& code, size_t frame_size = 0);
void ABI_PushCallerSaveRegistersAndAdjustStack(BlockOfCode& code, size_t frame_size = 0);
void ABI_PopCallerSaveRegistersAndAdjustStack(BlockOfCode& code, size_t frame_size = 0);

void ABI_PushCallerSaveRegistersAndAdjustStackExcept(BlockOfCode& code, HostLoc exception);
void ABI_PopCallerSaveRegistersAndAdjustStackExcept(BlockOfCode& code, HostLoc exception);

}  // namespace Dynarmic::Backend::X64
