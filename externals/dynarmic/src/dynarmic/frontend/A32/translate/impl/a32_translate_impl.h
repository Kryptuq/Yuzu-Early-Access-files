/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"
#include "dynarmic/frontend/A32/a32_ir_emitter.h"
#include "dynarmic/frontend/A32/a32_location_descriptor.h"
#include "dynarmic/frontend/A32/a32_types.h"
#include "dynarmic/frontend/A32/translate/a32_translate.h"
#include "dynarmic/frontend/A32/translate/conditional_state.h"
#include "dynarmic/frontend/imm.h"

namespace Dynarmic::A32 {

enum class Exception;

struct TranslatorVisitor final {
    using instruction_return_type = bool;

    explicit TranslatorVisitor(IR::Block& block, LocationDescriptor descriptor, const TranslationOptions& options)
            : ir(block, descriptor, options.arch_version), options(options) {}

    A32::IREmitter ir;
    ConditionalState cond_state = ConditionalState::None;
    TranslationOptions options;

    size_t current_instruction_size;

    bool ArmConditionPassed(Cond cond);
    bool ThumbConditionPassed();
    bool VFPConditionPassed(Cond cond);

    bool InterpretThisInstruction();
    bool UnpredictableInstruction();
    bool UndefinedInstruction();
    bool DecodeError();
    bool RaiseException(Exception exception);

    struct ImmAndCarry {
        u32 imm32;
        IR::U1 carry;
    };

    ImmAndCarry ArmExpandImm_C(int rotate, Imm<8> imm8, IR::U1 carry_in) {
        u32 imm32 = imm8.ZeroExtend();
        auto carry_out = carry_in;
        if (rotate) {
            imm32 = Common::RotateRight<u32>(imm8.ZeroExtend(), rotate * 2);
            carry_out = ir.Imm1(Common::Bit<31>(imm32));
        }
        return {imm32, carry_out};
    }

    u32 ArmExpandImm(int rotate, Imm<8> imm8) {
        return ArmExpandImm_C(rotate, imm8, ir.Imm1(0)).imm32;
    }

    ImmAndCarry ThumbExpandImm_C(Imm<1> i, Imm<3> imm3, Imm<8> imm8, IR::U1 carry_in) {
        const Imm<12> imm12 = concatenate(i, imm3, imm8);
        if (imm12.Bits<10, 11>() == 0) {
            const u32 imm32 = [&] {
                const u32 imm8 = imm12.Bits<0, 7>();
                switch (imm12.Bits<8, 9>()) {
                case 0b00:
                    return imm8;
                case 0b01:
                    return Common::Replicate(imm8, 16);
                case 0b10:
                    return Common::Replicate(imm8 << 8, 16);
                case 0b11:
                    return Common::Replicate(imm8, 8);
                }
                UNREACHABLE();
            }();
            return {imm32, carry_in};
        }
        const u32 imm32 = Common::RotateRight<u32>((1 << 7) | imm12.Bits<0, 6>(), imm12.Bits<7, 11>());
        return {imm32, ir.Imm1(Common::Bit<31>(imm32))};
    }

    u32 ThumbExpandImm(Imm<1> i, Imm<3> imm3, Imm<8> imm8) {
        return ThumbExpandImm_C(i, imm3, imm8, ir.Imm1(0)).imm32;
    }

    // Creates an immediate of the given value
    IR::UAny I(size_t bitsize, u64 value);

    IR::ResultAndCarry<IR::U32> EmitImmShift(IR::U32 value, ShiftType type, Imm<3> imm3, Imm<2> imm2, IR::U1 carry_in);
    IR::ResultAndCarry<IR::U32> EmitImmShift(IR::U32 value, ShiftType type, Imm<5> imm5, IR::U1 carry_in);
    IR::ResultAndCarry<IR::U32> EmitRegShift(IR::U32 value, ShiftType type, IR::U8 amount, IR::U1 carry_in);
    template<typename FnT>
    bool EmitVfpVectorOperation(bool sz, ExtReg d, ExtReg n, ExtReg m, const FnT& fn);
    template<typename FnT>
    bool EmitVfpVectorOperation(bool sz, ExtReg d, ExtReg m, const FnT& fn);

    // Barrier instructions
    bool arm_DMB(Imm<4> option);
    bool arm_DSB(Imm<4> option);
    bool arm_ISB(Imm<4> option);

    // Branch instructions
    bool arm_B(Cond cond, Imm<24> imm24);
    bool arm_BL(Cond cond, Imm<24> imm24);
    bool arm_BLX_imm(bool H, Imm<24> imm24);
    bool arm_BLX_reg(Cond cond, Reg m);
    bool arm_BX(Cond cond, Reg m);
    bool arm_BXJ(Cond cond, Reg m);

    // Coprocessor instructions
    bool arm_CDP(Cond cond, size_t opc1, CoprocReg CRn, CoprocReg CRd, size_t coproc_no, size_t opc2, CoprocReg CRm);
    bool arm_LDC(Cond cond, bool p, bool u, bool d, bool w, Reg n, CoprocReg CRd, size_t coproc_no, Imm<8> imm8);
    bool arm_MCR(Cond cond, size_t opc1, CoprocReg CRn, Reg t, size_t coproc_no, size_t opc2, CoprocReg CRm);
    bool arm_MCRR(Cond cond, Reg t2, Reg t, size_t coproc_no, size_t opc, CoprocReg CRm);
    bool arm_MRC(Cond cond, size_t opc1, CoprocReg CRn, Reg t, size_t coproc_no, size_t opc2, CoprocReg CRm);
    bool arm_MRRC(Cond cond, Reg t2, Reg t, size_t coproc_no, size_t opc, CoprocReg CRm);
    bool arm_STC(Cond cond, bool p, bool u, bool d, bool w, Reg n, CoprocReg CRd, size_t coproc_no, Imm<8> imm8);

    // CRC32 instructions
    bool arm_CRC32(Cond cond, Imm<2> sz, Reg n, Reg d, Reg m);
    bool arm_CRC32C(Cond cond, Imm<2> sz, Reg n, Reg d, Reg m);

    // Data processing instructions
    bool arm_ADC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8);
    bool arm_ADC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_ADC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_ADD_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8);
    bool arm_ADD_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_ADD_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_AND_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8);
    bool arm_AND_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_AND_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_BIC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8);
    bool arm_BIC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_BIC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_CMN_imm(Cond cond, Reg n, int rotate, Imm<8> imm8);
    bool arm_CMN_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_CMN_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m);
    bool arm_CMP_imm(Cond cond, Reg n, int rotate, Imm<8> imm8);
    bool arm_CMP_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_CMP_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m);
    bool arm_EOR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8);
    bool arm_EOR_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_EOR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_MOV_imm(Cond cond, bool S, Reg d, int rotate, Imm<8> imm8);
    bool arm_MOV_reg(Cond cond, bool S, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_MOV_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_MVN_imm(Cond cond, bool S, Reg d, int rotate, Imm<8> imm8);
    bool arm_MVN_reg(Cond cond, bool S, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_MVN_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_ORR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8);
    bool arm_ORR_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_ORR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_RSB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8);
    bool arm_RSB_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_RSB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_RSC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8);
    bool arm_RSC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_RSC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_SBC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8);
    bool arm_SBC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_SBC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_SUB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8);
    bool arm_SUB_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_SUB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_TEQ_imm(Cond cond, Reg n, int rotate, Imm<8> imm8);
    bool arm_TEQ_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_TEQ_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m);
    bool arm_TST_imm(Cond cond, Reg n, int rotate, Imm<8> imm8);
    bool arm_TST_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_TST_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m);

    // Exception generating instructions
    bool arm_BKPT(Cond cond, Imm<12> imm12, Imm<4> imm4);
    bool arm_SVC(Cond cond, Imm<24> imm24);
    bool arm_UDF();

    // Extension instructions
    bool arm_SXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool arm_SXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool arm_SXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool arm_SXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m);
    bool arm_SXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m);
    bool arm_SXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m);
    bool arm_UXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool arm_UXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool arm_UXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool arm_UXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m);
    bool arm_UXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m);
    bool arm_UXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m);

    // Hint instructions
    bool arm_PLD_imm(bool add, bool R, Reg n, Imm<12> imm12);
    bool arm_PLD_reg(bool add, bool R, Reg n, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_SEV();
    bool arm_SEVL();
    bool arm_WFE();
    bool arm_WFI();
    bool arm_YIELD();

    // Load/Store
    bool arm_LDRBT();
    bool arm_LDRHT();
    bool arm_LDRSBT();
    bool arm_LDRSHT();
    bool arm_LDRT();
    bool arm_STRBT();
    bool arm_STRHT();
    bool arm_STRT();
    bool arm_LDR_lit(Cond cond, bool U, Reg t, Imm<12> imm12);
    bool arm_LDR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm<12> imm12);
    bool arm_LDR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_LDRB_lit(Cond cond, bool U, Reg t, Imm<12> imm12);
    bool arm_LDRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12);
    bool arm_LDRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_LDRD_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b);
    bool arm_LDRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b);
    bool arm_LDRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m);
    bool arm_LDRH_lit(Cond cond, bool P, bool U, bool W, Reg t, Imm<4> imm8a, Imm<4> imm8b);
    bool arm_LDRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b);
    bool arm_LDRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m);
    bool arm_LDRSB_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b);
    bool arm_LDRSB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b);
    bool arm_LDRSB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m);
    bool arm_LDRSH_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b);
    bool arm_LDRSH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b);
    bool arm_LDRSH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m);
    bool arm_STR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12);
    bool arm_STR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_STRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12);
    bool arm_STRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m);
    bool arm_STRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b);
    bool arm_STRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m);
    bool arm_STRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b);
    bool arm_STRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m);

    // Load/Store multiple instructions
    bool arm_LDM(Cond cond, bool W, Reg n, RegList list);
    bool arm_LDMDA(Cond cond, bool W, Reg n, RegList list);
    bool arm_LDMDB(Cond cond, bool W, Reg n, RegList list);
    bool arm_LDMIB(Cond cond, bool W, Reg n, RegList list);
    bool arm_LDM_usr();
    bool arm_LDM_eret();
    bool arm_STM(Cond cond, bool W, Reg n, RegList list);
    bool arm_STMDA(Cond cond, bool W, Reg n, RegList list);
    bool arm_STMDB(Cond cond, bool W, Reg n, RegList list);
    bool arm_STMIB(Cond cond, bool W, Reg n, RegList list);
    bool arm_STM_usr();

    // Miscellaneous instructions
    bool arm_BFC(Cond cond, Imm<5> msb, Reg d, Imm<5> lsb);
    bool arm_BFI(Cond cond, Imm<5> msb, Reg d, Imm<5> lsb, Reg n);
    bool arm_CLZ(Cond cond, Reg d, Reg m);
    bool arm_MOVT(Cond cond, Imm<4> imm4, Reg d, Imm<12> imm12);
    bool arm_MOVW(Cond cond, Imm<4> imm4, Reg d, Imm<12> imm12);
    bool arm_NOP() { return true; }
    bool arm_RBIT(Cond cond, Reg d, Reg m);
    bool arm_SBFX(Cond cond, Imm<5> widthm1, Reg d, Imm<5> lsb, Reg n);
    bool arm_SEL(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UBFX(Cond cond, Imm<5> widthm1, Reg d, Imm<5> lsb, Reg n);

    // Unsigned sum of absolute difference functions
    bool arm_USAD8(Cond cond, Reg d, Reg m, Reg n);
    bool arm_USADA8(Cond cond, Reg d, Reg a, Reg m, Reg n);

    // Packing instructions
    bool arm_PKHBT(Cond cond, Reg n, Reg d, Imm<5> imm5, Reg m);
    bool arm_PKHTB(Cond cond, Reg n, Reg d, Imm<5> imm5, Reg m);

    // Reversal instructions
    bool arm_REV(Cond cond, Reg d, Reg m);
    bool arm_REV16(Cond cond, Reg d, Reg m);
    bool arm_REVSH(Cond cond, Reg d, Reg m);

    // Saturation instructions
    bool arm_SSAT(Cond cond, Imm<5> sat_imm, Reg d, Imm<5> imm5, bool sh, Reg n);
    bool arm_SSAT16(Cond cond, Imm<4> sat_imm, Reg d, Reg n);
    bool arm_USAT(Cond cond, Imm<5> sat_imm, Reg d, Imm<5> imm5, bool sh, Reg n);
    bool arm_USAT16(Cond cond, Imm<4> sat_imm, Reg d, Reg n);

    // Divide instructions
    bool arm_SDIV(Cond cond, Reg d, Reg m, Reg n);
    bool arm_UDIV(Cond cond, Reg d, Reg m, Reg n);

    // Multiply (Normal) instructions
    bool arm_MLA(Cond cond, bool S, Reg d, Reg a, Reg m, Reg n);
    bool arm_MLS(Cond cond, Reg d, Reg a, Reg m, Reg n);
    bool arm_MUL(Cond cond, bool S, Reg d, Reg m, Reg n);

    // Multiply (Long) instructions
    bool arm_SMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n);
    bool arm_SMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n);
    bool arm_UMAAL(Cond cond, Reg dHi, Reg dLo, Reg m, Reg n);
    bool arm_UMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n);
    bool arm_UMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n);

    // Multiply (Halfword) instructions
    bool arm_SMLALxy(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, bool N, Reg n);
    bool arm_SMLAxy(Cond cond, Reg d, Reg a, Reg m, bool M, bool N, Reg n);
    bool arm_SMULxy(Cond cond, Reg d, Reg m, bool M, bool N, Reg n);

    // Multiply (word by halfword) instructions
    bool arm_SMLAWy(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n);
    bool arm_SMULWy(Cond cond, Reg d, Reg m, bool M, Reg n);

    // Multiply (Most significant word) instructions
    bool arm_SMMLA(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n);
    bool arm_SMMLS(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n);
    bool arm_SMMUL(Cond cond, Reg d, Reg m, bool R, Reg n);

    // Multiply (Dual) instructions
    bool arm_SMLAD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n);
    bool arm_SMLALD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n);
    bool arm_SMLSD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n);
    bool arm_SMLSLD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n);
    bool arm_SMUAD(Cond cond, Reg d, Reg m, bool M, Reg n);
    bool arm_SMUSD(Cond cond, Reg d, Reg m, bool M, Reg n);

    // Parallel Add/Subtract (Modulo arithmetic) instructions
    bool arm_SADD8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_SADD16(Cond cond, Reg n, Reg d, Reg m);
    bool arm_SASX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_SSAX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_SSUB8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_SSUB16(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UADD8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UADD16(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UASX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_USAX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_USUB8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_USUB16(Cond cond, Reg n, Reg d, Reg m);

    // Parallel Add/Subtract (Saturating) instructions
    bool arm_QADD8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_QADD16(Cond cond, Reg n, Reg d, Reg m);
    bool arm_QASX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_QSAX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_QSUB8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_QSUB16(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UQADD8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UQADD16(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UQASX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UQSAX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UQSUB8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UQSUB16(Cond cond, Reg n, Reg d, Reg m);

    // Parallel Add/Subtract (Halving) instructions
    bool arm_SHADD8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_SHADD16(Cond cond, Reg n, Reg d, Reg m);
    bool arm_SHASX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_SHSAX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_SHSUB8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_SHSUB16(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UHADD8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UHADD16(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UHASX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UHSAX(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UHSUB8(Cond cond, Reg n, Reg d, Reg m);
    bool arm_UHSUB16(Cond cond, Reg n, Reg d, Reg m);

    // Saturated Add/Subtract instructions
    bool arm_QADD(Cond cond, Reg n, Reg d, Reg m);
    bool arm_QSUB(Cond cond, Reg n, Reg d, Reg m);
    bool arm_QDADD(Cond cond, Reg n, Reg d, Reg m);
    bool arm_QDSUB(Cond cond, Reg n, Reg d, Reg m);

    // Synchronization Primitive instructions
    bool arm_CLREX();
    bool arm_SWP(Cond cond, Reg n, Reg t, Reg t2);
    bool arm_SWPB(Cond cond, Reg n, Reg t, Reg t2);
    bool arm_STL(Cond cond, Reg n, Reg t);
    bool arm_STLEX(Cond cond, Reg n, Reg d, Reg t);
    bool arm_STREX(Cond cond, Reg n, Reg d, Reg t);
    bool arm_LDA(Cond cond, Reg n, Reg t);
    bool arm_LDAEX(Cond cond, Reg n, Reg t);
    bool arm_LDREX(Cond cond, Reg n, Reg t);
    bool arm_STLEXD(Cond cond, Reg n, Reg d, Reg t);
    bool arm_STREXD(Cond cond, Reg n, Reg d, Reg t);
    bool arm_LDAEXD(Cond cond, Reg n, Reg t);
    bool arm_LDREXD(Cond cond, Reg n, Reg t);
    bool arm_STLB(Cond cond, Reg n, Reg t);
    bool arm_STLEXB(Cond cond, Reg n, Reg d, Reg t);
    bool arm_STREXB(Cond cond, Reg n, Reg d, Reg t);
    bool arm_LDAB(Cond cond, Reg n, Reg t);
    bool arm_LDAEXB(Cond cond, Reg n, Reg t);
    bool arm_LDREXB(Cond cond, Reg n, Reg t);
    bool arm_STLH(Cond cond, Reg n, Reg t);
    bool arm_STLEXH(Cond cond, Reg n, Reg d, Reg t);
    bool arm_STREXH(Cond cond, Reg n, Reg d, Reg t);
    bool arm_LDAH(Cond cond, Reg n, Reg t);
    bool arm_LDAEXH(Cond cond, Reg n, Reg t);
    bool arm_LDREXH(Cond cond, Reg n, Reg t);

    // Status register access instructions
    bool arm_CPS();
    bool arm_MRS(Cond cond, Reg d);
    bool arm_MSR_imm(Cond cond, int mask, int rotate, Imm<8> imm8);
    bool arm_MSR_reg(Cond cond, int mask, Reg n);
    bool arm_RFE();
    bool arm_SETEND(bool E);
    bool arm_SRS();

    // thumb16
    bool thumb16_LSL_imm(Imm<5> imm5, Reg m, Reg d);
    bool thumb16_LSR_imm(Imm<5> imm5, Reg m, Reg d);
    bool thumb16_ASR_imm(Imm<5> imm5, Reg m, Reg d);
    bool thumb16_ADD_reg_t1(Reg m, Reg n, Reg d);
    bool thumb16_SUB_reg(Reg m, Reg n, Reg d);
    bool thumb16_ADD_imm_t1(Imm<3> imm3, Reg n, Reg d);
    bool thumb16_SUB_imm_t1(Imm<3> imm3, Reg n, Reg d);
    bool thumb16_MOV_imm(Reg d, Imm<8> imm8);
    bool thumb16_CMP_imm(Reg n, Imm<8> imm8);
    bool thumb16_ADD_imm_t2(Reg d_n, Imm<8> imm8);
    bool thumb16_SUB_imm_t2(Reg d_n, Imm<8> imm8);
    bool thumb16_AND_reg(Reg m, Reg d_n);
    bool thumb16_EOR_reg(Reg m, Reg d_n);
    bool thumb16_LSL_reg(Reg m, Reg d_n);
    bool thumb16_LSR_reg(Reg m, Reg d_n);
    bool thumb16_ASR_reg(Reg m, Reg d_n);
    bool thumb16_ADC_reg(Reg m, Reg d_n);
    bool thumb16_SBC_reg(Reg m, Reg d_n);
    bool thumb16_ROR_reg(Reg m, Reg d_n);
    bool thumb16_TST_reg(Reg m, Reg n);
    bool thumb16_RSB_imm(Reg n, Reg d);
    bool thumb16_CMP_reg_t1(Reg m, Reg n);
    bool thumb16_CMN_reg(Reg m, Reg n);
    bool thumb16_ORR_reg(Reg m, Reg d_n);
    bool thumb16_MUL_reg(Reg n, Reg d_m);
    bool thumb16_BIC_reg(Reg m, Reg d_n);
    bool thumb16_MVN_reg(Reg m, Reg d);
    bool thumb16_ADD_reg_t2(bool d_n_hi, Reg m, Reg d_n_lo);
    bool thumb16_CMP_reg_t2(bool n_hi, Reg m, Reg n_lo);
    bool thumb16_MOV_reg(bool d_hi, Reg m, Reg d_lo);
    bool thumb16_LDR_literal(Reg t, Imm<8> imm8);
    bool thumb16_STR_reg(Reg m, Reg n, Reg t);
    bool thumb16_STRH_reg(Reg m, Reg n, Reg t);
    bool thumb16_STRB_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRSB_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDR_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRH_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRB_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRSH_reg(Reg m, Reg n, Reg t);
    bool thumb16_STR_imm_t1(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_LDR_imm_t1(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_STRB_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_LDRB_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_STRH_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_LDRH_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_STR_imm_t2(Reg t, Imm<8> imm8);
    bool thumb16_LDR_imm_t2(Reg t, Imm<8> imm8);
    bool thumb16_ADR(Reg d, Imm<8> imm8);
    bool thumb16_ADD_sp_t1(Reg d, Imm<8> imm8);
    bool thumb16_ADD_sp_t2(Imm<7> imm7);
    bool thumb16_SUB_sp(Imm<7> imm7);
    bool thumb16_SEV();
    bool thumb16_SEVL();
    bool thumb16_WFE();
    bool thumb16_WFI();
    bool thumb16_YIELD();
    bool thumb16_NOP();
    bool thumb16_IT(Imm<8> imm8);
    bool thumb16_SXTH(Reg m, Reg d);
    bool thumb16_SXTB(Reg m, Reg d);
    bool thumb16_UXTH(Reg m, Reg d);
    bool thumb16_UXTB(Reg m, Reg d);
    bool thumb16_PUSH(bool M, RegList reg_list);
    bool thumb16_POP(bool P, RegList reg_list);
    bool thumb16_SETEND(bool E);
    bool thumb16_CPS(bool, bool, bool, bool);
    bool thumb16_REV(Reg m, Reg d);
    bool thumb16_REV16(Reg m, Reg d);
    bool thumb16_REVSH(Reg m, Reg d);
    bool thumb16_BKPT(Imm<8> imm8);
    bool thumb16_STMIA(Reg n, RegList reg_list);
    bool thumb16_LDMIA(Reg n, RegList reg_list);
    bool thumb16_CBZ_CBNZ(bool nonzero, Imm<1> i, Imm<5> imm5, Reg n);
    bool thumb16_UDF();
    bool thumb16_BX(Reg m);
    bool thumb16_BLX_reg(Reg m);
    bool thumb16_SVC(Imm<8> imm8);
    bool thumb16_B_t1(Cond cond, Imm<8> imm8);
    bool thumb16_B_t2(Imm<11> imm11);

    // thumb32 load/store multiple instructions
    bool thumb32_LDMDB(bool W, Reg n, Imm<16> reg_list);
    bool thumb32_LDMIA(bool W, Reg n, Imm<16> reg_list);
    bool thumb32_POP(Imm<16> reg_list);
    bool thumb32_PUSH(Imm<15> reg_list);
    bool thumb32_STMIA(bool W, Reg n, Imm<15> reg_list);
    bool thumb32_STMDB(bool W, Reg n, Imm<15> reg_list);

    // thumb32 load/store dual, load/store exclusive, table branch instructions
    bool thumb32_LDA(Reg n, Reg t);
    bool thumb32_LDRD_imm_1(bool U, Reg n, Reg t, Reg t2, Imm<8> imm8);
    bool thumb32_LDRD_imm_2(bool U, bool W, Reg n, Reg t, Reg t2, Imm<8> imm8);
    bool thumb32_LDRD_lit_1(bool U, Reg t, Reg t2, Imm<8> imm8);
    bool thumb32_LDRD_lit_2(bool U, bool W, Reg t, Reg t2, Imm<8> imm8);
    bool thumb32_STRD_imm_1(bool U, Reg n, Reg t, Reg t2, Imm<8> imm8);
    bool thumb32_STRD_imm_2(bool U, bool W, Reg n, Reg t, Reg t2, Imm<8> imm8);
    bool thumb32_LDREX(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_LDREXD(Reg n, Reg t, Reg t2);
    bool thumb32_LDREXB(Reg n, Reg t);
    bool thumb32_LDREXH(Reg n, Reg t);
    bool thumb32_STL(Reg n, Reg t);
    bool thumb32_STREX(Reg n, Reg t, Reg d, Imm<8> imm8);
    bool thumb32_STREXB(Reg n, Reg t, Reg d);
    bool thumb32_STREXD(Reg n, Reg t, Reg t2, Reg d);
    bool thumb32_STREXH(Reg n, Reg t, Reg d);
    bool thumb32_TBB(Reg n, Reg m);
    bool thumb32_TBH(Reg n, Reg m);

    // thumb32 data processing (shifted register) instructions
    bool thumb32_TST_reg(Reg n, Imm<3> imm3, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_AND_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_BIC_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_MOV_reg(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_ORR_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_MVN_reg(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_ORN_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_TEQ_reg(Reg n, Imm<3> imm3, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_EOR_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_PKH(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<1> tb, Reg m);
    bool thumb32_CMN_reg(Reg n, Imm<3> imm3, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_ADD_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_ADC_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_SBC_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_CMP_reg(Reg n, Imm<3> imm3, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_SUB_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_RSB_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);

    // thumb32 data processing (modified immediate) instructions
    bool thumb32_TST_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_AND_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_BIC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_MOV_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ORR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_MVN_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ORN_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_TEQ_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_EOR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_CMN_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_ADD_imm_1(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ADC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_SBC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_CMP_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_SUB_imm_1(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_RSB_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);

    // thumb32 data processing (plain binary immediate) instructions.
    bool thumb32_ADR_t2(Imm<1> imm1, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ADR_t3(Imm<1> imm1, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ADD_imm_2(Imm<1> imm1, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_BFC(Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> msb);
    bool thumb32_BFI(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> msb);
    bool thumb32_MOVT(Imm<1> imm1, Imm<4> imm4, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_MOVW_imm(Imm<1> imm1, Imm<4> imm4, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_SBFX(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> widthm1);
    bool thumb32_SSAT(bool sh, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> sat_imm);
    bool thumb32_SSAT16(Reg n, Reg d, Imm<4> sat_imm);
    bool thumb32_SUB_imm_2(Imm<1> imm1, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_UBFX(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> widthm1);
    bool thumb32_USAT(bool sh, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> sat_imm);
    bool thumb32_USAT16(Reg n, Reg d, Imm<4> sat_imm);

    // thumb32 miscellaneous control instructions
    bool thumb32_BXJ(Reg m);
    bool thumb32_CLREX();
    bool thumb32_DMB(Imm<4> option);
    bool thumb32_DSB(Imm<4> option);
    bool thumb32_ISB(Imm<4> option);
    bool thumb32_NOP();
    bool thumb32_SEV();
    bool thumb32_SEVL();
    bool thumb32_UDF();
    bool thumb32_WFE();
    bool thumb32_WFI();
    bool thumb32_YIELD();
    bool thumb32_MSR_reg(bool R, Reg n, Imm<4> mask);
    bool thumb32_MRS_reg(bool R, Reg d);

    // thumb32 branch instructions
    bool thumb32_BL_imm(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo);
    bool thumb32_BLX_imm(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo);
    bool thumb32_B(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo);
    bool thumb32_B_cond(Imm<1> S, Cond cond, Imm<6> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo);

    // thumb32 store single data item instructions
    bool thumb32_STRB_imm_1(Reg n, Reg t, bool P, bool U, Imm<8> imm8);
    bool thumb32_STRB_imm_2(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STRB_imm_3(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_STRBT(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STRB(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_STRH_imm_1(Reg n, Reg t, bool P, bool U, Imm<8> imm8);
    bool thumb32_STRH_imm_2(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STRH_imm_3(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_STRHT(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STRH(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_STR_imm_1(Reg n, Reg t, bool P, bool U, Imm<8> imm8);
    bool thumb32_STR_imm_2(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STR_imm_3(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_STRT(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STR_reg(Reg n, Reg t, Imm<2> imm2, Reg m);

    // thumb32 load byte and memory hints
    bool thumb32_PLD_lit(bool U, Imm<12> imm12);
    bool thumb32_PLD_imm8(bool W, Reg n, Imm<8> imm8);
    bool thumb32_PLD_imm12(bool W, Reg n, Imm<12> imm12);
    bool thumb32_PLD_reg(bool W, Reg n, Imm<2> imm2, Reg m);
    bool thumb32_PLI_lit(bool U, Imm<12> imm12);
    bool thumb32_PLI_imm8(Reg n, Imm<8> imm8);
    bool thumb32_PLI_imm12(Reg n, Imm<12> imm12);
    bool thumb32_PLI_reg(Reg n, Imm<2> imm2, Reg m);
    bool thumb32_LDRB_lit(bool U, Reg t, Imm<12> imm12);
    bool thumb32_LDRB_reg(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_LDRB_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8);
    bool thumb32_LDRB_imm12(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_LDRBT(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_LDRSB_lit(bool U, Reg t, Imm<12> imm12);
    bool thumb32_LDRSB_reg(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_LDRSB_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8);
    bool thumb32_LDRSB_imm12(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_LDRSBT(Reg n, Reg t, Imm<8> imm8);

    // thumb32 load halfword instructions
    bool thumb32_LDRH_lit(bool U, Reg t, Imm<12> imm12);
    bool thumb32_LDRH_reg(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_LDRH_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8);
    bool thumb32_LDRH_imm12(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_LDRHT(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_LDRSH_lit(bool U, Reg t, Imm<12> imm12);
    bool thumb32_LDRSH_reg(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_LDRSH_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8);
    bool thumb32_LDRSH_imm12(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_LDRSHT(Reg n, Reg t, Imm<8> imm8);

    // thumb32 load word instructions
    bool thumb32_LDR_lit(bool U, Reg t, Imm<12> imm12);
    bool thumb32_LDR_reg(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_LDR_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8);
    bool thumb32_LDR_imm12(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_LDRT(Reg n, Reg t, Imm<8> imm8);

    // thumb32 data processing (register) instructions
    bool thumb32_ASR_reg(bool S, Reg m, Reg d, Reg s);
    bool thumb32_LSL_reg(bool S, Reg m, Reg d, Reg s);
    bool thumb32_LSR_reg(bool S, Reg m, Reg d, Reg s);
    bool thumb32_ROR_reg(bool S, Reg m, Reg d, Reg s);
    bool thumb32_SXTB(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_SXTB16(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_SXTAB(Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_SXTAB16(Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_SXTH(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_SXTAH(Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTB(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTB16(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTAB(Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTAB16(Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTH(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTAH(Reg n, Reg d, SignExtendRotation rotate, Reg m);

    // thumb32 long multiply, long multiply accumulate, and divide instructions
    bool thumb32_SDIV(Reg n, Reg d, Reg m);
    bool thumb32_SMLAL(Reg n, Reg dLo, Reg dHi, Reg m);
    bool thumb32_SMLALD(Reg n, Reg dLo, Reg dHi, bool M, Reg m);
    bool thumb32_SMLALXY(Reg n, Reg dLo, Reg dHi, bool N, bool M, Reg m);
    bool thumb32_SMLSLD(Reg n, Reg dLo, Reg dHi, bool M, Reg m);
    bool thumb32_SMULL(Reg n, Reg dLo, Reg dHi, Reg m);
    bool thumb32_UDIV(Reg n, Reg d, Reg m);
    bool thumb32_UMAAL(Reg n, Reg dLo, Reg dHi, Reg m);
    bool thumb32_UMLAL(Reg n, Reg dLo, Reg dHi, Reg m);
    bool thumb32_UMULL(Reg n, Reg dLo, Reg dHi, Reg m);

    // thumb32 miscellaneous instructions
    bool thumb32_CLZ(Reg n, Reg d, Reg m);
    bool thumb32_QADD(Reg n, Reg d, Reg m);
    bool thumb32_QDADD(Reg n, Reg d, Reg m);
    bool thumb32_QDSUB(Reg n, Reg d, Reg m);
    bool thumb32_QSUB(Reg n, Reg d, Reg m);
    bool thumb32_RBIT(Reg n, Reg d, Reg m);
    bool thumb32_REV(Reg n, Reg d, Reg m);
    bool thumb32_REV16(Reg n, Reg d, Reg m);
    bool thumb32_REVSH(Reg n, Reg d, Reg m);
    bool thumb32_SEL(Reg n, Reg d, Reg m);

    // thumb32 multiply instructions
    bool thumb32_MLA(Reg n, Reg a, Reg d, Reg m);
    bool thumb32_MLS(Reg n, Reg a, Reg d, Reg m);
    bool thumb32_MUL(Reg n, Reg d, Reg m);
    bool thumb32_SMLAD(Reg n, Reg a, Reg d, bool X, Reg m);
    bool thumb32_SMLAXY(Reg n, Reg a, Reg d, bool N, bool M, Reg m);
    bool thumb32_SMLAWY(Reg n, Reg a, Reg d, bool M, Reg m);
    bool thumb32_SMLSD(Reg n, Reg a, Reg d, bool X, Reg m);
    bool thumb32_SMMLA(Reg n, Reg a, Reg d, bool R, Reg m);
    bool thumb32_SMMLS(Reg n, Reg a, Reg d, bool R, Reg m);
    bool thumb32_SMMUL(Reg n, Reg d, bool R, Reg m);
    bool thumb32_SMUAD(Reg n, Reg d, bool M, Reg m);
    bool thumb32_SMUSD(Reg n, Reg d, bool M, Reg m);
    bool thumb32_SMULXY(Reg n, Reg d, bool N, bool M, Reg m);
    bool thumb32_SMULWY(Reg n, Reg d, bool M, Reg m);
    bool thumb32_USAD8(Reg n, Reg d, Reg m);
    bool thumb32_USADA8(Reg n, Reg a, Reg d, Reg m);

    // thumb32 parallel add/sub instructions
    bool thumb32_SADD8(Reg n, Reg d, Reg m);
    bool thumb32_SADD16(Reg n, Reg d, Reg m);
    bool thumb32_SASX(Reg n, Reg d, Reg m);
    bool thumb32_SSAX(Reg n, Reg d, Reg m);
    bool thumb32_SSUB8(Reg n, Reg d, Reg m);
    bool thumb32_SSUB16(Reg n, Reg d, Reg m);
    bool thumb32_UADD8(Reg n, Reg d, Reg m);
    bool thumb32_UADD16(Reg n, Reg d, Reg m);
    bool thumb32_UASX(Reg n, Reg d, Reg m);
    bool thumb32_USAX(Reg n, Reg d, Reg m);
    bool thumb32_USUB8(Reg n, Reg d, Reg m);
    bool thumb32_USUB16(Reg n, Reg d, Reg m);

    bool thumb32_QADD8(Reg n, Reg d, Reg m);
    bool thumb32_QADD16(Reg n, Reg d, Reg m);
    bool thumb32_QASX(Reg n, Reg d, Reg m);
    bool thumb32_QSAX(Reg n, Reg d, Reg m);
    bool thumb32_QSUB8(Reg n, Reg d, Reg m);
    bool thumb32_QSUB16(Reg n, Reg d, Reg m);
    bool thumb32_UQADD8(Reg n, Reg d, Reg m);
    bool thumb32_UQADD16(Reg n, Reg d, Reg m);
    bool thumb32_UQASX(Reg n, Reg d, Reg m);
    bool thumb32_UQSAX(Reg n, Reg d, Reg m);
    bool thumb32_UQSUB8(Reg n, Reg d, Reg m);
    bool thumb32_UQSUB16(Reg n, Reg d, Reg m);

    bool thumb32_SHADD8(Reg n, Reg d, Reg m);
    bool thumb32_SHADD16(Reg n, Reg d, Reg m);
    bool thumb32_SHASX(Reg n, Reg d, Reg m);
    bool thumb32_SHSAX(Reg n, Reg d, Reg m);
    bool thumb32_SHSUB8(Reg n, Reg d, Reg m);
    bool thumb32_SHSUB16(Reg n, Reg d, Reg m);
    bool thumb32_UHADD8(Reg n, Reg d, Reg m);
    bool thumb32_UHADD16(Reg n, Reg d, Reg m);
    bool thumb32_UHASX(Reg n, Reg d, Reg m);
    bool thumb32_UHSAX(Reg n, Reg d, Reg m);
    bool thumb32_UHSUB8(Reg n, Reg d, Reg m);
    bool thumb32_UHSUB16(Reg n, Reg d, Reg m);

    // thumb32 coprocessor insturctions
    bool thumb32_MCRR(bool two, Reg t2, Reg t, size_t coproc_no, size_t opc, CoprocReg CRm);
    bool thumb32_MRRC(bool two, Reg t2, Reg t, size_t coproc_no, size_t opc, CoprocReg CRm);
    bool thumb32_STC(bool two, bool p, bool u, bool d, bool w, Reg n, CoprocReg CRd, size_t coproc_no, Imm<8> imm8);
    bool thumb32_LDC(bool two, bool p, bool u, bool d, bool w, Reg n, CoprocReg CRd, size_t coproc_no, Imm<8> imm8);
    bool thumb32_CDP(bool two, size_t opc1, CoprocReg CRn, CoprocReg CRd, size_t coproc_no, size_t opc2, CoprocReg CRm);
    bool thumb32_MCR(bool two, size_t opc1, CoprocReg CRn, Reg t, size_t coproc_no, size_t opc2, CoprocReg CRm);
    bool thumb32_MRC(bool two, size_t opc1, CoprocReg CRn, Reg t, size_t coproc_no, size_t opc2, CoprocReg CRm);

    // Floating-point three-register data processing instructions
    bool vfp_VADD(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VSUB(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VMUL(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VMLA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VMLS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VNMUL(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VNMLA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VNMLS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VDIV(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VFNMS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VFNMA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VFMA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VFMS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VSEL(bool D, Imm<2> cc, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VMAXNM(bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);
    bool vfp_VMINNM(bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm);

    // Floating-point move instructions
    bool vfp_VMOV_u32_f64(Cond cond, size_t Vd, Reg t, bool D);
    bool vfp_VMOV_f64_u32(Cond cond, size_t Vn, Reg t, bool N);
    bool vfp_VMOV_u32_f32(Cond cond, size_t Vn, Reg t, bool N);
    bool vfp_VMOV_f32_u32(Cond cond, size_t Vn, Reg t, bool N);
    bool vfp_VMOV_2u32_2f32(Cond cond, Reg t2, Reg t, bool M, size_t Vm);
    bool vfp_VMOV_2f32_2u32(Cond cond, Reg t2, Reg t, bool M, size_t Vm);
    bool vfp_VMOV_2u32_f64(Cond cond, Reg t2, Reg t, bool M, size_t Vm);
    bool vfp_VMOV_f64_2u32(Cond cond, Reg t2, Reg t, bool M, size_t Vm);
    bool vfp_VMOV_from_i32(Cond cond, Imm<1> i, size_t Vd, Reg t, bool D);
    bool vfp_VMOV_from_i16(Cond cond, Imm<1> i1, size_t Vd, Reg t, bool D, Imm<1> i2);
    bool vfp_VMOV_from_i8(Cond cond, Imm<1> i1, size_t Vd, Reg t, bool D, Imm<2> i2);
    bool vfp_VMOV_to_i32(Cond cond, Imm<1> i, size_t Vn, Reg t, bool N);
    bool vfp_VMOV_to_i16(Cond cond, bool U, Imm<1> i1, size_t Vn, Reg t, bool N, Imm<1> i2);
    bool vfp_VMOV_to_i8(Cond cond, bool U, Imm<1> i1, size_t Vn, Reg t, bool N, Imm<2> i2);
    bool vfp_VDUP(Cond cond, Imm<1> B, bool Q, size_t Vd, Reg t, bool D, Imm<1> E);
    bool vfp_VMOV_imm(Cond cond, bool D, Imm<4> imm4H, size_t Vd, bool sz, Imm<4> imm4L);
    bool vfp_VMOV_reg(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm);

    // Floating-point misc instructions
    bool vfp_VABS(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm);
    bool vfp_VNEG(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm);
    bool vfp_VSQRT(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm);
    bool vfp_VCVTB(Cond cond, bool D, bool op, size_t Vd, bool sz, bool M, size_t Vm);
    bool vfp_VCVTT(Cond cond, bool D, bool op, size_t Vd, bool sz, bool M, size_t Vm);
    bool vfp_VCMP(Cond cond, bool D, size_t Vd, bool sz, bool E, bool M, size_t Vm);
    bool vfp_VCMP_zero(Cond cond, bool D, size_t Vd, bool sz, bool E);
    bool vfp_VRINTR(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm);
    bool vfp_VRINTZ(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm);
    bool vfp_VRINTX(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm);
    bool vfp_VCVT_f_to_f(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm);
    bool vfp_VCVT_from_int(Cond cond, bool D, size_t Vd, bool sz, bool is_signed, bool M, size_t Vm);
    bool vfp_VCVT_from_fixed(Cond cond, bool D, bool U, size_t Vd, bool sz, bool sx, Imm<1> i, Imm<4> imm4);
    bool vfp_VCVT_to_u32(Cond cond, bool D, size_t Vd, bool sz, bool round_towards_zero, bool M, size_t Vm);
    bool vfp_VCVT_to_s32(Cond cond, bool D, size_t Vd, bool sz, bool round_towards_zero, bool M, size_t Vm);
    bool vfp_VCVT_to_fixed(Cond cond, bool D, bool U, size_t Vd, bool sz, bool sx, Imm<1> i, Imm<4> imm4);
    bool vfp_VRINT_rm(bool D, size_t rm, size_t Vd, bool sz, bool M, size_t Vm);
    bool vfp_VCVT_rm(bool D, size_t rm, size_t Vd, bool sz, bool U, bool M, size_t Vm);

    // Floating-point system register access
    bool vfp_VMSR(Cond cond, Reg t);
    bool vfp_VMRS(Cond cond, Reg t);

    // Floating-point load-store instructions
    bool vfp_VLDR(Cond cond, bool U, bool D, Reg n, size_t Vd, bool sz, Imm<8> imm8);
    bool vfp_VSTR(Cond cond, bool U, bool D, Reg n, size_t Vd, bool sz, Imm<8> imm8);
    bool vfp_VPOP(Cond cond, bool D, size_t Vd, bool sz, Imm<8> imm8);
    bool vfp_VPUSH(Cond cond, bool D, size_t Vd, bool sz, Imm<8> imm8);
    bool vfp_VSTM_a1(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8);
    bool vfp_VSTM_a2(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8);
    bool vfp_VLDM_a1(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8);
    bool vfp_VLDM_a2(Cond cond, bool p, bool u, bool D, bool w, Reg n, size_t Vd, Imm<8> imm8);

    // Advanced SIMD one register, modified immediate
    bool asimd_VMOV_imm(Imm<1> a, bool D, Imm<1> b, Imm<1> c, Imm<1> d, size_t Vd, Imm<4> cmode, bool Q, bool op, Imm<1> e, Imm<1> f, Imm<1> g, Imm<1> h);

    // Advanced SIMD three register with same length
    bool asimd_VHADD(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VQADD(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VRHADD(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VAND_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VBIC_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VORR_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VORN_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VEOR_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VBSL(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VBIT(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VBIF(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VHSUB(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VQSUB(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VCGT_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VCGE_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VABD(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VABA(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VADD_int(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VSUB_int(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VSHL_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VQSHL_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VRSHL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VMAX(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, bool op, size_t Vm);
    bool asimd_VTST(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VCEQ_reg(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VMLA(bool op, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VMUL(bool P, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VPMAX_int(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, bool op, size_t Vm);
    bool asimd_VQDMULH(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VQRDMULH(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VPADD(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VFMA(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VFMS(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VADD_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VSUB_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VPADD_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VABD_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VMLA_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VMLS_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VMUL_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VCEQ_reg_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VCGE_reg_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VCGT_reg_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VACGE(bool D, bool op, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VMAX_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VMIN_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VPMAX_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VPMIN_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VRECPS(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VRSQRTS(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool v8_SHA256H(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool v8_SHA256H2(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);
    bool v8_SHA256SU1(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm);

    // Advanced SIMD three registers with different lengths
    bool asimd_VADDL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool op, bool N, bool M, size_t Vm);
    bool asimd_VSUBL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool op, bool N, bool M, size_t Vm);
    bool asimd_VABAL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm);
    bool asimd_VABDL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm);
    bool asimd_VMLAL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool op, bool N, bool M, size_t Vm);
    bool asimd_VMULL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool P, bool N, bool M, size_t Vm);

    // Advanced SIMD two registers and a scalar
    bool asimd_VMLA_scalar(bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool op, bool F, bool N, bool M, size_t Vm);
    bool asimd_VMLAL_scalar(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool op, bool N, bool M, size_t Vm);
    bool asimd_VMUL_scalar(bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool F, bool N, bool M, size_t Vm);
    bool asimd_VMULL_scalar(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm);
    bool asimd_VQDMULL_scalar(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm);
    bool asimd_VQDMULH_scalar(bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm);
    bool asimd_VQRDMULH_scalar(bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm);

    // Two registers and a shift amount
    bool asimd_SHR(bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm);
    bool asimd_SRA(bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm);
    bool asimd_VRSHR(bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm);
    bool asimd_VRSRA(bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm);
    bool asimd_VSRI(bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm);
    bool asimd_VSHL(bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm);
    bool asimd_VSLI(bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm);
    bool asimd_VQSHL(bool U, bool D, size_t imm6, size_t Vd, bool op, bool L, bool Q, bool M, size_t Vm);
    bool asimd_VSHRN(bool D, size_t imm6, size_t Vd, bool M, size_t Vm);
    bool asimd_VRSHRN(bool D, size_t imm6, size_t Vd, bool M, size_t Vm);
    bool asimd_VQSHRUN(bool D, size_t imm6, size_t Vd, bool M, size_t Vm);
    bool asimd_VQRSHRUN(bool D, size_t imm6, size_t Vd, bool M, size_t Vm);
    bool asimd_VQSHRN(bool U, bool D, size_t imm6, size_t Vd, bool M, size_t Vm);
    bool asimd_VQRSHRN(bool U, bool D, size_t imm6, size_t Vd, bool M, size_t Vm);
    bool asimd_VSHLL(bool U, bool D, size_t imm6, size_t Vd, bool M, size_t Vm);
    bool asimd_VCVT_fixed(bool U, bool D, size_t imm6, size_t Vd, bool to_fixed, bool Q, bool M, size_t Vm);

    // Advanced SIMD two register, miscellaneous
    bool asimd_VREV(bool D, size_t sz, size_t Vd, size_t op, bool Q, bool M, size_t Vm);
    bool asimd_VPADDL(bool D, size_t sz, size_t Vd, bool op, bool Q, bool M, size_t Vm);
    bool v8_AESD(bool D, size_t sz, size_t Vd, bool M, size_t Vm);
    bool v8_AESE(bool D, size_t sz, size_t Vd, bool M, size_t Vm);
    bool v8_AESIMC(bool D, size_t sz, size_t Vd, bool M, size_t Vm);
    bool v8_AESMC(bool D, size_t sz, size_t Vd, bool M, size_t Vm);
    bool v8_SHA256SU0(bool D, size_t sz, size_t Vd, bool M, size_t Vm);
    bool asimd_VCLS(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm);
    bool asimd_VCLZ(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm);
    bool asimd_VCNT(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm);
    bool asimd_VMVN_reg(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm);
    bool asimd_VPADAL(bool D, size_t sz, size_t Vd, bool op, bool Q, bool M, size_t Vm);
    bool asimd_VQABS(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm);
    bool asimd_VQNEG(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm);
    bool asimd_VCGT_zero(bool D, size_t sz, size_t Vd, bool F, bool Q, bool M, size_t Vm);
    bool asimd_VCGE_zero(bool D, size_t sz, size_t Vd, bool F, bool Q, bool M, size_t Vm);
    bool asimd_VCEQ_zero(bool D, size_t sz, size_t Vd, bool F, bool Q, bool M, size_t Vm);
    bool asimd_VCLE_zero(bool D, size_t sz, size_t Vd, bool F, bool Q, bool M, size_t Vm);
    bool asimd_VCLT_zero(bool D, size_t sz, size_t Vd, bool F, bool Q, bool M, size_t Vm);
    bool asimd_VABS(bool D, size_t sz, size_t Vd, bool F, bool Q, bool M, size_t Vm);
    bool asimd_VNEG(bool D, size_t sz, size_t Vd, bool F, bool Q, bool M, size_t Vm);
    bool asimd_VSWP(bool D, size_t Vd, bool Q, bool M, size_t Vm);
    bool asimd_VTRN(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm);
    bool asimd_VUZP(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm);
    bool asimd_VZIP(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm);
    bool asimd_VMOVN(bool D, size_t sz, size_t Vd, bool M, size_t Vm);
    bool asimd_VQMOVUN(bool D, size_t sz, size_t Vd, bool M, size_t Vm);
    bool asimd_VQMOVN(bool D, size_t sz, size_t Vd, bool op, bool M, size_t Vm);
    bool asimd_VSHLL_max(bool D, size_t sz, size_t Vd, bool M, size_t Vm);
    bool asimd_VCVT_half(bool D, size_t sz, size_t Vd, bool op, bool M, size_t Vm);
    bool asimd_VRECPE(bool D, size_t sz, size_t Vd, bool F, bool Q, bool M, size_t Vm);
    bool asimd_VRSQRTE(bool D, size_t sz, size_t Vd, bool F, bool Q, bool M, size_t Vm);
    bool asimd_VCVT_integer(bool D, size_t sz, size_t Vd, bool op, bool U, bool Q, bool M, size_t Vm);

    // Advanced SIMD miscellaneous
    bool asimd_VEXT(bool D, size_t Vn, size_t Vd, Imm<4> imm4, bool N, bool Q, bool M, size_t Vm);
    bool asimd_VTBL(bool D, size_t Vn, size_t Vd, size_t len, bool N, bool M, size_t Vm);
    bool asimd_VTBX(bool D, size_t Vn, size_t Vd, size_t len, bool N, bool M, size_t Vm);
    bool asimd_VDUP_scalar(bool D, Imm<4> imm4, size_t Vd, bool Q, bool M, size_t Vm);

    // Advanced SIMD load/store structures
    bool v8_VST_multiple(bool D, Reg n, size_t Vd, Imm<4> type, size_t sz, size_t align, Reg m);
    bool v8_VLD_multiple(bool D, Reg n, size_t Vd, Imm<4> type, size_t sz, size_t align, Reg m);
    bool v8_VLD_all_lanes(bool D, Reg n, size_t Vd, size_t nn, size_t sz, bool T, bool a, Reg m);
    bool v8_VST_single(bool D, Reg n, size_t Vd, size_t sz, size_t nn, size_t index_align, Reg m);
    bool v8_VLD_single(bool D, Reg n, size_t Vd, size_t sz, size_t nn, size_t index_align, Reg m);
};

}  // namespace Dynarmic::A32
