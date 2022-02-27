/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/x64/a32_jitstate.h"

#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/nzcv_util.h"
#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/frontend/A32/a32_location_descriptor.h"

namespace Dynarmic::Backend::X64 {

/**
 * CPSR Bits
 * =========
 *
 * ARM CPSR flags
 * --------------
 * N        bit 31       Negative flag
 * Z        bit 30       Zero flag
 * C        bit 29       Carry flag
 * V        bit 28       oVerflow flag
 * Q        bit 27       Saturation flag
 * IT[1:0]  bits 25-26   If-Then execution state (lower 2 bits)
 * J        bit 24       Jazelle instruction set flag
 * GE       bits 16-19   Greater than or Equal flags
 * IT[7:2]  bits 10-15   If-Then execution state (upper 6 bits)
 * E        bit 9        Data Endianness flag
 * A        bit 8        Disable imprecise Aborts
 * I        bit 7        Disable IRQ interrupts
 * F        bit 6        Disable FIQ interrupts
 * T        bit 5        Thumb instruction set flag
 * M        bits 0-4     Processor Mode bits
 *
 * x64 LAHF+SETO flags
 * -------------------
 * SF   bit 15       Sign flag
 * ZF   bit 14       Zero flag
 * AF   bit 12       Auxiliary flag
 * PF   bit 10       Parity flag
 * CF   bit 8        Carry flag
 * OF   bit 0        Overflow flag
 */

u32 A32JitState::Cpsr() const {
    DEBUG_ASSERT((cpsr_q & ~1) == 0);
    DEBUG_ASSERT((cpsr_jaifm & ~0x010001DF) == 0);

    u32 cpsr = 0;

    // NZCV flags
    cpsr |= NZCV::FromX64(cpsr_nzcv);
    // Q flag
    cpsr |= cpsr_q ? 1 << 27 : 0;
    // GE flags
    cpsr |= Common::Bit<31>(cpsr_ge) ? 1 << 19 : 0;
    cpsr |= Common::Bit<23>(cpsr_ge) ? 1 << 18 : 0;
    cpsr |= Common::Bit<15>(cpsr_ge) ? 1 << 17 : 0;
    cpsr |= Common::Bit<7>(cpsr_ge) ? 1 << 16 : 0;
    // E flag, T flag
    cpsr |= Common::Bit<1>(upper_location_descriptor) ? 1 << 9 : 0;
    cpsr |= Common::Bit<0>(upper_location_descriptor) ? 1 << 5 : 0;
    // IT state
    cpsr |= static_cast<u32>(upper_location_descriptor & 0b11111100'00000000);
    cpsr |= static_cast<u32>(upper_location_descriptor & 0b00000011'00000000) << 17;
    // Other flags
    cpsr |= cpsr_jaifm;

    return cpsr;
}

void A32JitState::SetCpsr(u32 cpsr) {
    // NZCV flags
    cpsr_nzcv = NZCV::ToX64(cpsr);
    // Q flag
    cpsr_q = Common::Bit<27>(cpsr) ? 1 : 0;
    // GE flags
    cpsr_ge = 0;
    cpsr_ge |= Common::Bit<19>(cpsr) ? 0xFF000000 : 0;
    cpsr_ge |= Common::Bit<18>(cpsr) ? 0x00FF0000 : 0;
    cpsr_ge |= Common::Bit<17>(cpsr) ? 0x0000FF00 : 0;
    cpsr_ge |= Common::Bit<16>(cpsr) ? 0x000000FF : 0;

    upper_location_descriptor &= 0xFFFF0000;
    // E flag, T flag
    upper_location_descriptor |= Common::Bit<9>(cpsr) ? 2 : 0;
    upper_location_descriptor |= Common::Bit<5>(cpsr) ? 1 : 0;
    // IT state
    upper_location_descriptor |= (cpsr >> 0) & 0b11111100'00000000;
    upper_location_descriptor |= (cpsr >> 17) & 0b00000011'00000000;

    // Other flags
    cpsr_jaifm = cpsr & 0x010001DF;
}

void A32JitState::ResetRSB() {
    rsb_location_descriptors.fill(0xFFFFFFFFFFFFFFFFull);
    rsb_codeptrs.fill(0);
}

/**
 * Comparing MXCSR and FPSCR
 * =========================
 *
 * SSE MXCSR exception flags
 * -------------------------
 * PE   bit 5   Precision Flag
 * UE   bit 4   Underflow Flag
 * OE   bit 3   Overflow Flag
 * ZE   bit 2   Divide By Zero Flag
 * DE   bit 1   Denormal Flag                                 // Appears to only be set when MXCSR.DAZ = 0
 * IE   bit 0   Invalid Operation Flag
 *
 * VFP FPSCR cumulative exception bits
 * -----------------------------------
 * IDC  bit 7   Input Denormal cumulative exception bit       // Only ever set when FPSCR.FTZ = 1
 * IXC  bit 4   Inexact cumulative exception bit
 * UFC  bit 3   Underflow cumulative exception bit
 * OFC  bit 2   Overflow cumulative exception bit
 * DZC  bit 1   Division by Zero cumulative exception bit
 * IOC  bit 0   Invalid Operation cumulative exception bit
 *
 * SSE MSCSR exception masks
 * -------------------------
 * PM   bit 12  Precision Mask
 * UM   bit 11  Underflow Mask
 * OM   bit 10  Overflow Mask
 * ZM   bit 9   Divide By Zero Mask
 * DM   bit 8   Denormal Mask
 * IM   bit 7   Invalid Operation Mask
 *
 * VFP FPSCR exception trap enables
 * --------------------------------
 * IDE  bit 15  Input Denormal exception trap enable
 * IXE  bit 12  Inexact exception trap enable
 * UFE  bit 11  Underflow exception trap enable
 * OFE  bit 10  Overflow exception trap enable
 * DZE  bit 9   Division by Zero exception trap enable
 * IOE  bit 8   Invalid Operation exception trap enable
 *
 * SSE MXCSR mode bits
 * -------------------
 * FZ   bit 15      Flush To Zero
 * DAZ  bit 6       Denormals Are Zero
 * RN   bits 13-14  Round to {0 = Nearest, 1 = Negative, 2 = Positive, 3 = Zero}
 *
 * VFP FPSCR mode bits
 * -------------------
 * AHP      bit 26      Alternate half-precision
 * DN       bit 25      Default NaN
 * FZ       bit 24      Flush to Zero
 * RMode    bits 22-23  Round to {0 = Nearest, 1 = Positive, 2 = Negative, 3 = Zero}
 * Stride   bits 20-21  Vector stride
 * Len      bits 16-18  Vector length
 */

// NZCV; QC (ASIMD only), AHP; DN, FZ, RMode, Stride; SBZP; Len; trap enables; cumulative bits
constexpr u32 FPSCR_MODE_MASK = A32::LocationDescriptor::FPSCR_MODE_MASK;
constexpr u32 FPSCR_NZCV_MASK = 0xF0000000;

u32 A32JitState::Fpscr() const {
    DEBUG_ASSERT((fpsr_nzcv & ~FPSCR_NZCV_MASK) == 0);

    const u32 fpcr_mode = static_cast<u32>(upper_location_descriptor) & FPSCR_MODE_MASK;
    const u32 mxcsr = guest_MXCSR | asimd_MXCSR;

    u32 FPSCR = fpcr_mode | fpsr_nzcv;
    FPSCR |= (mxcsr & 0b0000000000001);       // IOC = IE
    FPSCR |= (mxcsr & 0b0000000111100) >> 1;  // IXC, UFC, OFC, DZC = PE, UE, OE, ZE
    FPSCR |= fpsr_exc;
    FPSCR |= fpsr_qc != 0 ? 1 << 27 : 0;

    return FPSCR;
}

void A32JitState::SetFpscr(u32 FPSCR) {
    // Ensure that only upper half of upper_location_descriptor is used for FPSCR bits.
    static_assert((FPSCR_MODE_MASK & 0xFFFF0000) == FPSCR_MODE_MASK);

    upper_location_descriptor &= 0x0000FFFF;
    upper_location_descriptor |= FPSCR & FPSCR_MODE_MASK;

    fpsr_nzcv = FPSCR & FPSCR_NZCV_MASK;
    fpsr_qc = (FPSCR >> 27) & 1;

    guest_MXCSR = 0x00001f80;
    asimd_MXCSR = 0x00009fc0;

    // RMode
    const std::array<u32, 4> MXCSR_RMode{0x0, 0x4000, 0x2000, 0x6000};
    guest_MXCSR |= MXCSR_RMode[(FPSCR >> 22) & 0x3];

    // Cumulative flags IDC, IOC, IXC, UFC, OFC, DZC
    fpsr_exc = FPSCR & 0x9F;

    if (Common::Bit<24>(FPSCR)) {
        // VFP Flush to Zero
        guest_MXCSR |= (1 << 15);  // SSE Flush to Zero
        guest_MXCSR |= (1 << 6);   // SSE Denormals are Zero
    }
}

}  // namespace Dynarmic::Backend::X64
