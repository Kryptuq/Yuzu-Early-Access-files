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
#include "dynarmic/frontend/A32/decoder/arm.h"
#include "dynarmic/frontend/A32/decoder/vfp.h"
#include "dynarmic/frontend/A32/disassembler/disassembler.h"
#include "dynarmic/frontend/imm.h"

namespace Dynarmic::A32 {

class DisassemblerVisitor {
public:
    using instruction_return_type = std::string;

    static u32 ArmExpandImm(int rotate, Imm<8> imm8) {
        return Common::RotateRight(static_cast<u32>(imm8.ZeroExtend()), rotate * 2);
    }

    static std::string ShiftStr(ShiftType shift, Imm<5> imm5) {
        switch (shift) {
        case ShiftType::LSL:
            if (imm5 == 0)
                return "";
            return fmt::format(", lsl #{}", imm5.ZeroExtend());
        case ShiftType::LSR:
            if (imm5 == 0)
                return ", lsr #32";
            return fmt::format(", lsr #{}", imm5.ZeroExtend());
        case ShiftType::ASR:
            if (imm5 == 0)
                return ", asr #32";
            return fmt::format(", asr #{}", imm5.ZeroExtend());
        case ShiftType::ROR:
            if (imm5 == 0)
                return ", rrx";
            return fmt::format(", ror #{}", imm5.ZeroExtend());
        }
        ASSERT(false);
        return "<internal error>";
    }

    static std::string RsrStr(Reg s, ShiftType shift, Reg m) {
        switch (shift) {
        case ShiftType::LSL:
            return fmt::format("{}, lsl {}", m, s);
        case ShiftType::LSR:
            return fmt::format("{}, lsr {}", m, s);
        case ShiftType::ASR:
            return fmt::format("{}, asr {}", m, s);
        case ShiftType::ROR:
            return fmt::format("{}, ror {}", m, s);
        }
        ASSERT(false);
        return "<internal error>";
    }

    static std::string RorStr(Reg m, SignExtendRotation rotate) {
        switch (rotate) {
        case SignExtendRotation::ROR_0:
            return RegToString(m);
        case SignExtendRotation::ROR_8:
            return fmt::format("{}, ror #8", m);
        case SignExtendRotation::ROR_16:
            return fmt::format("{}, ror #16", m);
        case SignExtendRotation::ROR_24:
            return fmt::format("{}, ror #24", m);
        }
        ASSERT(false);
        return "<internal error>";
    }

    static const char* BarrierOptionStr(Imm<4> option) {
        switch (option.ZeroExtend()) {
        case 0b0010:
            return " oshst";
        case 0b0011:
            return " osh";
        case 0b0110:
            return " nshst";
        case 0b0111:
            return " nsh";
        case 0b1010:
            return " ishst";
        case 0b1011:
            return " ish";
        case 0b1110:
            return " st";
        case 0b1111:  // SY can be omitted.
            return "";
        default:
            return " unknown";
        }
    }

    static std::string FPRegStr(bool dp_operation, size_t base, bool bit) {
        size_t reg_num;
        if (dp_operation) {
            reg_num = base + (bit ? 16 : 0);
        } else {
            reg_num = (base << 1) + (bit ? 1 : 0);
        }
        return fmt::format("{}{}", dp_operation ? 'd' : 's', reg_num);
    }

    static std::string FPNextRegStr(bool dp_operation, size_t base, bool bit) {
        size_t reg_num;
        if (dp_operation) {
            reg_num = base + (bit ? 16 : 0);
        } else {
            reg_num = (base << 1) + (bit ? 1 : 0);
        }
        return fmt::format("{}{}", dp_operation ? 'd' : 's', reg_num + 1);
    }

    static std::string VectorStr(bool Q, size_t base, bool bit) {
        size_t reg_num;
        if (Q) {
            reg_num = (base >> 1) + (bit ? 8 : 0);
        } else {
            reg_num = base + (bit ? 16 : 0);
        }
        return fmt::format("{}{}", Q ? 'q' : 'd', reg_num);
    }

    static std::string CondOrTwo(Cond cond) {
        return cond == Cond::NV ? "2" : CondToString(cond);
    }

    // Barrier instructions
    std::string arm_DMB(Imm<4> option) {
        return fmt::format("dmb{}", BarrierOptionStr(option));
    }
    std::string arm_DSB(Imm<4> option) {
        return fmt::format("dsb{}", BarrierOptionStr(option));
    }
    std::string arm_ISB([[maybe_unused]] Imm<4> option) {
        return "isb";
    }

    // Branch instructions
    std::string arm_B(Cond cond, Imm<24> imm24) {
        const s32 offset = Common::SignExtend<26, s32>(imm24.ZeroExtend() << 2) + 8;
        return fmt::format("b{} {}#{}", CondToString(cond), Common::SignToChar(offset), abs(offset));
    }
    std::string arm_BL(Cond cond, Imm<24> imm24) {
        const s32 offset = Common::SignExtend<26, s32>(imm24.ZeroExtend() << 2) + 8;
        return fmt::format("bl{} {}#{}", CondToString(cond), Common::SignToChar(offset), abs(offset));
    }
    std::string arm_BLX_imm(bool H, Imm<24> imm24) {
        const s32 offset = Common::SignExtend<26, s32>(imm24.ZeroExtend() << 2) + 8 + (H ? 2 : 0);
        return fmt::format("blx {}#{}", Common::SignToChar(offset), abs(offset));
    }
    std::string arm_BLX_reg(Cond cond, Reg m) {
        return fmt::format("blx{} {}", CondToString(cond), m);
    }
    std::string arm_BX(Cond cond, Reg m) {
        return fmt::format("bx{} {}", CondToString(cond), m);
    }
    std::string arm_BXJ(Cond cond, Reg m) {
        return fmt::format("bxj{} {}", CondToString(cond), m);
    }

    // Coprocessor instructions
    std::string arm_CDP(Cond cond, size_t opc1, CoprocReg CRn, CoprocReg CRd, size_t coproc_no, size_t opc2, CoprocReg CRm) {
        return fmt::format("cdp{} p{}, #{}, {}, {}, {}, #{}", CondToString(cond), coproc_no, opc1, CRd, CRn, CRm, opc2);
    }

    std::string arm_LDC(Cond cond, bool p, bool u, bool d, bool w, Reg n, CoprocReg CRd, size_t coproc_no, Imm<8> imm8) {
        const u32 imm32 = static_cast<u32>(imm8.ZeroExtend()) << 2;
        if (!p && !u && !d && !w) {
            return "<undefined>";
        }
        if (p) {
            return fmt::format("ldc{}{} {}, {}, [{}, #{}{}]{}", d ? "l" : "",
                               CondOrTwo(cond), coproc_no, CRd, n, u ? "+" : "-", imm32,
                               w ? "!" : "");
        }
        if (!p && w) {
            return fmt::format("ldc{}{} {}, {}, [{}], #{}{}", d ? "l" : "",
                               CondOrTwo(cond), coproc_no, CRd, n, u ? "+" : "-", imm32);
        }
        if (!p && !w && u) {
            return fmt::format("ldc{}{} {}, {}, [{}], {}", d ? "l" : "",
                               CondOrTwo(cond), coproc_no, CRd, n, imm8.ZeroExtend());
        }
        UNREACHABLE();
    }

    std::string arm_MCR(Cond cond, size_t opc1, CoprocReg CRn, Reg t, size_t coproc_no, size_t opc2, CoprocReg CRm) {
        return fmt::format("mcr{} p{}, #{}, {}, {}, {}, #{}", CondOrTwo(cond), coproc_no, opc1, t, CRn, CRm, opc2);
    }

    std::string arm_MCRR(Cond cond, Reg t2, Reg t, size_t coproc_no, size_t opc, CoprocReg CRm) {
        return fmt::format("mcr{} p{}, #{}, {}, {}, {}", CondOrTwo(cond), coproc_no, opc, t, t2, CRm);
    }

    std::string arm_MRC(Cond cond, size_t opc1, CoprocReg CRn, Reg t, size_t coproc_no, size_t opc2, CoprocReg CRm) {
        return fmt::format("mrc{} p{}, #{}, {}, {}, {}, #{}", CondOrTwo(cond), coproc_no, opc1, t, CRn, CRm, opc2);
    }

    std::string arm_MRRC(Cond cond, Reg t2, Reg t, size_t coproc_no, size_t opc, CoprocReg CRm) {
        return fmt::format("mrrc{} p{}, #{}, {}, {}, {}", CondOrTwo(cond), coproc_no, opc, t, t2, CRm);
    }

    std::string arm_STC(Cond cond, bool p, bool u, bool d, bool w, Reg n, CoprocReg CRd, size_t coproc_no, Imm<8> imm8) {
        const u32 imm32 = static_cast<u32>(imm8.ZeroExtend()) << 2;
        if (!p && !u && !d && !w) {
            return "<undefined>";
        }
        if (p) {
            return fmt::format("stc{}{} {}, {}, [{}, #{}{}]{}", d ? "l" : "",
                               CondOrTwo(cond), coproc_no, CRd, n,
                               u ? "+" : "-", imm32, w ? "!" : "");
        }
        if (!p && w) {
            return fmt::format("stc{}{} {}, {}, [{}], #{}{}", d ? "l" : "",
                               CondOrTwo(cond), coproc_no, CRd, n,
                               u ? "+" : "-", imm32);
        }
        if (!p && !w && u) {
            return fmt::format("stc{}{} {}, {}, [{}], {}", d ? "l" : "",
                               CondOrTwo(cond), coproc_no, CRd, n, imm8.ZeroExtend());
        }
        UNREACHABLE();
    }

    // CRC32 instructions
    std::string arm_CRC32([[maybe_unused]] Cond cond, Imm<2> sz, Reg n, Reg d, Reg m) {
        static constexpr std::array data_type{
            "b",
            "h",
            "w",
            "invalid",
        };

        return fmt::format("crc32{} {}, {}, {}", data_type[sz.ZeroExtend()], d, n, m);
    }
    std::string arm_CRC32C([[maybe_unused]] Cond cond, Imm<2> sz, Reg n, Reg d, Reg m) {
        static constexpr std::array data_type{
            "b",
            "h",
            "w",
            "invalid",
        };

        return fmt::format("crc32c{} {}, {}, {}", data_type[sz.ZeroExtend()], d, n, m);
    }

    // Data processing instructions
    std::string arm_ADC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("adc{}{} {}, {}, #{}", CondToString(cond), S ? "s" : "", d, n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_ADC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("adc{}{} {}, {}, {}{}", CondToString(cond), S ? "s" : "", d, n, m, ShiftStr(shift, imm5));
    }
    std::string arm_ADC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("adc{}{} {}, {}, {}", CondToString(cond), S ? "s" : "", d, n, RsrStr(s, shift, m));
    }
    std::string arm_ADD_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("add{}{} {}, {}, #{}", CondToString(cond), S ? "s" : "", d, n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_ADD_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("add{}{} {}, {}, {}{}", CondToString(cond), S ? "s" : "", d, n, m, ShiftStr(shift, imm5));
    }
    std::string arm_ADD_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("add{}{} {}, {}, {}", CondToString(cond), S ? "s" : "", d, n, RsrStr(s, shift, m));
    }
    std::string arm_AND_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("and{}{} {}, {}, #{}", CondToString(cond), S ? "s" : "", d, n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_AND_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("and{}{} {}, {}, {}{}", CondToString(cond), S ? "s" : "", d, n, m, ShiftStr(shift, imm5));
    }
    std::string arm_AND_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("and{}{} {}, {}, {}", CondToString(cond), S ? "s" : "", d, n, RsrStr(s, shift, m));
    }
    std::string arm_BIC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("bic{}{} {}, {}, #{}", CondToString(cond), S ? "s" : "", d, n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_BIC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("bic{}{} {}, {}, {}{}", CondToString(cond), S ? "s" : "", d, n, m, ShiftStr(shift, imm5));
    }
    std::string arm_BIC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("bic{}{} {}, {}, {}", CondToString(cond), S ? "s" : "", d, n, RsrStr(s, shift, m));
    }
    std::string arm_CMN_imm(Cond cond, Reg n, int rotate, Imm<8> imm8) {
        return fmt::format("cmn{} {}, #{}", CondToString(cond), n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_CMN_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("cmn{} {}, {}{}", CondToString(cond), n, m, ShiftStr(shift, imm5));
    }
    std::string arm_CMN_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return fmt::format("cmn{} {}, {}", CondToString(cond), n, RsrStr(s, shift, m));
    }
    std::string arm_CMP_imm(Cond cond, Reg n, int rotate, Imm<8> imm8) {
        return fmt::format("cmp{} {}, #{}", CondToString(cond), n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_CMP_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("cmp{} {}, {}{}", CondToString(cond), n, m, ShiftStr(shift, imm5));
    }
    std::string arm_CMP_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return fmt::format("cmp{} {}, {}", CondToString(cond), n, RsrStr(s, shift, m));
    }
    std::string arm_EOR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("eor{}{} {}, {}, #{}", CondToString(cond), S ? "s" : "", d, n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_EOR_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("eor{}{} {}, {}, {}{}", CondToString(cond), S ? "s" : "", d, n, m, ShiftStr(shift, imm5));
    }
    std::string arm_EOR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("eor{}{} {}, {}, {}", CondToString(cond), S ? "s" : "", d, n, RsrStr(s, shift, m));
    }
    std::string arm_MOV_imm(Cond cond, bool S, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("mov{}{} {}, #{}", CondToString(cond), S ? "s" : "", d, ArmExpandImm(rotate, imm8));
    }
    std::string arm_MOV_reg(Cond cond, bool S, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("mov{}{} {}, {}{}", CondToString(cond), S ? "s" : "", d, m, ShiftStr(shift, imm5));
    }
    std::string arm_MOV_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("mov{}{} {}, {}", CondToString(cond), S ? "s" : "", d, RsrStr(s, shift, m));
    }
    std::string arm_MVN_imm(Cond cond, bool S, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("mvn{}{} {}, #{}", CondToString(cond), S ? "s" : "", d, ArmExpandImm(rotate, imm8));
    }
    std::string arm_MVN_reg(Cond cond, bool S, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("mvn{}{} {}, {}{}", CondToString(cond), S ? "s" : "", d, m, ShiftStr(shift, imm5));
    }
    std::string arm_MVN_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("mvn{}{} {}, {}", CondToString(cond), S ? "s" : "", d, RsrStr(s, shift, m));
    }
    std::string arm_ORR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("orr{}{} {}, {}, #{}", CondToString(cond), S ? "s" : "", d, n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_ORR_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("orr{}{} {}, {}, {}{}", CondToString(cond), S ? "s" : "", d, n, m, ShiftStr(shift, imm5));
    }
    std::string arm_ORR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("orr{}{} {}, {}, {}", CondToString(cond), S ? "s" : "", d, n, RsrStr(s, shift, m));
    }
    std::string arm_RSB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("rsb{}{} {}, {}, #{}", CondToString(cond), S ? "s" : "", d, n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_RSB_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("rsb{}{} {}, {}, {}{}", CondToString(cond), S ? "s" : "", d, n, m, ShiftStr(shift, imm5));
    }
    std::string arm_RSB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("rsb{}{} {}, {}, {}", CondToString(cond), S ? "s" : "", d, n, RsrStr(s, shift, m));
    }
    std::string arm_RSC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("rsc{}{} {}, {}, #{}", CondToString(cond), S ? "s" : "", d, n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_RSC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("rsc{}{} {}, {}, {}{}", CondToString(cond), S ? "s" : "", d, n, m, ShiftStr(shift, imm5));
    }
    std::string arm_RSC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("rsc{}{} {}, {}, {}", CondToString(cond), S ? "s" : "", d, n, RsrStr(s, shift, m));
    }
    std::string arm_SBC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("sbc{}{} {}, {}, #{}", CondToString(cond), S ? "s" : "", d, n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_SBC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("sbc{}{} {}, {}, {}{}", CondToString(cond), S ? "s" : "", d, n, m, ShiftStr(shift, imm5));
    }
    std::string arm_SBC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("sbc{}{} {}, {}, {}", CondToString(cond), S ? "s" : "", d, n, RsrStr(s, shift, m));
    }
    std::string arm_SUB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
        return fmt::format("sub{}{} {}, {}, #{}", CondToString(cond), S ? "s" : "", d, n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_SUB_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("sub{}{} {}, {}, {}{}", CondToString(cond), S ? "s" : "", d, n, m, ShiftStr(shift, imm5));
    }
    std::string arm_SUB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return fmt::format("sub{}{} {}, {}, {}", CondToString(cond), S ? "s" : "", d, n, RsrStr(s, shift, m));
    }
    std::string arm_TEQ_imm(Cond cond, Reg n, int rotate, Imm<8> imm8) {
        return fmt::format("teq{} {}, #{}", CondToString(cond), n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_TEQ_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("teq{} {}, {}{}", CondToString(cond), n, m, ShiftStr(shift, imm5));
    }
    std::string arm_TEQ_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return fmt::format("teq{} {}, {}", CondToString(cond), n, RsrStr(s, shift, m));
    }
    std::string arm_TST_imm(Cond cond, Reg n, int rotate, Imm<8> imm8) {
        return fmt::format("tst{} {}, #{}", CondToString(cond), n, ArmExpandImm(rotate, imm8));
    }
    std::string arm_TST_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m) {
        return fmt::format("tst{} {}, {}{}", CondToString(cond), n, m, ShiftStr(shift, imm5));
    }
    std::string arm_TST_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return fmt::format("tst{} {}, {}", CondToString(cond), n, RsrStr(s, shift, m));
    }

    // Exception generation instructions
    std::string arm_BKPT(Cond cond, Imm<12> imm12, Imm<4> imm4) {
        return fmt::format("bkpt{} #{}", CondToString(cond), concatenate(imm12, imm4).ZeroExtend());
    }
    std::string arm_SVC(Cond cond, Imm<24> imm24) {
        return fmt::format("svc{} #{}", CondToString(cond), imm24.ZeroExtend());
    }
    std::string arm_UDF() {
        return fmt::format("udf");
    }

    // Extension functions
    std::string arm_SXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("sxtab{} {}, {}, {}", CondToString(cond), d, n, RorStr(m, rotate));
    }
    std::string arm_SXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("sxtab16{} {}, {}, {}", CondToString(cond), d, n, RorStr(m, rotate));
    }
    std::string arm_SXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("sxtah{} {}, {}, {}", CondToString(cond), d, n, RorStr(m, rotate));
    }
    std::string arm_SXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("sxtb{} {}, {}", CondToString(cond), d, RorStr(m, rotate));
    }
    std::string arm_SXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("sxtb16{} {}, {}", CondToString(cond), d, RorStr(m, rotate));
    }
    std::string arm_SXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("sxth{} {}, {}", CondToString(cond), d, RorStr(m, rotate));
    }
    std::string arm_UXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("uxtab{} {}, {}, {}", CondToString(cond), d, n, RorStr(m, rotate));
    }
    std::string arm_UXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("uxtab16{} {}, {}, {}", CondToString(cond), d, n, RorStr(m, rotate));
    }
    std::string arm_UXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("uxtah{} {}, {}, {}", CondToString(cond), d, n, RorStr(m, rotate));
    }
    std::string arm_UXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("uxtb{} {}, {}", CondToString(cond), d, RorStr(m, rotate));
    }
    std::string arm_UXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("uxtb16{} {}, {}", CondToString(cond), d, RorStr(m, rotate));
    }
    std::string arm_UXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return fmt::format("uxth{} {}, {}", CondToString(cond), d, RorStr(m, rotate));
    }

    // Hint instructions
    std::string arm_PLD_imm(bool add, bool R, Reg n, Imm<12> imm12) {
        const char sign = add ? '+' : '-';
        const char* const w = R ? "" : "w";

        return fmt::format("pld{} [{}, #{}{:x}]", w, n, sign, imm12.ZeroExtend());
    }
    std::string arm_PLD_reg(bool add, bool R, Reg n, Imm<5> imm5, ShiftType shift, Reg m) {
        const char sign = add ? '+' : '-';
        const char* const w = R ? "" : "w";

        return fmt::format("pld{} [{}, {}{}{}]", w, n, sign, m, ShiftStr(shift, imm5));
    }
    std::string arm_SEV() {
        return "sev";
    }
    std::string arm_SEVL() {
        return "sevl";
    }
    std::string arm_WFE() {
        return "wfe";
    }
    std::string arm_WFI() {
        return "wfi";
    }
    std::string arm_YIELD() {
        return "yield";
    }

    // Load/Store instructions
    std::string arm_LDR_lit(Cond cond, bool U, Reg t, Imm<12> imm12) {
        const bool P = true;
        const bool W = false;
        return arm_LDR_imm(cond, P, U, W, Reg::PC, t, imm12);
    }
    std::string arm_LDR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
        const u32 imm12_value = imm12.ZeroExtend();
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldr{} {}, [{}, #{}{}]{}",
                               CondToString(cond), t, n, sign,
                               imm12_value, W ? "!" : "");
        } else {
            return fmt::format("ldr{} {}, [{}], #{}{}{}",
                               CondToString(cond), t, n, sign,
                               imm12_value, W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldr{} {}, [{}, {}{}{}]{}",
                               CondToString(cond), t, n, sign, m,
                               ShiftStr(shift, imm5), W ? "!" : "");
        } else {
            return fmt::format("ldr{} {}, [{}], {}{}{}{}",
                               CondToString(cond), t, n, sign, m,
                               ShiftStr(shift, imm5), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRB_lit(Cond cond, bool U, Reg t, Imm<12> imm12) {
        const bool P = true;
        const bool W = false;
        return arm_LDRB_imm(cond, P, U, W, Reg::PC, t, imm12);
    }
    std::string arm_LDRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
        const u32 imm12_value = imm12.ZeroExtend();
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldrb{} {}, [{}, #{}{}]{}",
                               CondToString(cond), t, n, sign, imm12_value,
                               W ? "!" : "");
        } else {
            return fmt::format("ldrb{} {}, [{}], #{}{}{}",
                               CondToString(cond), t, n, sign, imm12_value,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldrb{} {}, [{}, {}{}{}]{}",
                               CondToString(cond), t, n, sign, m,
                               ShiftStr(shift, imm5), W ? "!" : "");
        } else {
            return fmt::format("ldrb{} {}, [{}], {}{}{}{}",
                               CondToString(cond), t, n, sign, m,
                               ShiftStr(shift, imm5), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRBT() { return "ice"; }
    std::string arm_LDRD_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
        const bool P = true;
        const bool W = false;
        return arm_LDRD_imm(cond, P, U, W, Reg::PC, t, imm8a, imm8b);
    }
    std::string arm_LDRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
        const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldrd{} {}, {}, [{}, #{}{}]{}",
                               CondToString(cond), t, t + 1, n, sign, imm32,
                               W ? "!" : "");
        } else {
            return fmt::format("ldrd{} {}, {}, [{}], #{}{}{}",
                               CondToString(cond), t, t + 1, n, sign, imm32,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldrd{} {}, {}, [{}, {}{}]{}",
                               CondToString(cond), t, t + 1, n, sign, m,
                               W ? "!" : "");
        } else {
            return fmt::format("ldrd{} {}, {}, [{}], {}{}{}",
                               CondToString(cond), t, t + 1, n, sign, m,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRH_lit(Cond cond, bool P, bool U, bool W, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
        return arm_LDRH_imm(cond, P, U, W, Reg::PC, t, imm8a, imm8b);
    }
    std::string arm_LDRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
        const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldrh{} {}, [{}, #{}{}]{}",
                               CondToString(cond), t, n, sign, imm32,
                               W ? "!" : "");
        } else {
            return fmt::format("ldrh{} {}, [{}], #{}{}{}",
                               CondToString(cond), t, n, sign, imm32,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldrh{} {}, [{}, {}{}]{}",
                               CondToString(cond), t, n, sign, m,
                               W ? "!" : "");
        } else {
            return fmt::format("ldrh{} {}, [{}], {}{}{}",
                               CondToString(cond), t, n, sign, m,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRHT() { return "ice"; }
    std::string arm_LDRSB_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
        const bool P = true;
        const bool W = false;
        return arm_LDRSB_imm(cond, P, U, W, Reg::PC, t, imm8a, imm8b);
    }
    std::string arm_LDRSB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
        const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldrsb{} {}, [{}, #{}{}]{}",
                               CondToString(cond), t, n, sign, imm32,
                               W ? "!" : "");
        } else {
            return fmt::format("ldrsb{} {}, [{}], #{}{}{}",
                               CondToString(cond), t, n, sign, imm32,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRSB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldrsb{} {}, [{}, {}{}]{}",
                               CondToString(cond), t, n, sign, m,
                               W ? "!" : "");
        } else {
            return fmt::format("ldrsb{} {}, [{}], {}{}{}",
                               CondToString(cond), t, n, sign, m,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRSBT() { return "ice"; }
    std::string arm_LDRSH_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
        const bool P = true;
        const bool W = false;
        return arm_LDRSH_imm(cond, P, U, W, Reg::PC, t, imm8a, imm8b);
    }
    std::string arm_LDRSH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
        const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldrsh{} {}, [{}, #{}{}]{}",
                               CondToString(cond), t, n, sign, imm32,
                               W ? "!" : "");
        } else {
            return fmt::format("ldrsh{} {}, [{}], #{}{}{}",
                               CondToString(cond), t, n, sign, imm32,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRSH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("ldrsh{} {}, [{}, {}{}]{}",
                               CondToString(cond), t, n, sign, m,
                               W ? "!" : "");
        } else {
            return fmt::format("ldrsh{} {}, [{}], {}{}{}",
                               CondToString(cond), t, n, sign, m,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRSHT() { return "ice"; }
    std::string arm_LDRT() { return "ice"; }
    std::string arm_STR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
        const u32 imm12_value = imm12.ZeroExtend();
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("str{} {}, [{}, #{}{}]{}",
                               CondToString(cond), t, n, sign, imm12_value,
                               W ? "!" : "");
        } else {
            return fmt::format("str{} {}, [{}], #{}{}{}",
                               CondToString(cond), t, n, sign, imm12_value,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("str{} {}, [{}, {}{}{}]{}",
                               CondToString(cond), t, n, sign, m,
                               ShiftStr(shift, imm5), W ? "!" : "");
        } else {
            return fmt::format("str{} {}, [{}], {}{}{}{}",
                               CondToString(cond), t, n, sign, m,
                               ShiftStr(shift, imm5), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
        const u32 imm12_value = imm12.ZeroExtend();
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("strb{} {}, [{}, #{}{}]{}",
                               CondToString(cond), t, n, sign, imm12_value,
                               W ? "!" : "");
        } else {
            return fmt::format("strb{} {}, [{}], #{}{}{}",
                               CondToString(cond), t, n, sign, imm12_value,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("strb{} {}, [{}, {}{}{}]{}",
                               CondToString(cond), t, n, sign, m,
                               ShiftStr(shift, imm5), W ? "!" : "");
        } else {
            return fmt::format("strb{} {}, [{}], {}{}{}{}",
                               CondToString(cond), t, n, sign, m,
                               ShiftStr(shift, imm5), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRBT() { return "ice"; }
    std::string arm_STRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
        const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("strd{} {}, {}, [{}, #{}{}]{}",
                               CondToString(cond), t, t + 1, n, sign, imm32,
                               W ? "!" : "");
        } else {
            return fmt::format("strd{} {}, {}, [{}], #{}{}{}",
                               CondToString(cond), t, t + 1, n, sign, imm32,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("strd{} {}, {}, [{}, {}{}]{}",
                               CondToString(cond), t, t + 1, n, sign, m,
                               W ? "!" : "");
        } else {
            return fmt::format("strd{} {}, {}, [{}], {}{}{}",
                               CondToString(cond), t, t + 1, n, sign, m,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
        const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("strh{} {}, [{}, #{}{}]{}",
                               CondToString(cond), t, n, sign, imm32,
                               W ? "!" : "");
        } else {
            return fmt::format("strh{} {}, [{}], #{}{}{}",
                               CondToString(cond), t, n, sign, imm32,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        const char sign = U ? '+' : '-';

        if (P) {
            return fmt::format("strd{} {}, [{}, {}{}]{}",
                               CondToString(cond), t, n, sign, m,
                               W ? "!" : "");
        } else {
            return fmt::format("strd{} {}, [{}], {}{}{}",
                               CondToString(cond), t, n, sign, m,
                               W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRHT() { return "ice"; }
    std::string arm_STRT() { return "ice"; }

    // Load/Store multiple instructions
    std::string arm_LDM(Cond cond, bool W, Reg n, RegList list) {
        return fmt::format("ldm{} {}{}, {{{}}}", CondToString(cond), n, W ? "!" : "", RegListToString(list));
    }
    std::string arm_LDMDA(Cond cond, bool W, Reg n, RegList list) {
        return fmt::format("ldmda{} {}{}, {{{}}}", CondToString(cond), n, W ? "!" : "", RegListToString(list));
    }
    std::string arm_LDMDB(Cond cond, bool W, Reg n, RegList list) {
        return fmt::format("ldmdb{} {}{}, {{{}}}", CondToString(cond), n, W ? "!" : "", RegListToString(list));
    }
    std::string arm_LDMIB(Cond cond, bool W, Reg n, RegList list) {
        return fmt::format("ldmib{} {}{}, {{{}}}", CondToString(cond), n, W ? "!" : "", RegListToString(list));
    }
    std::string arm_LDM_usr() { return "ice"; }
    std::string arm_LDM_eret() { return "ice"; }
    std::string arm_STM(Cond cond, bool W, Reg n, RegList list) {
        return fmt::format("stm{} {}{}, {{{}}}", CondToString(cond), n, W ? "!" : "", RegListToString(list));
    }
    std::string arm_STMDA(Cond cond, bool W, Reg n, RegList list) {
        return fmt::format("stmda{} {}{}, {{{}}}", CondToString(cond), n, W ? "!" : "", RegListToString(list));
    }
    std::string arm_STMDB(Cond cond, bool W, Reg n, RegList list) {
        return fmt::format("stmdb{} {}{}, {{{}}}", CondToString(cond), n, W ? "!" : "", RegListToString(list));
    }
    std::string arm_STMIB(Cond cond, bool W, Reg n, RegList list) {
        return fmt::format("stmib{} {}{}, {{{}}}", CondToString(cond), n, W ? "!" : "", RegListToString(list));
    }
    std::string arm_STM_usr() { return "ice"; }

    // Miscellaneous instructions
    std::string arm_BFC(Cond cond, Imm<5> msb, Reg d, Imm<5> lsb) {
        const u32 lsb_value = lsb.ZeroExtend();
        const u32 width = msb.ZeroExtend() - lsb_value + 1;
        return fmt::format("bfc{} {}, #{}, #{}",
                           CondToString(cond), d, lsb_value, width);
    }
    std::string arm_BFI(Cond cond, Imm<5> msb, Reg d, Imm<5> lsb, Reg n) {
        const u32 lsb_value = lsb.ZeroExtend();
        const u32 width = msb.ZeroExtend() - lsb_value + 1;
        return fmt::format("bfi{} {}, {}, #{}, #{}",
                           CondToString(cond), d, n, lsb_value, width);
    }
    std::string arm_CLZ(Cond cond, Reg d, Reg m) {
        return fmt::format("clz{} {}, {}", CondToString(cond), d, m);
    }
    std::string arm_MOVT(Cond cond, Imm<4> imm4, Reg d, Imm<12> imm12) {
        const u32 imm = concatenate(imm4, imm12).ZeroExtend();
        return fmt::format("movt{} {}, #{}", CondToString(cond), d, imm);
    }
    std::string arm_MOVW(Cond cond, Imm<4> imm4, Reg d, Imm<12> imm12) {
        const u32 imm = concatenate(imm4, imm12).ZeroExtend();
        return fmt::format("movw{}, {}, #{}", CondToString(cond), d, imm);
    }
    std::string arm_NOP() {
        return "nop";
    }
    std::string arm_RBIT(Cond cond, Reg d, Reg m) {
        return fmt::format("rbit{} {}, {}", CondToString(cond), d, m);
    }
    std::string arm_SBFX(Cond cond, Imm<5> widthm1, Reg d, Imm<5> lsb, Reg n) {
        const u32 lsb_value = lsb.ZeroExtend();
        const u32 width = widthm1.ZeroExtend() + 1;
        return fmt::format("sbfx{} {}, {}, #{}, #{}",
                           CondToString(cond), d, n, lsb_value, width);
    }
    std::string arm_SEL(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("sel{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UBFX(Cond cond, Imm<5> widthm1, Reg d, Imm<5> lsb, Reg n) {
        const u32 lsb_value = lsb.ZeroExtend();
        const u32 width = widthm1.ZeroExtend() + 1;
        return fmt::format("ubfx{} {}, {}, #{}, #{}",
                           CondToString(cond), d, n, lsb_value, width);
    }

    // Unsigned sum of absolute difference functions
    std::string arm_USAD8(Cond cond, Reg d, Reg m, Reg n) {
        return fmt::format("usad8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_USADA8(Cond cond, Reg d, Reg a, Reg m, Reg n) {
        return fmt::format("usad8a{} {}, {}, {}, {}", CondToString(cond), d, n, m, a);
    }

    // Packing instructions
    std::string arm_PKHBT(Cond cond, Reg n, Reg d, Imm<5> imm5, Reg m) {
        return fmt::format("pkhbt{} {}, {}, {}{}", CondToString(cond), d, n, m, ShiftStr(ShiftType::LSL, imm5));
    }
    std::string arm_PKHTB(Cond cond, Reg n, Reg d, Imm<5> imm5, Reg m) {
        return fmt::format("pkhtb{} {}, {}, {}{}", CondToString(cond), d, n, m, ShiftStr(ShiftType::ASR, imm5));
    }

    // Reversal instructions
    std::string arm_REV(Cond cond, Reg d, Reg m) {
        return fmt::format("rev{} {}, {}", CondToString(cond), d, m);
    }
    std::string arm_REV16(Cond cond, Reg d, Reg m) {
        return fmt::format("rev16{} {}, {}", CondToString(cond), d, m);
    }
    std::string arm_REVSH(Cond cond, Reg d, Reg m) {
        return fmt::format("revsh{} {}, {}", CondToString(cond), d, m);
    }

    // Saturation instructions
    std::string arm_SSAT(Cond cond, Imm<5> sat_imm, Reg d, Imm<5> imm5, bool sh, Reg n) {
        const u32 bit_position = sat_imm.ZeroExtend() + 1;
        return fmt::format("ssat{} {}, #{}, {}{}",
                           CondToString(cond), d, bit_position, n,
                           ShiftStr(ShiftType(sh << 1), imm5));
    }
    std::string arm_SSAT16(Cond cond, Imm<4> sat_imm, Reg d, Reg n) {
        const u32 bit_position = sat_imm.ZeroExtend() + 1;
        return fmt::format("ssat16{} {}, #{}, {}",
                           CondToString(cond), d, bit_position, n);
    }
    std::string arm_USAT(Cond cond, Imm<5> sat_imm, Reg d, Imm<5> imm5, bool sh, Reg n) {
        return fmt::format("usat{} {}, #{}, {}{}",
                           CondToString(cond), d, sat_imm.ZeroExtend(), n,
                           ShiftStr(ShiftType(sh << 1), imm5));
    }
    std::string arm_USAT16(Cond cond, Imm<4> sat_imm, Reg d, Reg n) {
        return fmt::format("usat16{} {}, #{}, {}",
                           CondToString(cond), d, sat_imm.ZeroExtend(), n);
    }

    // Divide instructions
    std::string arm_SDIV(Cond cond, Reg d, Reg m, Reg n) {
        return fmt::format("sdiv{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UDIV(Cond cond, Reg d, Reg m, Reg n) {
        return fmt::format("udiv{} {}, {}, {}", CondToString(cond), d, n, m);
    }

    // Multiply (Normal) instructions
    std::string arm_MLA(Cond cond, bool S, Reg d, Reg a, Reg m, Reg n) {
        return fmt::format("mla{}{} {}, {}, {}, {}", S ? "s" : "", CondToString(cond), d, n, m, a);
    }
    std::string arm_MLS(Cond cond, Reg d, Reg a, Reg m, Reg n) {
        return fmt::format("mls{} {}, {}, {}, {}", CondToString(cond), d, n, m, a);
    }
    std::string arm_MUL(Cond cond, bool S, Reg d, Reg m, Reg n) {
        return fmt::format("mul{}{} {}, {}, {}", S ? "s" : "", CondToString(cond), d, n, m);
    }

    // Multiply (Long) instructions
    std::string arm_SMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
        return fmt::format("smlal{}{} {}, {}, {}, {}", S ? "s" : "", CondToString(cond), dLo, dHi, n, m);
    }
    std::string arm_SMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
        return fmt::format("smull{}{} {}, {}, {}, {}", S ? "s" : "", CondToString(cond), dLo, dHi, n, m);
    }
    std::string arm_UMAAL(Cond cond, Reg dHi, Reg dLo, Reg m, Reg n) {
        return fmt::format("umaal{} {}, {}, {}, {}", CondToString(cond), dLo, dHi, n, m);
    }
    std::string arm_UMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
        return fmt::format("umlal{}{} {}, {}, {}, {}", S ? "s" : "", CondToString(cond), dLo, dHi, n, m);
    }
    std::string arm_UMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
        return fmt::format("umull{}{} {}, {}, {}, {}", S ? "s" : "", CondToString(cond), dLo, dHi, n, m);
    }

    // Multiply (Halfword) instructions
    std::string arm_SMLALxy(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, bool N, Reg n) {
        return fmt::format("smlal{}{}{} {}, {}, {}, {}", N ? 't' : 'b', M ? 't' : 'b', CondToString(cond), dLo, dHi, n, m);
    }
    std::string arm_SMLAxy(Cond cond, Reg d, Reg a, Reg m, bool M, bool N, Reg n) {
        return fmt::format("smla{}{}{} {}, {}, {}, {}", N ? 't' : 'b', M ? 't' : 'b', CondToString(cond), d, n, m, a);
    }
    std::string arm_SMULxy(Cond cond, Reg d, Reg m, bool M, bool N, Reg n) {
        return fmt::format("smul{}{}{} {}, {}, {}", N ? 't' : 'b', M ? 't' : 'b', CondToString(cond), d, n, m);
    }

    // Multiply (word by halfword) instructions
    std::string arm_SMLAWy(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
        return fmt::format("smlaw{}{} {}, {}, {}, {}", M ? 't' : 'b', CondToString(cond), d, n, m, a);
    }
    std::string arm_SMULWy(Cond cond, Reg d, Reg m, bool M, Reg n) {
        return fmt::format("smulw{}{} {}, {}, {}", M ? 't' : 'b', CondToString(cond), d, n, m);
    }

    // Multiply (Most significant word) instructions
    std::string arm_SMMLA(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n) {
        return fmt::format("smmla{}{} {}, {}, {}, {}", R ? "r" : "", CondToString(cond), d, n, m, a);
    }
    std::string arm_SMMLS(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n) {
        return fmt::format("smmls{}{} {}, {}, {}, {}", R ? "r" : "", CondToString(cond), d, n, m, a);
    }
    std::string arm_SMMUL(Cond cond, Reg d, Reg m, bool R, Reg n) {
        return fmt::format("smmul{}{} {}, {}, {}", R ? "r" : "", CondToString(cond), d, n, m);
    }

    // Multiply (Dual) instructions
    std::string arm_SMLAD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
        return fmt::format("smlad{}{} {}, {}, {}, {}", M ? "x" : "", CondToString(cond), d, n, m, a);
    }
    std::string arm_SMLALD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n) {
        return fmt::format("smlald{}{} {}, {}, {}, {}", M ? "x" : "", CondToString(cond), dLo, dHi, n, m);
    }
    std::string arm_SMLSD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
        return fmt::format("smlsd{}{} {}, {}, {}, {}", M ? "x" : "", CondToString(cond), d, n, m, a);
    }
    std::string arm_SMLSLD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n) {
        return fmt::format("smlsld{}{} {}, {}, {}, {}", M ? "x" : "", CondToString(cond), dLo, dHi, n, m);
    }
    std::string arm_SMUAD(Cond cond, Reg d, Reg m, bool M, Reg n) {
        return fmt::format("smuad{}{} {}, {}, {}", M ? "x" : "", CondToString(cond), d, n, m);
    }
    std::string arm_SMUSD(Cond cond, Reg d, Reg m, bool M, Reg n) {
        return fmt::format("smusd{}{} {}, {}, {}", M ? "x" : "", CondToString(cond), d, n, m);
    }

    // Parallel Add/Subtract (Modulo arithmetic) instructions
    std::string arm_SADD8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("sadd8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_SADD16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("sadd16{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_SASX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("sasx{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_SSAX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("ssax{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_SSUB8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("ssub8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_SSUB16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("ssub16{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UADD8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uadd8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UADD16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uadd16{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UASX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uasx{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_USAX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("usax{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_USUB8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("usub8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_USUB16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("usub16{} {}, {}, {}", CondToString(cond), d, n, m);
    }

    // Parallel Add/Subtract (Saturating) instructions
    std::string arm_QADD8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("qadd8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_QADD16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("qadd16{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_QASX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("qasx{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_QSAX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("qsax{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_QSUB8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("qsub8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_QSUB16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("qsub16{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UQADD8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uqadd8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UQADD16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uqadd16{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UQASX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uqasx{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UQSAX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uqsax{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UQSUB8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uqsub8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UQSUB16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uqsub16{} {}, {}, {}", CondToString(cond), d, n, m);
    }

    // Parallel Add/Subtract (Halving) instructions
    std::string arm_SHADD8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("shadd8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_SHADD16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("shadd16{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_SHASX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("shasx{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_SHSAX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("shsax{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_SHSUB8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("shsub8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_SHSUB16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("shsub16{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UHADD8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uhadd8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UHADD16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uhadd16{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UHASX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uhasx{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UHSAX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uhsax{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UHSUB8(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uhsub8{} {}, {}, {}", CondToString(cond), d, n, m);
    }
    std::string arm_UHSUB16(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("uhsub16{} {}, {}, {}", CondToString(cond), d, n, m);
    }

    // Saturated Add/Subtract instructions
    std::string arm_QADD(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("qadd{} {}, {}, {}", CondToString(cond), d, m, n);
    }
    std::string arm_QSUB(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("qsub{} {}, {}, {}", CondToString(cond), d, m, n);
    }
    std::string arm_QDADD(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("qdadd{} {}, {}, {}", CondToString(cond), d, m, n);
    }
    std::string arm_QDSUB(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("qdsub{} {}, {}, {}", CondToString(cond), d, m, n);
    }

    // Synchronization Primitive instructions
    std::string arm_CLREX() {
        return "clrex";
    }
    std::string arm_SWP(Cond cond, Reg n, Reg t, Reg t2) {
        return fmt::format("swp{} {}, {}, [{}]", CondToString(cond), t, t2, n);
    }
    std::string arm_SWPB(Cond cond, Reg n, Reg t, Reg t2) {
        return fmt::format("swpb{} {}, {}, [{}]", CondToString(cond), t, t2, n);
    }
    std::string arm_LDA(Cond cond, Reg n, Reg t) {
        return fmt::format("lda{} {}, [{}]", CondToString(cond), t, n);
    }
    std::string arm_LDAB(Cond cond, Reg n, Reg t) {
        return fmt::format("ldab{} {}, [{}]", CondToString(cond), t, n);
    }
    std::string arm_LDAH(Cond cond, Reg n, Reg t) {
        return fmt::format("ldah{} {}, [{}]", CondToString(cond), t, n);
    }
    std::string arm_LDAEX(Cond cond, Reg n, Reg t) {
        return fmt::format("ldaex{} {}, [{}]", CondToString(cond), t, n);
    }
    std::string arm_LDAEXB(Cond cond, Reg n, Reg t) {
        return fmt::format("ldaexb{} {}, [{}]", CondToString(cond), t, n);
    }
    std::string arm_LDAEXD(Cond cond, Reg n, Reg t) {
        return fmt::format("ldaexd{} {}, {}, [{}]", CondToString(cond), t, t + 1, n);
    }
    std::string arm_LDAEXH(Cond cond, Reg n, Reg t) {
        return fmt::format("ldaexh{} {}, [{}]", CondToString(cond), t, n);
    }
    std::string arm_STL(Cond cond, Reg n, Reg t) {
        return fmt::format("stl{} {}, [{}]", CondToString(cond), t, n);
    }
    std::string arm_STLB(Cond cond, Reg n, Reg t) {
        return fmt::format("stlb{} {}, [{}]", CondToString(cond), t, n);
    }
    std::string arm_STLH(Cond cond, Reg n, Reg t) {
        return fmt::format("stlh{} {}, [{}]", CondToString(cond), t, n);
    }
    std::string arm_STLEX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("stlex{} {}, {}, [{}]", CondToString(cond), d, m, n);
    }
    std::string arm_STLEXB(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("stlexb{} {}, {}, [{}]", CondToString(cond), d, m, n);
    }
    std::string arm_STLEXD(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("stlexd{} {}, {}, {}, [{}]", CondToString(cond), d, m, m + 1, n);
    }
    std::string arm_STLEXH(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("stlexh{} {}, {}, [{}]", CondToString(cond), d, m, n);
    }
    std::string arm_LDREX(Cond cond, Reg n, Reg d) {
        return fmt::format("ldrex{} {}, [{}]", CondToString(cond), d, n);
    }
    std::string arm_LDREXB(Cond cond, Reg n, Reg d) {
        return fmt::format("ldrexb{} {}, [{}]", CondToString(cond), d, n);
    }
    std::string arm_LDREXD(Cond cond, Reg n, Reg d) {
        return fmt::format("ldrexd{} {}, {}, [{}]", CondToString(cond), d, d + 1, n);
    }
    std::string arm_LDREXH(Cond cond, Reg n, Reg d) {
        return fmt::format("ldrexh{} {}, [{}]", CondToString(cond), d, n);
    }
    std::string arm_STREX(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("strex{} {}, {}, [{}]", CondToString(cond), d, m, n);
    }
    std::string arm_STREXB(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("strexb{} {}, {}, [{}]", CondToString(cond), d, m, n);
    }
    std::string arm_STREXD(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("strexd{} {}, {}, {}, [{}]", CondToString(cond), d, m, m + 1, n);
    }
    std::string arm_STREXH(Cond cond, Reg n, Reg d, Reg m) {
        return fmt::format("strexh{} {}, {}, [{}]", CondToString(cond), d, m, n);
    }

    // Status register access instructions
    std::string arm_CPS() { return "ice"; }
    std::string arm_MRS(Cond cond, Reg d) {
        return fmt::format("mrs{} {}, apsr", CondToString(cond), d);
    }
    std::string arm_MSR_imm(Cond cond, int mask, int rotate, Imm<8> imm8) {
        const bool write_c = Common::Bit<0>(mask);
        const bool write_x = Common::Bit<1>(mask);
        const bool write_s = Common::Bit<2>(mask);
        const bool write_f = Common::Bit<3>(mask);
        return fmt::format("msr{} cpsr_{}{}{}{}, #{}",
                           CondToString(cond),
                           write_c ? "c" : "",
                           write_x ? "x" : "",
                           write_s ? "s" : "",
                           write_f ? "f" : "",
                           ArmExpandImm(rotate, imm8));
    }
    std::string arm_MSR_reg(Cond cond, int mask, Reg n) {
        const bool write_c = Common::Bit<0>(mask);
        const bool write_x = Common::Bit<1>(mask);
        const bool write_s = Common::Bit<2>(mask);
        const bool write_f = Common::Bit<3>(mask);
        return fmt::format("msr{} cpsr_{}{}{}{}, {}",
                           CondToString(cond),
                           write_c ? "c" : "",
                           write_x ? "x" : "",
                           write_s ? "s" : "",
                           write_f ? "f" : "",
                           n);
    }
    std::string arm_RFE() { return "ice"; }
    std::string arm_SETEND(bool E) {
        return E ? "setend be" : "setend le";
    }
    std::string arm_SRS() { return "ice"; }

    // Floating point arithmetic instructions
    std::string vfp_VADD(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vadd{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VSUB(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vsub{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VMUL(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vmul{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VMLA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vmla{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VMLS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vmls{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VNMUL(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vnmul{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VNMLA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vnmla{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VNMLS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vnmls{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VDIV(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vdiv{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VFNMS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vfnms{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VFNMA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vfnma{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VFMS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vfms{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VFMA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vfma{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VSEL(bool D, Imm<2> cc, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        const Cond cond = concatenate(cc, Imm<1>{cc.Bit<0>() != cc.Bit<1>()}, Imm<1>{0}).ZeroExtend<Cond>();
        return fmt::format("vsel{}.{} {}, {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VMAXNM(bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vmaxnm.{} {}, {}, {}", sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VMINNM(bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return fmt::format("vminnm.{} {}, {}, {}", sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vn, N), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VMOV_imm(Cond cond, bool D, Imm<4> imm4H, size_t Vd, bool sz, Imm<4> imm4L) {
        const auto imm8 = concatenate(imm4H, imm4L);

        if (sz) {
            const u64 sign = static_cast<u64>(imm8.Bit<7>());
            const u64 exp = (imm8.Bit<6>() ? 0x3FC : 0x400) | imm8.Bits<4, 5, u64>();
            const u64 fract = imm8.Bits<0, 3, u64>() << 48;
            const u64 immediate = (sign << 63) | (exp << 52) | fract;
            return fmt::format("vmov{}.f64 {}, #0x{:016x}", CondToString(cond), FPRegStr(sz, Vd, D), immediate);
        } else {
            const u32 sign = static_cast<u32>(imm8.Bit<7>());
            const u32 exp = (imm8.Bit<6>() ? 0x7C : 0x80) | imm8.Bits<4, 5>();
            const u32 fract = imm8.Bits<0, 3>() << 19;
            const u32 immediate = (sign << 31) | (exp << 23) | fract;
            return fmt::format("vmov{}.f32 {}, #0x{:08x}", CondToString(cond), FPRegStr(sz, Vd, D), immediate);
        }
    }

    std::string vfp_VMOV_u32_f64(Cond cond, size_t Vd, Reg t, bool D) {
        return fmt::format("vmov{}.32 {}, {}", CondToString(cond), FPRegStr(true, Vd, D), t);
    }

    std::string vfp_VMOV_f64_u32(Cond cond, size_t Vn, Reg t, bool N) {
        return fmt::format("vmov{}.32 {}, {}", CondToString(cond), t, FPRegStr(true, Vn, N));
    }

    std::string vfp_VMOV_u32_f32(Cond cond, size_t Vn, Reg t, bool N) {
        return fmt::format("vmov{}.32 {}, {}", CondToString(cond), FPRegStr(false, Vn, N), t);
    }

    std::string vfp_VMOV_f32_u32(Cond cond, size_t Vn, Reg t, bool N) {
        return fmt::format("vmov{}.32 {}, {}", CondToString(cond), t, FPRegStr(false, Vn, N));
    }

    std::string vfp_VMOV_2u32_2f32(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
        return fmt::format("vmov{} {}, {}, {}, {}", CondToString(cond), FPRegStr(false, Vm, M), FPNextRegStr(false, Vm, M), t, t2);
    }

    std::string vfp_VMOV_2f32_2u32(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
        return fmt::format("vmov{} {}, {}, {}, {}", CondToString(cond), t, t2, FPRegStr(false, Vm, M), FPNextRegStr(false, Vm, M));
    }

    std::string vfp_VMOV_2u32_f64(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
        return fmt::format("vmov{} {}, {}, {}", CondToString(cond), FPRegStr(true, Vm, M), t, t2);
    }

    std::string vfp_VMOV_f64_2u32(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
        return fmt::format("vmov{} {}, {}, {}", CondToString(cond), t, t2, FPRegStr(true, Vm, M));
    }

    std::string vfp_VMOV_from_i32(Cond cond, Imm<1> i, size_t Vd, Reg t, bool D) {
        const size_t index = i.ZeroExtend();
        return fmt::format("vmov{}.32 {}[{}], {}", CondToString(cond), FPRegStr(true, Vd, D), index, t);
    }

    std::string vfp_VMOV_from_i16(Cond cond, Imm<1> i1, size_t Vd, Reg t, bool D, Imm<1> i2) {
        const size_t index = concatenate(i1, i2).ZeroExtend();
        return fmt::format("vmov{}.16 {}[{}], {}", CondToString(cond), FPRegStr(true, Vd, D), index, t);
    }

    std::string vfp_VMOV_from_i8(Cond cond, Imm<1> i1, size_t Vd, Reg t, bool D, Imm<2> i2) {
        const size_t index = concatenate(i1, i2).ZeroExtend();
        return fmt::format("vmov{}.8 {}[{}], {}", CondToString(cond), FPRegStr(true, Vd, D), index, t);
    }

    std::string vfp_VMOV_to_i32(Cond cond, Imm<1> i, size_t Vn, Reg t, bool N) {
        const size_t index = i.ZeroExtend();
        return fmt::format("vmov{}.32 {}, {}[{}]", CondToString(cond), t, FPRegStr(true, Vn, N), index);
    }

    std::string vfp_VMOV_to_i16(Cond cond, bool U, Imm<1> i1, size_t Vn, Reg t, bool N, Imm<1> i2) {
        const size_t index = concatenate(i1, i2).ZeroExtend();
        return fmt::format("vmov{}.{}16 {}, {}[{}]", CondToString(cond), U ? 'u' : 's', t, FPRegStr(true, Vn, N), index);
    }

    std::string vfp_VMOV_to_i8(Cond cond, bool U, Imm<1> i1, size_t Vn, Reg t, bool N, Imm<2> i2) {
        const size_t index = concatenate(i1, i2).ZeroExtend();
        return fmt::format("vmov{}.{}8 {}, {}[{}]", CondToString(cond), U ? 'u' : 's', t, FPRegStr(true, Vn, N), index);
    }

    std::string vfp_VDUP(Cond cond, Imm<1> B, bool Q, size_t Vd, Reg t, bool D, Imm<1> E) {
        const size_t esize = 32u >> concatenate(B, E).ZeroExtend();
        return fmt::format("vdup{}.{} {}, {}", CondToString(cond), esize, VectorStr(Q, Vd, D), t);
    }

    std::string vfp_VMOV_reg(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
        return fmt::format("vmov{}.{} {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VABS(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
        return fmt::format("vadd{}.{} {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VNEG(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
        return fmt::format("vneg{}.{} {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VSQRT(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
        return fmt::format("vsqrt{}.{} {}, {}", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VCVTB(Cond cond, bool D, bool op, size_t Vd, bool sz, bool M, size_t Vm) {
        const bool convert_from_half = !op;
        const char* const to = convert_from_half ? (sz ? "f64" : "f32") : "f16";
        const char* const from = convert_from_half ? "f16" : (sz ? "f64" : "f32");
        return fmt::format("vcvtb{}.{}.{} {}, {}", CondToString(cond), to, from, FPRegStr(convert_from_half ? sz : false, Vd, D), FPRegStr(convert_from_half ? false : sz, Vm, M));
    }

    std::string vfp_VCVTT(Cond cond, bool D, bool op, size_t Vd, bool sz, bool M, size_t Vm) {
        const bool convert_from_half = !op;
        const char* const to = convert_from_half ? (sz ? "f64" : "f32") : "f16";
        const char* const from = convert_from_half ? "f16" : (sz ? "f64" : "f32");
        return fmt::format("vcvtt{}.{}.{} {}, {}", CondToString(cond), to, from, FPRegStr(convert_from_half ? sz : false, Vd, D), FPRegStr(convert_from_half ? false : sz, Vm, M));
    }

    std::string vfp_VCMP(Cond cond, bool D, size_t Vd, bool sz, bool E, bool M, size_t Vm) {
        return fmt::format("vcmp{}{}.{} {}, {}", E ? "e" : "", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VCMP_zero(Cond cond, bool D, size_t Vd, bool sz, bool E) {
        return fmt::format("vcmp{}{}.{} {}, #0.0", E ? "e" : "", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D));
    }

    std::string vfp_VRINTR(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
        return fmt::format("vrintr{} {}, {}", CondToString(cond), FPRegStr(sz, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VRINTZ(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
        return fmt::format("vrintz{} {}, {}", CondToString(cond), FPRegStr(sz, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VRINTX(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
        return fmt::format("vrintx{} {}, {}", CondToString(cond), FPRegStr(sz, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VCVT_f_to_f(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
        return fmt::format("vcvt{}.{}.{} {}, {}", CondToString(cond), !sz ? "f64" : "f32", sz ? "f64" : "f32", FPRegStr(!sz, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VCVT_from_int(Cond cond, bool D, size_t Vd, bool sz, bool is_signed, bool M, size_t Vm) {
        return fmt::format("vcvt{}.{}.{} {}, {}", CondToString(cond), sz ? "f64" : "f32", is_signed ? "s32" : "u32", FPRegStr(sz, Vd, D), FPRegStr(false, Vm, M));
    }

    std::string vfp_VCVT_from_fixed(Cond cond, bool D, bool U, size_t Vd, bool sz, bool sx, Imm<1> i, Imm<4> imm4) {
        const size_t size = sx ? 32 : 16;
        const size_t fbits = size - concatenate(imm4, i).ZeroExtend();
        return fmt::format("vcvt{}.{}.{}{} {}, {}, #{}", CondToString(cond), sz ? "f64" : "f32", U ? 'u' : 's', size, FPRegStr(sz, Vd, D), FPRegStr(sz, Vd, D), fbits);
    }

    std::string vfp_VCVT_to_u32(Cond cond, bool D, size_t Vd, bool sz, bool round_towards_zero, bool M, size_t Vm) {
        return fmt::format("vcvt{}{}.u32.{} {}, {}", round_towards_zero ? "" : "r", CondToString(cond), sz ? "f64" : "f32", FPRegStr(false, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VCVT_to_s32(Cond cond, bool D, size_t Vd, bool sz, bool round_towards_zero, bool M, size_t Vm) {
        return fmt::format("vcvt{}{}.s32.{} {}, {}", round_towards_zero ? "" : "r", CondToString(cond), sz ? "f64" : "f32", FPRegStr(false, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VCVT_to_fixed(Cond cond, bool D, bool U, size_t Vd, bool sz, bool sx, Imm<1> i, Imm<4> imm4) {
        const size_t size = sx ? 32 : 16;
        const size_t fbits = size - concatenate(imm4, i).ZeroExtend();
        return fmt::format("vcvt{}.{}{}.{} {}, {}, #{}", CondToString(cond), U ? 'u' : 's', size, sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vd, D), fbits);
    }

    std::string vfp_VRINT_rm(bool D, size_t rm, size_t Vd, bool sz, bool M, size_t Vm) {
        return fmt::format("vrint{}.{} {}, {}", "anpm"[rm], sz ? "f64" : "f32", FPRegStr(sz, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VCVT_rm(bool D, size_t rm, size_t Vd, bool sz, bool U, bool M, size_t Vm) {
        return fmt::format("vcvt{}.{}.{} {}, {}", "anpm"[rm], U ? "u32" : "s32", sz ? "f64" : "f32", FPRegStr(false, Vd, D), FPRegStr(sz, Vm, M));
    }

    std::string vfp_VMSR(Cond cond, Reg t) {
        return fmt::format("vmsr{} fpscr, {}", CondToString(cond), t);
    }

    std::string vfp_VMRS(Cond cond, Reg t) {
        if (t == Reg::R15) {
            return fmt::format("vmrs{} apsr_nzcv, fpscr", CondToString(cond));
        } else {
            return fmt::format("vmrs{} {}, fpscr", CondToString(cond), t);
        }
    }

    std::string vfp_VPOP(Cond cond, bool D, size_t Vd, bool sz, Imm<8> imm8) {
        return fmt::format("vpop{} {}(+{})",
                           CondToString(cond), FPRegStr(sz, Vd, D),
                           imm8.ZeroExtend() >> (sz ? 1 : 0));
    }

    std::string vfp_VPUSH(Cond cond, bool D, size_t Vd, bool sz, Imm<8> imm8) {
        return fmt::format("vpush{} {}(+{})",
                           CondToString(cond), FPRegStr(sz, Vd, D),
                           imm8.ZeroExtend() >> (sz ? 1 : 0));
    }

    std::string vfp_VLDR(Cond cond, bool U, bool D, Reg n, size_t Vd, bool sz, Imm<8> imm8) {
        const u32 imm32 = imm8.ZeroExtend() << 2;
        const char sign = U ? '+' : '-';
        return fmt::format("vldr{} {}, [{}, #{}{}]",
                           CondToString(cond), FPRegStr(sz, Vd, D), n, sign, imm32);
    }

    std::string vfp_VSTR(Cond cond, bool U, bool D, Reg n, size_t Vd, bool sz, Imm<8> imm8) {
        const u32 imm32 = imm8.ZeroExtend() << 2;
        const char sign = U ? '+' : '-';
        return fmt::format("vstr{} {}, [{}, #{}{}]",
                           CondToString(cond), FPRegStr(sz, Vd, D), n, sign, imm32);
    }

    std::string vfp_VSTM_a1(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8) {
        const char* mode = "<invalid mode>";
        if (!p && u) {
            mode = "ia";
        }
        if (p && !u) {
            mode = "db";
        }
        return fmt::format("vstm{}{}.f64 {}{}, {}(+{})", mode,
                           CondToString(cond), n, w ? "!" : "",
                           FPRegStr(true, Vd, D), imm8.ZeroExtend());
    }

    std::string vfp_VSTM_a2(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8) {
        const char* mode = "<invalid mode>";
        if (!p && u) {
            mode = "ia";
        }
        if (p && !u) {
            mode = "db";
        }
        return fmt::format("vstm{}{}.f32 {}{}, {}(+{})", mode,
                           CondToString(cond), n, w ? "!" : "",
                           FPRegStr(false, Vd, D), imm8.ZeroExtend());
    }

    std::string vfp_VLDM_a1(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8) {
        const char* mode = "<invalid mode>";
        if (!p && u) {
            mode = "ia";
        }
        if (p && !u) {
            mode = "db";
        }
        return fmt::format("vldm{}{}.f64 {}{}, {}(+{})", mode,
                           CondToString(cond), n, w ? "!" : "",
                           FPRegStr(true, Vd, D), imm8.ZeroExtend());
    }

    std::string vfp_VLDM_a2(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8) {
        const char* mode = "<invalid mode>";
        if (!p && u) {
            mode = "ia";
        }
        if (p && !u) {
            mode = "db";
        }
        return fmt::format("vldm{}{}.f32 {}{}, {}(+{})", mode,
                           CondToString(cond), n, w ? "!" : "",
                           FPRegStr(false, Vd, D), imm8.ZeroExtend());
    }
};

std::string DisassembleArm(u32 instruction) {
    DisassemblerVisitor visitor;
    if (auto vfp_decoder = DecodeVFP<DisassemblerVisitor>(instruction)) {
        return vfp_decoder->get().call(visitor, instruction);
    } else if (auto decoder = DecodeArm<DisassemblerVisitor>(instruction)) {
        return decoder->get().call(visitor, instruction);
    } else {
        return fmt::format("UNKNOWN: {:x}", instruction);
    }
}

}  // namespace Dynarmic::A32
