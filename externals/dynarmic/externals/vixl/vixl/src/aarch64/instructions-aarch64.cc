// Copyright 2015, VIXL authors
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of ARM Limited nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "instructions-aarch64.h"
#include "assembler-aarch64.h"

namespace vixl {
namespace aarch64 {

static uint64_t RepeatBitsAcrossReg(unsigned reg_size,
                                    uint64_t value,
                                    unsigned width) {
  VIXL_ASSERT((width == 2) || (width == 4) || (width == 8) || (width == 16) ||
              (width == 32));
  VIXL_ASSERT((reg_size == kBRegSize) || (reg_size == kHRegSize) ||
              (reg_size == kSRegSize) || (reg_size == kDRegSize));
  uint64_t result = value & ((UINT64_C(1) << width) - 1);
  for (unsigned i = width; i < reg_size; i *= 2) {
    result |= (result << i);
  }
  return result;
}

bool Instruction::CanTakeSVEMovprfx(const Instruction* movprfx) const {
  bool movprfx_is_predicated = movprfx->Mask(SVEMovprfxMask) == MOVPRFX_z_p_z;
  bool movprfx_is_unpredicated =
      movprfx->Mask(SVEConstructivePrefix_UnpredicatedMask) == MOVPRFX_z_z;
  VIXL_ASSERT(movprfx_is_predicated != movprfx_is_unpredicated);

  int movprfx_zd = movprfx->GetRd();
  int movprfx_pg = movprfx_is_predicated ? movprfx->GetPgLow8() : -1;
  VectorFormat movprfx_vform =
      movprfx_is_predicated ? movprfx->GetSVEVectorFormat() : kFormatUndefined;

  bool pg_matches_low8 = movprfx_pg == GetPgLow8();
  bool vform_matches = movprfx_vform == GetSVEVectorFormat();
  bool zd_matches = movprfx_zd == GetRd();
  bool zd_matches_zm = movprfx_zd == GetRm();
  bool zd_matches_zn = movprfx_zd == GetRn();

  switch (Mask(SVEBitwiseLogicalWithImm_UnpredicatedMask)) {
    case AND_z_zi:
    case EOR_z_zi:
    case ORR_z_zi:
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVEBitwiseLogical_PredicatedMask)) {
    case AND_z_p_zz:
    case BIC_z_p_zz:
    case EOR_z_p_zz:
    case ORR_z_p_zz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return zd_matches;
  }
  switch (Mask(SVEBitwiseShiftByImm_PredicatedMask)) {
    case ASRD_z_p_zi:
    case ASR_z_p_zi:
    case LSL_z_p_zi:
    case LSR_z_p_zi:
      if (movprfx_is_predicated) {
        if (!pg_matches_low8) return false;
        unsigned tsz = ExtractBits<0x00c00300>();
        VectorFormat instr_vform =
            SVEFormatFromLaneSizeInBytesLog2(HighestSetBitPosition(tsz));
        if (movprfx_vform != instr_vform) return false;
      }
      return zd_matches;
  }
  switch (Mask(SVEBitwiseShiftByVector_PredicatedMask)) {
    case ASRR_z_p_zz:
    case ASR_z_p_zz:
    case LSLR_z_p_zz:
    case LSL_z_p_zz:
    case LSRR_z_p_zz:
    case LSR_z_p_zz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return zd_matches;
  }
  switch (Mask(SVEBitwiseShiftByWideElements_PredicatedMask)) {
    case ASR_z_p_zw:
    case LSL_z_p_zw:
    case LSR_z_p_zw:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return zd_matches;
  }
  switch (Mask(SVEConditionallyBroadcastElementToVectorMask)) {
    case CLASTA_z_p_zz:
    case CLASTB_z_p_zz:
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVECopyFPImm_PredicatedMask)) {
    case FCPY_z_p_i:
      if (movprfx_is_predicated) {
        if (!vform_matches) return false;
        if (movprfx_pg != GetRx<19, 16>()) return false;
      }
      return zd_matches;
  }
  switch (Mask(SVECopyGeneralRegisterToVector_PredicatedMask)) {
    case CPY_z_p_r:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      return zd_matches;
  }
  switch (Mask(SVECopyIntImm_PredicatedMask)) {
    case CPY_z_p_i:
      if (movprfx_is_predicated) {
        if (!vform_matches) return false;
        if (movprfx_pg != GetRx<19, 16>()) return false;
      }
      // Only the merging form can take movprfx.
      if (ExtractBit(14) == 0) return false;
      return zd_matches;
  }
  switch (Mask(SVECopySIMDFPScalarRegisterToVector_PredicatedMask)) {
    case CPY_z_p_v:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      return zd_matches && !zd_matches_zn;
  }
  switch (Mask(SVEFPArithmeticWithImm_PredicatedMask)) {
    case FADD_z_p_zs:
    case FMAXNM_z_p_zs:
    case FMAX_z_p_zs:
    case FMINNM_z_p_zs:
    case FMIN_z_p_zs:
    case FMUL_z_p_zs:
    case FSUBR_z_p_zs:
    case FSUB_z_p_zs:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      return zd_matches;
  }
  switch (Mask(SVEFPArithmetic_PredicatedMask)) {
    case FABD_z_p_zz:
    case FADD_z_p_zz:
    case FDIVR_z_p_zz:
    case FDIV_z_p_zz:
    case FMAXNM_z_p_zz:
    case FMAX_z_p_zz:
    case FMINNM_z_p_zz:
    case FMIN_z_p_zz:
    case FMULX_z_p_zz:
    case FMUL_z_p_zz:
    case FSCALE_z_p_zz:
    case FSUBR_z_p_zz:
    case FSUB_z_p_zz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return zd_matches;
  }
  switch (Mask(SVEFPComplexAdditionMask)) {
    case FCADD_z_p_zz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return zd_matches;
  }
  switch (Mask(SVEFPComplexMulAddIndexMask)) {
    case FCMLA_z_zzzi_h:
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<18, 16>()) return false;
      return movprfx_is_unpredicated && zd_matches && !zd_matches_zn;
    case FCMLA_z_zzzi_s:
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<19, 16>()) return false;
      return movprfx_is_unpredicated && zd_matches && !zd_matches_zn;
  }
  switch (Mask(SVEFPComplexMulAddMask)) {
    case FCMLA_z_p_zzz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      return zd_matches && !zd_matches_zm && !zd_matches_zn;
  }
  switch (Mask(SVEFPConvertPrecisionMask)) {
    case FCVT_z_p_z_d2h:
    case FCVT_z_p_z_d2s:
    case FCVT_z_p_z_h2d:
    case FCVT_z_p_z_h2s:
    case FCVT_z_p_z_s2d:
    case FCVT_z_p_z_s2h:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      return zd_matches && !zd_matches_zn;
  }
  switch (Mask(SVEFPConvertToIntMask)) {
    case FCVTZS_z_p_z_d2w:
    case FCVTZS_z_p_z_d2x:
    case FCVTZS_z_p_z_fp162h:
    case FCVTZS_z_p_z_fp162w:
    case FCVTZS_z_p_z_fp162x:
    case FCVTZS_z_p_z_s2w:
    case FCVTZS_z_p_z_s2x:
    case FCVTZU_z_p_z_d2w:
    case FCVTZU_z_p_z_d2x:
    case FCVTZU_z_p_z_fp162h:
    case FCVTZU_z_p_z_fp162w:
    case FCVTZU_z_p_z_fp162x:
    case FCVTZU_z_p_z_s2w:
    case FCVTZU_z_p_z_s2x:
      if (movprfx_is_predicated) {
        if (!pg_matches_low8) return false;
        // The movprfx element size must match the instruction's maximum encoded
        // element size. We have to partially decode the opc and opc2 fields to
        // find this.
        unsigned opc = ExtractBits(23, 22);
        unsigned opc2 = ExtractBits(18, 17);
        VectorFormat instr_vform =
            SVEFormatFromLaneSizeInBytesLog2(std::max(opc, opc2));
        if (movprfx_vform != instr_vform) return false;
      }
      return zd_matches && !zd_matches_zn;
  }
  switch (Mask(SVEFPMulAddIndexMask)) {
    case FMLA_z_zzzi_h:
    case FMLA_z_zzzi_h_i3h:
    case FMLA_z_zzzi_s:
    case FMLS_z_zzzi_h:
    case FMLS_z_zzzi_h_i3h:
    case FMLS_z_zzzi_s:
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<18, 16>()) return false;
      return movprfx_is_unpredicated && zd_matches && !zd_matches_zn;
    case FMLA_z_zzzi_d:
    case FMLS_z_zzzi_d:
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<19, 16>()) return false;
      return movprfx_is_unpredicated && zd_matches && !zd_matches_zn;
  }
  switch (Mask(SVEFPMulAddMask)) {
    case FMAD_z_p_zzz:
    case FMSB_z_p_zzz:
    case FNMAD_z_p_zzz:
    case FNMSB_z_p_zzz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<20, 16>()) return false;
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return zd_matches;
    case FMLA_z_p_zzz:
    case FMLS_z_p_zzz:
    case FNMLA_z_p_zzz:
    case FNMLS_z_p_zzz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      return zd_matches && !zd_matches_zm && !zd_matches_zn;
  }
  switch (Mask(SVEFPRoundToIntegralValueMask)) {
    case FRINTA_z_p_z:
    case FRINTI_z_p_z:
    case FRINTM_z_p_z:
    case FRINTN_z_p_z:
    case FRINTP_z_p_z:
    case FRINTX_z_p_z:
    case FRINTZ_z_p_z:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      return zd_matches && !zd_matches_zn;
  }
  switch (Mask(SVEFPTrigMulAddCoefficientMask)) {
    case FTMAD_z_zzi:
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVEFPUnaryOpMask)) {
    case FRECPX_z_p_z:
    case FSQRT_z_p_z:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      return zd_matches && !zd_matches_zn;
  }
  switch (Mask(SVEIncDecByPredicateCountMask)) {
    case DECP_z_p_z:
    case INCP_z_p_z:
    case SQDECP_z_p_z:
    case SQINCP_z_p_z:
    case UQDECP_z_p_z:
    case UQINCP_z_p_z:
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVEIncDecVectorByElementCountMask)) {
    case DECD_z_zs:
    case DECH_z_zs:
    case DECW_z_zs:
    case INCD_z_zs:
    case INCH_z_zs:
    case INCW_z_zs:
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVEInsertGeneralRegisterMask)) {
    case INSR_z_r:
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVEInsertSIMDFPScalarRegisterMask)) {
    case INSR_z_v:
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVEIntAddSubtractImm_UnpredicatedMask)) {
    case ADD_z_zi:
    case SQADD_z_zi:
    case SQSUB_z_zi:
    case SUBR_z_zi:
    case SUB_z_zi:
    case UQADD_z_zi:
    case UQSUB_z_zi:
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVEIntAddSubtractVectors_PredicatedMask)) {
    case ADD_z_p_zz:
    case SUBR_z_p_zz:
    case SUB_z_p_zz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return zd_matches;
  }
  switch (Mask(SVEIntConvertToFPMask)) {
    case SCVTF_z_p_z_h2fp16:
    case SCVTF_z_p_z_w2d:
    case SCVTF_z_p_z_w2fp16:
    case SCVTF_z_p_z_w2s:
    case SCVTF_z_p_z_x2d:
    case SCVTF_z_p_z_x2fp16:
    case SCVTF_z_p_z_x2s:
    case UCVTF_z_p_z_h2fp16:
    case UCVTF_z_p_z_w2d:
    case UCVTF_z_p_z_w2fp16:
    case UCVTF_z_p_z_w2s:
    case UCVTF_z_p_z_x2d:
    case UCVTF_z_p_z_x2fp16:
    case UCVTF_z_p_z_x2s:
      if (movprfx_is_predicated) {
        if (!pg_matches_low8) return false;
        // The movprfx element size must match the instruction's maximum encoded
        // element size. We have to partially decode the opc and opc2 fields to
        // find this.
        unsigned opc = ExtractBits(23, 22);
        unsigned opc2 = ExtractBits(18, 17);
        VectorFormat instr_vform =
            SVEFormatFromLaneSizeInBytesLog2(std::max(opc, opc2));
        if (movprfx_vform != instr_vform) return false;
      }
      return zd_matches && !zd_matches_zn;
  }
  switch (Mask(SVEIntDivideVectors_PredicatedMask)) {
    case SDIVR_z_p_zz:
    case SDIV_z_p_zz:
    case UDIVR_z_p_zz:
    case UDIV_z_p_zz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return zd_matches;
  }
  switch (Mask(SVEIntMinMaxDifference_PredicatedMask)) {
    case SABD_z_p_zz:
    case SMAX_z_p_zz:
    case SMIN_z_p_zz:
    case UABD_z_p_zz:
    case UMAX_z_p_zz:
    case UMIN_z_p_zz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return zd_matches;
  }
  switch (Mask(SVEIntMinMaxImm_UnpredicatedMask)) {
    case SMAX_z_zi:
    case SMIN_z_zi:
    case UMAX_z_zi:
    case UMIN_z_zi:
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVEIntMulAddPredicatedMask)) {
    case MAD_z_p_zzz:
    case MSB_z_p_zzz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return zd_matches && !zd_matches_zm;
    case MLA_z_p_zzz:
    case MLS_z_p_zzz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      return zd_matches && !zd_matches_zm && !zd_matches_zn;
  }
  switch (Mask(SVEIntMulAddUnpredicatedMask)) {
    case SDOT_z_zzz:
    case UDOT_z_zzz:
      return movprfx_is_unpredicated && zd_matches && !zd_matches_zm &&
             !zd_matches_zn;
  }
  switch (Mask(SVEIntMulImm_UnpredicatedMask)) {
    case MUL_z_zi:
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVEIntMulVectors_PredicatedMask)) {
    case MUL_z_p_zz:
    case SMULH_z_p_zz:
    case UMULH_z_p_zz:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return zd_matches;
  }
  switch (Mask(SVEIntUnaryArithmeticPredicatedMask)) {
    case ABS_z_p_z:
    case CLS_z_p_z:
    case CLZ_z_p_z:
    case CNOT_z_p_z:
    case CNT_z_p_z:
    case FABS_z_p_z:
    case FNEG_z_p_z:
    case NEG_z_p_z:
    case NOT_z_p_z:
    case SXTB_z_p_z:
    case SXTH_z_p_z:
    case SXTW_z_p_z:
    case UXTB_z_p_z:
    case UXTH_z_p_z:
    case UXTW_z_p_z:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      return zd_matches && !zd_matches_zn;
  }
  switch (Mask(SVEMulIndexMask)) {
    case SDOT_z_zzzi_s:
    case UDOT_z_zzzi_s:
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<18, 16>()) return false;
      return movprfx_is_unpredicated && zd_matches && !zd_matches_zn;
    case SDOT_z_zzzi_d:
    case UDOT_z_zzzi_d:
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<19, 16>()) return false;
      return movprfx_is_unpredicated && zd_matches && !zd_matches_zn;
  }
  switch (Mask(SVEPermuteVectorExtractMask)) {
    case EXT_z_zi_des:
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVEReverseWithinElementsMask)) {
    case RBIT_z_p_z:
    case REVB_z_z:
    case REVH_z_z:
    case REVW_z_z:
      if (movprfx_is_predicated && !(pg_matches_low8 && vform_matches)) {
        return false;
      }
      return zd_matches && !zd_matches_zn;
  }
  switch (Mask(SVESaturatingIncDecVectorByElementCountMask)) {
    case SQDECD_z_zs:
    case SQDECH_z_zs:
    case SQDECW_z_zs:
    case SQINCD_z_zs:
    case SQINCH_z_zs:
    case SQINCW_z_zs:
    case UQDECD_z_zs:
    case UQDECH_z_zs:
    case UQDECW_z_zs:
    case UQINCD_z_zs:
    case UQINCH_z_zs:
    case UQINCW_z_zs:
      return movprfx_is_unpredicated && zd_matches;
  }
  switch (Mask(SVEVectorSplice_DestructiveMask)) {
    case SPLICE_z_p_zz_des:
      // The movprfx's `zd` must not alias any other inputs.
      if (movprfx_zd == GetRx<9, 5>()) return false;
      return movprfx_is_unpredicated && zd_matches;
  }
  return false;
}  // NOLINT(readability/fn_size)

bool Instruction::IsLoad() const {
  if (Mask(LoadStoreAnyFMask) != LoadStoreAnyFixed) {
    return false;
  }

  if (Mask(LoadStorePairAnyFMask) == LoadStorePairAnyFixed) {
    return Mask(LoadStorePairLBit) != 0;
  } else {
    LoadStoreOp op = static_cast<LoadStoreOp>(Mask(LoadStoreMask));
    switch (op) {
      case LDRB_w:
      case LDRH_w:
      case LDR_w:
      case LDR_x:
      case LDRSB_w:
      case LDRSB_x:
      case LDRSH_w:
      case LDRSH_x:
      case LDRSW_x:
      case LDR_b:
      case LDR_h:
      case LDR_s:
      case LDR_d:
      case LDR_q:
        return true;
      default:
        return false;
    }
  }
}


bool Instruction::IsStore() const {
  if (Mask(LoadStoreAnyFMask) != LoadStoreAnyFixed) {
    return false;
  }

  if (Mask(LoadStorePairAnyFMask) == LoadStorePairAnyFixed) {
    return Mask(LoadStorePairLBit) == 0;
  } else {
    LoadStoreOp op = static_cast<LoadStoreOp>(Mask(LoadStoreMask));
    switch (op) {
      case STRB_w:
      case STRH_w:
      case STR_w:
      case STR_x:
      case STR_b:
      case STR_h:
      case STR_s:
      case STR_d:
      case STR_q:
        return true;
      default:
        return false;
    }
  }
}


std::pair<int, int> Instruction::GetSVEPermuteIndexAndLaneSizeLog2() const {
  uint32_t imm_2 = ExtractBits<0x00C00000>();
  uint32_t tsz_5 = ExtractBits<0x001F0000>();
  uint32_t imm_7 = (imm_2 << 5) | tsz_5;
  int lane_size_in_byte_log_2 = std::min(CountTrailingZeros(tsz_5), 5);
  int index = ExtractUnsignedBitfield32(6, lane_size_in_byte_log_2 + 1, imm_7);
  return std::make_pair(index, lane_size_in_byte_log_2);
}


// Logical immediates can't encode zero, so a return value of zero is used to
// indicate a failure case. Specifically, where the constraints on imm_s are
// not met.
uint64_t Instruction::GetImmLogical() const {
  unsigned reg_size = GetSixtyFourBits() ? kXRegSize : kWRegSize;
  int32_t n = GetBitN();
  int32_t imm_s = GetImmSetBits();
  int32_t imm_r = GetImmRotate();
  return DecodeImmBitMask(n, imm_s, imm_r, reg_size);
}

// Logical immediates can't encode zero, so a return value of zero is used to
// indicate a failure case. Specifically, where the constraints on imm_s are
// not met.
uint64_t Instruction::GetSVEImmLogical() const {
  int n = GetSVEBitN();
  int imm_s = GetSVEImmSetBits();
  int imm_r = GetSVEImmRotate();
  int lane_size_in_bytes_log2 = GetSVEBitwiseImmLaneSizeInBytesLog2();
  switch (lane_size_in_bytes_log2) {
    case kDRegSizeInBytesLog2:
    case kSRegSizeInBytesLog2:
    case kHRegSizeInBytesLog2:
    case kBRegSizeInBytesLog2: {
      int lane_size_in_bits = 1 << (lane_size_in_bytes_log2 + 3);
      return DecodeImmBitMask(n, imm_s, imm_r, lane_size_in_bits);
    }
    default:
      return 0;
  }
}

std::pair<int, int> Instruction::GetSVEImmShiftAndLaneSizeLog2(
    bool is_predicated) const {
  Instr tsize =
      is_predicated ? ExtractBits<0x00C00300>() : ExtractBits<0x00D80000>();
  Instr imm_3 =
      is_predicated ? ExtractBits<0x000000E0>() : ExtractBits<0x00070000>();
  if (tsize == 0) {
    // The bit field `tsize` means undefined if it is zero, so return a
    // convenience value kWMinInt to indicate a failure case.
    return std::make_pair(kWMinInt, kWMinInt);
  }

  int lane_size_in_bytes_log_2 = 32 - CountLeadingZeros(tsize, 32) - 1;
  int esize = (1 << lane_size_in_bytes_log_2) * kBitsPerByte;
  int shift = (2 * esize) - ((tsize << 3) | imm_3);
  return std::make_pair(shift, lane_size_in_bytes_log_2);
}

int Instruction::GetSVEMsizeFromDtype(bool is_signed, int dtype_h_lsb) const {
  Instr dtype_h = ExtractBits(dtype_h_lsb + 1, dtype_h_lsb);
  if (is_signed) {
    dtype_h = dtype_h ^ 0x3;
  }
  return dtype_h;
}

int Instruction::GetSVEEsizeFromDtype(bool is_signed, int dtype_l_lsb) const {
  Instr dtype_l = ExtractBits(dtype_l_lsb + 1, dtype_l_lsb);
  if (is_signed) {
    dtype_l = dtype_l ^ 0x3;
  }
  return dtype_l;
}

int Instruction::GetSVEBitwiseImmLaneSizeInBytesLog2() const {
  int n = GetSVEBitN();
  int imm_s = GetSVEImmSetBits();
  unsigned type_bitset =
      (n << SVEImmSetBits_width) | (~imm_s & GetUintMask(SVEImmSetBits_width));

  // An lane size is constructed from the n and imm_s bits according to
  // the following table:
  //
  // N   imms   size
  // 0  0xxxxx   32
  // 0  10xxxx   16
  // 0  110xxx    8
  // 0  1110xx    8
  // 0  11110x    8
  // 1  xxxxxx   64

  if (type_bitset == 0) {
    // Bail out early since `HighestSetBitPosition` doesn't accept zero
    // value input.
    return -1;
  }

  switch (HighestSetBitPosition(type_bitset)) {
    case 6:
      return kDRegSizeInBytesLog2;
    case 5:
      return kSRegSizeInBytesLog2;
    case 4:
      return kHRegSizeInBytesLog2;
    case 3:
    case 2:
    case 1:
      return kBRegSizeInBytesLog2;
    default:
      // RESERVED encoding.
      return -1;
  }
}

uint64_t Instruction::DecodeImmBitMask(int32_t n,
                                       int32_t imm_s,
                                       int32_t imm_r,
                                       int32_t size) const {
  // An integer is constructed from the n, imm_s and imm_r bits according to
  // the following table:
  //
  //  N   imms    immr    size        S             R
  //  1  ssssss  rrrrrr    64    UInt(ssssss)  UInt(rrrrrr)
  //  0  0sssss  xrrrrr    32    UInt(sssss)   UInt(rrrrr)
  //  0  10ssss  xxrrrr    16    UInt(ssss)    UInt(rrrr)
  //  0  110sss  xxxrrr     8    UInt(sss)     UInt(rrr)
  //  0  1110ss  xxxxrr     4    UInt(ss)      UInt(rr)
  //  0  11110s  xxxxxr     2    UInt(s)       UInt(r)
  // (s bits must not be all set)
  //
  // A pattern is constructed of size bits, where the least significant S+1
  // bits are set. The pattern is rotated right by R, and repeated across a
  // 32 or 64-bit value, depending on destination register width.
  //

  if (n == 1) {
    if (imm_s == 0x3f) {
      return 0;
    }
    uint64_t bits = (UINT64_C(1) << (imm_s + 1)) - 1;
    return RotateRight(bits, imm_r, 64);
  } else {
    if ((imm_s >> 1) == 0x1f) {
      return 0;
    }
    for (int width = 0x20; width >= 0x2; width >>= 1) {
      if ((imm_s & width) == 0) {
        int mask = width - 1;
        if ((imm_s & mask) == mask) {
          return 0;
        }
        uint64_t bits = (UINT64_C(1) << ((imm_s & mask) + 1)) - 1;
        return RepeatBitsAcrossReg(size,
                                   RotateRight(bits, imm_r & mask, width),
                                   width);
      }
    }
  }
  VIXL_UNREACHABLE();
  return 0;
}


uint32_t Instruction::GetImmNEONabcdefgh() const {
  return GetImmNEONabc() << 5 | GetImmNEONdefgh();
}


Float16 Instruction::Imm8ToFloat16(uint32_t imm8) {
  // Imm8: abcdefgh (8 bits)
  // Half: aBbb.cdef.gh00.0000 (16 bits)
  // where B is b ^ 1
  uint32_t bits = imm8;
  uint16_t bit7 = (bits >> 7) & 0x1;
  uint16_t bit6 = (bits >> 6) & 0x1;
  uint16_t bit5_to_0 = bits & 0x3f;
  uint16_t result = (bit7 << 15) | ((4 - bit6) << 12) | (bit5_to_0 << 6);
  return RawbitsToFloat16(result);
}


float Instruction::Imm8ToFP32(uint32_t imm8) {
  // Imm8: abcdefgh (8 bits)
  // Single: aBbb.bbbc.defg.h000.0000.0000.0000.0000 (32 bits)
  // where B is b ^ 1
  uint32_t bits = imm8;
  uint32_t bit7 = (bits >> 7) & 0x1;
  uint32_t bit6 = (bits >> 6) & 0x1;
  uint32_t bit5_to_0 = bits & 0x3f;
  uint32_t result = (bit7 << 31) | ((32 - bit6) << 25) | (bit5_to_0 << 19);

  return RawbitsToFloat(result);
}


Float16 Instruction::GetImmFP16() const { return Imm8ToFloat16(GetImmFP()); }


float Instruction::GetImmFP32() const { return Imm8ToFP32(GetImmFP()); }


double Instruction::Imm8ToFP64(uint32_t imm8) {
  // Imm8: abcdefgh (8 bits)
  // Double: aBbb.bbbb.bbcd.efgh.0000.0000.0000.0000
  //         0000.0000.0000.0000.0000.0000.0000.0000 (64 bits)
  // where B is b ^ 1
  uint32_t bits = imm8;
  uint64_t bit7 = (bits >> 7) & 0x1;
  uint64_t bit6 = (bits >> 6) & 0x1;
  uint64_t bit5_to_0 = bits & 0x3f;
  uint64_t result = (bit7 << 63) | ((256 - bit6) << 54) | (bit5_to_0 << 48);

  return RawbitsToDouble(result);
}


double Instruction::GetImmFP64() const { return Imm8ToFP64(GetImmFP()); }


Float16 Instruction::GetImmNEONFP16() const {
  return Imm8ToFloat16(GetImmNEONabcdefgh());
}


float Instruction::GetImmNEONFP32() const {
  return Imm8ToFP32(GetImmNEONabcdefgh());
}


double Instruction::GetImmNEONFP64() const {
  return Imm8ToFP64(GetImmNEONabcdefgh());
}


unsigned CalcLSDataSize(LoadStoreOp op) {
  VIXL_ASSERT((LSSize_offset + LSSize_width) == (kInstructionSize * 8));
  unsigned size = static_cast<Instr>(op) >> LSSize_offset;
  if ((op & LSVector_mask) != 0) {
    // Vector register memory operations encode the access size in the "size"
    // and "opc" fields.
    if ((size == 0) && ((op & LSOpc_mask) >> LSOpc_offset) >= 2) {
      size = kQRegSizeInBytesLog2;
    }
  }
  return size;
}


unsigned CalcLSPairDataSize(LoadStorePairOp op) {
  VIXL_STATIC_ASSERT(kXRegSizeInBytes == kDRegSizeInBytes);
  VIXL_STATIC_ASSERT(kWRegSizeInBytes == kSRegSizeInBytes);
  switch (op) {
    case STP_q:
    case LDP_q:
      return kQRegSizeInBytesLog2;
    case STP_x:
    case LDP_x:
    case STP_d:
    case LDP_d:
      return kXRegSizeInBytesLog2;
    default:
      return kWRegSizeInBytesLog2;
  }
}


int Instruction::GetImmBranchRangeBitwidth(ImmBranchType branch_type) {
  switch (branch_type) {
    case UncondBranchType:
      return ImmUncondBranch_width;
    case CondBranchType:
      return ImmCondBranch_width;
    case CompareBranchType:
      return ImmCmpBranch_width;
    case TestBranchType:
      return ImmTestBranch_width;
    default:
      VIXL_UNREACHABLE();
      return 0;
  }
}


int32_t Instruction::GetImmBranchForwardRange(ImmBranchType branch_type) {
  int32_t encoded_max = 1 << (GetImmBranchRangeBitwidth(branch_type) - 1);
  return encoded_max * kInstructionSize;
}


bool Instruction::IsValidImmPCOffset(ImmBranchType branch_type,
                                     int64_t offset) {
  return IsIntN(GetImmBranchRangeBitwidth(branch_type), offset);
}


const Instruction* Instruction::GetImmPCOffsetTarget() const {
  const Instruction* base = this;
  ptrdiff_t offset;
  if (IsPCRelAddressing()) {
    // ADR and ADRP.
    offset = GetImmPCRel();
    if (Mask(PCRelAddressingMask) == ADRP) {
      base = AlignDown(base, kPageSize);
      offset *= kPageSize;
    } else {
      VIXL_ASSERT(Mask(PCRelAddressingMask) == ADR);
    }
  } else {
    // All PC-relative branches.
    VIXL_ASSERT(GetBranchType() != UnknownBranchType);
    // Relative branch offsets are instruction-size-aligned.
    offset = GetImmBranch() * static_cast<int>(kInstructionSize);
  }
  return base + offset;
}


int Instruction::GetImmBranch() const {
  switch (GetBranchType()) {
    case CondBranchType:
      return GetImmCondBranch();
    case UncondBranchType:
      return GetImmUncondBranch();
    case CompareBranchType:
      return GetImmCmpBranch();
    case TestBranchType:
      return GetImmTestBranch();
    default:
      VIXL_UNREACHABLE();
  }
  return 0;
}


void Instruction::SetImmPCOffsetTarget(const Instruction* target) {
  if (IsPCRelAddressing()) {
    SetPCRelImmTarget(target);
  } else {
    SetBranchImmTarget(target);
  }
}


void Instruction::SetPCRelImmTarget(const Instruction* target) {
  ptrdiff_t imm21;
  if ((Mask(PCRelAddressingMask) == ADR)) {
    imm21 = target - this;
  } else {
    VIXL_ASSERT(Mask(PCRelAddressingMask) == ADRP);
    uintptr_t this_page = reinterpret_cast<uintptr_t>(this) / kPageSize;
    uintptr_t target_page = reinterpret_cast<uintptr_t>(target) / kPageSize;
    imm21 = target_page - this_page;
  }
  Instr imm = Assembler::ImmPCRelAddress(static_cast<int32_t>(imm21));

  SetInstructionBits(Mask(~ImmPCRel_mask) | imm);
}


void Instruction::SetBranchImmTarget(const Instruction* target) {
  VIXL_ASSERT(((target - this) & 3) == 0);
  Instr branch_imm = 0;
  uint32_t imm_mask = 0;
  int offset = static_cast<int>((target - this) >> kInstructionSizeLog2);
  switch (GetBranchType()) {
    case CondBranchType: {
      branch_imm = Assembler::ImmCondBranch(offset);
      imm_mask = ImmCondBranch_mask;
      break;
    }
    case UncondBranchType: {
      branch_imm = Assembler::ImmUncondBranch(offset);
      imm_mask = ImmUncondBranch_mask;
      break;
    }
    case CompareBranchType: {
      branch_imm = Assembler::ImmCmpBranch(offset);
      imm_mask = ImmCmpBranch_mask;
      break;
    }
    case TestBranchType: {
      branch_imm = Assembler::ImmTestBranch(offset);
      imm_mask = ImmTestBranch_mask;
      break;
    }
    default:
      VIXL_UNREACHABLE();
  }
  SetInstructionBits(Mask(~imm_mask) | branch_imm);
}


void Instruction::SetImmLLiteral(const Instruction* source) {
  VIXL_ASSERT(IsWordAligned(source));
  ptrdiff_t offset = (source - this) >> kLiteralEntrySizeLog2;
  Instr imm = Assembler::ImmLLiteral(static_cast<int>(offset));
  Instr mask = ImmLLiteral_mask;

  SetInstructionBits(Mask(~mask) | imm);
}


VectorFormat VectorFormatHalfWidth(VectorFormat vform) {
  switch (vform) {
    case kFormat8H:
      return kFormat8B;
    case kFormat4S:
      return kFormat4H;
    case kFormat2D:
      return kFormat2S;
    case kFormatH:
      return kFormatB;
    case kFormatS:
      return kFormatH;
    case kFormatD:
      return kFormatS;
    case kFormatVnH:
      return kFormatVnB;
    case kFormatVnS:
      return kFormatVnH;
    case kFormatVnD:
      return kFormatVnS;
      break;
    default:
      VIXL_UNREACHABLE();
      return kFormatUndefined;
  }
}


VectorFormat VectorFormatDoubleWidth(VectorFormat vform) {
  VIXL_ASSERT(vform == kFormat8B || vform == kFormat4H || vform == kFormat2S ||
              vform == kFormatB || vform == kFormatH || vform == kFormatS);
  switch (vform) {
    case kFormat8B:
      return kFormat8H;
    case kFormat4H:
      return kFormat4S;
    case kFormat2S:
      return kFormat2D;
    case kFormatB:
      return kFormatH;
    case kFormatH:
      return kFormatS;
    case kFormatS:
      return kFormatD;
    default:
      VIXL_UNREACHABLE();
      return kFormatUndefined;
  }
}


VectorFormat VectorFormatFillQ(VectorFormat vform) {
  switch (vform) {
    case kFormatB:
    case kFormat8B:
    case kFormat16B:
      return kFormat16B;
    case kFormatH:
    case kFormat4H:
    case kFormat8H:
      return kFormat8H;
    case kFormatS:
    case kFormat2S:
    case kFormat4S:
      return kFormat4S;
    case kFormatD:
    case kFormat1D:
    case kFormat2D:
      return kFormat2D;
    default:
      VIXL_UNREACHABLE();
      return kFormatUndefined;
  }
}

VectorFormat VectorFormatHalfWidthDoubleLanes(VectorFormat vform) {
  switch (vform) {
    case kFormat4H:
      return kFormat8B;
    case kFormat8H:
      return kFormat16B;
    case kFormat2S:
      return kFormat4H;
    case kFormat4S:
      return kFormat8H;
    case kFormat1D:
      return kFormat2S;
    case kFormat2D:
      return kFormat4S;
    case kFormatVnH:
      return kFormatVnB;
    case kFormatVnS:
      return kFormatVnH;
    case kFormatVnD:
      return kFormatVnS;
    default:
      VIXL_UNREACHABLE();
      return kFormatUndefined;
  }
}

VectorFormat VectorFormatDoubleLanes(VectorFormat vform) {
  VIXL_ASSERT(vform == kFormat8B || vform == kFormat4H || vform == kFormat2S);
  switch (vform) {
    case kFormat8B:
      return kFormat16B;
    case kFormat4H:
      return kFormat8H;
    case kFormat2S:
      return kFormat4S;
    default:
      VIXL_UNREACHABLE();
      return kFormatUndefined;
  }
}


VectorFormat VectorFormatHalfLanes(VectorFormat vform) {
  VIXL_ASSERT(vform == kFormat16B || vform == kFormat8H || vform == kFormat4S);
  switch (vform) {
    case kFormat16B:
      return kFormat8B;
    case kFormat8H:
      return kFormat4H;
    case kFormat4S:
      return kFormat2S;
    default:
      VIXL_UNREACHABLE();
      return kFormatUndefined;
  }
}


VectorFormat ScalarFormatFromLaneSize(int lane_size_in_bits) {
  switch (lane_size_in_bits) {
    case 8:
      return kFormatB;
    case 16:
      return kFormatH;
    case 32:
      return kFormatS;
    case 64:
      return kFormatD;
    default:
      VIXL_UNREACHABLE();
      return kFormatUndefined;
  }
}


bool IsSVEFormat(VectorFormat vform) {
  switch (vform) {
    case kFormatVnB:
    case kFormatVnH:
    case kFormatVnS:
    case kFormatVnD:
    case kFormatVnQ:
      return true;
    default:
      return false;
  }
}


VectorFormat SVEFormatFromLaneSizeInBytes(int lane_size_in_bytes) {
  switch (lane_size_in_bytes) {
    case 1:
      return kFormatVnB;
    case 2:
      return kFormatVnH;
    case 4:
      return kFormatVnS;
    case 8:
      return kFormatVnD;
    case 16:
      return kFormatVnQ;
    default:
      VIXL_UNREACHABLE();
      return kFormatUndefined;
  }
}


VectorFormat SVEFormatFromLaneSizeInBits(int lane_size_in_bits) {
  switch (lane_size_in_bits) {
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
      return SVEFormatFromLaneSizeInBytes(lane_size_in_bits / kBitsPerByte);
    default:
      VIXL_UNREACHABLE();
      return kFormatUndefined;
  }
}


VectorFormat SVEFormatFromLaneSizeInBytesLog2(int lane_size_in_bytes_log2) {
  switch (lane_size_in_bytes_log2) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
      return SVEFormatFromLaneSizeInBytes(1 << lane_size_in_bytes_log2);
    default:
      VIXL_UNREACHABLE();
      return kFormatUndefined;
  }
}


VectorFormat ScalarFormatFromFormat(VectorFormat vform) {
  return ScalarFormatFromLaneSize(LaneSizeInBitsFromFormat(vform));
}


unsigned RegisterSizeInBitsFromFormat(VectorFormat vform) {
  VIXL_ASSERT(vform != kFormatUndefined);
  VIXL_ASSERT(!IsSVEFormat(vform));
  switch (vform) {
    case kFormatB:
      return kBRegSize;
    case kFormatH:
      return kHRegSize;
    case kFormatS:
    case kFormat2H:
      return kSRegSize;
    case kFormatD:
    case kFormat8B:
    case kFormat4H:
    case kFormat2S:
    case kFormat1D:
      return kDRegSize;
    case kFormat16B:
    case kFormat8H:
    case kFormat4S:
    case kFormat2D:
      return kQRegSize;
    default:
      VIXL_UNREACHABLE();
      return 0;
  }
}


unsigned RegisterSizeInBytesFromFormat(VectorFormat vform) {
  return RegisterSizeInBitsFromFormat(vform) / 8;
}


unsigned LaneSizeInBitsFromFormat(VectorFormat vform) {
  VIXL_ASSERT(vform != kFormatUndefined);
  switch (vform) {
    case kFormatB:
    case kFormat8B:
    case kFormat16B:
    case kFormatVnB:
      return 8;
    case kFormatH:
    case kFormat2H:
    case kFormat4H:
    case kFormat8H:
    case kFormatVnH:
      return 16;
    case kFormatS:
    case kFormat2S:
    case kFormat4S:
    case kFormatVnS:
      return 32;
    case kFormatD:
    case kFormat1D:
    case kFormat2D:
    case kFormatVnD:
      return 64;
    case kFormatVnQ:
      return 128;
    default:
      VIXL_UNREACHABLE();
      return 0;
  }
}


int LaneSizeInBytesFromFormat(VectorFormat vform) {
  return LaneSizeInBitsFromFormat(vform) / 8;
}


int LaneSizeInBytesLog2FromFormat(VectorFormat vform) {
  VIXL_ASSERT(vform != kFormatUndefined);
  switch (vform) {
    case kFormatB:
    case kFormat8B:
    case kFormat16B:
    case kFormatVnB:
      return 0;
    case kFormatH:
    case kFormat2H:
    case kFormat4H:
    case kFormat8H:
    case kFormatVnH:
      return 1;
    case kFormatS:
    case kFormat2S:
    case kFormat4S:
    case kFormatVnS:
      return 2;
    case kFormatD:
    case kFormat1D:
    case kFormat2D:
    case kFormatVnD:
      return 3;
    case kFormatVnQ:
      return 4;
    default:
      VIXL_UNREACHABLE();
      return 0;
  }
}


int LaneCountFromFormat(VectorFormat vform) {
  VIXL_ASSERT(vform != kFormatUndefined);
  switch (vform) {
    case kFormat16B:
      return 16;
    case kFormat8B:
    case kFormat8H:
      return 8;
    case kFormat4H:
    case kFormat4S:
      return 4;
    case kFormat2H:
    case kFormat2S:
    case kFormat2D:
      return 2;
    case kFormat1D:
    case kFormatB:
    case kFormatH:
    case kFormatS:
    case kFormatD:
      return 1;
    default:
      VIXL_UNREACHABLE();
      return 0;
  }
}


int MaxLaneCountFromFormat(VectorFormat vform) {
  VIXL_ASSERT(vform != kFormatUndefined);
  switch (vform) {
    case kFormatB:
    case kFormat8B:
    case kFormat16B:
      return 16;
    case kFormatH:
    case kFormat4H:
    case kFormat8H:
      return 8;
    case kFormatS:
    case kFormat2S:
    case kFormat4S:
      return 4;
    case kFormatD:
    case kFormat1D:
    case kFormat2D:
      return 2;
    default:
      VIXL_UNREACHABLE();
      return 0;
  }
}


// Does 'vform' indicate a vector format or a scalar format?
bool IsVectorFormat(VectorFormat vform) {
  VIXL_ASSERT(vform != kFormatUndefined);
  switch (vform) {
    case kFormatB:
    case kFormatH:
    case kFormatS:
    case kFormatD:
      return false;
    default:
      return true;
  }
}


int64_t MaxIntFromFormat(VectorFormat vform) {
  int lane_size = LaneSizeInBitsFromFormat(vform);
  return static_cast<int64_t>(GetUintMask(lane_size) >> 1);
}


int64_t MinIntFromFormat(VectorFormat vform) {
  return -MaxIntFromFormat(vform) - 1;
}


uint64_t MaxUintFromFormat(VectorFormat vform) {
  return GetUintMask(LaneSizeInBitsFromFormat(vform));
}

}  // namespace aarch64
}  // namespace vixl
