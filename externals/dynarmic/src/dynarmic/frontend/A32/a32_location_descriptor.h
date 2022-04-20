/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <functional>
#include <iosfwd>
#include <tuple>

#include <mcl/stdint.hpp>

#include "dynarmic/frontend/A32/FPSCR.h"
#include "dynarmic/frontend/A32/ITState.h"
#include "dynarmic/frontend/A32/PSR.h"
#include "dynarmic/ir/location_descriptor.h"

namespace Dynarmic::A32 {

/**
 * LocationDescriptor describes the location of a basic block.
 * The location is not solely based on the PC because other flags influence the way
 * instructions should be translated. The CPSR.T flag is most notable since it
 * tells us if the processor is in Thumb or Arm mode.
 */
class LocationDescriptor {
public:
    // Indicates bits that should be preserved within descriptors.
    static constexpr u32 CPSR_MODE_MASK = 0x0600FE20;
    static constexpr u32 FPSCR_MODE_MASK = 0x07F70000;

    LocationDescriptor(u32 arm_pc, PSR cpsr, FPSCR fpscr, bool single_stepping = false)
            : arm_pc(arm_pc)
            , cpsr(cpsr.Value() & CPSR_MODE_MASK)
            , fpscr(fpscr.Value() & FPSCR_MODE_MASK)
            , single_stepping(single_stepping) {}

    explicit LocationDescriptor(const IR::LocationDescriptor& o) {
        arm_pc = static_cast<u32>(o.Value());
        cpsr.T((o.Value() >> 32) & 1);
        cpsr.E((o.Value() >> 32) & 2);
        fpscr = (o.Value() >> 32) & FPSCR_MODE_MASK;
        cpsr.IT(ITState{static_cast<u8>(o.Value() >> 40)});
        single_stepping = (o.Value() >> 32) & 4;
    }

    u32 PC() const { return arm_pc; }
    bool TFlag() const { return cpsr.T(); }
    bool EFlag() const { return cpsr.E(); }
    ITState IT() const { return cpsr.IT(); }

    A32::PSR CPSR() const { return cpsr; }
    A32::FPSCR FPSCR() const { return fpscr; }

    bool SingleStepping() const { return single_stepping; }

    bool operator==(const LocationDescriptor& o) const {
        return std::tie(arm_pc, cpsr, fpscr, single_stepping) == std::tie(o.arm_pc, o.cpsr, o.fpscr, o.single_stepping);
    }

    bool operator!=(const LocationDescriptor& o) const {
        return !operator==(o);
    }

    LocationDescriptor SetPC(u32 new_arm_pc) const {
        return LocationDescriptor(new_arm_pc, cpsr, fpscr, single_stepping);
    }

    LocationDescriptor AdvancePC(int amount) const {
        return LocationDescriptor(static_cast<u32>(arm_pc + amount), cpsr, fpscr, single_stepping);
    }

    LocationDescriptor SetTFlag(bool new_tflag) const {
        PSR new_cpsr = cpsr;
        new_cpsr.T(new_tflag);

        return LocationDescriptor(arm_pc, new_cpsr, fpscr, single_stepping);
    }

    LocationDescriptor SetEFlag(bool new_eflag) const {
        PSR new_cpsr = cpsr;
        new_cpsr.E(new_eflag);

        return LocationDescriptor(arm_pc, new_cpsr, fpscr, single_stepping);
    }

    LocationDescriptor SetFPSCR(u32 new_fpscr) const {
        return LocationDescriptor(arm_pc, cpsr, A32::FPSCR{new_fpscr & FPSCR_MODE_MASK}, single_stepping);
    }

    LocationDescriptor SetIT(ITState new_it) const {
        PSR new_cpsr = cpsr;
        new_cpsr.IT(new_it);

        return LocationDescriptor(arm_pc, new_cpsr, fpscr, single_stepping);
    }

    LocationDescriptor AdvanceIT() const {
        return SetIT(IT().Advance());
    }

    LocationDescriptor SetSingleStepping(bool new_single_stepping) const {
        return LocationDescriptor(arm_pc, cpsr, fpscr, new_single_stepping);
    }

    u64 UniqueHash() const noexcept {
        // This value MUST BE UNIQUE.
        // This calculation has to match up with EmitX64::EmitTerminalPopRSBHint
        const u64 pc_u64 = arm_pc;
        const u64 fpscr_u64 = fpscr.Value();
        const u64 t_u64 = cpsr.T() ? 1 : 0;
        const u64 e_u64 = cpsr.E() ? 2 : 0;
        const u64 single_stepping_u64 = single_stepping ? 4 : 0;
        const u64 it_u64 = u64(cpsr.IT().Value()) << 8;
        const u64 upper = (fpscr_u64 | t_u64 | e_u64 | single_stepping_u64 | it_u64) << 32;
        return pc_u64 | upper;
    }

    operator IR::LocationDescriptor() const {
        return IR::LocationDescriptor{UniqueHash()};
    }

private:
    u32 arm_pc;        ///< Current program counter value.
    PSR cpsr;          ///< Current program status register.
    A32::FPSCR fpscr;  ///< Floating point status control register.
    bool single_stepping;
};

/**
 * Provides a string representation of a LocationDescriptor.
 *
 * @param o          Output stream
 * @param descriptor The descriptor to get a string representation of
 */
std::ostream& operator<<(std::ostream& o, const LocationDescriptor& descriptor);

}  // namespace Dynarmic::A32

namespace std {
template<>
struct less<Dynarmic::A32::LocationDescriptor> {
    bool operator()(const Dynarmic::A32::LocationDescriptor& x, const Dynarmic::A32::LocationDescriptor& y) const noexcept {
        return x.UniqueHash() < y.UniqueHash();
    }
};
template<>
struct hash<Dynarmic::A32::LocationDescriptor> {
    size_t operator()(const Dynarmic::A32::LocationDescriptor& x) const noexcept {
        return std::hash<u64>()(x.UniqueHash());
    }
};
}  // namespace std
