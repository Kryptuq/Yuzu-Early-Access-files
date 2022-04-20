/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>

#include <mcl/stdint.hpp>

#include "dynarmic/backend/x64/nzcv_util.h"
#include "dynarmic/frontend/A64/a64_location_descriptor.h"

namespace Dynarmic::Backend::X64 {

class BlockOfCode;

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4324)  // Structure was padded due to alignment specifier
#endif

struct A64JitState {
    using ProgramCounterType = u64;

    A64JitState() { ResetRSB(); }

    std::array<u64, 31> reg{};
    u64 sp = 0;
    u64 pc = 0;

    u32 cpsr_nzcv = 0;

    u32 GetPstate() const {
        return NZCV::FromX64(cpsr_nzcv);
    }
    void SetPstate(u32 new_pstate) {
        cpsr_nzcv = NZCV::ToX64(new_pstate);
    }

    alignas(16) std::array<u64, 64> vec{};  // Extension registers.

    // For internal use (See: BlockOfCode::RunCode)
    u32 guest_MXCSR = 0x00001f80;
    u32 asimd_MXCSR = 0x00009fc0;
    volatile u32 halt_reason = 0;

    // Exclusive state
    static constexpr u64 RESERVATION_GRANULE_MASK = 0xFFFF'FFFF'FFFF'FFF0ull;
    u8 exclusive_state = 0;

    static constexpr size_t RSBSize = 8;  // MUST be a power of 2.
    static constexpr size_t RSBPtrMask = RSBSize - 1;
    u32 rsb_ptr = 0;
    std::array<u64, RSBSize> rsb_location_descriptors;
    std::array<u64, RSBSize> rsb_codeptrs;
    void ResetRSB() {
        rsb_location_descriptors.fill(0xFFFFFFFFFFFFFFFFull);
        rsb_codeptrs.fill(0);
    }

    u32 fpsr_exc = 0;
    u32 fpsr_qc = 0;
    u32 fpcr = 0;
    u32 GetFpcr() const;
    u32 GetFpsr() const;
    void SetFpcr(u32 value);
    void SetFpsr(u32 value);

    u64 GetUniqueHash() const noexcept {
        const u64 fpcr_u64 = static_cast<u64>(fpcr & A64::LocationDescriptor::fpcr_mask) << A64::LocationDescriptor::fpcr_shift;
        const u64 pc_u64 = pc & A64::LocationDescriptor::pc_mask;
        return pc_u64 | fpcr_u64;
    }
};

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

using CodePtr = const void*;

}  // namespace Dynarmic::Backend::X64
