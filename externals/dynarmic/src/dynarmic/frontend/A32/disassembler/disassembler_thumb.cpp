/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <cstdlib>
#include <string>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/string_util.h"
#include "dynarmic/frontend/A32/a32_types.h"
#include "dynarmic/frontend/A32/decoder/thumb16.h"
#include "dynarmic/frontend/A32/disassembler/disassembler.h"
#include "dynarmic/frontend/imm.h"

namespace Dynarmic::A32 {

class DisassemblerVisitor {
public:
    using instruction_return_type = std::string;

    std::string thumb16_LSL_imm(Imm<5> imm5, Reg m, Reg d) {
        return fmt::format("lsls {}, {}, #{}", d, m, imm5.ZeroExtend());
    }

    std::string thumb16_LSR_imm(Imm<5> imm5, Reg m, Reg d) {
        const u32 shift = imm5 != 0 ? imm5.ZeroExtend() : 32U;
        return fmt::format("lsrs {}, {}, #{}", d, m, shift);
    }

    std::string thumb16_ASR_imm(Imm<5> imm5, Reg m, Reg d) {
        const u32 shift = imm5 != 0 ? imm5.ZeroExtend() : 32U;
        return fmt::format("asrs {}, {}, #{}", d, m, shift);
    }

    std::string thumb16_ADD_reg_t1(Reg m, Reg n, Reg d) {
        return fmt::format("adds {}, {}, {}", d, n, m);
    }

    std::string thumb16_SUB_reg(Reg m, Reg n, Reg d) {
        return fmt::format("subs {}, {}, {}", d, n, m);
    }

    std::string thumb16_ADD_imm_t1(Imm<3> imm3, Reg n, Reg d) {
        return fmt::format("adds {}, {}, #{}", d, n, imm3.ZeroExtend());
    }

    std::string thumb16_SUB_imm_t1(Imm<3> imm3, Reg n, Reg d) {
        return fmt::format("subs {}, {}, #{}", d, n, imm3.ZeroExtend());
    }

    std::string thumb16_MOV_imm(Reg d, Imm<8> imm8) {
        return fmt::format("movs {}, #{}", d, imm8.ZeroExtend());
    }

    std::string thumb16_CMP_imm(Reg n, Imm<8> imm8) {
        return fmt::format("cmp {}, #{}", n, imm8.ZeroExtend());
    }

    std::string thumb16_ADD_imm_t2(Reg d_n, Imm<8> imm8) {
        return fmt::format("adds {}, #{}", d_n, imm8.ZeroExtend());
    }

    std::string thumb16_SUB_imm_t2(Reg d_n, Imm<8> imm8) {
        return fmt::format("subs {}, #{}", d_n, imm8.ZeroExtend());
    }

    std::string thumb16_AND_reg(Reg m, Reg d_n) {
        return fmt::format("ands {}, {}", d_n, m);
    }

    std::string thumb16_EOR_reg(Reg m, Reg d_n) {
        return fmt::format("eors {}, {}", d_n, m);
    }

    std::string thumb16_LSL_reg(Reg m, Reg d_n) {
        return fmt::format("lsls {}, {}", d_n, m);
    }

    std::string thumb16_LSR_reg(Reg m, Reg d_n) {
        return fmt::format("lsrs {}, {}", d_n, m);
    }

    std::string thumb16_ASR_reg(Reg m, Reg d_n) {
        return fmt::format("asrs {}, {}", d_n, m);
    }

    std::string thumb16_ADC_reg(Reg m, Reg d_n) {
        return fmt::format("adcs {}, {}", d_n, m);
    }

    std::string thumb16_SBC_reg(Reg m, Reg d_n) {
        return fmt::format("sbcs {}, {}", d_n, m);
    }

    std::string thumb16_ROR_reg(Reg m, Reg d_n) {
        return fmt::format("rors {}, {}", d_n, m);
    }

    std::string thumb16_TST_reg(Reg m, Reg n) {
        return fmt::format("tst {}, {}", n, m);
    }

    std::string thumb16_RSB_imm(Reg n, Reg d) {
        // Pre-UAL syntax: NEGS <Rd>, <Rn>
        return fmt::format("rsbs {}, {}, #0", d, n);
    }

    std::string thumb16_CMP_reg_t1(Reg m, Reg n) {
        return fmt::format("cmp {}, {}", n, m);
    }

    std::string thumb16_CMN_reg(Reg m, Reg n) {
        return fmt::format("cmn {}, {}", n, m);
    }

    std::string thumb16_ORR_reg(Reg m, Reg d_n) {
        return fmt::format("orrs {}, {}", d_n, m);
    }

    std::string thumb16_MUL_reg(Reg n, Reg d_m) {
        return fmt::format("muls {}, {}, {}", d_m, n, d_m);
    }

    std::string thumb16_BIC_reg(Reg m, Reg d_n) {
        return fmt::format("bics {}, {}", d_n, m);
    }

    std::string thumb16_MVN_reg(Reg m, Reg d) {
        return fmt::format("mvns {}, {}", d, m);
    }

    std::string thumb16_ADD_reg_t2(bool d_n_hi, Reg m, Reg d_n_lo) {
        const Reg d_n = d_n_hi ? (d_n_lo + 8) : d_n_lo;
        return fmt::format("add {}, {}", d_n, m);
    }

    std::string thumb16_CMP_reg_t2(bool n_hi, Reg m, Reg n_lo) {
        const Reg n = n_hi ? (n_lo + 8) : n_lo;
        return fmt::format("cmp {}, {}", n, m);
    }

    std::string thumb16_MOV_reg(bool d_hi, Reg m, Reg d_lo) {
        const Reg d = d_hi ? (d_lo + 8) : d_lo;
        return fmt::format("mov {}, {}", d, m);
    }

    std::string thumb16_LDR_literal(Reg t, Imm<8> imm8) {
        const u32 imm32 = imm8.ZeroExtend() << 2;
        return fmt::format("ldr {}, [pc, #{}]", t, imm32);
    }

    std::string thumb16_STR_reg(Reg m, Reg n, Reg t) {
        return fmt::format("str {}, [{}, {}]", t, n, m);
    }

    std::string thumb16_STRH_reg(Reg m, Reg n, Reg t) {
        return fmt::format("strh {}, [{}, {}]", t, n, m);
    }

    std::string thumb16_STRB_reg(Reg m, Reg n, Reg t) {
        return fmt::format("strb {}, [{}, {}]", t, n, m);
    }

    std::string thumb16_LDRSB_reg(Reg m, Reg n, Reg t) {
        return fmt::format("ldrsb {}, [{}, {}]", t, n, m);
    }

    std::string thumb16_LDR_reg(Reg m, Reg n, Reg t) {
        return fmt::format("ldr {}, [{}, {}]", t, n, m);
    }

    std::string thumb16_LDRH_reg(Reg m, Reg n, Reg t) {
        return fmt::format("ldrh {}, [%s, %s]", t, n, m);
    }

    std::string thumb16_LDRB_reg(Reg m, Reg n, Reg t) {
        return fmt::format("ldrb {}, [{}, {}]", t, n, m);
    }

    std::string thumb16_LDRSH_reg(Reg m, Reg n, Reg t) {
        return fmt::format("ldrsh {}, [{}, {}]", t, n, m);
    }

    std::string thumb16_STR_imm_t1(Imm<5> imm5, Reg n, Reg t) {
        const u32 imm32 = imm5.ZeroExtend() << 2;
        return fmt::format("str {}, [{}, #{}]", t, n, imm32);
    }

    std::string thumb16_LDR_imm_t1(Imm<5> imm5, Reg n, Reg t) {
        const u32 imm32 = imm5.ZeroExtend() << 2;
        return fmt::format("ldr {}, [{}, #{}]", t, n, imm32);
    }

    std::string thumb16_STRB_imm(Imm<5> imm5, Reg n, Reg t) {
        const u32 imm32 = imm5.ZeroExtend();
        return fmt::format("strb {}, [{}, #{}]", t, n, imm32);
    }

    std::string thumb16_LDRB_imm(Imm<5> imm5, Reg n, Reg t) {
        const u32 imm32 = imm5.ZeroExtend();
        return fmt::format("ldrb {}, [{}, #{}]", t, n, imm32);
    }

    std::string thumb16_STRH_imm(Imm<5> imm5, Reg n, Reg t) {
        const u32 imm32 = imm5.ZeroExtend() << 1;
        return fmt::format("strh {}, [{}, #{}]", t, n, imm32);
    }

    std::string thumb16_LDRH_imm(Imm<5> imm5, Reg n, Reg t) {
        const u32 imm32 = imm5.ZeroExtend() << 1;
        return fmt::format("ldrh {}, [{}, #{}]", t, n, imm32);
    }

    std::string thumb16_STR_imm_t2(Reg t, Imm<8> imm8) {
        const u32 imm32 = imm8.ZeroExtend() << 2;
        return fmt::format("str {}, [sp, #{}]", t, imm32);
    }

    std::string thumb16_LDR_imm_t2(Reg t, Imm<8> imm8) {
        const u32 imm32 = imm8.ZeroExtend() << 2;
        return fmt::format("ldr {}, [sp, #{}]", t, imm32);
    }

    std::string thumb16_ADR(Reg d, Imm<8> imm8) {
        const u32 imm32 = imm8.ZeroExtend() << 2;
        return fmt::format("adr {}, +#{}", d, imm32);
    }

    std::string thumb16_ADD_sp_t1(Reg d, Imm<8> imm8) {
        const u32 imm32 = imm8.ZeroExtend() << 2;
        return fmt::format("add {}, sp, #{}", d, imm32);
    }

    std::string thumb16_ADD_sp_t2(Imm<7> imm7) {
        const u32 imm32 = imm7.ZeroExtend() << 2;
        return fmt::format("add sp, sp, #{}", imm32);
    }

    std::string thumb16_SUB_sp(Imm<7> imm7) {
        const u32 imm32 = imm7.ZeroExtend() << 2;
        return fmt::format("sub sp, sp, #{}", imm32);
    }

    std::string thumb16_NOP() {
        return "nop";
    }

    std::string thumb16_SEV() {
        return "sev";
    }

    std::string thumb16_SEVL() {
        return "sevl";
    }

    std::string thumb16_WFE() {
        return "wfe";
    }

    std::string thumb16_WFI() {
        return "wfi";
    }

    std::string thumb16_YIELD() {
        return "yield";
    }

    std::string thumb16_IT(Imm<8> imm8) {
        const Cond firstcond = imm8.Bits<4, 7, Cond>();
        const bool firstcond0 = imm8.Bit<4>();
        const auto [x, y, z] = [&] {
            if (imm8.Bits<0, 3>() == 0b1000) {
                return std::make_tuple("", "", "");
            }
            if (imm8.Bits<0, 2>() == 0b100) {
                return std::make_tuple(imm8.Bit<3>() == firstcond0 ? "t" : "e", "", "");
            }
            if (imm8.Bits<0, 1>() == 0b10) {
                return std::make_tuple(imm8.Bit<3>() == firstcond0 ? "t" : "e", imm8.Bit<2>() == firstcond0 ? "t" : "e", "");
            }
            // Sanity note: Here imm8.Bit<0>() is guaranteed to be == 1. (imm8 can never be 0bxxxx0000)
            return std::make_tuple(imm8.Bit<3>() == firstcond0 ? "t" : "e", imm8.Bit<2>() == firstcond0 ? "t" : "e", imm8.Bit<1>() == firstcond0 ? "t" : "e");
        }();
        return fmt::format("it{}{}{} {}", x, y, z, firstcond);
    }

    std::string thumb16_SXTH(Reg m, Reg d) {
        return fmt::format("sxth {}, {}", d, m);
    }

    std::string thumb16_SXTB(Reg m, Reg d) {
        return fmt::format("sxtb {}, {}", d, m);
    }

    std::string thumb16_UXTH(Reg m, Reg d) {
        return fmt::format("uxth {}, {}", d, m);
    }

    std::string thumb16_UXTB(Reg m, Reg d) {
        return fmt::format("uxtb {}, {}", d, m);
    }

    std::string thumb16_PUSH(bool M, RegList reg_list) {
        if (M)
            reg_list |= 1 << 14;
        return fmt::format("push {{{}}}", RegListToString(reg_list));
    }

    std::string thumb16_POP(bool P, RegList reg_list) {
        if (P)
            reg_list |= 1 << 15;
        return fmt::format("pop {{{}}}", RegListToString(reg_list));
    }

    std::string thumb16_SETEND(bool E) {
        return fmt::format("setend {}", E ? "BE" : "LE");
    }

    std::string thumb16_CPS(bool im, bool a, bool i, bool f) {
        return fmt::format("cps{} {}{}{}", im ? "id" : "ie", a ? "a" : "", i ? "i" : "", f ? "f" : "");
    }

    std::string thumb16_REV(Reg m, Reg d) {
        return fmt::format("rev {}, {}", d, m);
    }

    std::string thumb16_REV16(Reg m, Reg d) {
        return fmt::format("rev16 {}, {}", d, m);
    }

    std::string thumb16_REVSH(Reg m, Reg d) {
        return fmt::format("revsh {}, {}", d, m);
    }

    std::string thumb16_BKPT(Imm<8> imm8) {
        return fmt::format("bkpt #{}", imm8.ZeroExtend());
    }

    std::string thumb16_STMIA(Reg n, RegList reg_list) {
        return fmt::format("stm {}!, {{{}}}", n, RegListToString(reg_list));
    }

    std::string thumb16_LDMIA(Reg n, RegList reg_list) {
        const bool write_back = !Common::Bit(static_cast<size_t>(n), reg_list);
        return fmt::format("ldm {}{}, {{{}}}", n, write_back ? "!" : "", RegListToString(reg_list));
    }

    std::string thumb16_BX(Reg m) {
        return fmt::format("bx {}", m);
    }

    std::string thumb16_BLX_reg(Reg m) {
        return fmt::format("blx {}", m);
    }

    std::string thumb16_CBZ_CBNZ(bool nonzero, Imm<1> i, Imm<5> imm5, Reg n) {
        const char* const name = nonzero ? "cbnz" : "cbz";
        const u32 imm = concatenate(i, imm5, Imm<1>{0}).ZeroExtend();

        return fmt::format("{} {}, #{}", name, n, imm);
    }

    std::string thumb16_UDF() {
        return fmt::format("udf");
    }

    std::string thumb16_SVC(Imm<8> imm8) {
        return fmt::format("svc #{}", imm8.ZeroExtend());
    }

    std::string thumb16_B_t1(Cond cond, Imm<8> imm8) {
        const s32 imm32 = static_cast<s32>((imm8.SignExtend<u32>() << 1) + 4);
        return fmt::format("b{} {}#{}",
                           CondToString(cond),
                           Common::SignToChar(imm32),
                           abs(imm32));
    }

    std::string thumb16_B_t2(Imm<11> imm11) {
        const s32 imm32 = static_cast<s32>((imm11.SignExtend<u32>() << 1) + 4);
        return fmt::format("b {}#{}",
                           Common::SignToChar(imm32),
                           abs(imm32));
    }
};

std::string DisassembleThumb16(u16 instruction) {
    DisassemblerVisitor visitor;
    auto decoder = DecodeThumb16<DisassemblerVisitor>(instruction);
    return !decoder ? fmt::format("UNKNOWN: {:x}", instruction) : decoder->get().call(visitor, instruction);
}

}  // namespace Dynarmic::A32
