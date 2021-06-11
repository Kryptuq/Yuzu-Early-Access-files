// Copyright 2016, VIXL authors
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

#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fstream>
#include <regex>

#include "test-runner.h"
#include "test-utils-aarch64.h"

#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/macro-assembler-aarch64.h"
#include "aarch64/simulator-aarch64.h"

namespace vixl {
namespace aarch64 {

#define __ masm->
#define TEST(name) TEST_(TRACE_##name)

#define REF(name) "test/test-trace-reference/" name

static void GenerateTestSequenceBase(MacroAssembler* masm) {
  ExactAssemblyScope guard(masm,
                           masm->GetBuffer()->GetRemainingBytes(),
                           ExactAssemblyScope::kMaximumSize);

  __ adc(w3, w4, w5);
  __ adc(x6, x7, x8);
  __ adcs(w9, w10, w11);
  __ adcs(x12, x13, x14);
  __ add(w15, w16, w17);
  __ add(x18, x19, x20);
  __ adds(w21, w22, w23);
  __ adds(x24, x25, x26);
  __ and_(w27, w28, w29);
  __ and_(x2, x3, x4);
  __ ands(w5, w6, w7);
  __ ands(x8, x9, x10);
  __ asr(w11, w12, 0);
  __ asr(x13, x14, 1);
  __ asrv(w15, w16, w17);
  __ asrv(x18, x19, x20);
  __ bfm(w21, w22, 5, 6);
  __ bfm(x23, x24, 7, 8);
  __ bic(w25, w26, w27);
  __ bic(x28, x29, x2);
  __ bics(w3, w4, w5);
  __ bics(x6, x7, x8);
  __ ccmn(w9, w10, NoFlag, al);
  __ ccmn(w9, w10, NoFlag, eq);
  __ ccmn(w9, w10, NoFlag, ne);
  __ ccmn(x11, x12, CFlag, al);
  __ ccmn(x11, x12, CFlag, cc);
  __ ccmn(x11, x12, CFlag, cs);
  __ ccmp(w13, w14, VFlag, al);
  __ ccmp(w13, w14, VFlag, hi);
  __ ccmp(w13, w14, VFlag, ls);
  __ ccmp(x15, x16, CVFlag, al);
  __ ccmp(x15, x16, CVFlag, eq);
  __ ccmp(x15, x16, CVFlag, ne);
  __ cinc(w17, w18, cc);
  __ cinc(w17, w18, cs);
  __ cinc(x19, x20, hi);
  __ cinc(x19, x20, ls);
  __ cinv(w21, w22, eq);
  __ cinv(w21, w22, ne);
  __ cinv(x23, x24, cc);
  __ cinv(x23, x24, cs);
  __ clrex();
  __ cls(w25, w26);
  __ cls(x27, x28);
  __ clz(w29, w2);
  __ clz(x3, x4);
  __ cmn(w5, w6);
  __ cmn(x7, x8);
  __ cmp(w9, w10);
  __ cmp(x11, x12);
  __ cneg(w13, w14, hi);
  __ cneg(w13, w14, ls);
  __ cneg(x15, x16, eq);
  __ cneg(x15, x16, ne);
  __ crc32b(w17, w18, w19);
  __ crc32cb(w20, w21, w22);
  __ crc32ch(w23, w24, w25);
  __ crc32cw(w26, w27, w28);
  __ crc32h(w4, w5, w6);
  __ crc32w(w7, w8, w9);
  __ csel(w13, w14, w15, cc);
  __ csel(w13, w14, w15, cs);
  __ csel(x16, x17, x18, hi);
  __ csel(x16, x17, x18, ls);
  __ cset(w19, eq);
  __ cset(w19, ne);
  __ cset(x20, cc);
  __ cset(x20, cs);
  __ csetm(w21, hi);
  __ csetm(w21, ls);
  __ csetm(x22, eq);
  __ csetm(x22, ne);
  __ csinc(w23, w24, w25, cc);
  __ csinc(w23, w24, w25, cs);
  __ csinc(x26, x27, x28, hi);
  __ csinc(x26, x27, x28, ls);
  __ csinv(w29, w2, w3, eq);
  __ csinv(w29, w2, w3, ne);
  __ csinv(x4, x5, x6, cc);
  __ csinv(x4, x5, x6, cs);
  __ csneg(w7, w8, w9, hi);
  __ csneg(w7, w8, w9, ls);
  __ csneg(x10, x11, x12, eq);
  __ csneg(x10, x11, x12, ne);
  __ dc(CVAC, x0);
  __ dmb(InnerShareable, BarrierAll);
  __ dsb(InnerShareable, BarrierAll);
  __ eon(w13, w14, w15);
  __ eon(x16, x17, x18);
  __ eor(w19, w20, w21);
  __ eor(x22, x23, x24);
  __ extr(w25, w26, w27, 9);
  __ extr(x28, x29, x2, 10);
  __ hint(NOP);
  __ ic(IVAU, x0);
  __ isb();
  __ ldar(w3, MemOperand(x0));
  __ ldar(x4, MemOperand(x0));
  __ ldarb(w5, MemOperand(x0));
  __ ldarb(x6, MemOperand(x0));
  __ ldarh(w7, MemOperand(x0));
  __ ldarh(x8, MemOperand(x0));
  __ ldaxp(w9, w10, MemOperand(x0));
  __ ldaxp(x11, x12, MemOperand(x0));
  __ ldaxr(w13, MemOperand(x0));
  __ ldaxr(x14, MemOperand(x0));
  __ ldaxrb(w15, MemOperand(x0));
  __ ldaxrb(x16, MemOperand(x0));
  __ ldaxrh(w17, MemOperand(x0));
  __ ldaxrh(x18, MemOperand(x0));
  __ ldnp(w19, w20, MemOperand(x0));
  __ ldnp(x21, x22, MemOperand(x0));
  __ ldp(w23, w24, MemOperand(x0));
  __ ldp(w23, w24, MemOperand(x1, 8, PostIndex));
  __ ldp(w23, w24, MemOperand(x1, 8, PreIndex));
  __ ldp(x25, x26, MemOperand(x0));
  __ ldp(x25, x26, MemOperand(x1, 16, PostIndex));
  __ ldp(x25, x26, MemOperand(x1, 16, PreIndex));
  __ ldpsw(x27, x28, MemOperand(x0));
  __ ldpsw(x27, x28, MemOperand(x1, 8, PostIndex));
  __ ldpsw(x27, x28, MemOperand(x1, 8, PreIndex));
  __ ldr(w29, MemOperand(x0));
  __ ldr(w29, MemOperand(x1, 4, PostIndex));
  __ ldr(w29, MemOperand(x1, 4, PreIndex));
  __ ldr(x2, MemOperand(x0));
  __ ldr(x2, MemOperand(x1, 8, PostIndex));
  __ ldr(x2, MemOperand(x1, 8, PreIndex));
  __ ldrb(w3, MemOperand(x0));
  __ ldrb(w3, MemOperand(x1, 1, PostIndex));
  __ ldrb(w3, MemOperand(x1, 1, PreIndex));
  __ ldrb(x4, MemOperand(x0));
  __ ldrb(x4, MemOperand(x1, 1, PostIndex));
  __ ldrb(x4, MemOperand(x1, 1, PreIndex));
  __ ldrh(w5, MemOperand(x0));
  __ ldrh(w5, MemOperand(x1, 2, PostIndex));
  __ ldrh(w5, MemOperand(x1, 2, PreIndex));
  __ ldrh(x6, MemOperand(x0));
  __ ldrh(x6, MemOperand(x1, 2, PostIndex));
  __ ldrh(x6, MemOperand(x1, 2, PreIndex));
  __ ldrsb(w7, MemOperand(x0));
  __ ldrsb(w7, MemOperand(x1, 1, PostIndex));
  __ ldrsb(w7, MemOperand(x1, 1, PreIndex));
  __ ldrsb(x8, MemOperand(x0));
  __ ldrsb(x8, MemOperand(x1, 1, PostIndex));
  __ ldrsb(x8, MemOperand(x1, 1, PreIndex));
  __ ldrsh(w9, MemOperand(x0));
  __ ldrsh(w9, MemOperand(x1, 2, PostIndex));
  __ ldrsh(w9, MemOperand(x1, 2, PreIndex));
  __ ldrsh(x10, MemOperand(x0));
  __ ldrsh(x10, MemOperand(x1, 2, PostIndex));
  __ ldrsh(x10, MemOperand(x1, 2, PreIndex));
  __ ldrsw(x11, MemOperand(x0));
  __ ldrsw(x11, MemOperand(x1, 4, PostIndex));
  __ ldrsw(x11, MemOperand(x1, 4, PreIndex));
  __ ldur(w12, MemOperand(x0, 7));
  __ ldur(x13, MemOperand(x0, 15));
  __ ldurb(w14, MemOperand(x0, 1));
  __ ldurb(x15, MemOperand(x0, 1));
  __ ldurh(w16, MemOperand(x0, 3));
  __ ldurh(x17, MemOperand(x0, 3));
  __ ldursb(w18, MemOperand(x0, 1));
  __ ldursb(x19, MemOperand(x0, 1));
  __ ldursh(w20, MemOperand(x0, 3));
  __ ldursh(x21, MemOperand(x0, 3));
  __ ldursw(x22, MemOperand(x0, 7));
  __ ldxp(w23, w24, MemOperand(x0));
  __ ldxp(x25, x26, MemOperand(x0));
  __ ldxr(w27, MemOperand(x0));
  __ ldxr(x28, MemOperand(x0));
  __ ldxrb(w29, MemOperand(x0));
  __ ldxrb(x2, MemOperand(x0));
  __ ldxrh(w3, MemOperand(x0));
  __ ldxrh(x4, MemOperand(x0));
  __ lsl(w5, w6, 2);
  __ lsl(x7, x8, 3);
  __ lslv(w9, w10, w11);
  __ lslv(x12, x13, x14);
  __ lsr(w15, w16, 4);
  __ lsr(x17, x18, 5);
  __ lsrv(w19, w20, w21);
  __ lsrv(x22, x23, x24);
  __ madd(w25, w26, w27, w28);
  __ madd(x29, x2, x3, x4);
  __ mneg(w5, w6, w7);
  __ mneg(x8, x9, x10);
  __ mov(w11, w12);
  __ mov(x13, x14);
  __ movk(w15, 130);
  __ movk(x16, 131);
  __ movn(w17, 132);
  __ movn(x18, 133);
  __ movz(w19, 134);
  __ movz(x20, 135);
  __ msub(w22, w23, w24, w25);
  __ msub(x26, x27, x28, x29);
  __ mul(w2, w3, w4);
  __ mul(x5, x6, x7);
  __ mvn(w8, w9);
  __ mvn(x10, x11);
  __ neg(w12, w13);
  __ neg(x14, x15);
  __ negs(w16, w17);
  __ negs(x18, x19);
  __ ngc(w20, w21);
  __ ngc(x22, x23);
  __ ngcs(w24, w25);
  __ ngcs(x26, x27);
  __ nop();
  __ orn(w28, w29, w2);
  __ orn(x3, x4, x5);
  __ orr(w6, w7, w8);
  __ orr(x9, x10, x11);
  __ prfm(PLDL1KEEP, MemOperand(x0, 4));
  __ prfum(PLDL1KEEP, MemOperand(x0, 1));
  __ rbit(w12, w13);
  __ rbit(x14, x15);
  __ rev(w16, w17);
  __ rev(x18, x19);
  __ rev16(w20, w21);
  __ rev16(x22, x23);
  __ rev32(x24, x25);
  __ rorv(w26, w27, w28);
  __ rorv(x29, x2, x3);
  __ sbc(w4, w5, w6);
  __ sbc(x7, x8, x9);
  __ sbcs(w10, w11, w12);
  __ sbcs(x13, x14, x15);
  __ sbfiz(w16, w17, 2, 3);
  __ sbfiz(x18, x19, 4, 5);
  __ sbfx(w22, w23, 6, 7);
  __ sbfx(x24, x25, 8, 9);
  __ sdiv(w26, w27, w28);
  __ sdiv(x29, x2, x3);
  __ smulh(x12, x13, x14);
  __ stlr(w18, MemOperand(x0));
  __ stlr(x19, MemOperand(x0));
  __ stlrb(w20, MemOperand(x0));
  __ stlrb(x21, MemOperand(x0));
  __ stlrh(w22, MemOperand(x0));
  __ stlrh(x23, MemOperand(x0));
  __ stlxp(w24, w25, w26, MemOperand(x0));
  __ stlxp(x27, x28, x29, MemOperand(x0));
  __ stlxr(w2, w3, MemOperand(x0));
  __ stlxr(x4, x5, MemOperand(x0));
  __ stlxrb(w6, w7, MemOperand(x0));
  __ stlxrb(x8, x9, MemOperand(x0));
  __ stlxrh(w10, w11, MemOperand(x0));
  __ stlxrh(x12, x13, MemOperand(x0));
  __ stnp(w14, w15, MemOperand(x0));
  __ stnp(x16, x17, MemOperand(x0));
  __ stp(w18, w19, MemOperand(x0));
  __ stp(w18, w19, MemOperand(x1, 8, PostIndex));
  __ stp(w18, w19, MemOperand(x1, 8, PreIndex));
  __ stp(x20, x21, MemOperand(x0));
  __ stp(x20, x21, MemOperand(x1, 16, PostIndex));
  __ stp(x20, x21, MemOperand(x1, 16, PreIndex));
  __ str(w22, MemOperand(x0));
  __ str(w22, MemOperand(x1, 4, PostIndex));
  __ str(w22, MemOperand(x1, 4, PreIndex));
  __ str(x23, MemOperand(x0));
  __ str(x23, MemOperand(x1, 8, PostIndex));
  __ str(x23, MemOperand(x1, 8, PreIndex));
  __ strb(w24, MemOperand(x0));
  __ strb(w24, MemOperand(x1, 1, PostIndex));
  __ strb(w24, MemOperand(x1, 1, PreIndex));
  __ strb(x25, MemOperand(x0));
  __ strb(x25, MemOperand(x1, 1, PostIndex));
  __ strb(x25, MemOperand(x1, 1, PreIndex));
  __ strh(w26, MemOperand(x0));
  __ strh(w26, MemOperand(x1, 2, PostIndex));
  __ strh(w26, MemOperand(x1, 2, PreIndex));
  __ strh(x27, MemOperand(x0));
  __ strh(x27, MemOperand(x1, 2, PostIndex));
  __ strh(x27, MemOperand(x1, 2, PreIndex));
  __ stur(w28, MemOperand(x0, 7));
  __ stur(x29, MemOperand(x0, 15));
  __ sturb(w2, MemOperand(x0, 1));
  __ sturb(x3, MemOperand(x0, 1));
  __ sturh(w4, MemOperand(x0, 3));
  __ sturh(x5, MemOperand(x0, 3));
  __ stxp(w6, w7, w8, MemOperand(x0));
  __ stxp(x9, x10, x11, MemOperand(x0));
  __ stxr(w12, w13, MemOperand(x0));
  __ stxr(x14, x15, MemOperand(x0));
  __ stxrb(w16, w17, MemOperand(x0));
  __ stxrb(x18, x19, MemOperand(x0));
  __ stxrh(w20, w21, MemOperand(x0));
  __ stxrh(x22, x23, MemOperand(x0));
  __ sub(w24, w25, w26);
  __ sub(x27, x28, x29);
  __ subs(w2, w3, w4);
  __ subs(x5, x6, x7);
  __ sxtb(w8, w9);
  __ sxtb(x10, x11);
  __ sxth(w12, w13);
  __ sxth(x14, x15);
  __ sxtw(w16, w17);
  __ sxtw(x18, x19);
  __ tst(w20, w21);
  __ tst(x22, x23);
  __ ubfiz(w24, w25, 10, 11);
  __ ubfiz(x26, x27, 12, 13);
  __ ubfm(w28, w29, 14, 15);
  __ ubfm(x2, x3, 1, 2);
  __ ubfx(w4, w5, 3, 4);
  __ ubfx(x6, x7, 5, 6);
  __ udiv(w8, w9, w10);
  __ udiv(x11, x12, x13);
  __ umulh(x22, x23, x24);
  __ uxtb(w28, w29);
  __ uxtb(x2, x3);
  __ uxth(w4, w5);
  __ uxth(x6, x7);
  __ uxtw(w8, w9);
  __ uxtw(x10, x11);

  // Branch tests.
  {
    Label end;
    // Branch to the next instruction.
    __ b(&end);
    __ bind(&end);
  }
  {
    Label loop, end;
    __ subs(x3, x3, x3);
    __ bind(&loop);
    // Not-taken branch (the first time).
    // Taken branch (the second time).
    __ b(&end, ne);
    __ cmp(x3, 1);
    // Backwards branch.
    __ b(&loop);
    __ bind(&end);
  }
}


static void GenerateTestSequenceFP(MacroAssembler* masm) {
  ExactAssemblyScope guard(masm,
                           masm->GetBuffer()->GetRemainingBytes(),
                           ExactAssemblyScope::kMaximumSize);

  // Scalar floating point instructions.
  __ fabd(d13, d2, d19);
  __ fabd(s8, s10, s30);
  __ fabs(d1, d1);
  __ fabs(s25, s7);
  __ facge(d1, d23, d16);
  __ facge(s4, s17, s1);
  __ facgt(d2, d21, d24);
  __ facgt(s12, s26, s12);
  __ fadd(d13, d11, d22);
  __ fadd(s27, s19, s8);
  __ fccmp(d6, d10, NoFlag, hs);
  __ fccmp(s29, s20, NZVFlag, ne);
  __ fccmpe(d10, d2, NZCFlag, al);
  __ fccmpe(s3, s3, NZVFlag, pl);
  __ fcmeq(d19, d8, d10);
  __ fcmeq(d0, d18, 0.0);
  __ fcmeq(s1, s4, s30);
  __ fcmeq(s22, s29, 0.0);
  __ fcmge(d27, d18, d1);
  __ fcmge(d31, d28, 0.0);
  __ fcmge(s31, s19, s9);
  __ fcmge(s1, s25, 0.0);
  __ fcmgt(d18, d1, d15);
  __ fcmgt(d3, d31, 0.0);
  __ fcmgt(s11, s25, s2);
  __ fcmgt(s17, s16, 0.0);
  __ fcmle(d24, d17, 0.0);
  __ fcmle(s11, s8, 0.0);
  __ fcmlt(d5, d31, 0.0);
  __ fcmlt(s18, s23, 0.0);
  __ fcmp(d10, d24);
  __ fcmp(d13, 0.0);
  __ fcmp(s18, s6);
  __ fcmp(s16, 0.0);
  __ fcmpe(d9, d17);
  __ fcmpe(d29, 0.0);
  __ fcmpe(s16, s17);
  __ fcmpe(s22, 0.0);
  __ fcsel(d10, d14, d19, gt);
  __ fcsel(s22, s18, s2, ge);
  __ fcvt(d4, h24);
  __ fcvt(d11, s2);
  __ fcvt(h8, d9);
  __ fcvt(h12, s1);
  __ fcvt(s12, d31);
  __ fcvt(s27, h25);
  __ fcvtas(d28, d16);
  __ fcvtas(s3, s5);
  __ fcvtas(w18, d31);
  __ fcvtas(w29, s24);
  __ fcvtas(x9, d1);
  __ fcvtas(x30, s2);
  __ fcvtau(d14, d0);
  __ fcvtau(s31, s14);
  __ fcvtau(w16, d2);
  __ fcvtau(w18, s0);
  __ fcvtau(x26, d7);
  __ fcvtau(x25, s19);
  __ fcvtms(d30, d25);
  __ fcvtms(s12, s15);
  __ fcvtms(w9, d7);
  __ fcvtms(w19, s6);
  __ fcvtms(x6, d6);
  __ fcvtms(x22, s7);
  __ fcvtmu(d27, d0);
  __ fcvtmu(s8, s22);
  __ fcvtmu(w29, d19);
  __ fcvtmu(w26, s0);
  __ fcvtmu(x13, d5);
  __ fcvtmu(x5, s18);
  __ fcvtns(d30, d15);
  __ fcvtns(s10, s11);
  __ fcvtns(w21, d15);
  __ fcvtns(w18, s10);
  __ fcvtns(x8, d17);
  __ fcvtns(x17, s12);
  __ fcvtnu(d0, d21);
  __ fcvtnu(s6, s25);
  __ fcvtnu(w29, d11);
  __ fcvtnu(w25, s31);
  __ fcvtnu(x30, d11);
  __ fcvtnu(x27, s18);
  __ fcvtps(d11, d22);
  __ fcvtps(s29, s20);
  __ fcvtps(w15, d25);
  __ fcvtps(w16, s7);
  __ fcvtps(x13, d20);
  __ fcvtps(x3, s23);
  __ fcvtpu(d24, d1);
  __ fcvtpu(s14, s24);
  __ fcvtpu(w26, d29);
  __ fcvtpu(wzr, s26);
  __ fcvtpu(x27, d6);
  __ fcvtpu(x29, s14);
  __ fcvtxn(s12, d12);
  __ fcvtzs(d15, d0);
  __ fcvtzs(d13, d4, 42);
  __ fcvtzs(s8, s11);
  __ fcvtzs(s31, s6, 25);
  __ fcvtzs(w6, d9);
  __ fcvtzs(w25, d10, 20);
  __ fcvtzs(w9, s1);
  __ fcvtzs(w17, s29, 30);
  __ fcvtzs(x19, d2);
  __ fcvtzs(x22, d14, 1);
  __ fcvtzs(x14, s20);
  __ fcvtzs(x3, s30, 33);
  __ fcvtzu(d28, d15);
  __ fcvtzu(d0, d4, 3);
  __ fcvtzu(s2, s5);
  __ fcvtzu(s4, s0, 30);
  __ fcvtzu(w11, d4);
  __ fcvtzu(w7, d24, 32);
  __ fcvtzu(w18, s24);
  __ fcvtzu(w14, s27, 4);
  __ fcvtzu(x22, d11);
  __ fcvtzu(x8, d27, 52);
  __ fcvtzu(x7, s20);
  __ fcvtzu(x22, s7, 44);
  __ fdiv(d6, d14, d15);
  __ fdiv(s26, s5, s25);
  __ fmadd(d18, d26, d12, d30);
  __ fmadd(s13, s9, s28, s4);
  __ fmax(d12, d5, d5);
  __ fmax(s12, s28, s6);
  __ fmaxnm(d28, d4, d2);
  __ fmaxnm(s6, s10, s8);
  __ fmin(d20, d20, d18);
  __ fmin(s7, s13, s16);
  __ fminnm(d19, d14, d30);
  __ fminnm(s0, s1, s1);
  __ fmov(d13, d6);
  __ fmov(d2, x17);
  __ fmov(d8, -2.5000);
  __ fmov(s5, s3);
  __ fmov(s25, w20);
  __ fmov(s21, 2.8750f);
  __ fmov(w18, s24);
  __ fmov(x18, d2);
  __ fmsub(d20, d30, d3, d19);
  __ fmsub(s5, s19, s4, s12);
  __ fmul(d30, d27, d23);
  __ fmul(s25, s17, s15);
  __ fmulx(d4, d17, d1);
  __ fmulx(s14, s25, s4);
  __ fneg(d15, d0);
  __ fneg(s14, s15);
  __ fnmadd(d0, d16, d22, d31);
  __ fnmadd(s0, s18, s26, s18);
  __ fnmsub(d19, d12, d15, d21);
  __ fnmsub(s29, s0, s11, s26);
  __ fnmul(d31, d19, d1);
  __ fnmul(s18, s3, s17);
  __ frecpe(d7, d21);
  __ frecpe(s29, s17);
  __ frecps(d11, d26, d17);
  __ frecps(s18, s27, s1);
  __ frecpx(d15, d18);
  __ frecpx(s5, s10);
  __ frinta(d16, d30);
  __ frinta(s1, s22);
  __ frinti(d19, d29);
  __ frinti(s14, s21);
  __ frintm(d20, d30);
  __ frintm(s1, s16);
  __ frintn(d30, d1);
  __ frintn(s24, s10);
  __ frintp(d4, d20);
  __ frintp(s13, s3);
  __ frintx(d13, d20);
  __ frintx(s17, s7);
  __ frintz(d0, d8);
  __ frintz(s15, s29);
  __ frsqrte(d21, d10);
  __ frsqrte(s17, s25);
  __ frsqrts(d4, d29, d17);
  __ frsqrts(s14, s3, s24);
  __ fsqrt(d14, d17);
  __ fsqrt(s4, s14);
  __ fsub(d13, d19, d7);
  __ fsub(s3, s21, s27);
  __ scvtf(d31, d16);
  __ scvtf(d26, d31, 24);
  __ scvtf(d6, w16);
  __ scvtf(d5, w20, 6);
  __ scvtf(d16, x8);
  __ scvtf(d15, x8, 10);
  __ scvtf(s7, s4);
  __ scvtf(s8, s15, 14);
  __ scvtf(s29, w10);
  __ scvtf(s15, w21, 11);
  __ scvtf(s27, x26);
  __ scvtf(s26, x12, 38);
  __ ucvtf(d0, d9);
  __ ucvtf(d5, d22, 47);
  __ ucvtf(d30, w27);
  __ ucvtf(d3, w19, 1);
  __ ucvtf(d28, x21);
  __ ucvtf(d27, x30, 35);
  __ ucvtf(s11, s5);
  __ ucvtf(s0, s23, 14);
  __ ucvtf(s20, w19);
  __ ucvtf(s21, w22, 18);
  __ ucvtf(s6, x13);
  __ ucvtf(s7, x2, 21);
}


static void GenerateTestSequenceNEON(MacroAssembler* masm) {
  ExactAssemblyScope guard(masm,
                           masm->GetBuffer()->GetRemainingBytes(),
                           ExactAssemblyScope::kMaximumSize);

  // NEON integer instructions.
  __ abs(d19, d0);
  __ abs(v16.V16B(), v11.V16B());
  __ abs(v0.V2D(), v31.V2D());
  __ abs(v27.V2S(), v25.V2S());
  __ abs(v21.V4H(), v27.V4H());
  __ abs(v16.V4S(), v1.V4S());
  __ abs(v31.V8B(), v5.V8B());
  __ abs(v29.V8H(), v13.V8H());
  __ add(d10, d5, d17);
  __ add(v31.V16B(), v15.V16B(), v23.V16B());
  __ add(v10.V2D(), v31.V2D(), v14.V2D());
  __ add(v15.V2S(), v14.V2S(), v19.V2S());
  __ add(v27.V4H(), v23.V4H(), v17.V4H());
  __ add(v25.V4S(), v28.V4S(), v29.V4S());
  __ add(v13.V8B(), v7.V8B(), v18.V8B());
  __ add(v4.V8H(), v2.V8H(), v1.V8H());
  __ addhn(v10.V2S(), v14.V2D(), v15.V2D());
  __ addhn(v10.V4H(), v30.V4S(), v26.V4S());
  __ addhn(v31.V8B(), v12.V8H(), v22.V8H());
  __ addhn2(v16.V16B(), v21.V8H(), v20.V8H());
  __ addhn2(v0.V4S(), v2.V2D(), v17.V2D());
  __ addhn2(v31.V8H(), v7.V4S(), v17.V4S());
  __ addp(d14, v19.V2D());
  __ addp(v3.V16B(), v8.V16B(), v28.V16B());
  __ addp(v8.V2D(), v5.V2D(), v17.V2D());
  __ addp(v22.V2S(), v30.V2S(), v26.V2S());
  __ addp(v29.V4H(), v24.V4H(), v14.V4H());
  __ addp(v30.V4S(), v26.V4S(), v24.V4S());
  __ addp(v12.V8B(), v26.V8B(), v7.V8B());
  __ addp(v17.V8H(), v8.V8H(), v12.V8H());
  __ addv(b27, v23.V16B());
  __ addv(b12, v20.V8B());
  __ addv(h27, v30.V4H());
  __ addv(h19, v14.V8H());
  __ addv(s14, v27.V4S());
  __ and_(v10.V16B(), v8.V16B(), v27.V16B());
  __ and_(v5.V8B(), v1.V8B(), v16.V8B());
  __ bic(v26.V16B(), v3.V16B(), v24.V16B());
  __ bic(v7.V2S(), 0xe4, 16);
  __ bic(v28.V4H(), 0x23, 8);
  __ bic(v29.V4S(), 0xac);
  __ bic(v12.V8B(), v31.V8B(), v21.V8B());
  __ bic(v18.V8H(), 0x98);
  __ bif(v12.V16B(), v26.V16B(), v8.V16B());
  __ bif(v2.V8B(), v23.V8B(), v27.V8B());
  __ bit(v8.V16B(), v3.V16B(), v13.V16B());
  __ bit(v5.V8B(), v5.V8B(), v23.V8B());
  __ bsl(v9.V16B(), v31.V16B(), v23.V16B());
  __ bsl(v14.V8B(), v7.V8B(), v3.V8B());
  __ cls(v29.V16B(), v5.V16B());
  __ cls(v21.V2S(), v0.V2S());
  __ cls(v1.V4H(), v12.V4H());
  __ cls(v27.V4S(), v10.V4S());
  __ cls(v19.V8B(), v4.V8B());
  __ cls(v15.V8H(), v14.V8H());
  __ clz(v1.V16B(), v4.V16B());
  __ clz(v27.V2S(), v17.V2S());
  __ clz(v9.V4H(), v9.V4H());
  __ clz(v31.V4S(), v15.V4S());
  __ clz(v14.V8B(), v19.V8B());
  __ clz(v6.V8H(), v11.V8H());
  __ cmeq(d18, d5, d29);
  __ cmeq(d14, d31, 0);
  __ cmeq(v19.V16B(), v3.V16B(), v22.V16B());
  __ cmeq(v15.V16B(), v9.V16B(), 0);
  __ cmeq(v12.V2D(), v16.V2D(), v10.V2D());
  __ cmeq(v8.V2D(), v22.V2D(), 0);
  __ cmeq(v2.V2S(), v3.V2S(), v9.V2S());
  __ cmeq(v16.V2S(), v25.V2S(), 0);
  __ cmeq(v6.V4H(), v23.V4H(), v20.V4H());
  __ cmeq(v16.V4H(), v13.V4H(), 0);
  __ cmeq(v21.V4S(), v17.V4S(), v2.V4S());
  __ cmeq(v6.V4S(), v25.V4S(), 0);
  __ cmeq(v16.V8B(), v13.V8B(), v2.V8B());
  __ cmeq(v21.V8B(), v16.V8B(), 0);
  __ cmeq(v20.V8H(), v7.V8H(), v25.V8H());
  __ cmeq(v26.V8H(), v8.V8H(), 0);
  __ cmge(d16, d13, d31);
  __ cmge(d25, d24, 0);
  __ cmge(v17.V16B(), v19.V16B(), v17.V16B());
  __ cmge(v22.V16B(), v30.V16B(), 0);
  __ cmge(v28.V2D(), v20.V2D(), v26.V2D());
  __ cmge(v6.V2D(), v23.V2D(), 0);
  __ cmge(v25.V2S(), v22.V2S(), v3.V2S());
  __ cmge(v21.V2S(), v11.V2S(), 0);
  __ cmge(v16.V4H(), v3.V4H(), v12.V4H());
  __ cmge(v23.V4H(), v9.V4H(), 0);
  __ cmge(v7.V4S(), v2.V4S(), v11.V4S());
  __ cmge(v0.V4S(), v22.V4S(), 0);
  __ cmge(v10.V8B(), v30.V8B(), v9.V8B());
  __ cmge(v21.V8B(), v8.V8B(), 0);
  __ cmge(v2.V8H(), v7.V8H(), v26.V8H());
  __ cmge(v19.V8H(), v10.V8H(), 0);
  __ cmgt(d6, d13, d1);
  __ cmgt(d30, d24, 0);
  __ cmgt(v20.V16B(), v25.V16B(), v27.V16B());
  __ cmgt(v0.V16B(), v25.V16B(), 0);
  __ cmgt(v22.V2D(), v25.V2D(), v1.V2D());
  __ cmgt(v16.V2D(), v16.V2D(), 0);
  __ cmgt(v5.V2S(), v9.V2S(), v15.V2S());
  __ cmgt(v12.V2S(), v18.V2S(), 0);
  __ cmgt(v28.V4H(), v18.V4H(), v11.V4H());
  __ cmgt(v22.V4H(), v3.V4H(), 0);
  __ cmgt(v5.V4S(), v11.V4S(), v27.V4S());
  __ cmgt(v13.V4S(), v20.V4S(), 0);
  __ cmgt(v27.V8B(), v31.V8B(), v7.V8B());
  __ cmgt(v5.V8B(), v0.V8B(), 0);
  __ cmgt(v22.V8H(), v28.V8H(), v13.V8H());
  __ cmgt(v6.V8H(), v2.V8H(), 0);
  __ cmhi(d21, d8, d22);
  __ cmhi(v18.V16B(), v19.V16B(), v19.V16B());
  __ cmhi(v7.V2D(), v0.V2D(), v21.V2D());
  __ cmhi(v15.V2S(), v19.V2S(), v0.V2S());
  __ cmhi(v31.V4H(), v7.V4H(), v12.V4H());
  __ cmhi(v9.V4S(), v16.V4S(), v22.V4S());
  __ cmhi(v7.V8B(), v24.V8B(), v28.V8B());
  __ cmhi(v11.V8H(), v10.V8H(), v25.V8H());
  __ cmhs(d1, d12, d17);
  __ cmhs(v21.V16B(), v25.V16B(), v30.V16B());
  __ cmhs(v8.V2D(), v2.V2D(), v26.V2D());
  __ cmhs(v1.V2S(), v22.V2S(), v29.V2S());
  __ cmhs(v26.V4H(), v30.V4H(), v30.V4H());
  __ cmhs(v19.V4S(), v20.V4S(), v16.V4S());
  __ cmhs(v1.V8B(), v3.V8B(), v26.V8B());
  __ cmhs(v20.V8H(), v28.V8H(), v8.V8H());
  __ cmle(d30, d24, 0);
  __ cmle(v0.V16B(), v3.V16B(), 0);
  __ cmle(v2.V2D(), v30.V2D(), 0);
  __ cmle(v7.V2S(), v10.V2S(), 0);
  __ cmle(v9.V4H(), v31.V4H(), 0);
  __ cmle(v9.V4S(), v18.V4S(), 0);
  __ cmle(v21.V8B(), v31.V8B(), 0);
  __ cmle(v29.V8H(), v21.V8H(), 0);
  __ cmlt(d25, d23, 0);
  __ cmlt(v7.V16B(), v21.V16B(), 0);
  __ cmlt(v7.V2D(), v30.V2D(), 0);
  __ cmlt(v25.V2S(), v28.V2S(), 0);
  __ cmlt(v0.V4H(), v11.V4H(), 0);
  __ cmlt(v24.V4S(), v5.V4S(), 0);
  __ cmlt(v26.V8B(), v11.V8B(), 0);
  __ cmlt(v1.V8H(), v21.V8H(), 0);
  __ cmtst(d28, d23, d30);
  __ cmtst(v26.V16B(), v6.V16B(), v31.V16B());
  __ cmtst(v1.V2D(), v21.V2D(), v4.V2D());
  __ cmtst(v27.V2S(), v26.V2S(), v20.V2S());
  __ cmtst(v26.V4H(), v0.V4H(), v18.V4H());
  __ cmtst(v25.V4S(), v16.V4S(), v4.V4S());
  __ cmtst(v11.V8B(), v10.V8B(), v9.V8B());
  __ cmtst(v0.V8H(), v2.V8H(), v1.V8H());
  __ cnt(v25.V16B(), v15.V16B());
  __ cnt(v28.V8B(), v6.V8B());
  __ dup(v6.V16B(), v7.B(), 7);
  __ dup(v9.V16B(), w20);
  __ dup(v12.V2D(), v13.D(), 1);
  __ dup(v9.V2D(), xzr);
  __ dup(v4.V2S(), v26.S(), 2);
  __ dup(v3.V2S(), w12);
  __ dup(v22.V4H(), v5.H(), 7);
  __ dup(v16.V4H(), w25);
  __ dup(v20.V4S(), v10.S(), 2);
  __ dup(v10.V4S(), w7);
  __ dup(v30.V8B(), v30.B(), 2);
  __ dup(v31.V8B(), w15);
  __ dup(v28.V8H(), v17.H(), 4);
  __ dup(v2.V8H(), w3);
  __ eor(v29.V16B(), v25.V16B(), v3.V16B());
  __ eor(v3.V8B(), v16.V8B(), v28.V8B());
  __ ext(v1.V16B(), v26.V16B(), v6.V16B(), 1);
  __ ext(v2.V8B(), v30.V8B(), v1.V8B(), 1);
  __ ld1(v18.V16B(), v19.V16B(), v20.V16B(), v21.V16B(), MemOperand(x0));
  __ ld1(v23.V16B(),
         v24.V16B(),
         v25.V16B(),
         v26.V16B(),
         MemOperand(x1, x2, PostIndex));
  __ ld1(v5.V16B(),
         v6.V16B(),
         v7.V16B(),
         v8.V16B(),
         MemOperand(x1, 64, PostIndex));
  __ ld1(v18.V16B(), v19.V16B(), v20.V16B(), MemOperand(x0));
  __ ld1(v13.V16B(), v14.V16B(), v15.V16B(), MemOperand(x1, x2, PostIndex));
  __ ld1(v19.V16B(), v20.V16B(), v21.V16B(), MemOperand(x1, 48, PostIndex));
  __ ld1(v17.V16B(), v18.V16B(), MemOperand(x0));
  __ ld1(v20.V16B(), v21.V16B(), MemOperand(x1, x2, PostIndex));
  __ ld1(v28.V16B(), v29.V16B(), MemOperand(x1, 32, PostIndex));
  __ ld1(v29.V16B(), MemOperand(x0));
  __ ld1(v21.V16B(), MemOperand(x1, x2, PostIndex));
  __ ld1(v4.V16B(), MemOperand(x1, 16, PostIndex));
  __ ld1(v4.V1D(), v5.V1D(), v6.V1D(), v7.V1D(), MemOperand(x0));
  __ ld1(v17.V1D(),
         v18.V1D(),
         v19.V1D(),
         v20.V1D(),
         MemOperand(x1, x2, PostIndex));
  __ ld1(v28.V1D(),
         v29.V1D(),
         v30.V1D(),
         v31.V1D(),
         MemOperand(x1, 32, PostIndex));
  __ ld1(v20.V1D(), v21.V1D(), v22.V1D(), MemOperand(x0));
  __ ld1(v19.V1D(), v20.V1D(), v21.V1D(), MemOperand(x1, x2, PostIndex));
  __ ld1(v12.V1D(), v13.V1D(), v14.V1D(), MemOperand(x1, 24, PostIndex));
  __ ld1(v29.V1D(), v30.V1D(), MemOperand(x0));
  __ ld1(v31.V1D(), v0.V1D(), MemOperand(x1, x2, PostIndex));
  __ ld1(v3.V1D(), v4.V1D(), MemOperand(x1, 16, PostIndex));
  __ ld1(v28.V1D(), MemOperand(x0));
  __ ld1(v11.V1D(), MemOperand(x1, x2, PostIndex));
  __ ld1(v29.V1D(), MemOperand(x1, 8, PostIndex));
  __ ld1(v28.V2D(), v29.V2D(), v30.V2D(), v31.V2D(), MemOperand(x0));
  __ ld1(v8.V2D(),
         v9.V2D(),
         v10.V2D(),
         v11.V2D(),
         MemOperand(x1, x2, PostIndex));
  __ ld1(v14.V2D(),
         v15.V2D(),
         v16.V2D(),
         v17.V2D(),
         MemOperand(x1, 64, PostIndex));
  __ ld1(v26.V2D(), v27.V2D(), v28.V2D(), MemOperand(x0));
  __ ld1(v5.V2D(), v6.V2D(), v7.V2D(), MemOperand(x1, x2, PostIndex));
  __ ld1(v26.V2D(), v27.V2D(), v28.V2D(), MemOperand(x1, 48, PostIndex));
  __ ld1(v18.V2D(), v19.V2D(), MemOperand(x0));
  __ ld1(v21.V2D(), v22.V2D(), MemOperand(x1, x2, PostIndex));
  __ ld1(v17.V2D(), v18.V2D(), MemOperand(x1, 32, PostIndex));
  __ ld1(v5.V2D(), MemOperand(x0));
  __ ld1(v6.V2D(), MemOperand(x1, x2, PostIndex));
  __ ld1(v15.V2D(), MemOperand(x1, 16, PostIndex));
  __ ld1(v30.V2S(), v31.V2S(), v0.V2S(), v1.V2S(), MemOperand(x0));
  __ ld1(v24.V2S(),
         v25.V2S(),
         v26.V2S(),
         v27.V2S(),
         MemOperand(x1, x2, PostIndex));
  __ ld1(v27.V2S(),
         v28.V2S(),
         v29.V2S(),
         v30.V2S(),
         MemOperand(x1, 32, PostIndex));
  __ ld1(v11.V2S(), v12.V2S(), v13.V2S(), MemOperand(x0));
  __ ld1(v8.V2S(), v9.V2S(), v10.V2S(), MemOperand(x1, x2, PostIndex));
  __ ld1(v31.V2S(), v0.V2S(), v1.V2S(), MemOperand(x1, 24, PostIndex));
  __ ld1(v0.V2S(), v1.V2S(), MemOperand(x0));
  __ ld1(v13.V2S(), v14.V2S(), MemOperand(x1, x2, PostIndex));
  __ ld1(v3.V2S(), v4.V2S(), MemOperand(x1, 16, PostIndex));
  __ ld1(v26.V2S(), MemOperand(x0));
  __ ld1(v0.V2S(), MemOperand(x1, x2, PostIndex));
  __ ld1(v11.V2S(), MemOperand(x1, 8, PostIndex));
  __ ld1(v16.V4H(), v17.V4H(), v18.V4H(), v19.V4H(), MemOperand(x0));
  __ ld1(v24.V4H(),
         v25.V4H(),
         v26.V4H(),
         v27.V4H(),
         MemOperand(x1, x2, PostIndex));
  __ ld1(v1.V4H(), v2.V4H(), v3.V4H(), v4.V4H(), MemOperand(x1, 32, PostIndex));
  __ ld1(v30.V4H(), v31.V4H(), v0.V4H(), MemOperand(x0));
  __ ld1(v25.V4H(), v26.V4H(), v27.V4H(), MemOperand(x1, x2, PostIndex));
  __ ld1(v3.V4H(), v4.V4H(), v5.V4H(), MemOperand(x1, 24, PostIndex));
  __ ld1(v3.V4H(), v4.V4H(), MemOperand(x0));
  __ ld1(v3.V4H(), v4.V4H(), MemOperand(x1, x2, PostIndex));
  __ ld1(v23.V4H(), v24.V4H(), MemOperand(x1, 16, PostIndex));
  __ ld1(v26.V4H(), MemOperand(x0));
  __ ld1(v1.V4H(), MemOperand(x1, x2, PostIndex));
  __ ld1(v14.V4H(), MemOperand(x1, 8, PostIndex));
  __ ld1(v26.V4S(), v27.V4S(), v28.V4S(), v29.V4S(), MemOperand(x0));
  __ ld1(v28.V4S(),
         v29.V4S(),
         v30.V4S(),
         v31.V4S(),
         MemOperand(x1, x2, PostIndex));
  __ ld1(v4.V4S(), v5.V4S(), v6.V4S(), v7.V4S(), MemOperand(x1, 64, PostIndex));
  __ ld1(v2.V4S(), v3.V4S(), v4.V4S(), MemOperand(x0));
  __ ld1(v22.V4S(), v23.V4S(), v24.V4S(), MemOperand(x1, x2, PostIndex));
  __ ld1(v15.V4S(), v16.V4S(), v17.V4S(), MemOperand(x1, 48, PostIndex));
  __ ld1(v20.V4S(), v21.V4S(), MemOperand(x0));
  __ ld1(v30.V4S(), v31.V4S(), MemOperand(x1, x2, PostIndex));
  __ ld1(v11.V4S(), v12.V4S(), MemOperand(x1, 32, PostIndex));
  __ ld1(v15.V4S(), MemOperand(x0));
  __ ld1(v12.V4S(), MemOperand(x1, x2, PostIndex));
  __ ld1(v0.V4S(), MemOperand(x1, 16, PostIndex));
  __ ld1(v17.V8B(), v18.V8B(), v19.V8B(), v20.V8B(), MemOperand(x0));
  __ ld1(v5.V8B(), v6.V8B(), v7.V8B(), v8.V8B(), MemOperand(x1, x2, PostIndex));
  __ ld1(v9.V8B(),
         v10.V8B(),
         v11.V8B(),
         v12.V8B(),
         MemOperand(x1, 32, PostIndex));
  __ ld1(v4.V8B(), v5.V8B(), v6.V8B(), MemOperand(x0));
  __ ld1(v2.V8B(), v3.V8B(), v4.V8B(), MemOperand(x1, x2, PostIndex));
  __ ld1(v12.V8B(), v13.V8B(), v14.V8B(), MemOperand(x1, 24, PostIndex));
  __ ld1(v10.V8B(), v11.V8B(), MemOperand(x0));
  __ ld1(v11.V8B(), v12.V8B(), MemOperand(x1, x2, PostIndex));
  __ ld1(v27.V8B(), v28.V8B(), MemOperand(x1, 16, PostIndex));
  __ ld1(v31.V8B(), MemOperand(x0));
  __ ld1(v10.V8B(), MemOperand(x1, x2, PostIndex));
  __ ld1(v28.V8B(), MemOperand(x1, 8, PostIndex));
  __ ld1(v5.V8H(), v6.V8H(), v7.V8H(), v8.V8H(), MemOperand(x0));
  __ ld1(v2.V8H(), v3.V8H(), v4.V8H(), v5.V8H(), MemOperand(x1, x2, PostIndex));
  __ ld1(v10.V8H(),
         v11.V8H(),
         v12.V8H(),
         v13.V8H(),
         MemOperand(x1, 64, PostIndex));
  __ ld1(v26.V8H(), v27.V8H(), v28.V8H(), MemOperand(x0));
  __ ld1(v3.V8H(), v4.V8H(), v5.V8H(), MemOperand(x1, x2, PostIndex));
  __ ld1(v17.V8H(), v18.V8H(), v19.V8H(), MemOperand(x1, 48, PostIndex));
  __ ld1(v4.V8H(), v5.V8H(), MemOperand(x0));
  __ ld1(v21.V8H(), v22.V8H(), MemOperand(x1, x2, PostIndex));
  __ ld1(v4.V8H(), v5.V8H(), MemOperand(x1, 32, PostIndex));
  __ ld1(v9.V8H(), MemOperand(x0));
  __ ld1(v27.V8H(), MemOperand(x1, x2, PostIndex));
  __ ld1(v26.V8H(), MemOperand(x1, 16, PostIndex));
  __ ld1(v19.B(), 1, MemOperand(x0));
  __ ld1(v12.B(), 3, MemOperand(x1, x2, PostIndex));
  __ ld1(v27.B(), 12, MemOperand(x1, 1, PostIndex));
  __ ld1(v10.D(), 1, MemOperand(x0));
  __ ld1(v26.D(), 1, MemOperand(x1, x2, PostIndex));
  __ ld1(v7.D(), 1, MemOperand(x1, 8, PostIndex));
  __ ld1(v19.H(), 5, MemOperand(x0));
  __ ld1(v10.H(), 1, MemOperand(x1, x2, PostIndex));
  __ ld1(v5.H(), 4, MemOperand(x1, 2, PostIndex));
  __ ld1(v21.S(), 2, MemOperand(x0));
  __ ld1(v13.S(), 2, MemOperand(x1, x2, PostIndex));
  __ ld1(v1.S(), 2, MemOperand(x1, 4, PostIndex));
  __ ld1r(v2.V16B(), MemOperand(x0));
  __ ld1r(v2.V16B(), MemOperand(x1, x2, PostIndex));
  __ ld1r(v22.V16B(), MemOperand(x1, 1, PostIndex));
  __ ld1r(v25.V1D(), MemOperand(x0));
  __ ld1r(v9.V1D(), MemOperand(x1, x2, PostIndex));
  __ ld1r(v23.V1D(), MemOperand(x1, 8, PostIndex));
  __ ld1r(v19.V2D(), MemOperand(x0));
  __ ld1r(v21.V2D(), MemOperand(x1, x2, PostIndex));
  __ ld1r(v30.V2D(), MemOperand(x1, 8, PostIndex));
  __ ld1r(v24.V2S(), MemOperand(x0));
  __ ld1r(v26.V2S(), MemOperand(x1, x2, PostIndex));
  __ ld1r(v28.V2S(), MemOperand(x1, 4, PostIndex));
  __ ld1r(v19.V4H(), MemOperand(x0));
  __ ld1r(v1.V4H(), MemOperand(x1, x2, PostIndex));
  __ ld1r(v21.V4H(), MemOperand(x1, 2, PostIndex));
  __ ld1r(v15.V4S(), MemOperand(x0));
  __ ld1r(v21.V4S(), MemOperand(x1, x2, PostIndex));
  __ ld1r(v23.V4S(), MemOperand(x1, 4, PostIndex));
  __ ld1r(v26.V8B(), MemOperand(x0));
  __ ld1r(v14.V8B(), MemOperand(x1, x2, PostIndex));
  __ ld1r(v19.V8B(), MemOperand(x1, 1, PostIndex));
  __ ld1r(v13.V8H(), MemOperand(x0));
  __ ld1r(v30.V8H(), MemOperand(x1, x2, PostIndex));
  __ ld1r(v27.V8H(), MemOperand(x1, 2, PostIndex));
  __ ld2(v21.V16B(), v22.V16B(), MemOperand(x0));
  __ ld2(v21.V16B(), v22.V16B(), MemOperand(x1, x2, PostIndex));
  __ ld2(v12.V16B(), v13.V16B(), MemOperand(x1, 32, PostIndex));
  __ ld2(v14.V2D(), v15.V2D(), MemOperand(x0));
  __ ld2(v0.V2D(), v1.V2D(), MemOperand(x1, x2, PostIndex));
  __ ld2(v12.V2D(), v13.V2D(), MemOperand(x1, 32, PostIndex));
  __ ld2(v27.V2S(), v28.V2S(), MemOperand(x0));
  __ ld2(v2.V2S(), v3.V2S(), MemOperand(x1, x2, PostIndex));
  __ ld2(v12.V2S(), v13.V2S(), MemOperand(x1, 16, PostIndex));
  __ ld2(v9.V4H(), v10.V4H(), MemOperand(x0));
  __ ld2(v23.V4H(), v24.V4H(), MemOperand(x1, x2, PostIndex));
  __ ld2(v1.V4H(), v2.V4H(), MemOperand(x1, 16, PostIndex));
  __ ld2(v20.V4S(), v21.V4S(), MemOperand(x0));
  __ ld2(v10.V4S(), v11.V4S(), MemOperand(x1, x2, PostIndex));
  __ ld2(v24.V4S(), v25.V4S(), MemOperand(x1, 32, PostIndex));
  __ ld2(v17.V8B(), v18.V8B(), MemOperand(x0));
  __ ld2(v13.V8B(), v14.V8B(), MemOperand(x1, x2, PostIndex));
  __ ld2(v7.V8B(), v8.V8B(), MemOperand(x1, 16, PostIndex));
  __ ld2(v30.V8H(), v31.V8H(), MemOperand(x0));
  __ ld2(v4.V8H(), v5.V8H(), MemOperand(x1, x2, PostIndex));
  __ ld2(v13.V8H(), v14.V8H(), MemOperand(x1, 32, PostIndex));
  __ ld2(v5.B(), v6.B(), 12, MemOperand(x0));
  __ ld2(v16.B(), v17.B(), 7, MemOperand(x1, x2, PostIndex));
  __ ld2(v29.B(), v30.B(), 2, MemOperand(x1, 2, PostIndex));
  __ ld2(v11.D(), v12.D(), 1, MemOperand(x0));
  __ ld2(v26.D(), v27.D(), 0, MemOperand(x1, x2, PostIndex));
  __ ld2(v25.D(), v26.D(), 0, MemOperand(x1, 16, PostIndex));
  __ ld2(v18.H(), v19.H(), 7, MemOperand(x0));
  __ ld2(v17.H(), v18.H(), 5, MemOperand(x1, x2, PostIndex));
  __ ld2(v30.H(), v31.H(), 2, MemOperand(x1, 4, PostIndex));
  __ ld2(v29.S(), v30.S(), 3, MemOperand(x0));
  __ ld2(v28.S(), v29.S(), 0, MemOperand(x1, x2, PostIndex));
  __ ld2(v6.S(), v7.S(), 1, MemOperand(x1, 8, PostIndex));
  __ ld2r(v26.V16B(), v27.V16B(), MemOperand(x0));
  __ ld2r(v21.V16B(), v22.V16B(), MemOperand(x1, x2, PostIndex));
  __ ld2r(v5.V16B(), v6.V16B(), MemOperand(x1, 2, PostIndex));
  __ ld2r(v26.V1D(), v27.V1D(), MemOperand(x0));
  __ ld2r(v14.V1D(), v15.V1D(), MemOperand(x1, x2, PostIndex));
  __ ld2r(v23.V1D(), v24.V1D(), MemOperand(x1, 16, PostIndex));
  __ ld2r(v11.V2D(), v12.V2D(), MemOperand(x0));
  __ ld2r(v29.V2D(), v30.V2D(), MemOperand(x1, x2, PostIndex));
  __ ld2r(v15.V2D(), v16.V2D(), MemOperand(x1, 16, PostIndex));
  __ ld2r(v26.V2S(), v27.V2S(), MemOperand(x0));
  __ ld2r(v22.V2S(), v23.V2S(), MemOperand(x1, x2, PostIndex));
  __ ld2r(v2.V2S(), v3.V2S(), MemOperand(x1, 8, PostIndex));
  __ ld2r(v2.V4H(), v3.V4H(), MemOperand(x0));
  __ ld2r(v9.V4H(), v10.V4H(), MemOperand(x1, x2, PostIndex));
  __ ld2r(v6.V4H(), v7.V4H(), MemOperand(x1, 4, PostIndex));
  __ ld2r(v7.V4S(), v8.V4S(), MemOperand(x0));
  __ ld2r(v19.V4S(), v20.V4S(), MemOperand(x1, x2, PostIndex));
  __ ld2r(v21.V4S(), v22.V4S(), MemOperand(x1, 8, PostIndex));
  __ ld2r(v26.V8B(), v27.V8B(), MemOperand(x0));
  __ ld2r(v20.V8B(), v21.V8B(), MemOperand(x1, x2, PostIndex));
  __ ld2r(v11.V8B(), v12.V8B(), MemOperand(x1, 2, PostIndex));
  __ ld2r(v12.V8H(), v13.V8H(), MemOperand(x0));
  __ ld2r(v6.V8H(), v7.V8H(), MemOperand(x1, x2, PostIndex));
  __ ld2r(v25.V8H(), v26.V8H(), MemOperand(x1, 4, PostIndex));
  __ ld3(v20.V16B(), v21.V16B(), v22.V16B(), MemOperand(x0));
  __ ld3(v28.V16B(), v29.V16B(), v30.V16B(), MemOperand(x1, x2, PostIndex));
  __ ld3(v20.V16B(), v21.V16B(), v22.V16B(), MemOperand(x1, 48, PostIndex));
  __ ld3(v21.V2D(), v22.V2D(), v23.V2D(), MemOperand(x0));
  __ ld3(v18.V2D(), v19.V2D(), v20.V2D(), MemOperand(x1, x2, PostIndex));
  __ ld3(v27.V2D(), v28.V2D(), v29.V2D(), MemOperand(x1, 48, PostIndex));
  __ ld3(v7.V2S(), v8.V2S(), v9.V2S(), MemOperand(x0));
  __ ld3(v20.V2S(), v21.V2S(), v22.V2S(), MemOperand(x1, x2, PostIndex));
  __ ld3(v26.V2S(), v27.V2S(), v28.V2S(), MemOperand(x1, 24, PostIndex));
  __ ld3(v27.V4H(), v28.V4H(), v29.V4H(), MemOperand(x0));
  __ ld3(v28.V4H(), v29.V4H(), v30.V4H(), MemOperand(x1, x2, PostIndex));
  __ ld3(v7.V4H(), v8.V4H(), v9.V4H(), MemOperand(x1, 24, PostIndex));
  __ ld3(v2.V4S(), v3.V4S(), v4.V4S(), MemOperand(x0));
  __ ld3(v24.V4S(), v25.V4S(), v26.V4S(), MemOperand(x1, x2, PostIndex));
  __ ld3(v11.V4S(), v12.V4S(), v13.V4S(), MemOperand(x1, 48, PostIndex));
  __ ld3(v29.V8B(), v30.V8B(), v31.V8B(), MemOperand(x0));
  __ ld3(v1.V8B(), v2.V8B(), v3.V8B(), MemOperand(x1, x2, PostIndex));
  __ ld3(v12.V8B(), v13.V8B(), v14.V8B(), MemOperand(x1, 24, PostIndex));
  __ ld3(v22.V8H(), v23.V8H(), v24.V8H(), MemOperand(x0));
  __ ld3(v13.V8H(), v14.V8H(), v15.V8H(), MemOperand(x1, x2, PostIndex));
  __ ld3(v28.V8H(), v29.V8H(), v30.V8H(), MemOperand(x1, 48, PostIndex));
  __ ld3(v21.B(), v22.B(), v23.B(), 11, MemOperand(x0));
  __ ld3(v5.B(), v6.B(), v7.B(), 9, MemOperand(x1, x2, PostIndex));
  __ ld3(v23.B(), v24.B(), v25.B(), 0, MemOperand(x1, 3, PostIndex));
  __ ld3(v16.D(), v17.D(), v18.D(), 0, MemOperand(x0));
  __ ld3(v30.D(), v31.D(), v0.D(), 0, MemOperand(x1, x2, PostIndex));
  __ ld3(v28.D(), v29.D(), v30.D(), 1, MemOperand(x1, 24, PostIndex));
  __ ld3(v13.H(), v14.H(), v15.H(), 2, MemOperand(x0));
  __ ld3(v22.H(), v23.H(), v24.H(), 7, MemOperand(x1, x2, PostIndex));
  __ ld3(v14.H(), v15.H(), v16.H(), 3, MemOperand(x1, 6, PostIndex));
  __ ld3(v22.S(), v23.S(), v24.S(), 3, MemOperand(x0));
  __ ld3(v30.S(), v31.S(), v0.S(), 2, MemOperand(x1, x2, PostIndex));
  __ ld3(v12.S(), v13.S(), v14.S(), 1, MemOperand(x1, 12, PostIndex));
  __ ld3r(v24.V16B(), v25.V16B(), v26.V16B(), MemOperand(x0));
  __ ld3r(v24.V16B(), v25.V16B(), v26.V16B(), MemOperand(x1, x2, PostIndex));
  __ ld3r(v3.V16B(), v4.V16B(), v5.V16B(), MemOperand(x1, 3, PostIndex));
  __ ld3r(v4.V1D(), v5.V1D(), v6.V1D(), MemOperand(x0));
  __ ld3r(v7.V1D(), v8.V1D(), v9.V1D(), MemOperand(x1, x2, PostIndex));
  __ ld3r(v17.V1D(), v18.V1D(), v19.V1D(), MemOperand(x1, 24, PostIndex));
  __ ld3r(v16.V2D(), v17.V2D(), v18.V2D(), MemOperand(x0));
  __ ld3r(v20.V2D(), v21.V2D(), v22.V2D(), MemOperand(x1, x2, PostIndex));
  __ ld3r(v14.V2D(), v15.V2D(), v16.V2D(), MemOperand(x1, 24, PostIndex));
  __ ld3r(v10.V2S(), v11.V2S(), v12.V2S(), MemOperand(x0));
  __ ld3r(v0.V2S(), v1.V2S(), v2.V2S(), MemOperand(x1, x2, PostIndex));
  __ ld3r(v23.V2S(), v24.V2S(), v25.V2S(), MemOperand(x1, 12, PostIndex));
  __ ld3r(v22.V4H(), v23.V4H(), v24.V4H(), MemOperand(x0));
  __ ld3r(v6.V4H(), v7.V4H(), v8.V4H(), MemOperand(x1, x2, PostIndex));
  __ ld3r(v7.V4H(), v8.V4H(), v9.V4H(), MemOperand(x1, 6, PostIndex));
  __ ld3r(v26.V4S(), v27.V4S(), v28.V4S(), MemOperand(x0));
  __ ld3r(v0.V4S(), v1.V4S(), v2.V4S(), MemOperand(x1, x2, PostIndex));
  __ ld3r(v30.V4S(), v31.V4S(), v0.V4S(), MemOperand(x1, 12, PostIndex));
  __ ld3r(v2.V8B(), v3.V8B(), v4.V8B(), MemOperand(x0));
  __ ld3r(v10.V8B(), v11.V8B(), v12.V8B(), MemOperand(x1, x2, PostIndex));
  __ ld3r(v28.V8B(), v29.V8B(), v30.V8B(), MemOperand(x1, 3, PostIndex));
  __ ld3r(v6.V8H(), v7.V8H(), v8.V8H(), MemOperand(x0));
  __ ld3r(v29.V8H(), v30.V8H(), v31.V8H(), MemOperand(x1, x2, PostIndex));
  __ ld3r(v7.V8H(), v8.V8H(), v9.V8H(), MemOperand(x1, 6, PostIndex));
  __ ld4(v3.V16B(), v4.V16B(), v5.V16B(), v6.V16B(), MemOperand(x0));
  __ ld4(v2.V16B(),
         v3.V16B(),
         v4.V16B(),
         v5.V16B(),
         MemOperand(x1, x2, PostIndex));
  __ ld4(v5.V16B(),
         v6.V16B(),
         v7.V16B(),
         v8.V16B(),
         MemOperand(x1, 64, PostIndex));
  __ ld4(v18.V2D(), v19.V2D(), v20.V2D(), v21.V2D(), MemOperand(x0));
  __ ld4(v4.V2D(), v5.V2D(), v6.V2D(), v7.V2D(), MemOperand(x1, x2, PostIndex));
  __ ld4(v29.V2D(),
         v30.V2D(),
         v31.V2D(),
         v0.V2D(),
         MemOperand(x1, 64, PostIndex));
  __ ld4(v27.V2S(), v28.V2S(), v29.V2S(), v30.V2S(), MemOperand(x0));
  __ ld4(v24.V2S(),
         v25.V2S(),
         v26.V2S(),
         v27.V2S(),
         MemOperand(x1, x2, PostIndex));
  __ ld4(v4.V2S(), v5.V2S(), v6.V2S(), v7.V2S(), MemOperand(x1, 32, PostIndex));
  __ ld4(v16.V4H(), v17.V4H(), v18.V4H(), v19.V4H(), MemOperand(x0));
  __ ld4(v23.V4H(),
         v24.V4H(),
         v25.V4H(),
         v26.V4H(),
         MemOperand(x1, x2, PostIndex));
  __ ld4(v2.V4H(), v3.V4H(), v4.V4H(), v5.V4H(), MemOperand(x1, 32, PostIndex));
  __ ld4(v7.V4S(), v8.V4S(), v9.V4S(), v10.V4S(), MemOperand(x0));
  __ ld4(v28.V4S(),
         v29.V4S(),
         v30.V4S(),
         v31.V4S(),
         MemOperand(x1, x2, PostIndex));
  __ ld4(v29.V4S(),
         v30.V4S(),
         v31.V4S(),
         v0.V4S(),
         MemOperand(x1, 64, PostIndex));
  __ ld4(v15.V8B(), v16.V8B(), v17.V8B(), v18.V8B(), MemOperand(x0));
  __ ld4(v27.V8B(),
         v28.V8B(),
         v29.V8B(),
         v30.V8B(),
         MemOperand(x1, x2, PostIndex));
  __ ld4(v5.V8B(), v6.V8B(), v7.V8B(), v8.V8B(), MemOperand(x1, 32, PostIndex));
  __ ld4(v25.V8H(), v26.V8H(), v27.V8H(), v28.V8H(), MemOperand(x0));
  __ ld4(v2.V8H(), v3.V8H(), v4.V8H(), v5.V8H(), MemOperand(x1, x2, PostIndex));
  __ ld4(v20.V8H(),
         v21.V8H(),
         v22.V8H(),
         v23.V8H(),
         MemOperand(x1, 64, PostIndex));
  __ ld4(v20.B(), v21.B(), v22.B(), v23.B(), 3, MemOperand(x0));
  __ ld4(v12.B(), v13.B(), v14.B(), v15.B(), 3, MemOperand(x1, x2, PostIndex));
  __ ld4(v27.B(), v28.B(), v29.B(), v30.B(), 6, MemOperand(x1, 4, PostIndex));
  __ ld4(v28.D(), v29.D(), v30.D(), v31.D(), 1, MemOperand(x0));
  __ ld4(v15.D(), v16.D(), v17.D(), v18.D(), 1, MemOperand(x1, x2, PostIndex));
  __ ld4(v16.D(), v17.D(), v18.D(), v19.D(), 1, MemOperand(x1, 32, PostIndex));
  __ ld4(v2.H(), v3.H(), v4.H(), v5.H(), 6, MemOperand(x0));
  __ ld4(v5.H(), v6.H(), v7.H(), v8.H(), 3, MemOperand(x1, x2, PostIndex));
  __ ld4(v7.H(), v8.H(), v9.H(), v10.H(), 6, MemOperand(x1, 8, PostIndex));
  __ ld4(v6.S(), v7.S(), v8.S(), v9.S(), 1, MemOperand(x0));
  __ ld4(v25.S(), v26.S(), v27.S(), v28.S(), 2, MemOperand(x1, x2, PostIndex));
  __ ld4(v8.S(), v9.S(), v10.S(), v11.S(), 3, MemOperand(x1, 16, PostIndex));
  __ ld4r(v14.V16B(), v15.V16B(), v16.V16B(), v17.V16B(), MemOperand(x0));
  __ ld4r(v13.V16B(),
          v14.V16B(),
          v15.V16B(),
          v16.V16B(),
          MemOperand(x1, x2, PostIndex));
  __ ld4r(v9.V16B(),
          v10.V16B(),
          v11.V16B(),
          v12.V16B(),
          MemOperand(x1, 4, PostIndex));
  __ ld4r(v8.V1D(), v9.V1D(), v10.V1D(), v11.V1D(), MemOperand(x0));
  __ ld4r(v4.V1D(),
          v5.V1D(),
          v6.V1D(),
          v7.V1D(),
          MemOperand(x1, x2, PostIndex));
  __ ld4r(v26.V1D(),
          v27.V1D(),
          v28.V1D(),
          v29.V1D(),
          MemOperand(x1, 32, PostIndex));
  __ ld4r(v19.V2D(), v20.V2D(), v21.V2D(), v22.V2D(), MemOperand(x0));
  __ ld4r(v28.V2D(),
          v29.V2D(),
          v30.V2D(),
          v31.V2D(),
          MemOperand(x1, x2, PostIndex));
  __ ld4r(v15.V2D(),
          v16.V2D(),
          v17.V2D(),
          v18.V2D(),
          MemOperand(x1, 32, PostIndex));
  __ ld4r(v31.V2S(), v0.V2S(), v1.V2S(), v2.V2S(), MemOperand(x0));
  __ ld4r(v28.V2S(),
          v29.V2S(),
          v30.V2S(),
          v31.V2S(),
          MemOperand(x1, x2, PostIndex));
  __ ld4r(v11.V2S(),
          v12.V2S(),
          v13.V2S(),
          v14.V2S(),
          MemOperand(x1, 16, PostIndex));
  __ ld4r(v19.V4H(), v20.V4H(), v21.V4H(), v22.V4H(), MemOperand(x0));
  __ ld4r(v22.V4H(),
          v23.V4H(),
          v24.V4H(),
          v25.V4H(),
          MemOperand(x1, x2, PostIndex));
  __ ld4r(v20.V4H(),
          v21.V4H(),
          v22.V4H(),
          v23.V4H(),
          MemOperand(x1, 8, PostIndex));
  __ ld4r(v16.V4S(), v17.V4S(), v18.V4S(), v19.V4S(), MemOperand(x0));
  __ ld4r(v25.V4S(),
          v26.V4S(),
          v27.V4S(),
          v28.V4S(),
          MemOperand(x1, x2, PostIndex));
  __ ld4r(v23.V4S(),
          v24.V4S(),
          v25.V4S(),
          v26.V4S(),
          MemOperand(x1, 16, PostIndex));
  __ ld4r(v22.V8B(), v23.V8B(), v24.V8B(), v25.V8B(), MemOperand(x0));
  __ ld4r(v27.V8B(),
          v28.V8B(),
          v29.V8B(),
          v30.V8B(),
          MemOperand(x1, x2, PostIndex));
  __ ld4r(v29.V8B(),
          v30.V8B(),
          v31.V8B(),
          v0.V8B(),
          MemOperand(x1, 4, PostIndex));
  __ ld4r(v28.V8H(), v29.V8H(), v30.V8H(), v31.V8H(), MemOperand(x0));
  __ ld4r(v25.V8H(),
          v26.V8H(),
          v27.V8H(),
          v28.V8H(),
          MemOperand(x1, x2, PostIndex));
  __ ld4r(v22.V8H(),
          v23.V8H(),
          v24.V8H(),
          v25.V8H(),
          MemOperand(x1, 8, PostIndex));
  __ mla(v29.V16B(), v7.V16B(), v26.V16B());
  __ mla(v6.V2S(), v4.V2S(), v14.V2S());
  __ mla(v9.V2S(), v11.V2S(), v0.S(), 2);
  __ mla(v5.V4H(), v17.V4H(), v25.V4H());
  __ mla(v24.V4H(), v7.V4H(), v11.H(), 3);
  __ mla(v12.V4S(), v3.V4S(), v4.V4S());
  __ mla(v10.V4S(), v7.V4S(), v7.S(), 3);
  __ mla(v3.V8B(), v16.V8B(), v9.V8B());
  __ mla(v19.V8H(), v22.V8H(), v18.V8H());
  __ mla(v6.V8H(), v2.V8H(), v0.H(), 0);
  __ mls(v23.V16B(), v10.V16B(), v11.V16B());
  __ mls(v14.V2S(), v31.V2S(), v22.V2S());
  __ mls(v28.V2S(), v13.V2S(), v1.S(), 3);
  __ mls(v2.V4H(), v19.V4H(), v13.V4H());
  __ mls(v18.V4H(), v15.V4H(), v12.H(), 6);
  __ mls(v6.V4S(), v11.V4S(), v16.V4S());
  __ mls(v23.V4S(), v16.V4S(), v10.S(), 2);
  __ mls(v26.V8B(), v13.V8B(), v23.V8B());
  __ mls(v10.V8H(), v10.V8H(), v12.V8H());
  __ mls(v14.V8H(), v0.V8H(), v14.H(), 7);
  __ mov(b22, v1.B(), 3);
  __ mov(d7, v13.D(), 1);
  __ mov(h26, v21.H(), 2);
  __ mov(s26, v19.S(), 0);
  __ mov(v26.V16B(), v11.V16B());
  __ mov(v20.V8B(), v0.V8B());
  __ mov(v19.B(), 13, v6.B(), 4);
  __ mov(v4.B(), 13, w19);
  __ mov(v11.D(), 1, v8.D(), 0);
  __ mov(v3.D(), 0, x30);
  __ mov(v29.H(), 4, v11.H(), 7);
  __ mov(v2.H(), 6, w6);
  __ mov(v22.S(), 0, v5.S(), 2);
  __ mov(v24.S(), 3, w8);
  __ mov(w18, v1.S(), 3);
  __ mov(x28, v21.D(), 0);
  __ movi(d24, 0xffff0000ffffff);
  __ movi(v29.V16B(), 0x80);
  __ movi(v12.V2D(), 0xffff00ff00ffff00);
  __ movi(v12.V2S(), 0xec, LSL, 24);
  __ movi(v10.V2S(), 0x4c, MSL, 16);
  __ movi(v26.V4H(), 0xc0, LSL);
  __ movi(v24.V4S(), 0x98, LSL, 16);
  __ movi(v1.V4S(), 0xde, MSL, 16);
  __ movi(v21.V8B(), 0x4d);
  __ movi(v29.V8H(), 0x69, LSL);
  __ mul(v1.V16B(), v15.V16B(), v17.V16B());
  __ mul(v21.V2S(), v19.V2S(), v29.V2S());
  __ mul(v19.V2S(), v5.V2S(), v3.S(), 0);
  __ mul(v29.V4H(), v11.V4H(), v2.V4H());
  __ mul(v2.V4H(), v7.V4H(), v0.H(), 0);
  __ mul(v25.V4S(), v26.V4S(), v16.V4S());
  __ mul(v26.V4S(), v6.V4S(), v15.S(), 2);
  __ mul(v11.V8B(), v15.V8B(), v31.V8B());
  __ mul(v20.V8H(), v31.V8H(), v15.V8H());
  __ mul(v29.V8H(), v5.V8H(), v9.H(), 4);
  __ mvn(v13.V16B(), v21.V16B());
  __ mvn(v28.V8B(), v19.V8B());
  __ mvni(v25.V2S(), 0xb8, LSL, 8);
  __ mvni(v17.V2S(), 0x6c, MSL, 16);
  __ mvni(v29.V4H(), 0x48, LSL);
  __ mvni(v20.V4S(), 0x7a, LSL, 16);
  __ mvni(v0.V4S(), 0x1e, MSL, 8);
  __ mvni(v31.V8H(), 0x3e, LSL);
  __ neg(d25, d11);
  __ neg(v4.V16B(), v9.V16B());
  __ neg(v11.V2D(), v25.V2D());
  __ neg(v7.V2S(), v18.V2S());
  __ neg(v7.V4H(), v15.V4H());
  __ neg(v17.V4S(), v18.V4S());
  __ neg(v20.V8B(), v17.V8B());
  __ neg(v0.V8H(), v11.V8H());
  __ orn(v13.V16B(), v11.V16B(), v31.V16B());
  __ orn(v22.V8B(), v16.V8B(), v22.V8B());
  __ orr(v17.V16B(), v17.V16B(), v23.V16B());
  __ orr(v8.V2S(), 0xe3);
  __ orr(v11.V4H(), 0x97, 8);
  __ orr(v7.V4S(), 0xab);
  __ orr(v8.V8B(), v4.V8B(), v3.V8B());
  __ orr(v31.V8H(), 0xb0, 8);
  __ pmul(v11.V16B(), v18.V16B(), v23.V16B());
  __ pmul(v8.V8B(), v24.V8B(), v5.V8B());
  __ pmull(v24.V8H(), v18.V8B(), v22.V8B());
  __ pmull2(v13.V8H(), v3.V16B(), v21.V16B());
  __ raddhn(v22.V2S(), v10.V2D(), v21.V2D());
  __ raddhn(v5.V4H(), v13.V4S(), v13.V4S());
  __ raddhn(v10.V8B(), v17.V8H(), v26.V8H());
  __ raddhn2(v9.V16B(), v29.V8H(), v13.V8H());
  __ raddhn2(v27.V4S(), v23.V2D(), v26.V2D());
  __ raddhn2(v0.V8H(), v29.V4S(), v7.V4S());
  __ rbit(v22.V16B(), v15.V16B());
  __ rbit(v30.V8B(), v3.V8B());
  __ rev16(v31.V16B(), v27.V16B());
  __ rev16(v12.V8B(), v26.V8B());
  __ rev32(v5.V16B(), v4.V16B());
  __ rev32(v16.V4H(), v26.V4H());
  __ rev32(v20.V8B(), v3.V8B());
  __ rev32(v20.V8H(), v28.V8H());
  __ rev64(v9.V16B(), v19.V16B());
  __ rev64(v5.V2S(), v16.V2S());
  __ rev64(v7.V4H(), v31.V4H());
  __ rev64(v15.V4S(), v26.V4S());
  __ rev64(v25.V8B(), v9.V8B());
  __ rev64(v11.V8H(), v5.V8H());
  __ rshrn(v18.V2S(), v13.V2D(), 1);
  __ rshrn(v25.V4H(), v30.V4S(), 2);
  __ rshrn(v13.V8B(), v9.V8H(), 8);
  __ rshrn2(v3.V16B(), v6.V8H(), 8);
  __ rshrn2(v0.V4S(), v29.V2D(), 25);
  __ rshrn2(v27.V8H(), v26.V4S(), 15);
  __ rsubhn(v15.V2S(), v25.V2D(), v4.V2D());
  __ rsubhn(v23.V4H(), v9.V4S(), v3.V4S());
  __ rsubhn(v6.V8B(), v30.V8H(), v24.V8H());
  __ rsubhn2(v4.V16B(), v24.V8H(), v20.V8H());
  __ rsubhn2(v1.V4S(), v23.V2D(), v22.V2D());
  __ rsubhn2(v19.V8H(), v2.V4S(), v20.V4S());
  __ saba(v28.V16B(), v9.V16B(), v25.V16B());
  __ saba(v9.V2S(), v28.V2S(), v20.V2S());
  __ saba(v17.V4H(), v22.V4H(), v22.V4H());
  __ saba(v29.V4S(), v5.V4S(), v27.V4S());
  __ saba(v20.V8B(), v21.V8B(), v18.V8B());
  __ saba(v27.V8H(), v17.V8H(), v30.V8H());
  __ sabal(v20.V2D(), v13.V2S(), v7.V2S());
  __ sabal(v4.V4S(), v12.V4H(), v4.V4H());
  __ sabal(v23.V8H(), v24.V8B(), v20.V8B());
  __ sabal2(v26.V2D(), v21.V4S(), v18.V4S());
  __ sabal2(v27.V4S(), v28.V8H(), v8.V8H());
  __ sabal2(v12.V8H(), v16.V16B(), v21.V16B());
  __ sabd(v0.V16B(), v15.V16B(), v13.V16B());
  __ sabd(v15.V2S(), v7.V2S(), v30.V2S());
  __ sabd(v17.V4H(), v17.V4H(), v12.V4H());
  __ sabd(v7.V4S(), v4.V4S(), v22.V4S());
  __ sabd(v23.V8B(), v3.V8B(), v26.V8B());
  __ sabd(v20.V8H(), v28.V8H(), v5.V8H());
  __ sabdl(v27.V2D(), v22.V2S(), v20.V2S());
  __ sabdl(v31.V4S(), v20.V4H(), v23.V4H());
  __ sabdl(v0.V8H(), v20.V8B(), v27.V8B());
  __ sabdl2(v31.V2D(), v11.V4S(), v3.V4S());
  __ sabdl2(v26.V4S(), v11.V8H(), v27.V8H());
  __ sabdl2(v6.V8H(), v8.V16B(), v18.V16B());
  __ sadalp(v8.V1D(), v26.V2S());
  __ sadalp(v12.V2D(), v26.V4S());
  __ sadalp(v12.V2S(), v26.V4H());
  __ sadalp(v4.V4H(), v1.V8B());
  __ sadalp(v15.V4S(), v17.V8H());
  __ sadalp(v21.V8H(), v25.V16B());
  __ saddl(v5.V2D(), v10.V2S(), v14.V2S());
  __ saddl(v18.V4S(), v3.V4H(), v15.V4H());
  __ saddl(v15.V8H(), v2.V8B(), v23.V8B());
  __ saddl2(v16.V2D(), v16.V4S(), v27.V4S());
  __ saddl2(v6.V4S(), v24.V8H(), v0.V8H());
  __ saddl2(v7.V8H(), v20.V16B(), v28.V16B());
  __ saddlp(v10.V1D(), v25.V2S());
  __ saddlp(v15.V2D(), v16.V4S());
  __ saddlp(v18.V2S(), v10.V4H());
  __ saddlp(v29.V4H(), v26.V8B());
  __ saddlp(v10.V4S(), v1.V8H());
  __ saddlp(v0.V8H(), v21.V16B());
  __ saddlv(d12, v7.V4S());
  __ saddlv(h14, v28.V16B());
  __ saddlv(h30, v30.V8B());
  __ saddlv(s27, v3.V4H());
  __ saddlv(s16, v16.V8H());
  __ saddw(v24.V2D(), v11.V2D(), v18.V2S());
  __ saddw(v13.V4S(), v12.V4S(), v6.V4H());
  __ saddw(v19.V8H(), v19.V8H(), v7.V8B());
  __ saddw2(v27.V2D(), v9.V2D(), v26.V4S());
  __ saddw2(v19.V4S(), v23.V4S(), v21.V8H());
  __ saddw2(v15.V8H(), v25.V8H(), v30.V16B());
  __ shadd(v7.V16B(), v4.V16B(), v9.V16B());
  __ shadd(v29.V2S(), v25.V2S(), v24.V2S());
  __ shadd(v31.V4H(), v10.V4H(), v13.V4H());
  __ shadd(v21.V4S(), v16.V4S(), v8.V4S());
  __ shadd(v14.V8B(), v29.V8B(), v22.V8B());
  __ shadd(v19.V8H(), v24.V8H(), v20.V8H());
  __ shl(d22, d25, 23);
  __ shl(v5.V16B(), v17.V16B(), 7);
  __ shl(v2.V2D(), v4.V2D(), 21);
  __ shl(v4.V2S(), v3.V2S(), 26);
  __ shl(v3.V4H(), v28.V4H(), 8);
  __ shl(v4.V4S(), v31.V4S(), 24);
  __ shl(v18.V8B(), v16.V8B(), 2);
  __ shl(v0.V8H(), v11.V8H(), 3);
  __ shll(v5.V2D(), v24.V2S(), 32);
  __ shll(v26.V4S(), v20.V4H(), 16);
  __ shll(v5.V8H(), v9.V8B(), 8);
  __ shll2(v21.V2D(), v28.V4S(), 32);
  __ shll2(v22.V4S(), v1.V8H(), 16);
  __ shll2(v30.V8H(), v25.V16B(), 8);
  __ shrn(v5.V2S(), v1.V2D(), 28);
  __ shrn(v29.V4H(), v18.V4S(), 7);
  __ shrn(v17.V8B(), v29.V8H(), 2);
  __ shrn2(v5.V16B(), v30.V8H(), 3);
  __ shrn2(v24.V4S(), v1.V2D(), 1);
  __ shrn2(v5.V8H(), v14.V4S(), 16);
  __ shsub(v30.V16B(), v22.V16B(), v23.V16B());
  __ shsub(v22.V2S(), v27.V2S(), v25.V2S());
  __ shsub(v13.V4H(), v22.V4H(), v1.V4H());
  __ shsub(v10.V4S(), v8.V4S(), v23.V4S());
  __ shsub(v6.V8B(), v9.V8B(), v31.V8B());
  __ shsub(v8.V8H(), v31.V8H(), v8.V8H());
  __ sli(d19, d29, 20);
  __ sli(v9.V16B(), v24.V16B(), 0);
  __ sli(v22.V2D(), v9.V2D(), 10);
  __ sli(v11.V2S(), v27.V2S(), 20);
  __ sli(v16.V4H(), v15.V4H(), 5);
  __ sli(v8.V4S(), v8.V4S(), 25);
  __ sli(v10.V8B(), v30.V8B(), 0);
  __ sli(v7.V8H(), v28.V8H(), 6);
  __ smax(v18.V16B(), v8.V16B(), v1.V16B());
  __ smax(v30.V2S(), v5.V2S(), v1.V2S());
  __ smax(v17.V4H(), v25.V4H(), v19.V4H());
  __ smax(v1.V4S(), v24.V4S(), v31.V4S());
  __ smax(v17.V8B(), v24.V8B(), v24.V8B());
  __ smax(v11.V8H(), v26.V8H(), v10.V8H());
  __ smaxp(v12.V16B(), v14.V16B(), v7.V16B());
  __ smaxp(v31.V2S(), v24.V2S(), v6.V2S());
  __ smaxp(v10.V4H(), v29.V4H(), v10.V4H());
  __ smaxp(v18.V4S(), v11.V4S(), v7.V4S());
  __ smaxp(v21.V8B(), v0.V8B(), v18.V8B());
  __ smaxp(v26.V8H(), v8.V8H(), v15.V8H());
  __ smaxv(b4, v5.V16B());
  __ smaxv(b23, v0.V8B());
  __ smaxv(h6, v0.V4H());
  __ smaxv(h24, v8.V8H());
  __ smaxv(s3, v16.V4S());
  __ smin(v24.V16B(), v8.V16B(), v18.V16B());
  __ smin(v29.V2S(), v8.V2S(), v23.V2S());
  __ smin(v6.V4H(), v11.V4H(), v21.V4H());
  __ smin(v24.V4S(), v23.V4S(), v15.V4S());
  __ smin(v8.V8B(), v16.V8B(), v4.V8B());
  __ smin(v12.V8H(), v1.V8H(), v10.V8H());
  __ sminp(v13.V16B(), v18.V16B(), v28.V16B());
  __ sminp(v22.V2S(), v28.V2S(), v16.V2S());
  __ sminp(v15.V4H(), v12.V4H(), v5.V4H());
  __ sminp(v15.V4S(), v17.V4S(), v8.V4S());
  __ sminp(v21.V8B(), v2.V8B(), v6.V8B());
  __ sminp(v21.V8H(), v12.V8H(), v6.V8H());
  __ sminv(b8, v6.V16B());
  __ sminv(b6, v18.V8B());
  __ sminv(h20, v1.V4H());
  __ sminv(h7, v17.V8H());
  __ sminv(s21, v4.V4S());
  __ smlal(v24.V2D(), v14.V2S(), v21.V2S());
  __ smlal(v31.V2D(), v3.V2S(), v14.S(), 2);
  __ smlal(v7.V4S(), v20.V4H(), v21.V4H());
  __ smlal(v19.V4S(), v16.V4H(), v9.H(), 3);
  __ smlal(v29.V8H(), v14.V8B(), v1.V8B());
  __ smlal2(v30.V2D(), v26.V4S(), v16.V4S());
  __ smlal2(v31.V2D(), v30.V4S(), v1.S(), 0);
  __ smlal2(v17.V4S(), v6.V8H(), v3.V8H());
  __ smlal2(v11.V4S(), v31.V8H(), v5.H(), 7);
  __ smlal2(v30.V8H(), v16.V16B(), v29.V16B());
  __ smlsl(v1.V2D(), v20.V2S(), v17.V2S());
  __ smlsl(v29.V2D(), v12.V2S(), v5.S(), 3);
  __ smlsl(v0.V4S(), v26.V4H(), v1.V4H());
  __ smlsl(v3.V4S(), v5.V4H(), v6.H(), 5);
  __ smlsl(v4.V8H(), v0.V8B(), v26.V8B());
  __ smlsl2(v14.V2D(), v14.V4S(), v5.V4S());
  __ smlsl2(v15.V2D(), v5.V4S(), v0.S(), 1);
  __ smlsl2(v29.V4S(), v17.V8H(), v31.V8H());
  __ smlsl2(v6.V4S(), v15.V8H(), v9.H(), 6);
  __ smlsl2(v30.V8H(), v15.V16B(), v15.V16B());
  __ smov(w21, v6.B(), 3);
  __ smov(w13, v26.H(), 7);
  __ smov(x24, v16.B(), 7);
  __ smov(x7, v4.H(), 3);
  __ smov(x29, v7.S(), 1);
  __ smull(v4.V2D(), v29.V2S(), v17.V2S());
  __ smull(v30.V2D(), v21.V2S(), v6.S(), 2);
  __ smull(v23.V4S(), v5.V4H(), v23.V4H());
  __ smull(v8.V4S(), v9.V4H(), v2.H(), 1);
  __ smull(v31.V8H(), v17.V8B(), v1.V8B());
  __ smull2(v3.V2D(), v3.V4S(), v23.V4S());
  __ smull2(v15.V2D(), v29.V4S(), v6.S(), 1);
  __ smull2(v19.V4S(), v20.V8H(), v30.V8H());
  __ smull2(v6.V4S(), v10.V8H(), v7.H(), 4);
  __ smull2(v25.V8H(), v8.V16B(), v27.V16B());
  __ sqabs(b3, b15);
  __ sqabs(d14, d9);
  __ sqabs(h31, h28);
  __ sqabs(s8, s0);
  __ sqabs(v14.V16B(), v7.V16B());
  __ sqabs(v23.V2D(), v19.V2D());
  __ sqabs(v10.V2S(), v24.V2S());
  __ sqabs(v31.V4H(), v19.V4H());
  __ sqabs(v23.V4S(), v0.V4S());
  __ sqabs(v29.V8B(), v23.V8B());
  __ sqabs(v17.V8H(), v21.V8H());
  __ sqadd(b9, b23, b13);
  __ sqadd(d2, d25, d26);
  __ sqadd(h7, h29, h25);
  __ sqadd(s11, s7, s24);
  __ sqadd(v20.V16B(), v16.V16B(), v29.V16B());
  __ sqadd(v23.V2D(), v30.V2D(), v28.V2D());
  __ sqadd(v8.V2S(), v19.V2S(), v2.V2S());
  __ sqadd(v20.V4H(), v12.V4H(), v31.V4H());
  __ sqadd(v14.V4S(), v15.V4S(), v17.V4S());
  __ sqadd(v2.V8B(), v29.V8B(), v13.V8B());
  __ sqadd(v7.V8H(), v19.V8H(), v14.V8H());
  __ sqdmlal(d15, s5, s30);
  __ sqdmlal(d24, s10, v2.S(), 3);
  __ sqdmlal(s9, h19, h8);
  __ sqdmlal(s14, h1, v12.H(), 3);
  __ sqdmlal(v30.V2D(), v5.V2S(), v31.V2S());
  __ sqdmlal(v25.V2D(), v14.V2S(), v10.S(), 1);
  __ sqdmlal(v19.V4S(), v17.V4H(), v16.V4H());
  __ sqdmlal(v8.V4S(), v5.V4H(), v8.H(), 1);
  __ sqdmlal2(v1.V2D(), v23.V4S(), v3.V4S());
  __ sqdmlal2(v19.V2D(), v0.V4S(), v9.S(), 0);
  __ sqdmlal2(v26.V4S(), v22.V8H(), v11.V8H());
  __ sqdmlal2(v6.V4S(), v28.V8H(), v13.H(), 4);
  __ sqdmlsl(d10, s29, s20);
  __ sqdmlsl(d10, s9, v10.S(), 1);
  __ sqdmlsl(s30, h9, h24);
  __ sqdmlsl(s13, h24, v6.H(), 1);
  __ sqdmlsl(v27.V2D(), v10.V2S(), v20.V2S());
  __ sqdmlsl(v23.V2D(), v23.V2S(), v3.S(), 3);
  __ sqdmlsl(v7.V4S(), v17.V4H(), v29.V4H());
  __ sqdmlsl(v22.V4S(), v21.V4H(), v3.H(), 4);
  __ sqdmlsl2(v12.V2D(), v7.V4S(), v22.V4S());
  __ sqdmlsl2(v20.V2D(), v25.V4S(), v8.S(), 0);
  __ sqdmlsl2(v25.V4S(), v26.V8H(), v18.V8H());
  __ sqdmlsl2(v25.V4S(), v19.V8H(), v5.H(), 0);
  __ sqdmulh(h17, h27, h12);
  __ sqdmulh(h16, h5, v11.H(), 0);
  __ sqdmulh(s1, s19, s16);
  __ sqdmulh(s1, s16, v2.S(), 0);
  __ sqdmulh(v28.V2S(), v1.V2S(), v8.V2S());
  __ sqdmulh(v28.V2S(), v8.V2S(), v3.S(), 0);
  __ sqdmulh(v11.V4H(), v25.V4H(), v5.V4H());
  __ sqdmulh(v30.V4H(), v14.V4H(), v8.H(), 5);
  __ sqdmulh(v25.V4S(), v21.V4S(), v13.V4S());
  __ sqdmulh(v23.V4S(), v2.V4S(), v10.S(), 3);
  __ sqdmulh(v26.V8H(), v5.V8H(), v23.V8H());
  __ sqdmulh(v4.V8H(), v22.V8H(), v4.H(), 3);
  __ sqdmull(d25, s2, s26);
  __ sqdmull(d30, s14, v5.S(), 1);
  __ sqdmull(s29, h18, h11);
  __ sqdmull(s11, h13, v7.H(), 6);
  __ sqdmull(v23.V2D(), v9.V2S(), v8.V2S());
  __ sqdmull(v18.V2D(), v29.V2S(), v4.S(), 1);
  __ sqdmull(v17.V4S(), v24.V4H(), v7.V4H());
  __ sqdmull(v8.V4S(), v15.V4H(), v5.H(), 1);
  __ sqdmull2(v28.V2D(), v14.V4S(), v2.V4S());
  __ sqdmull2(v1.V2D(), v24.V4S(), v13.S(), 2);
  __ sqdmull2(v11.V4S(), v17.V8H(), v31.V8H());
  __ sqdmull2(v1.V4S(), v20.V8H(), v11.H(), 3);
  __ sqneg(b2, b0);
  __ sqneg(d24, d2);
  __ sqneg(h29, h3);
  __ sqneg(s4, s9);
  __ sqneg(v14.V16B(), v29.V16B());
  __ sqneg(v30.V2D(), v12.V2D());
  __ sqneg(v28.V2S(), v26.V2S());
  __ sqneg(v4.V4H(), v4.V4H());
  __ sqneg(v9.V4S(), v8.V4S());
  __ sqneg(v20.V8B(), v20.V8B());
  __ sqneg(v27.V8H(), v10.V8H());
  __ sqrdmulh(h7, h24, h0);
  __ sqrdmulh(h14, h3, v4.H(), 6);
  __ sqrdmulh(s27, s19, s24);
  __ sqrdmulh(s31, s21, v4.S(), 0);
  __ sqrdmulh(v18.V2S(), v25.V2S(), v1.V2S());
  __ sqrdmulh(v22.V2S(), v5.V2S(), v13.S(), 0);
  __ sqrdmulh(v22.V4H(), v24.V4H(), v9.V4H());
  __ sqrdmulh(v13.V4H(), v2.V4H(), v12.H(), 6);
  __ sqrdmulh(v9.V4S(), v27.V4S(), v2.V4S());
  __ sqrdmulh(v3.V4S(), v23.V4S(), v7.S(), 1);
  __ sqrdmulh(v2.V8H(), v0.V8H(), v7.V8H());
  __ sqrdmulh(v16.V8H(), v9.V8H(), v8.H(), 2);
  __ sqrshl(b8, b21, b13);
  __ sqrshl(d29, d7, d20);
  __ sqrshl(h28, h14, h10);
  __ sqrshl(s26, s18, s2);
  __ sqrshl(v18.V16B(), v31.V16B(), v26.V16B());
  __ sqrshl(v28.V2D(), v4.V2D(), v0.V2D());
  __ sqrshl(v3.V2S(), v6.V2S(), v0.V2S());
  __ sqrshl(v1.V4H(), v18.V4H(), v22.V4H());
  __ sqrshl(v16.V4S(), v25.V4S(), v7.V4S());
  __ sqrshl(v0.V8B(), v21.V8B(), v5.V8B());
  __ sqrshl(v30.V8H(), v19.V8H(), v8.V8H());
  __ sqrshrn(b6, h21, 4);
  __ sqrshrn(h14, s17, 11);
  __ sqrshrn(s25, d27, 10);
  __ sqrshrn(v6.V2S(), v13.V2D(), 18);
  __ sqrshrn(v5.V4H(), v9.V4S(), 15);
  __ sqrshrn(v19.V8B(), v12.V8H(), 1);
  __ sqrshrn2(v19.V16B(), v21.V8H(), 7);
  __ sqrshrn2(v29.V4S(), v24.V2D(), 13);
  __ sqrshrn2(v12.V8H(), v2.V4S(), 10);
  __ sqrshrun(b16, h9, 5);
  __ sqrshrun(h3, s24, 15);
  __ sqrshrun(s16, d18, 8);
  __ sqrshrun(v28.V2S(), v23.V2D(), 8);
  __ sqrshrun(v31.V4H(), v25.V4S(), 10);
  __ sqrshrun(v19.V8B(), v23.V8H(), 2);
  __ sqrshrun2(v24.V16B(), v0.V8H(), 8);
  __ sqrshrun2(v22.V4S(), v1.V2D(), 23);
  __ sqrshrun2(v28.V8H(), v21.V4S(), 13);
  __ sqshl(b6, b21, b8);
  __ sqshl(b11, b26, 2);
  __ sqshl(d29, d0, d4);
  __ sqshl(d21, d7, 35);
  __ sqshl(h20, h25, h17);
  __ sqshl(h20, h0, 8);
  __ sqshl(s29, s13, s4);
  __ sqshl(s10, s11, 20);
  __ sqshl(v8.V16B(), v18.V16B(), v28.V16B());
  __ sqshl(v29.V16B(), v29.V16B(), 2);
  __ sqshl(v8.V2D(), v31.V2D(), v16.V2D());
  __ sqshl(v7.V2D(), v14.V2D(), 37);
  __ sqshl(v0.V2S(), v26.V2S(), v7.V2S());
  __ sqshl(v5.V2S(), v11.V2S(), 19);
  __ sqshl(v11.V4H(), v30.V4H(), v0.V4H());
  __ sqshl(v1.V4H(), v18.V4H(), 7);
  __ sqshl(v22.V4S(), v3.V4S(), v30.V4S());
  __ sqshl(v16.V4S(), v15.V4S(), 28);
  __ sqshl(v6.V8B(), v28.V8B(), v25.V8B());
  __ sqshl(v0.V8B(), v15.V8B(), 0);
  __ sqshl(v6.V8H(), v16.V8H(), v30.V8H());
  __ sqshl(v3.V8H(), v20.V8H(), 14);
  __ sqshlu(b13, b14, 6);
  __ sqshlu(d0, d16, 44);
  __ sqshlu(h5, h29, 15);
  __ sqshlu(s29, s8, 13);
  __ sqshlu(v27.V16B(), v20.V16B(), 2);
  __ sqshlu(v24.V2D(), v12.V2D(), 11);
  __ sqshlu(v12.V2S(), v19.V2S(), 22);
  __ sqshlu(v8.V4H(), v12.V4H(), 11);
  __ sqshlu(v18.V4S(), v3.V4S(), 8);
  __ sqshlu(v3.V8B(), v10.V8B(), 1);
  __ sqshlu(v30.V8H(), v24.V8H(), 4);
  __ sqshrn(b1, h28, 1);
  __ sqshrn(h31, s7, 10);
  __ sqshrn(s4, d10, 24);
  __ sqshrn(v10.V2S(), v1.V2D(), 29);
  __ sqshrn(v3.V4H(), v13.V4S(), 14);
  __ sqshrn(v27.V8B(), v6.V8H(), 7);
  __ sqshrn2(v14.V16B(), v23.V8H(), 1);
  __ sqshrn2(v25.V4S(), v22.V2D(), 27);
  __ sqshrn2(v31.V8H(), v12.V4S(), 10);
  __ sqshrun(b9, h0, 1);
  __ sqshrun(h11, s6, 7);
  __ sqshrun(s13, d12, 13);
  __ sqshrun(v10.V2S(), v30.V2D(), 1);
  __ sqshrun(v31.V4H(), v3.V4S(), 11);
  __ sqshrun(v28.V8B(), v30.V8H(), 8);
  __ sqshrun2(v16.V16B(), v27.V8H(), 3);
  __ sqshrun2(v27.V4S(), v14.V2D(), 18);
  __ sqshrun2(v23.V8H(), v14.V4S(), 1);
  __ sqsub(b19, b29, b11);
  __ sqsub(d21, d31, d6);
  __ sqsub(h18, h10, h19);
  __ sqsub(s6, s5, s0);
  __ sqsub(v21.V16B(), v22.V16B(), v0.V16B());
  __ sqsub(v22.V2D(), v10.V2D(), v17.V2D());
  __ sqsub(v8.V2S(), v21.V2S(), v2.V2S());
  __ sqsub(v18.V4H(), v25.V4H(), v27.V4H());
  __ sqsub(v13.V4S(), v3.V4S(), v6.V4S());
  __ sqsub(v28.V8B(), v29.V8B(), v16.V8B());
  __ sqsub(v17.V8H(), v6.V8H(), v10.V8H());
  __ sqxtn(b27, h26);
  __ sqxtn(h17, s11);
  __ sqxtn(s22, d31);
  __ sqxtn(v26.V2S(), v5.V2D());
  __ sqxtn(v13.V4H(), v7.V4S());
  __ sqxtn(v19.V8B(), v19.V8H());
  __ sqxtn2(v19.V16B(), v3.V8H());
  __ sqxtn2(v23.V4S(), v1.V2D());
  __ sqxtn2(v13.V8H(), v3.V4S());
  __ sqxtun(b26, h9);
  __ sqxtun(h19, s12);
  __ sqxtun(s3, d6);
  __ sqxtun(v29.V2S(), v26.V2D());
  __ sqxtun(v26.V4H(), v10.V4S());
  __ sqxtun(v7.V8B(), v29.V8H());
  __ sqxtun2(v21.V16B(), v14.V8H());
  __ sqxtun2(v24.V4S(), v15.V2D());
  __ sqxtun2(v30.V8H(), v1.V4S());
  __ srhadd(v21.V16B(), v17.V16B(), v15.V16B());
  __ srhadd(v28.V2S(), v21.V2S(), v29.V2S());
  __ srhadd(v9.V4H(), v1.V4H(), v30.V4H());
  __ srhadd(v24.V4S(), v0.V4S(), v2.V4S());
  __ srhadd(v6.V8B(), v17.V8B(), v15.V8B());
  __ srhadd(v5.V8H(), v7.V8H(), v21.V8H());
  __ sri(d14, d14, 49);
  __ sri(v23.V16B(), v8.V16B(), 4);
  __ sri(v20.V2D(), v13.V2D(), 20);
  __ sri(v16.V2S(), v2.V2S(), 24);
  __ sri(v5.V4H(), v23.V4H(), 11);
  __ sri(v27.V4S(), v15.V4S(), 23);
  __ sri(v19.V8B(), v29.V8B(), 4);
  __ sri(v7.V8H(), v29.V8H(), 3);
  __ srshl(d2, d9, d26);
  __ srshl(v29.V16B(), v17.V16B(), v11.V16B());
  __ srshl(v8.V2D(), v15.V2D(), v4.V2D());
  __ srshl(v25.V2S(), v17.V2S(), v8.V2S());
  __ srshl(v19.V4H(), v7.V4H(), v7.V4H());
  __ srshl(v13.V4S(), v2.V4S(), v17.V4S());
  __ srshl(v22.V8B(), v6.V8B(), v21.V8B());
  __ srshl(v10.V8H(), v17.V8H(), v4.V8H());
  __ srshr(d21, d18, 45);
  __ srshr(v3.V16B(), v11.V16B(), 7);
  __ srshr(v21.V2D(), v26.V2D(), 53);
  __ srshr(v11.V2S(), v5.V2S(), 28);
  __ srshr(v7.V4H(), v18.V4H(), 12);
  __ srshr(v7.V4S(), v3.V4S(), 30);
  __ srshr(v14.V8B(), v2.V8B(), 6);
  __ srshr(v21.V8H(), v20.V8H(), 3);
  __ srsra(d21, d30, 63);
  __ srsra(v27.V16B(), v30.V16B(), 6);
  __ srsra(v20.V2D(), v12.V2D(), 27);
  __ srsra(v0.V2S(), v17.V2S(), 5);
  __ srsra(v14.V4H(), v16.V4H(), 15);
  __ srsra(v18.V4S(), v3.V4S(), 20);
  __ srsra(v21.V8B(), v1.V8B(), 1);
  __ srsra(v31.V8H(), v25.V8H(), 2);
  __ sshl(d1, d13, d9);
  __ sshl(v17.V16B(), v31.V16B(), v15.V16B());
  __ sshl(v13.V2D(), v16.V2D(), v0.V2D());
  __ sshl(v0.V2S(), v7.V2S(), v22.V2S());
  __ sshl(v23.V4H(), v19.V4H(), v4.V4H());
  __ sshl(v5.V4S(), v5.V4S(), v11.V4S());
  __ sshl(v23.V8B(), v27.V8B(), v7.V8B());
  __ sshl(v29.V8H(), v10.V8H(), v5.V8H());
  __ sshll(v0.V2D(), v2.V2S(), 23);
  __ sshll(v11.V4S(), v8.V4H(), 8);
  __ sshll(v4.V8H(), v29.V8B(), 1);
  __ sshll2(v10.V2D(), v4.V4S(), 14);
  __ sshll2(v26.V4S(), v31.V8H(), 6);
  __ sshll2(v3.V8H(), v26.V16B(), 4);
  __ sshr(d19, d21, 20);
  __ sshr(v15.V16B(), v23.V16B(), 5);
  __ sshr(v17.V2D(), v14.V2D(), 38);
  __ sshr(v3.V2S(), v29.V2S(), 23);
  __ sshr(v23.V4H(), v27.V4H(), 4);
  __ sshr(v28.V4S(), v3.V4S(), 4);
  __ sshr(v14.V8B(), v2.V8B(), 6);
  __ sshr(v3.V8H(), v8.V8H(), 6);
  __ ssra(d12, d28, 44);
  __ ssra(v29.V16B(), v31.V16B(), 4);
  __ ssra(v3.V2D(), v0.V2D(), 24);
  __ ssra(v14.V2S(), v28.V2S(), 6);
  __ ssra(v18.V4H(), v8.V4H(), 7);
  __ ssra(v31.V4S(), v14.V4S(), 24);
  __ ssra(v28.V8B(), v26.V8B(), 5);
  __ ssra(v9.V8H(), v9.V8H(), 14);
  __ ssubl(v13.V2D(), v14.V2S(), v3.V2S());
  __ ssubl(v5.V4S(), v16.V4H(), v8.V4H());
  __ ssubl(v0.V8H(), v28.V8B(), v6.V8B());
  __ ssubl2(v5.V2D(), v13.V4S(), v25.V4S());
  __ ssubl2(v3.V4S(), v15.V8H(), v17.V8H());
  __ ssubl2(v15.V8H(), v15.V16B(), v14.V16B());
  __ ssubw(v25.V2D(), v23.V2D(), v26.V2S());
  __ ssubw(v21.V4S(), v18.V4S(), v24.V4H());
  __ ssubw(v30.V8H(), v22.V8H(), v3.V8B());
  __ ssubw2(v16.V2D(), v24.V2D(), v28.V4S());
  __ ssubw2(v31.V4S(), v11.V4S(), v15.V8H());
  __ ssubw2(v4.V8H(), v8.V8H(), v16.V16B());
  __ st1(v18.V16B(), v19.V16B(), v20.V16B(), v21.V16B(), MemOperand(x0));
  __ st1(v10.V16B(),
         v11.V16B(),
         v12.V16B(),
         v13.V16B(),
         MemOperand(x1, x2, PostIndex));
  __ st1(v27.V16B(),
         v28.V16B(),
         v29.V16B(),
         v30.V16B(),
         MemOperand(x1, 64, PostIndex));
  __ st1(v16.V16B(), v17.V16B(), v18.V16B(), MemOperand(x0));
  __ st1(v21.V16B(), v22.V16B(), v23.V16B(), MemOperand(x1, x2, PostIndex));
  __ st1(v9.V16B(), v10.V16B(), v11.V16B(), MemOperand(x1, 48, PostIndex));
  __ st1(v7.V16B(), v8.V16B(), MemOperand(x0));
  __ st1(v26.V16B(), v27.V16B(), MemOperand(x1, x2, PostIndex));
  __ st1(v22.V16B(), v23.V16B(), MemOperand(x1, 32, PostIndex));
  __ st1(v23.V16B(), MemOperand(x0));
  __ st1(v28.V16B(), MemOperand(x1, x2, PostIndex));
  __ st1(v2.V16B(), MemOperand(x1, 16, PostIndex));
  __ st1(v29.V1D(), v30.V1D(), v31.V1D(), v0.V1D(), MemOperand(x0));
  __ st1(v12.V1D(),
         v13.V1D(),
         v14.V1D(),
         v15.V1D(),
         MemOperand(x1, x2, PostIndex));
  __ st1(v30.V1D(),
         v31.V1D(),
         v0.V1D(),
         v1.V1D(),
         MemOperand(x1, 32, PostIndex));
  __ st1(v16.V1D(), v17.V1D(), v18.V1D(), MemOperand(x0));
  __ st1(v3.V1D(), v4.V1D(), v5.V1D(), MemOperand(x1, x2, PostIndex));
  __ st1(v14.V1D(), v15.V1D(), v16.V1D(), MemOperand(x1, 24, PostIndex));
  __ st1(v18.V1D(), v19.V1D(), MemOperand(x0));
  __ st1(v5.V1D(), v6.V1D(), MemOperand(x1, x2, PostIndex));
  __ st1(v2.V1D(), v3.V1D(), MemOperand(x1, 16, PostIndex));
  __ st1(v4.V1D(), MemOperand(x0));
  __ st1(v27.V1D(), MemOperand(x1, x2, PostIndex));
  __ st1(v23.V1D(), MemOperand(x1, 8, PostIndex));
  __ st1(v2.V2D(), v3.V2D(), v4.V2D(), v5.V2D(), MemOperand(x0));
  __ st1(v22.V2D(),
         v23.V2D(),
         v24.V2D(),
         v25.V2D(),
         MemOperand(x1, x2, PostIndex));
  __ st1(v28.V2D(),
         v29.V2D(),
         v30.V2D(),
         v31.V2D(),
         MemOperand(x1, 64, PostIndex));
  __ st1(v17.V2D(), v18.V2D(), v19.V2D(), MemOperand(x0));
  __ st1(v16.V2D(), v17.V2D(), v18.V2D(), MemOperand(x1, x2, PostIndex));
  __ st1(v22.V2D(), v23.V2D(), v24.V2D(), MemOperand(x1, 48, PostIndex));
  __ st1(v21.V2D(), v22.V2D(), MemOperand(x0));
  __ st1(v6.V2D(), v7.V2D(), MemOperand(x1, x2, PostIndex));
  __ st1(v27.V2D(), v28.V2D(), MemOperand(x1, 32, PostIndex));
  __ st1(v21.V2D(), MemOperand(x0));
  __ st1(v29.V2D(), MemOperand(x1, x2, PostIndex));
  __ st1(v20.V2D(), MemOperand(x1, 16, PostIndex));
  __ st1(v22.V2S(), v23.V2S(), v24.V2S(), v25.V2S(), MemOperand(x0));
  __ st1(v8.V2S(),
         v9.V2S(),
         v10.V2S(),
         v11.V2S(),
         MemOperand(x1, x2, PostIndex));
  __ st1(v15.V2S(),
         v16.V2S(),
         v17.V2S(),
         v18.V2S(),
         MemOperand(x1, 32, PostIndex));
  __ st1(v2.V2S(), v3.V2S(), v4.V2S(), MemOperand(x0));
  __ st1(v23.V2S(), v24.V2S(), v25.V2S(), MemOperand(x1, x2, PostIndex));
  __ st1(v7.V2S(), v8.V2S(), v9.V2S(), MemOperand(x1, 24, PostIndex));
  __ st1(v28.V2S(), v29.V2S(), MemOperand(x0));
  __ st1(v29.V2S(), v30.V2S(), MemOperand(x1, x2, PostIndex));
  __ st1(v23.V2S(), v24.V2S(), MemOperand(x1, 16, PostIndex));
  __ st1(v6.V2S(), MemOperand(x0));
  __ st1(v11.V2S(), MemOperand(x1, x2, PostIndex));
  __ st1(v17.V2S(), MemOperand(x1, 8, PostIndex));
  __ st1(v6.V4H(), v7.V4H(), v8.V4H(), v9.V4H(), MemOperand(x0));
  __ st1(v9.V4H(),
         v10.V4H(),
         v11.V4H(),
         v12.V4H(),
         MemOperand(x1, x2, PostIndex));
  __ st1(v25.V4H(),
         v26.V4H(),
         v27.V4H(),
         v28.V4H(),
         MemOperand(x1, 32, PostIndex));
  __ st1(v11.V4H(), v12.V4H(), v13.V4H(), MemOperand(x0));
  __ st1(v10.V4H(), v11.V4H(), v12.V4H(), MemOperand(x1, x2, PostIndex));
  __ st1(v12.V4H(), v13.V4H(), v14.V4H(), MemOperand(x1, 24, PostIndex));
  __ st1(v13.V4H(), v14.V4H(), MemOperand(x0));
  __ st1(v15.V4H(), v16.V4H(), MemOperand(x1, x2, PostIndex));
  __ st1(v21.V4H(), v22.V4H(), MemOperand(x1, 16, PostIndex));
  __ st1(v16.V4H(), MemOperand(x0));
  __ st1(v8.V4H(), MemOperand(x1, x2, PostIndex));
  __ st1(v30.V4H(), MemOperand(x1, 8, PostIndex));
  __ st1(v3.V4S(), v4.V4S(), v5.V4S(), v6.V4S(), MemOperand(x0));
  __ st1(v25.V4S(),
         v26.V4S(),
         v27.V4S(),
         v28.V4S(),
         MemOperand(x1, x2, PostIndex));
  __ st1(v5.V4S(), v6.V4S(), v7.V4S(), v8.V4S(), MemOperand(x1, 64, PostIndex));
  __ st1(v31.V4S(), v0.V4S(), v1.V4S(), MemOperand(x0));
  __ st1(v30.V4S(), v31.V4S(), v0.V4S(), MemOperand(x1, x2, PostIndex));
  __ st1(v6.V4S(), v7.V4S(), v8.V4S(), MemOperand(x1, 48, PostIndex));
  __ st1(v17.V4S(), v18.V4S(), MemOperand(x0));
  __ st1(v31.V4S(), v0.V4S(), MemOperand(x1, x2, PostIndex));
  __ st1(v1.V4S(), v2.V4S(), MemOperand(x1, 32, PostIndex));
  __ st1(v26.V4S(), MemOperand(x0));
  __ st1(v15.V4S(), MemOperand(x1, x2, PostIndex));
  __ st1(v13.V4S(), MemOperand(x1, 16, PostIndex));
  __ st1(v26.V8B(), v27.V8B(), v28.V8B(), v29.V8B(), MemOperand(x0));
  __ st1(v10.V8B(),
         v11.V8B(),
         v12.V8B(),
         v13.V8B(),
         MemOperand(x1, x2, PostIndex));
  __ st1(v15.V8B(),
         v16.V8B(),
         v17.V8B(),
         v18.V8B(),
         MemOperand(x1, 32, PostIndex));
  __ st1(v19.V8B(), v20.V8B(), v21.V8B(), MemOperand(x0));
  __ st1(v31.V8B(), v0.V8B(), v1.V8B(), MemOperand(x1, x2, PostIndex));
  __ st1(v9.V8B(), v10.V8B(), v11.V8B(), MemOperand(x1, 24, PostIndex));
  __ st1(v12.V8B(), v13.V8B(), MemOperand(x0));
  __ st1(v2.V8B(), v3.V8B(), MemOperand(x1, x2, PostIndex));
  __ st1(v0.V8B(), v1.V8B(), MemOperand(x1, 16, PostIndex));
  __ st1(v16.V8B(), MemOperand(x0));
  __ st1(v25.V8B(), MemOperand(x1, x2, PostIndex));
  __ st1(v31.V8B(), MemOperand(x1, 8, PostIndex));
  __ st1(v4.V8H(), v5.V8H(), v6.V8H(), v7.V8H(), MemOperand(x0));
  __ st1(v3.V8H(), v4.V8H(), v5.V8H(), v6.V8H(), MemOperand(x1, x2, PostIndex));
  __ st1(v26.V8H(),
         v27.V8H(),
         v28.V8H(),
         v29.V8H(),
         MemOperand(x1, 64, PostIndex));
  __ st1(v10.V8H(), v11.V8H(), v12.V8H(), MemOperand(x0));
  __ st1(v21.V8H(), v22.V8H(), v23.V8H(), MemOperand(x1, x2, PostIndex));
  __ st1(v18.V8H(), v19.V8H(), v20.V8H(), MemOperand(x1, 48, PostIndex));
  __ st1(v26.V8H(), v27.V8H(), MemOperand(x0));
  __ st1(v24.V8H(), v25.V8H(), MemOperand(x1, x2, PostIndex));
  __ st1(v17.V8H(), v18.V8H(), MemOperand(x1, 32, PostIndex));
  __ st1(v29.V8H(), MemOperand(x0));
  __ st1(v19.V8H(), MemOperand(x1, x2, PostIndex));
  __ st1(v23.V8H(), MemOperand(x1, 16, PostIndex));
  __ st1(v19.B(), 15, MemOperand(x0));
  __ st1(v25.B(), 9, MemOperand(x1, x2, PostIndex));
  __ st1(v4.B(), 8, MemOperand(x1, 1, PostIndex));
  __ st1(v13.D(), 0, MemOperand(x0));
  __ st1(v30.D(), 0, MemOperand(x1, x2, PostIndex));
  __ st1(v3.D(), 0, MemOperand(x1, 8, PostIndex));
  __ st1(v22.H(), 0, MemOperand(x0));
  __ st1(v31.H(), 7, MemOperand(x1, x2, PostIndex));
  __ st1(v23.H(), 3, MemOperand(x1, 2, PostIndex));
  __ st1(v0.S(), 0, MemOperand(x0));
  __ st1(v11.S(), 3, MemOperand(x1, x2, PostIndex));
  __ st1(v24.S(), 3, MemOperand(x1, 4, PostIndex));
  __ st2(v7.V16B(), v8.V16B(), MemOperand(x0));
  __ st2(v5.V16B(), v6.V16B(), MemOperand(x1, x2, PostIndex));
  __ st2(v18.V16B(), v19.V16B(), MemOperand(x1, 32, PostIndex));
  __ st2(v14.V2D(), v15.V2D(), MemOperand(x0));
  __ st2(v7.V2D(), v8.V2D(), MemOperand(x1, x2, PostIndex));
  __ st2(v24.V2D(), v25.V2D(), MemOperand(x1, 32, PostIndex));
  __ st2(v22.V2S(), v23.V2S(), MemOperand(x0));
  __ st2(v4.V2S(), v5.V2S(), MemOperand(x1, x2, PostIndex));
  __ st2(v2.V2S(), v3.V2S(), MemOperand(x1, 16, PostIndex));
  __ st2(v23.V4H(), v24.V4H(), MemOperand(x0));
  __ st2(v8.V4H(), v9.V4H(), MemOperand(x1, x2, PostIndex));
  __ st2(v7.V4H(), v8.V4H(), MemOperand(x1, 16, PostIndex));
  __ st2(v17.V4S(), v18.V4S(), MemOperand(x0));
  __ st2(v6.V4S(), v7.V4S(), MemOperand(x1, x2, PostIndex));
  __ st2(v26.V4S(), v27.V4S(), MemOperand(x1, 32, PostIndex));
  __ st2(v31.V8B(), v0.V8B(), MemOperand(x0));
  __ st2(v0.V8B(), v1.V8B(), MemOperand(x1, x2, PostIndex));
  __ st2(v21.V8B(), v22.V8B(), MemOperand(x1, 16, PostIndex));
  __ st2(v7.V8H(), v8.V8H(), MemOperand(x0));
  __ st2(v22.V8H(), v23.V8H(), MemOperand(x1, x2, PostIndex));
  __ st2(v4.V8H(), v5.V8H(), MemOperand(x1, 32, PostIndex));
  __ st2(v8.B(), v9.B(), 15, MemOperand(x0));
  __ st2(v8.B(), v9.B(), 15, MemOperand(x1, x2, PostIndex));
  __ st2(v7.B(), v8.B(), 4, MemOperand(x1, 2, PostIndex));
  __ st2(v25.D(), v26.D(), 0, MemOperand(x0));
  __ st2(v17.D(), v18.D(), 1, MemOperand(x1, x2, PostIndex));
  __ st2(v3.D(), v4.D(), 1, MemOperand(x1, 16, PostIndex));
  __ st2(v4.H(), v5.H(), 3, MemOperand(x0));
  __ st2(v0.H(), v1.H(), 5, MemOperand(x1, x2, PostIndex));
  __ st2(v22.H(), v23.H(), 2, MemOperand(x1, 4, PostIndex));
  __ st2(v14.S(), v15.S(), 3, MemOperand(x0));
  __ st2(v23.S(), v24.S(), 3, MemOperand(x1, x2, PostIndex));
  __ st2(v0.S(), v1.S(), 2, MemOperand(x1, 8, PostIndex));
  __ st3(v26.V16B(), v27.V16B(), v28.V16B(), MemOperand(x0));
  __ st3(v21.V16B(), v22.V16B(), v23.V16B(), MemOperand(x1, x2, PostIndex));
  __ st3(v24.V16B(), v25.V16B(), v26.V16B(), MemOperand(x1, 48, PostIndex));
  __ st3(v17.V2D(), v18.V2D(), v19.V2D(), MemOperand(x0));
  __ st3(v23.V2D(), v24.V2D(), v25.V2D(), MemOperand(x1, x2, PostIndex));
  __ st3(v10.V2D(), v11.V2D(), v12.V2D(), MemOperand(x1, 48, PostIndex));
  __ st3(v9.V2S(), v10.V2S(), v11.V2S(), MemOperand(x0));
  __ st3(v13.V2S(), v14.V2S(), v15.V2S(), MemOperand(x1, x2, PostIndex));
  __ st3(v22.V2S(), v23.V2S(), v24.V2S(), MemOperand(x1, 24, PostIndex));
  __ st3(v31.V4H(), v0.V4H(), v1.V4H(), MemOperand(x0));
  __ st3(v8.V4H(), v9.V4H(), v10.V4H(), MemOperand(x1, x2, PostIndex));
  __ st3(v19.V4H(), v20.V4H(), v21.V4H(), MemOperand(x1, 24, PostIndex));
  __ st3(v18.V4S(), v19.V4S(), v20.V4S(), MemOperand(x0));
  __ st3(v25.V4S(), v26.V4S(), v27.V4S(), MemOperand(x1, x2, PostIndex));
  __ st3(v16.V4S(), v17.V4S(), v18.V4S(), MemOperand(x1, 48, PostIndex));
  __ st3(v27.V8B(), v28.V8B(), v29.V8B(), MemOperand(x0));
  __ st3(v29.V8B(), v30.V8B(), v31.V8B(), MemOperand(x1, x2, PostIndex));
  __ st3(v30.V8B(), v31.V8B(), v0.V8B(), MemOperand(x1, 24, PostIndex));
  __ st3(v8.V8H(), v9.V8H(), v10.V8H(), MemOperand(x0));
  __ st3(v18.V8H(), v19.V8H(), v20.V8H(), MemOperand(x1, x2, PostIndex));
  __ st3(v18.V8H(), v19.V8H(), v20.V8H(), MemOperand(x1, 48, PostIndex));
  __ st3(v31.B(), v0.B(), v1.B(), 10, MemOperand(x0));
  __ st3(v4.B(), v5.B(), v6.B(), 5, MemOperand(x1, x2, PostIndex));
  __ st3(v5.B(), v6.B(), v7.B(), 1, MemOperand(x1, 3, PostIndex));
  __ st3(v5.D(), v6.D(), v7.D(), 0, MemOperand(x0));
  __ st3(v6.D(), v7.D(), v8.D(), 0, MemOperand(x1, x2, PostIndex));
  __ st3(v0.D(), v1.D(), v2.D(), 0, MemOperand(x1, 24, PostIndex));
  __ st3(v31.H(), v0.H(), v1.H(), 2, MemOperand(x0));
  __ st3(v14.H(), v15.H(), v16.H(), 5, MemOperand(x1, x2, PostIndex));
  __ st3(v21.H(), v22.H(), v23.H(), 6, MemOperand(x1, 6, PostIndex));
  __ st3(v21.S(), v22.S(), v23.S(), 0, MemOperand(x0));
  __ st3(v11.S(), v12.S(), v13.S(), 1, MemOperand(x1, x2, PostIndex));
  __ st3(v15.S(), v16.S(), v17.S(), 0, MemOperand(x1, 12, PostIndex));
  __ st4(v22.V16B(), v23.V16B(), v24.V16B(), v25.V16B(), MemOperand(x0));
  __ st4(v24.V16B(),
         v25.V16B(),
         v26.V16B(),
         v27.V16B(),
         MemOperand(x1, x2, PostIndex));
  __ st4(v15.V16B(),
         v16.V16B(),
         v17.V16B(),
         v18.V16B(),
         MemOperand(x1, 64, PostIndex));
  __ st4(v16.V2D(), v17.V2D(), v18.V2D(), v19.V2D(), MemOperand(x0));
  __ st4(v17.V2D(),
         v18.V2D(),
         v19.V2D(),
         v20.V2D(),
         MemOperand(x1, x2, PostIndex));
  __ st4(v9.V2D(),
         v10.V2D(),
         v11.V2D(),
         v12.V2D(),
         MemOperand(x1, 64, PostIndex));
  __ st4(v23.V2S(), v24.V2S(), v25.V2S(), v26.V2S(), MemOperand(x0));
  __ st4(v15.V2S(),
         v16.V2S(),
         v17.V2S(),
         v18.V2S(),
         MemOperand(x1, x2, PostIndex));
  __ st4(v24.V2S(),
         v25.V2S(),
         v26.V2S(),
         v27.V2S(),
         MemOperand(x1, 32, PostIndex));
  __ st4(v14.V4H(), v15.V4H(), v16.V4H(), v17.V4H(), MemOperand(x0));
  __ st4(v18.V4H(),
         v19.V4H(),
         v20.V4H(),
         v21.V4H(),
         MemOperand(x1, x2, PostIndex));
  __ st4(v1.V4H(), v2.V4H(), v3.V4H(), v4.V4H(), MemOperand(x1, 32, PostIndex));
  __ st4(v13.V4S(), v14.V4S(), v15.V4S(), v16.V4S(), MemOperand(x0));
  __ st4(v6.V4S(), v7.V4S(), v8.V4S(), v9.V4S(), MemOperand(x1, x2, PostIndex));
  __ st4(v15.V4S(),
         v16.V4S(),
         v17.V4S(),
         v18.V4S(),
         MemOperand(x1, 64, PostIndex));
  __ st4(v26.V8B(), v27.V8B(), v28.V8B(), v29.V8B(), MemOperand(x0));
  __ st4(v25.V8B(),
         v26.V8B(),
         v27.V8B(),
         v28.V8B(),
         MemOperand(x1, x2, PostIndex));
  __ st4(v19.V8B(),
         v20.V8B(),
         v21.V8B(),
         v22.V8B(),
         MemOperand(x1, 32, PostIndex));
  __ st4(v19.V8H(), v20.V8H(), v21.V8H(), v22.V8H(), MemOperand(x0));
  __ st4(v15.V8H(),
         v16.V8H(),
         v17.V8H(),
         v18.V8H(),
         MemOperand(x1, x2, PostIndex));
  __ st4(v31.V8H(),
         v0.V8H(),
         v1.V8H(),
         v2.V8H(),
         MemOperand(x1, 64, PostIndex));
  __ st4(v0.B(), v1.B(), v2.B(), v3.B(), 13, MemOperand(x0));
  __ st4(v4.B(), v5.B(), v6.B(), v7.B(), 10, MemOperand(x1, x2, PostIndex));
  __ st4(v9.B(), v10.B(), v11.B(), v12.B(), 9, MemOperand(x1, 4, PostIndex));
  __ st4(v2.D(), v3.D(), v4.D(), v5.D(), 1, MemOperand(x0));
  __ st4(v7.D(), v8.D(), v9.D(), v10.D(), 0, MemOperand(x1, x2, PostIndex));
  __ st4(v31.D(), v0.D(), v1.D(), v2.D(), 1, MemOperand(x1, 32, PostIndex));
  __ st4(v2.H(), v3.H(), v4.H(), v5.H(), 1, MemOperand(x0));
  __ st4(v27.H(), v28.H(), v29.H(), v30.H(), 3, MemOperand(x1, x2, PostIndex));
  __ st4(v24.H(), v25.H(), v26.H(), v27.H(), 4, MemOperand(x1, 8, PostIndex));
  __ st4(v18.S(), v19.S(), v20.S(), v21.S(), 2, MemOperand(x0));
  __ st4(v6.S(), v7.S(), v8.S(), v9.S(), 2, MemOperand(x1, x2, PostIndex));
  __ st4(v25.S(), v26.S(), v27.S(), v28.S(), 1, MemOperand(x1, 16, PostIndex));
  __ sub(d12, d17, d2);
  __ sub(v20.V16B(), v24.V16B(), v8.V16B());
  __ sub(v8.V2D(), v29.V2D(), v5.V2D());
  __ sub(v2.V2S(), v28.V2S(), v24.V2S());
  __ sub(v24.V4H(), v10.V4H(), v4.V4H());
  __ sub(v28.V4S(), v4.V4S(), v17.V4S());
  __ sub(v16.V8B(), v27.V8B(), v2.V8B());
  __ sub(v20.V8H(), v10.V8H(), v13.V8H());
  __ subhn(v5.V2S(), v14.V2D(), v13.V2D());
  __ subhn(v10.V4H(), v5.V4S(), v8.V4S());
  __ subhn(v6.V8B(), v10.V8H(), v22.V8H());
  __ subhn2(v11.V16B(), v6.V8H(), v9.V8H());
  __ subhn2(v25.V4S(), v18.V2D(), v24.V2D());
  __ subhn2(v20.V8H(), v21.V4S(), v1.V4S());
  __ suqadd(b25, b11);
  __ suqadd(d13, d1);
  __ suqadd(h0, h9);
  __ suqadd(s22, s8);
  __ suqadd(v24.V16B(), v27.V16B());
  __ suqadd(v26.V2D(), v14.V2D());
  __ suqadd(v7.V2S(), v10.V2S());
  __ suqadd(v25.V4H(), v12.V4H());
  __ suqadd(v4.V4S(), v3.V4S());
  __ suqadd(v14.V8B(), v18.V8B());
  __ suqadd(v31.V8H(), v8.V8H());
  __ sxtl(v16.V2D(), v20.V2S());
  __ sxtl(v27.V4S(), v28.V4H());
  __ sxtl(v0.V8H(), v22.V8B());
  __ sxtl2(v6.V2D(), v7.V4S());
  __ sxtl2(v9.V4S(), v27.V8H());
  __ sxtl2(v16.V8H(), v16.V16B());
  __ tbl(v25.V16B(),
         v17.V16B(),
         v18.V16B(),
         v19.V16B(),
         v20.V16B(),
         v22.V16B());
  __ tbl(v28.V16B(), v13.V16B(), v14.V16B(), v15.V16B(), v4.V16B());
  __ tbl(v3.V16B(), v0.V16B(), v1.V16B(), v2.V16B());
  __ tbl(v20.V16B(), v15.V16B(), v4.V16B());
  __ tbl(v7.V8B(), v23.V16B(), v24.V16B(), v25.V16B(), v26.V16B(), v20.V8B());
  __ tbl(v8.V8B(), v1.V16B(), v2.V16B(), v3.V16B(), v31.V8B());
  __ tbl(v8.V8B(), v25.V16B(), v26.V16B(), v16.V8B());
  __ tbl(v11.V8B(), v19.V16B(), v30.V8B());
  __ tbx(v25.V16B(), v25.V16B(), v26.V16B(), v27.V16B(), v28.V16B(), v5.V16B());
  __ tbx(v21.V16B(), v29.V16B(), v30.V16B(), v31.V16B(), v24.V16B());
  __ tbx(v6.V16B(), v16.V16B(), v17.V16B(), v1.V16B());
  __ tbx(v13.V16B(), v3.V16B(), v20.V16B());
  __ tbx(v24.V8B(), v29.V16B(), v30.V16B(), v31.V16B(), v0.V16B(), v9.V8B());
  __ tbx(v17.V8B(), v9.V16B(), v10.V16B(), v11.V16B(), v26.V8B());
  __ tbx(v5.V8B(), v3.V16B(), v4.V16B(), v21.V8B());
  __ tbx(v16.V8B(), v11.V16B(), v29.V8B());
  __ trn1(v19.V16B(), v24.V16B(), v12.V16B());
  __ trn1(v2.V2D(), v7.V2D(), v10.V2D());
  __ trn1(v22.V2S(), v0.V2S(), v21.V2S());
  __ trn1(v12.V4H(), v15.V4H(), v20.V4H());
  __ trn1(v30.V4S(), v17.V4S(), v9.V4S());
  __ trn1(v12.V8B(), v19.V8B(), v29.V8B());
  __ trn1(v23.V8H(), v8.V8H(), v9.V8H());
  __ trn2(v28.V16B(), v30.V16B(), v25.V16B());
  __ trn2(v7.V2D(), v27.V2D(), v7.V2D());
  __ trn2(v30.V2S(), v16.V2S(), v19.V2S());
  __ trn2(v24.V4H(), v6.V4H(), v25.V4H());
  __ trn2(v2.V4S(), v19.V4S(), v11.V4S());
  __ trn2(v25.V8B(), v27.V8B(), v18.V8B());
  __ trn2(v12.V8H(), v4.V8H(), v15.V8H());
  __ uaba(v31.V16B(), v12.V16B(), v28.V16B());
  __ uaba(v18.V2S(), v5.V2S(), v14.V2S());
  __ uaba(v9.V4H(), v20.V4H(), v21.V4H());
  __ uaba(v6.V4S(), v20.V4S(), v2.V4S());
  __ uaba(v16.V8B(), v12.V8B(), v5.V8B());
  __ uaba(v15.V8H(), v26.V8H(), v30.V8H());
  __ uabal(v10.V2D(), v18.V2S(), v15.V2S());
  __ uabal(v30.V4S(), v19.V4H(), v7.V4H());
  __ uabal(v4.V8H(), v27.V8B(), v0.V8B());
  __ uabal2(v19.V2D(), v12.V4S(), v2.V4S());
  __ uabal2(v26.V4S(), v5.V8H(), v12.V8H());
  __ uabal2(v19.V8H(), v20.V16B(), v28.V16B());
  __ uabd(v18.V16B(), v4.V16B(), v21.V16B());
  __ uabd(v30.V2S(), v21.V2S(), v16.V2S());
  __ uabd(v8.V4H(), v28.V4H(), v25.V4H());
  __ uabd(v28.V4S(), v12.V4S(), v21.V4S());
  __ uabd(v19.V8B(), v16.V8B(), v28.V8B());
  __ uabd(v9.V8H(), v12.V8H(), v29.V8H());
  __ uabdl(v26.V2D(), v0.V2S(), v8.V2S());
  __ uabdl(v29.V4S(), v31.V4H(), v25.V4H());
  __ uabdl(v27.V8H(), v29.V8B(), v14.V8B());
  __ uabdl2(v20.V2D(), v20.V4S(), v8.V4S());
  __ uabdl2(v22.V4S(), v15.V8H(), v18.V8H());
  __ uabdl2(v9.V8H(), v18.V16B(), v23.V16B());
  __ uadalp(v9.V1D(), v15.V2S());
  __ uadalp(v14.V2D(), v12.V4S());
  __ uadalp(v28.V2S(), v12.V4H());
  __ uadalp(v0.V4H(), v17.V8B());
  __ uadalp(v1.V4S(), v29.V8H());
  __ uadalp(v15.V8H(), v22.V16B());
  __ uaddl(v1.V2D(), v20.V2S(), v27.V2S());
  __ uaddl(v31.V4S(), v25.V4H(), v5.V4H());
  __ uaddl(v12.V8H(), v3.V8B(), v3.V8B());
  __ uaddl2(v5.V2D(), v23.V4S(), v6.V4S());
  __ uaddl2(v1.V4S(), v5.V8H(), v25.V8H());
  __ uaddl2(v22.V8H(), v30.V16B(), v28.V16B());
  __ uaddlp(v7.V1D(), v9.V2S());
  __ uaddlp(v26.V2D(), v4.V4S());
  __ uaddlp(v28.V2S(), v1.V4H());
  __ uaddlp(v20.V4H(), v31.V8B());
  __ uaddlp(v16.V4S(), v17.V8H());
  __ uaddlp(v6.V8H(), v2.V16B());
  __ uaddlv(d28, v22.V4S());
  __ uaddlv(h0, v19.V16B());
  __ uaddlv(h30, v30.V8B());
  __ uaddlv(s24, v18.V4H());
  __ uaddlv(s10, v0.V8H());
  __ uaddw(v9.V2D(), v17.V2D(), v14.V2S());
  __ uaddw(v9.V4S(), v25.V4S(), v3.V4H());
  __ uaddw(v18.V8H(), v1.V8H(), v0.V8B());
  __ uaddw2(v18.V2D(), v5.V2D(), v6.V4S());
  __ uaddw2(v17.V4S(), v15.V4S(), v11.V8H());
  __ uaddw2(v29.V8H(), v11.V8H(), v7.V16B());
  __ uhadd(v13.V16B(), v9.V16B(), v3.V16B());
  __ uhadd(v17.V2S(), v25.V2S(), v24.V2S());
  __ uhadd(v25.V4H(), v23.V4H(), v13.V4H());
  __ uhadd(v0.V4S(), v20.V4S(), v16.V4S());
  __ uhadd(v5.V8B(), v5.V8B(), v25.V8B());
  __ uhadd(v3.V8H(), v29.V8H(), v18.V8H());
  __ uhsub(v1.V16B(), v22.V16B(), v13.V16B());
  __ uhsub(v14.V2S(), v30.V2S(), v30.V2S());
  __ uhsub(v29.V4H(), v14.V4H(), v17.V4H());
  __ uhsub(v26.V4S(), v5.V4S(), v18.V4S());
  __ uhsub(v3.V8B(), v7.V8B(), v12.V8B());
  __ uhsub(v25.V8H(), v21.V8H(), v5.V8H());
  __ umax(v28.V16B(), v12.V16B(), v6.V16B());
  __ umax(v20.V2S(), v19.V2S(), v26.V2S());
  __ umax(v0.V4H(), v31.V4H(), v18.V4H());
  __ umax(v6.V4S(), v21.V4S(), v28.V4S());
  __ umax(v0.V8B(), v2.V8B(), v20.V8B());
  __ umax(v4.V8H(), v11.V8H(), v22.V8H());
  __ umaxp(v1.V16B(), v6.V16B(), v29.V16B());
  __ umaxp(v19.V2S(), v17.V2S(), v27.V2S());
  __ umaxp(v21.V4H(), v16.V4H(), v7.V4H());
  __ umaxp(v9.V4S(), v20.V4S(), v29.V4S());
  __ umaxp(v13.V8B(), v1.V8B(), v16.V8B());
  __ umaxp(v19.V8H(), v23.V8H(), v26.V8H());
  __ umaxv(b17, v30.V16B());
  __ umaxv(b23, v12.V8B());
  __ umaxv(h31, v15.V4H());
  __ umaxv(h15, v25.V8H());
  __ umaxv(s18, v21.V4S());
  __ umin(v22.V16B(), v0.V16B(), v18.V16B());
  __ umin(v1.V2S(), v21.V2S(), v16.V2S());
  __ umin(v17.V4H(), v4.V4H(), v25.V4H());
  __ umin(v24.V4S(), v26.V4S(), v13.V4S());
  __ umin(v20.V8B(), v1.V8B(), v5.V8B());
  __ umin(v26.V8H(), v25.V8H(), v23.V8H());
  __ uminp(v5.V16B(), v1.V16B(), v23.V16B());
  __ uminp(v7.V2S(), v26.V2S(), v30.V2S());
  __ uminp(v9.V4H(), v5.V4H(), v25.V4H());
  __ uminp(v23.V4S(), v10.V4S(), v1.V4S());
  __ uminp(v4.V8B(), v29.V8B(), v14.V8B());
  __ uminp(v21.V8H(), v0.V8H(), v14.V8H());
  __ uminv(b0, v17.V16B());
  __ uminv(b0, v31.V8B());
  __ uminv(h24, v0.V4H());
  __ uminv(h29, v14.V8H());
  __ uminv(s30, v3.V4S());
  __ umlal(v11.V2D(), v11.V2S(), v24.V2S());
  __ umlal(v30.V2D(), v16.V2S(), v11.S(), 3);
  __ umlal(v0.V4S(), v9.V4H(), v26.V4H());
  __ umlal(v20.V4S(), v24.V4H(), v12.H(), 4);
  __ umlal(v16.V8H(), v21.V8B(), v6.V8B());
  __ umlal2(v17.V2D(), v19.V4S(), v23.V4S());
  __ umlal2(v5.V2D(), v30.V4S(), v8.S(), 0);
  __ umlal2(v16.V4S(), v8.V8H(), v15.V8H());
  __ umlal2(v15.V4S(), v26.V8H(), v1.H(), 5);
  __ umlal2(v30.V8H(), v1.V16B(), v17.V16B());
  __ umlsl(v18.V2D(), v19.V2S(), v28.V2S());
  __ umlsl(v7.V2D(), v7.V2S(), v8.S(), 0);
  __ umlsl(v24.V4S(), v8.V4H(), v4.V4H());
  __ umlsl(v18.V4S(), v22.V4H(), v12.H(), 4);
  __ umlsl(v28.V8H(), v14.V8B(), v20.V8B());
  __ umlsl2(v11.V2D(), v0.V4S(), v9.V4S());
  __ umlsl2(v26.V2D(), v16.V4S(), v9.S(), 2);
  __ umlsl2(v3.V4S(), v11.V8H(), v9.V8H());
  __ umlsl2(v10.V4S(), v25.V8H(), v9.H(), 4);
  __ umlsl2(v24.V8H(), v16.V16B(), v28.V16B());
  __ umov(x30, v25.D(), 1);
  __ umull(v12.V2D(), v10.V2S(), v29.V2S());
  __ umull(v22.V2D(), v30.V2S(), v5.S(), 3);
  __ umull(v7.V4S(), v0.V4H(), v25.V4H());
  __ umull(v11.V4S(), v13.V4H(), v3.H(), 2);
  __ umull(v25.V8H(), v16.V8B(), v10.V8B());
  __ umull2(v17.V2D(), v3.V4S(), v26.V4S());
  __ umull2(v26.V2D(), v11.V4S(), v2.S(), 3);
  __ umull2(v12.V4S(), v17.V8H(), v23.V8H());
  __ umull2(v4.V4S(), v31.V8H(), v1.H(), 2);
  __ umull2(v5.V8H(), v12.V16B(), v17.V16B());
  __ uqadd(b30, b4, b28);
  __ uqadd(d27, d20, d16);
  __ uqadd(h7, h14, h28);
  __ uqadd(s28, s17, s4);
  __ uqadd(v19.V16B(), v22.V16B(), v21.V16B());
  __ uqadd(v16.V2D(), v4.V2D(), v11.V2D());
  __ uqadd(v20.V2S(), v14.V2S(), v4.V2S());
  __ uqadd(v5.V4H(), v0.V4H(), v16.V4H());
  __ uqadd(v21.V4S(), v31.V4S(), v9.V4S());
  __ uqadd(v23.V8B(), v24.V8B(), v3.V8B());
  __ uqadd(v17.V8H(), v27.V8H(), v11.V8H());
  __ uqrshl(b10, b22, b10);
  __ uqrshl(d29, d5, d11);
  __ uqrshl(h27, h24, h30);
  __ uqrshl(s10, s13, s8);
  __ uqrshl(v9.V16B(), v18.V16B(), v14.V16B());
  __ uqrshl(v24.V2D(), v15.V2D(), v17.V2D());
  __ uqrshl(v4.V2S(), v14.V2S(), v27.V2S());
  __ uqrshl(v15.V4H(), v5.V4H(), v8.V4H());
  __ uqrshl(v21.V4S(), v29.V4S(), v0.V4S());
  __ uqrshl(v16.V8B(), v24.V8B(), v9.V8B());
  __ uqrshl(v2.V8H(), v0.V8H(), v15.V8H());
  __ uqrshrn(b11, h26, 4);
  __ uqrshrn(h7, s30, 5);
  __ uqrshrn(s10, d8, 21);
  __ uqrshrn(v15.V2S(), v6.V2D(), 11);
  __ uqrshrn(v5.V4H(), v26.V4S(), 12);
  __ uqrshrn(v28.V8B(), v25.V8H(), 5);
  __ uqrshrn2(v25.V16B(), v30.V8H(), 2);
  __ uqrshrn2(v21.V4S(), v14.V2D(), 32);
  __ uqrshrn2(v13.V8H(), v7.V4S(), 2);
  __ uqshl(b13, b0, b23);
  __ uqshl(b9, b17, 4);
  __ uqshl(d23, d6, d4);
  __ uqshl(d8, d11, 44);
  __ uqshl(h19, h13, h15);
  __ uqshl(h25, h26, 6);
  __ uqshl(s4, s24, s10);
  __ uqshl(s19, s14, 1);
  __ uqshl(v14.V16B(), v30.V16B(), v25.V16B());
  __ uqshl(v6.V16B(), v10.V16B(), 5);
  __ uqshl(v18.V2D(), v8.V2D(), v7.V2D());
  __ uqshl(v25.V2D(), v14.V2D(), 18);
  __ uqshl(v25.V2S(), v16.V2S(), v23.V2S());
  __ uqshl(v13.V2S(), v15.V2S(), 31);
  __ uqshl(v28.V4H(), v24.V4H(), v15.V4H());
  __ uqshl(v4.V4H(), v17.V4H(), 1);
  __ uqshl(v9.V4S(), v31.V4S(), v23.V4S());
  __ uqshl(v18.V4S(), v28.V4S(), 31);
  __ uqshl(v31.V8B(), v21.V8B(), v15.V8B());
  __ uqshl(v6.V8B(), v21.V8B(), 1);
  __ uqshl(v28.V8H(), v2.V8H(), v17.V8H());
  __ uqshl(v24.V8H(), v8.V8H(), 14);
  __ uqshrn(b21, h27, 7);
  __ uqshrn(h28, s26, 11);
  __ uqshrn(s13, d31, 17);
  __ uqshrn(v21.V2S(), v16.V2D(), 8);
  __ uqshrn(v24.V4H(), v24.V4S(), 2);
  __ uqshrn(v5.V8B(), v1.V8H(), 8);
  __ uqshrn2(v16.V16B(), v29.V8H(), 6);
  __ uqshrn2(v2.V4S(), v6.V2D(), 1);
  __ uqshrn2(v16.V8H(), v10.V4S(), 14);
  __ uqsub(b28, b20, b26);
  __ uqsub(d0, d7, d10);
  __ uqsub(h26, h24, h7);
  __ uqsub(s23, s23, s16);
  __ uqsub(v14.V16B(), v16.V16B(), v24.V16B());
  __ uqsub(v11.V2D(), v17.V2D(), v6.V2D());
  __ uqsub(v10.V2S(), v10.V2S(), v8.V2S());
  __ uqsub(v9.V4H(), v15.V4H(), v12.V4H());
  __ uqsub(v23.V4S(), v18.V4S(), v7.V4S());
  __ uqsub(v9.V8B(), v19.V8B(), v17.V8B());
  __ uqsub(v20.V8H(), v2.V8H(), v6.V8H());
  __ uqxtn(b29, h19);
  __ uqxtn(h0, s13);
  __ uqxtn(s26, d22);
  __ uqxtn(v5.V2S(), v31.V2D());
  __ uqxtn(v30.V4H(), v19.V4S());
  __ uqxtn(v15.V8B(), v2.V8H());
  __ uqxtn2(v29.V16B(), v3.V8H());
  __ uqxtn2(v13.V4S(), v17.V2D());
  __ uqxtn2(v28.V8H(), v11.V4S());
  __ urecpe(v23.V2S(), v15.V2S());
  __ urecpe(v27.V4S(), v7.V4S());
  __ urhadd(v2.V16B(), v15.V16B(), v27.V16B());
  __ urhadd(v15.V2S(), v1.V2S(), v18.V2S());
  __ urhadd(v17.V4H(), v4.V4H(), v26.V4H());
  __ urhadd(v2.V4S(), v27.V4S(), v14.V4S());
  __ urhadd(v5.V8B(), v17.V8B(), v14.V8B());
  __ urhadd(v30.V8H(), v2.V8H(), v25.V8H());
  __ urshl(d4, d28, d30);
  __ urshl(v13.V16B(), v31.V16B(), v19.V16B());
  __ urshl(v14.V2D(), v23.V2D(), v21.V2D());
  __ urshl(v10.V2S(), v7.V2S(), v8.V2S());
  __ urshl(v15.V4H(), v21.V4H(), v28.V4H());
  __ urshl(v30.V4S(), v8.V4S(), v23.V4S());
  __ urshl(v31.V8B(), v20.V8B(), v5.V8B());
  __ urshl(v30.V8H(), v27.V8H(), v30.V8H());
  __ urshr(d4, d13, 49);
  __ urshr(v2.V16B(), v20.V16B(), 1);
  __ urshr(v13.V2D(), v11.V2D(), 51);
  __ urshr(v21.V2S(), v31.V2S(), 10);
  __ urshr(v21.V4H(), v17.V4H(), 11);
  __ urshr(v4.V4S(), v22.V4S(), 1);
  __ urshr(v0.V8B(), v1.V8B(), 7);
  __ urshr(v13.V8H(), v20.V8H(), 1);
  __ ursqrte(v20.V2S(), v16.V2S());
  __ ursqrte(v28.V4S(), v8.V4S());
  __ ursra(d27, d16, 45);
  __ ursra(v18.V16B(), v17.V16B(), 3);
  __ ursra(v26.V2D(), v28.V2D(), 58);
  __ ursra(v8.V2S(), v22.V2S(), 31);
  __ ursra(v31.V4H(), v4.V4H(), 7);
  __ ursra(v31.V4S(), v15.V4S(), 2);
  __ ursra(v3.V8B(), v1.V8B(), 5);
  __ ursra(v18.V8H(), v14.V8H(), 13);
  __ ushl(d31, d0, d16);
  __ ushl(v0.V16B(), v6.V16B(), v2.V16B());
  __ ushl(v18.V2D(), v1.V2D(), v18.V2D());
  __ ushl(v27.V2S(), v7.V2S(), v29.V2S());
  __ ushl(v14.V4H(), v14.V4H(), v13.V4H());
  __ ushl(v22.V4S(), v4.V4S(), v9.V4S());
  __ ushl(v23.V8B(), v22.V8B(), v27.V8B());
  __ ushl(v21.V8H(), v25.V8H(), v8.V8H());
  __ ushll(v11.V2D(), v0.V2S(), 21);
  __ ushll(v2.V4S(), v17.V4H(), 8);
  __ ushll(v11.V8H(), v14.V8B(), 1);
  __ ushll2(v8.V2D(), v29.V4S(), 7);
  __ ushll2(v29.V4S(), v9.V8H(), 2);
  __ ushll2(v5.V8H(), v24.V16B(), 6);
  __ ushr(d28, d27, 53);
  __ ushr(v1.V16B(), v9.V16B(), 7);
  __ ushr(v2.V2D(), v24.V2D(), 43);
  __ ushr(v30.V2S(), v25.V2S(), 11);
  __ ushr(v10.V4H(), v26.V4H(), 12);
  __ ushr(v4.V4S(), v5.V4S(), 30);
  __ ushr(v30.V8B(), v2.V8B(), 1);
  __ ushr(v6.V8H(), v12.V8H(), 2);
  __ usqadd(b19, b5);
  __ usqadd(d9, d2);
  __ usqadd(h2, h16);
  __ usqadd(s16, s3);
  __ usqadd(v31.V16B(), v29.V16B());
  __ usqadd(v8.V2D(), v10.V2D());
  __ usqadd(v18.V2S(), v9.V2S());
  __ usqadd(v24.V4H(), v14.V4H());
  __ usqadd(v10.V4S(), v30.V4S());
  __ usqadd(v16.V8B(), v20.V8B());
  __ usqadd(v12.V8H(), v16.V8H());
  __ usra(d28, d27, 37);
  __ usra(v5.V16B(), v22.V16B(), 5);
  __ usra(v2.V2D(), v19.V2D(), 33);
  __ usra(v0.V2S(), v0.V2S(), 21);
  __ usra(v7.V4H(), v6.V4H(), 12);
  __ usra(v4.V4S(), v17.V4S(), 9);
  __ usra(v9.V8B(), v12.V8B(), 7);
  __ usra(v3.V8H(), v27.V8H(), 14);
  __ usubl(v29.V2D(), v12.V2S(), v30.V2S());
  __ usubl(v29.V4S(), v28.V4H(), v6.V4H());
  __ usubl(v12.V8H(), v4.V8B(), v14.V8B());
  __ usubl2(v1.V2D(), v24.V4S(), v17.V4S());
  __ usubl2(v4.V4S(), v1.V8H(), v3.V8H());
  __ usubl2(v23.V8H(), v4.V16B(), v7.V16B());
  __ usubw(v9.V2D(), v20.V2D(), v30.V2S());
  __ usubw(v20.V4S(), v16.V4S(), v23.V4H());
  __ usubw(v25.V8H(), v8.V8H(), v29.V8B());
  __ usubw2(v18.V2D(), v29.V2D(), v6.V4S());
  __ usubw2(v6.V4S(), v6.V4S(), v20.V8H());
  __ usubw2(v18.V8H(), v4.V8H(), v16.V16B());
  __ uxtl(v27.V2D(), v21.V2S());
  __ uxtl(v0.V4S(), v31.V4H());
  __ uxtl(v27.V8H(), v10.V8B());
  __ uxtl2(v6.V2D(), v16.V4S());
  __ uxtl2(v22.V4S(), v20.V8H());
  __ uxtl2(v20.V8H(), v21.V16B());
  __ uzp1(v30.V16B(), v9.V16B(), v17.V16B());
  __ uzp1(v7.V2D(), v26.V2D(), v28.V2D());
  __ uzp1(v26.V2S(), v16.V2S(), v22.V2S());
  __ uzp1(v14.V4H(), v19.V4H(), v6.V4H());
  __ uzp1(v17.V4S(), v23.V4S(), v30.V4S());
  __ uzp1(v28.V8B(), v27.V8B(), v13.V8B());
  __ uzp1(v17.V8H(), v1.V8H(), v12.V8H());
  __ uzp2(v8.V16B(), v18.V16B(), v26.V16B());
  __ uzp2(v21.V2D(), v22.V2D(), v24.V2D());
  __ uzp2(v20.V2S(), v21.V2S(), v2.V2S());
  __ uzp2(v16.V4H(), v31.V4H(), v6.V4H());
  __ uzp2(v25.V4S(), v11.V4S(), v8.V4S());
  __ uzp2(v31.V8B(), v31.V8B(), v13.V8B());
  __ uzp2(v8.V8H(), v17.V8H(), v1.V8H());
  __ xtn(v17.V2S(), v26.V2D());
  __ xtn(v3.V4H(), v0.V4S());
  __ xtn(v18.V8B(), v8.V8H());
  __ xtn2(v0.V16B(), v0.V8H());
  __ xtn2(v15.V4S(), v4.V2D());
  __ xtn2(v31.V8H(), v18.V4S());
  __ zip1(v22.V16B(), v9.V16B(), v6.V16B());
  __ zip1(v23.V2D(), v11.V2D(), v2.V2D());
  __ zip1(v26.V2S(), v16.V2S(), v9.V2S());
  __ zip1(v1.V4H(), v9.V4H(), v7.V4H());
  __ zip1(v0.V4S(), v30.V4S(), v20.V4S());
  __ zip1(v30.V8B(), v17.V8B(), v15.V8B());
  __ zip1(v17.V8H(), v8.V8H(), v2.V8H());
  __ zip2(v23.V16B(), v10.V16B(), v11.V16B());
  __ zip2(v30.V2D(), v6.V2D(), v14.V2D());
  __ zip2(v9.V2S(), v10.V2S(), v21.V2S());
  __ zip2(v8.V4H(), v24.V4H(), v29.V4H());
  __ zip2(v0.V4S(), v21.V4S(), v23.V4S());
  __ zip2(v25.V8B(), v23.V8B(), v30.V8B());
  __ zip2(v7.V8H(), v10.V8H(), v30.V8H());
}  // NOLINT(readability/fn_size)


static void GenerateTestSequenceNEONFP(MacroAssembler* masm) {
  ExactAssemblyScope guard(masm,
                           masm->GetBuffer()->GetRemainingBytes(),
                           ExactAssemblyScope::kMaximumSize);

  // NEON floating point instructions.
  __ fabd(v3.V2D(), v25.V2D(), v8.V2D());
  __ fabd(v14.V2S(), v27.V2S(), v11.V2S());
  __ fabd(v9.V4S(), v22.V4S(), v18.V4S());
  __ fabs(v1.V2D(), v29.V2D());
  __ fabs(v6.V2S(), v21.V2S());
  __ fabs(v12.V4S(), v25.V4S());
  __ facge(v18.V2D(), v5.V2D(), v0.V2D());
  __ facge(v15.V2S(), v11.V2S(), v6.V2S());
  __ facge(v30.V4S(), v10.V4S(), v25.V4S());
  __ facgt(v28.V2D(), v16.V2D(), v31.V2D());
  __ facgt(v15.V2S(), v1.V2S(), v4.V2S());
  __ facgt(v22.V4S(), v3.V4S(), v10.V4S());
  __ fadd(v7.V2D(), v10.V2D(), v24.V2D());
  __ fadd(v10.V2S(), v23.V2S(), v7.V2S());
  __ fadd(v16.V4S(), v22.V4S(), v11.V4S());
  __ faddp(d27, v28.V2D());
  __ faddp(s20, v23.V2S());
  __ faddp(v21.V2D(), v4.V2D(), v11.V2D());
  __ faddp(v31.V2S(), v26.V2S(), v1.V2S());
  __ faddp(v13.V4S(), v27.V4S(), v28.V4S());
  __ fcmeq(v17.V2D(), v13.V2D(), v20.V2D());
  __ fcmeq(v24.V2D(), v16.V2D(), 0.0);
  __ fcmeq(v26.V2S(), v17.V2S(), v10.V2S());
  __ fcmeq(v24.V2S(), v4.V2S(), 0.0);
  __ fcmeq(v8.V4S(), v4.V4S(), v14.V4S());
  __ fcmeq(v26.V4S(), v25.V4S(), 0.0);
  __ fcmge(v27.V2D(), v0.V2D(), v0.V2D());
  __ fcmge(v22.V2D(), v30.V2D(), 0.0);
  __ fcmge(v7.V2S(), v21.V2S(), v25.V2S());
  __ fcmge(v15.V2S(), v15.V2S(), 0.0);
  __ fcmge(v29.V4S(), v4.V4S(), v27.V4S());
  __ fcmge(v22.V4S(), v21.V4S(), 0.0);
  __ fcmgt(v1.V2D(), v26.V2D(), v15.V2D());
  __ fcmgt(v15.V2D(), v23.V2D(), 0.0);
  __ fcmgt(v21.V2S(), v16.V2S(), v6.V2S());
  __ fcmgt(v1.V2S(), v13.V2S(), 0.0);
  __ fcmgt(v14.V4S(), v0.V4S(), v25.V4S());
  __ fcmgt(v13.V4S(), v8.V4S(), 0.0);
  __ fcmle(v4.V2D(), v6.V2D(), 0.0);
  __ fcmle(v24.V2S(), v31.V2S(), 0.0);
  __ fcmle(v8.V4S(), v23.V4S(), 0.0);
  __ fcmlt(v7.V2D(), v3.V2D(), 0.0);
  __ fcmlt(v15.V2S(), v21.V2S(), 0.0);
  __ fcmlt(v1.V4S(), v2.V4S(), 0.0);
  __ fcvtas(v6.V2D(), v8.V2D());
  __ fcvtas(v1.V2S(), v9.V2S());
  __ fcvtas(v8.V4S(), v19.V4S());
  __ fcvtau(v5.V2D(), v31.V2D());
  __ fcvtau(v28.V2S(), v29.V2S());
  __ fcvtau(v11.V4S(), v26.V4S());
  __ fcvtl(v8.V2D(), v25.V2S());
  __ fcvtl(v27.V4S(), v14.V4H());
  __ fcvtl2(v1.V2D(), v6.V4S());
  __ fcvtl2(v24.V4S(), v9.V8H());
  __ fcvtms(v9.V2D(), v24.V2D());
  __ fcvtms(v7.V2S(), v11.V2S());
  __ fcvtms(v23.V4S(), v21.V4S());
  __ fcvtmu(v13.V2D(), v1.V2D());
  __ fcvtmu(v26.V2S(), v12.V2S());
  __ fcvtmu(v21.V4S(), v21.V4S());
  __ fcvtn(v11.V2S(), v1.V2D());
  __ fcvtn(v8.V4H(), v2.V4S());
  __ fcvtn2(v24.V4S(), v29.V2D());
  __ fcvtn2(v4.V8H(), v10.V4S());
  __ fcvtns(v25.V2D(), v10.V2D());
  __ fcvtns(v4.V2S(), v8.V2S());
  __ fcvtns(v29.V4S(), v27.V4S());
  __ fcvtnu(v18.V2D(), v27.V2D());
  __ fcvtnu(v11.V2S(), v14.V2S());
  __ fcvtnu(v27.V4S(), v21.V4S());
  __ fcvtps(v23.V2D(), v5.V2D());
  __ fcvtps(v24.V2S(), v15.V2S());
  __ fcvtps(v5.V4S(), v19.V4S());
  __ fcvtpu(v3.V2D(), v21.V2D());
  __ fcvtpu(v3.V2S(), v21.V2S());
  __ fcvtpu(v0.V4S(), v7.V4S());
  __ fcvtxn(v29.V2S(), v11.V2D());
  __ fcvtxn2(v31.V4S(), v25.V2D());
  __ fcvtzs(v19.V2D(), v17.V2D());
  __ fcvtzs(v12.V2D(), v24.V2D(), 64);
  __ fcvtzs(v9.V2S(), v2.V2S());
  __ fcvtzs(v5.V2S(), v20.V2S(), 29);
  __ fcvtzs(v21.V4S(), v25.V4S());
  __ fcvtzs(v26.V4S(), v1.V4S(), 6);
  __ fcvtzu(v13.V2D(), v25.V2D());
  __ fcvtzu(v28.V2D(), v13.V2D(), 32);
  __ fcvtzu(v26.V2S(), v6.V2S());
  __ fcvtzu(v9.V2S(), v10.V2S(), 15);
  __ fcvtzu(v30.V4S(), v6.V4S());
  __ fcvtzu(v19.V4S(), v22.V4S(), 18);
  __ fdiv(v15.V2D(), v8.V2D(), v15.V2D());
  __ fdiv(v12.V2S(), v9.V2S(), v26.V2S());
  __ fdiv(v19.V4S(), v22.V4S(), v19.V4S());
  __ fmax(v19.V2D(), v7.V2D(), v8.V2D());
  __ fmax(v25.V2S(), v12.V2S(), v29.V2S());
  __ fmax(v6.V4S(), v15.V4S(), v5.V4S());
  __ fmaxnm(v16.V2D(), v8.V2D(), v20.V2D());
  __ fmaxnm(v15.V2S(), v26.V2S(), v25.V2S());
  __ fmaxnm(v23.V4S(), v14.V4S(), v16.V4S());
  __ fmaxnmp(d6, v19.V2D());
  __ fmaxnmp(s27, v26.V2S());
  __ fmaxnmp(v8.V2D(), v12.V2D(), v23.V2D());
  __ fmaxnmp(v13.V2S(), v25.V2S(), v22.V2S());
  __ fmaxnmp(v15.V4S(), v11.V4S(), v17.V4S());
  __ fmaxnmv(s27, v19.V4S());
  __ fmaxp(d20, v14.V2D());
  __ fmaxp(s18, v2.V2S());
  __ fmaxp(v9.V2D(), v23.V2D(), v31.V2D());
  __ fmaxp(v7.V2S(), v22.V2S(), v31.V2S());
  __ fmaxp(v18.V4S(), v7.V4S(), v29.V4S());
  __ fmaxv(s31, v29.V4S());
  __ fmin(v2.V2D(), v5.V2D(), v2.V2D());
  __ fmin(v31.V2S(), v17.V2S(), v10.V2S());
  __ fmin(v10.V4S(), v4.V4S(), v16.V4S());
  __ fminnm(v21.V2D(), v6.V2D(), v5.V2D());
  __ fminnm(v22.V2S(), v18.V2S(), v14.V2S());
  __ fminnm(v25.V4S(), v31.V4S(), v3.V4S());
  __ fminnmp(d9, v1.V2D());
  __ fminnmp(s21, v20.V2S());
  __ fminnmp(v16.V2D(), v21.V2D(), v19.V2D());
  __ fminnmp(v16.V2S(), v31.V2S(), v25.V2S());
  __ fminnmp(v26.V4S(), v16.V4S(), v15.V4S());
  __ fminnmv(s3, v4.V4S());
  __ fminp(d24, v26.V2D());
  __ fminp(s7, v17.V2S());
  __ fminp(v23.V2D(), v19.V2D(), v3.V2D());
  __ fminp(v29.V2S(), v21.V2S(), v9.V2S());
  __ fminp(v0.V4S(), v24.V4S(), v21.V4S());
  __ fminv(s25, v8.V4S());
  __ fmla(d23, d0, v9.D(), 1);
  __ fmla(s23, s15, v7.S(), 0);
  __ fmla(v17.V2D(), v11.V2D(), v6.V2D());
  __ fmla(v30.V2D(), v30.V2D(), v11.D(), 0);
  __ fmla(v19.V2S(), v12.V2S(), v6.V2S());
  __ fmla(v24.V2S(), v17.V2S(), v9.S(), 0);
  __ fmla(v16.V4S(), v11.V4S(), v11.V4S());
  __ fmla(v27.V4S(), v23.V4S(), v9.S(), 2);
  __ fmls(d27, d30, v6.D(), 0);
  __ fmls(s21, s16, v2.S(), 0);
  __ fmls(v5.V2D(), v19.V2D(), v21.V2D());
  __ fmls(v18.V2D(), v30.V2D(), v12.D(), 0);
  __ fmls(v5.V2S(), v16.V2S(), v7.V2S());
  __ fmls(v3.V2S(), v18.V2S(), v11.S(), 1);
  __ fmls(v27.V4S(), v5.V4S(), v30.V4S());
  __ fmls(v26.V4S(), v20.V4S(), v4.S(), 3);
  __ fmov(v14.V2D(), -0.34375);
  __ fmov(v26.V2S(), 0.90625f);
  __ fmov(v31.V4S(), -5.0000f);
  __ fmov(v28.D(), 1, x25);
  __ fmov(x18, v2.D(), 1);
  __ fmul(d12, d4, v1.D(), 1);
  __ fmul(s30, s1, v15.S(), 3);
  __ fmul(v25.V2D(), v0.V2D(), v21.V2D());
  __ fmul(v10.V2D(), v24.V2D(), v10.D(), 1);
  __ fmul(v7.V2S(), v24.V2S(), v16.V2S());
  __ fmul(v1.V2S(), v16.V2S(), v4.S(), 2);
  __ fmul(v5.V4S(), v28.V4S(), v25.V4S());
  __ fmul(v11.V4S(), v3.V4S(), v8.S(), 0);
  __ fmulx(d28, d9, v3.D(), 1);
  __ fmulx(s25, s21, v15.S(), 1);
  __ fmulx(v31.V2D(), v28.V2D(), v8.V2D());
  __ fmulx(v3.V2D(), v21.V2D(), v6.D(), 0);
  __ fmulx(v9.V2S(), v1.V2S(), v0.V2S());
  __ fmulx(v16.V2S(), v27.V2S(), v6.S(), 0);
  __ fmulx(v2.V4S(), v4.V4S(), v5.V4S());
  __ fmulx(v18.V4S(), v7.V4S(), v4.S(), 0);
  __ fneg(v1.V2D(), v25.V2D());
  __ fneg(v14.V2S(), v31.V2S());
  __ fneg(v5.V4S(), v4.V4S());
  __ frecpe(v18.V2D(), v12.V2D());
  __ frecpe(v10.V2S(), v22.V2S());
  __ frecpe(v5.V4S(), v6.V4S());
  __ frecps(v22.V2D(), v7.V2D(), v26.V2D());
  __ frecps(v31.V2S(), v27.V2S(), v2.V2S());
  __ frecps(v18.V4S(), v6.V4S(), v27.V4S());
  __ frinta(v26.V2D(), v13.V2D());
  __ frinta(v15.V2S(), v26.V2S());
  __ frinta(v13.V4S(), v16.V4S());
  __ frinti(v9.V2D(), v12.V2D());
  __ frinti(v5.V2S(), v19.V2S());
  __ frinti(v15.V4S(), v11.V4S());
  __ frintm(v17.V2D(), v29.V2D());
  __ frintm(v30.V2S(), v11.V2S());
  __ frintm(v1.V4S(), v20.V4S());
  __ frintn(v24.V2D(), v6.V2D());
  __ frintn(v12.V2S(), v17.V2S());
  __ frintn(v29.V4S(), v11.V4S());
  __ frintp(v10.V2D(), v7.V2D());
  __ frintp(v12.V2S(), v18.V2S());
  __ frintp(v26.V4S(), v31.V4S());
  __ frintx(v24.V2D(), v13.V2D());
  __ frintx(v7.V2S(), v9.V2S());
  __ frintx(v18.V4S(), v21.V4S());
  __ frintz(v19.V2D(), v25.V2D());
  __ frintz(v15.V2S(), v8.V2S());
  __ frintz(v20.V4S(), v3.V4S());
  __ frsqrte(v23.V2D(), v5.V2D());
  __ frsqrte(v9.V2S(), v7.V2S());
  __ frsqrte(v3.V4S(), v9.V4S());
  __ frsqrts(v25.V2D(), v28.V2D(), v15.V2D());
  __ frsqrts(v9.V2S(), v26.V2S(), v10.V2S());
  __ frsqrts(v5.V4S(), v1.V4S(), v10.V4S());
  __ fsqrt(v6.V2D(), v18.V2D());
  __ fsqrt(v6.V2S(), v18.V2S());
  __ fsqrt(v0.V4S(), v31.V4S());
  __ fsub(v31.V2D(), v30.V2D(), v31.V2D());
  __ fsub(v11.V2S(), v8.V2S(), v6.V2S());
  __ fsub(v16.V4S(), v0.V4S(), v31.V4S());
  __ scvtf(v25.V2D(), v31.V2D());
  __ scvtf(v10.V2D(), v13.V2D(), 45);
  __ scvtf(v10.V2S(), v15.V2S());
  __ scvtf(v18.V2S(), v4.V2S(), 27);
  __ scvtf(v17.V4S(), v5.V4S());
  __ scvtf(v11.V4S(), v25.V4S(), 24);
  __ ucvtf(v9.V2D(), v3.V2D());
  __ ucvtf(v26.V2D(), v30.V2D(), 46);
  __ ucvtf(v11.V2S(), v4.V2S());
  __ ucvtf(v29.V2S(), v3.V2S(), 25);
  __ ucvtf(v22.V4S(), v23.V4S());
  __ ucvtf(v18.V4S(), v9.V4S(), 25);
}


static void GenerateTestSequenceSVE(MacroAssembler* masm) {
  ExactAssemblyScope guard(masm,
                           masm->GetBuffer()->GetRemainingBytes(),
                           ExactAssemblyScope::kMaximumSize);
  CPUFeaturesScope feature_guard(masm, CPUFeatures::kSVE);

  // Simple, unpredicated loads and stores.
  __ str(p12.VnD(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ str(p13.VnS(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ str(p14.VnH(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ str(p15.VnB(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ ldr(p8.VnD(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ ldr(p9.VnS(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ ldr(p10.VnH(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ ldr(p11.VnB(), SVEMemOperand(x0, 11, SVE_MUL_VL));

  __ str(z0.VnD(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ str(z1.VnS(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ str(z2.VnH(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ str(z3.VnB(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ ldr(z20.VnD(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ ldr(z21.VnS(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ ldr(z22.VnH(), SVEMemOperand(x0, 11, SVE_MUL_VL));
  __ ldr(z23.VnB(), SVEMemOperand(x0, 11, SVE_MUL_VL));

  // Structured accesses.
  __ st1b(z0.VnB(), p2, SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ st1h(z1.VnH(), p1, SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ st1w(z2.VnS(), p1, SVEMemOperand(x0, x3, LSL, 2));
  __ st1d(z3.VnD(), p2, SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1b(z20.VnB(), p1.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1h(z21.VnH(), p2.Zeroing(), SVEMemOperand(x0, x2, LSL, 1));
  __ ld1w(z22.VnS(), p1.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1d(z23.VnD(), p1.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));

  // Structured, packed accesses.
  __ st1b(z2.VnH(), p1, SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ st1b(z3.VnS(), p2, SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ st1b(z4.VnD(), p2, SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ st1h(z0.VnS(), p1, SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ st1h(z1.VnD(), p1, SVEMemOperand(x0, x2, LSL, 1));
  __ st1w(z2.VnD(), p1, SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1b(z20.VnH(), p1.Zeroing(), SVEMemOperand(x0, x2));
  __ ld1b(z21.VnS(), p1.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1b(z22.VnD(), p1.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1h(z23.VnS(), p2.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1h(z24.VnD(), p2.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1w(z20.VnD(), p1.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1sb(z21.VnH(), p1.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1sb(z22.VnS(), p1.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1sb(z23.VnD(), p2.Zeroing(), SVEMemOperand(x0, x2));
  __ ld1sh(z24.VnS(), p2.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1sh(z20.VnD(), p1.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld1sw(z21.VnD(), p1.Zeroing(), SVEMemOperand(x0, 3, SVE_MUL_VL));

  // Structured, interleaved accesses.
  __ st2b(z0.VnB(), z1.VnB(), p4, SVEMemOperand(x0, 4, SVE_MUL_VL));
  __ st2h(z1.VnH(), z2.VnH(), p4, SVEMemOperand(x0, 4, SVE_MUL_VL));
  __ st2w(z2.VnS(), z3.VnS(), p3, SVEMemOperand(x0, x2, LSL, 2));
  __ st2d(z3.VnD(), z4.VnD(), p4, SVEMemOperand(x0, 4, SVE_MUL_VL));
  __ ld2b(z20.VnB(), z21.VnB(), p5.Zeroing(), SVEMemOperand(x0, x2));
  __ ld2h(z21.VnH(), z22.VnH(), p6.Zeroing(), SVEMemOperand(x0, 4, SVE_MUL_VL));
  __ ld2w(z22.VnS(), z23.VnS(), p6.Zeroing(), SVEMemOperand(x0, 4, SVE_MUL_VL));
  __ ld2d(z23.VnD(), z24.VnD(), p5.Zeroing(), SVEMemOperand(x0, 4, SVE_MUL_VL));

  __ st3b(z4.VnB(), z5.VnB(), z6.VnB(), p4, SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ st3h(z5.VnH(), z6.VnH(), z7.VnH(), p4, SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ st3w(z6.VnS(), z7.VnS(), z8.VnS(), p3, SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ st3d(z7.VnD(), z8.VnD(), z9.VnD(), p4, SVEMemOperand(x0, x2, LSL, 3));
  __ ld3b(z24.VnB(),
          z25.VnB(),
          z26.VnB(),
          p5.Zeroing(),
          SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld3h(z25.VnH(),
          z26.VnH(),
          z27.VnH(),
          p6.Zeroing(),
          SVEMemOperand(x0, x2, LSL, 1));
  __ ld3w(z26.VnS(),
          z27.VnS(),
          z28.VnS(),
          p6.Zeroing(),
          SVEMemOperand(x0, 3, SVE_MUL_VL));
  __ ld3d(z27.VnD(),
          z28.VnD(),
          z29.VnD(),
          p5.Zeroing(),
          SVEMemOperand(x0, 3, SVE_MUL_VL));

  __ st4b(z31.VnB(),
          z0.VnB(),
          z1.VnB(),
          z2.VnB(),
          p4,
          SVEMemOperand(x0, 4, SVE_MUL_VL));
  __ st4h(z0.VnH(),
          z1.VnH(),
          z2.VnH(),
          z3.VnH(),
          p4,
          SVEMemOperand(x0, 4, SVE_MUL_VL));
  __ st4w(z1.VnS(),
          z2.VnS(),
          z3.VnS(),
          z4.VnS(),
          p3,
          SVEMemOperand(x0, 4, SVE_MUL_VL));
  __ st4d(z2.VnD(),
          z3.VnD(),
          z4.VnD(),
          z5.VnD(),
          p4,
          SVEMemOperand(x0, x2, LSL, 3));
  __ ld4b(z25.VnB(),
          z26.VnB(),
          z27.VnB(),
          z28.VnB(),
          p5.Zeroing(),
          SVEMemOperand(x0, 4, SVE_MUL_VL));
  __ ld4h(z26.VnH(),
          z27.VnH(),
          z28.VnH(),
          z29.VnH(),
          p6.Zeroing(),
          SVEMemOperand(x0, 4, SVE_MUL_VL));
  __ ld4w(z27.VnS(),
          z28.VnS(),
          z29.VnS(),
          z30.VnS(),
          p6.Zeroing(),
          SVEMemOperand(x0, x2, LSL, 2));
  __ ld4d(z28.VnD(),
          z29.VnD(),
          z30.VnD(),
          z31.VnD(),
          p5.Zeroing(),
          SVEMemOperand(x0, 4, SVE_MUL_VL));
}

static void MaskAddresses(const char* trace) {
#define VIXL_COLOUR "(\x1b\\[[01];([0-9][0-9])?m)?"
  // All patterns are replaced with "$1~~~~~~~~~~~~~~~~".
  std::regex patterns[] =
      {// Mask registers that hold addresses that change from run to run.
       std::regex("((x0|x1|x2|sp): " VIXL_COLOUR "0x)[0-9a-f]{16}"),
       // Mask accessed memory addresses.
       std::regex("((<-|->) " VIXL_COLOUR "0x)[0-9a-f]{16}"),
       // Mask instruction addresses.
       std::regex("^(0x)[0-9a-f]{16}"),
       // Mask branch targets.
       std::regex("(Branch" VIXL_COLOUR " to 0x)[0-9a-f]{16}"),
       // Mask explicit address annotations.
       std::regex("(addr 0x)[0-9a-f]+")};
#undef VIXL_COLOUR

  std::vector<std::string> lines;
  std::ifstream in(trace);
  while (!in.eof()) {
    std::string line;
    std::getline(in, line);
    for (auto&& pattern : patterns) {
      line = std::regex_replace(line, pattern, "$1~~~~~~~~~~~~~~~~");
    }
    lines.push_back(line);
  }
  in.close();

  // `getline` produces an empty line after a terminal "\n".
  if (lines.back().empty()) lines.pop_back();

  std::ofstream out(trace, std::ofstream::trunc);
  for (auto&& line : lines) {
    out << line << "\n";
  }
}

static void PrintFile(const char* name) {
  FILE* file = fopen(name, "r");
  char buffer[1024];  // The buffer size is arbitrary.
  while (fgets(buffer, sizeof(buffer), file) != NULL) fputs(buffer, stdout);
  fclose(file);
}

static bool CheckOrGenerateTrace(const char* filename, const char* ref_file) {
  bool trace_matched_reference;
  if (Test::generate_test_trace()) {
    // Copy trace_stream to stdout.
    FILE* trace_stream = fopen(filename, "r");
    VIXL_ASSERT(trace_stream != NULL);
    fseek(trace_stream, 0, SEEK_SET);
    int c;
    while (1) {
      c = getc(trace_stream);
      if (c == EOF) break;
      putc(c, stdout);
    }
    fclose(trace_stream);
    trace_matched_reference = true;
  } else {
    // Check trace_stream against ref_file.
    char command[1024];
    size_t length =
        snprintf(command, sizeof(command), "diff -u %s %s", ref_file, filename);
    VIXL_CHECK(length < sizeof(command));
    trace_matched_reference = (system(command) == 0);
  }
  return trace_matched_reference;
}


// Trace tests can only work with the simulator.
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64

static void TraceTestHelper(bool coloured_trace,
                            TraceParameters trace_parameters,
                            const char* ref_file) {
  MacroAssembler masm(12 * KBytes);

  char trace_stream_filename[] = "/tmp/vixl-test-trace-XXXXXX";
  FILE* trace_stream = fdopen(mkstemp(trace_stream_filename), "w");

  Decoder decoder;
  Simulator simulator(&decoder, trace_stream);
  simulator.SetColouredTrace(coloured_trace);
  simulator.SetTraceParameters(trace_parameters);
  simulator.SilenceExclusiveAccessWarning();

  const int vl_in_bytes = 5 * kZRegMinSizeInBytes;
  const int vl_in_bits = vl_in_bytes * kBitsPerByte;
  const int pl_in_bits = vl_in_bits / kZRegBitsPerPRegBit;
  simulator.SetVectorLengthInBits(vl_in_bits);

  // Set up a scratch buffer so we can test loads and stores.
  const int kScratchSize = vl_in_bytes * 1024;
  const int kScratchGuardSize = vl_in_bytes;
  char scratch_buffer[kScratchSize + kScratchGuardSize];
  for (size_t i = 0; i < (sizeof(scratch_buffer) / sizeof(scratch_buffer[0]));
       i++) {
    scratch_buffer[i] = i & 0xff;
  }
  // Used for offset addressing.
  simulator.WriteXRegister(0, reinterpret_cast<uintptr_t>(scratch_buffer));
  // Used for pre-/post-index addressing.
  simulator.WriteXRegister(1, reinterpret_cast<uintptr_t>(scratch_buffer));

  const int kPostIndexRegisterStep = 13;  // Arbitrary interesting value.
  // Used for post-index offsets.
  simulator.WriteXRegister(2, kPostIndexRegisterStep);

  // Initialize the other registers with unique values.
  uint64_t initial_base_u64 = 0x0100001000100101;
  for (unsigned i = 3; i < kNumberOfRegisters; i++) {
    if (i == kLinkRegCode) continue;
    if (i == kZeroRegCode) continue;
    // NoRegLog suppresses the log now, but the registers will still be logged
    // before the first instruction is executed since they have been written but
    // not printed.
    simulator.WriteRegister(i, initial_base_u64 * i, Simulator::NoRegLog);
  }
  for (unsigned r = 0; r < kNumberOfVRegisters; r++) {
    LogicVRegister reg(simulator.ReadVRegister(r));
    // Try to initialise Z registers with reasonable FP values. We prioritise
    // setting double values, then floats and half-precision values. The lanes
    // overlap, so this is a compromise, but d0, s0 and h0 views all see similar
    // arithmetic values.
    //
    // The exponent of each value is set to the (biased) register number. We set
    // the double, float and half-precision exponents where we can.
    uint64_t base = 0x3ff000003f803c00 + (0x0010000000800400 * (0x7f + r));
    for (unsigned lane = 0; lane < (vl_in_bytes / kDRegSizeInBytes); lane++) {
      uint64_t mantissas = 0x0000000100010001 * (lane & 0x7f);
      reg.SetUint(kFormatVnD, lane, base | mantissas);
    }
  }
  for (unsigned r = 0; r < kNumberOfPRegisters; r++) {
    LogicPRegister reg(simulator.ReadPRegister(r));
    // Set `r` active lanes between each inactive lane.
    for (unsigned bit = 0; bit < pl_in_bits; bit++) {
      reg.SetActive(kFormatVnB, bit, ((bit + 1) % (r + 2)) != 0);
    }
    // Completely clear some Q-sized blocks. The trace will completely omit
    // these for stores.
    for (unsigned chunk = 0; chunk < (vl_in_bits / kQRegSize); chunk++) {
      if (((chunk + 1) % (r + 2)) == 0) {
        reg.SetActiveMask(chunk, static_cast<uint16_t>(0));
      }
    }
  }

  GenerateTestSequenceBase(&masm);
  GenerateTestSequenceFP(&masm);
  GenerateTestSequenceNEON(&masm);
  GenerateTestSequenceNEONFP(&masm);
  GenerateTestSequenceSVE(&masm);
  masm.Ret();
  masm.FinalizeCode();

  if (Test::disassemble()) {
    PrintDisassembler disasm(stdout);
    Instruction* start = masm.GetBuffer()->GetStartAddress<Instruction*>();
    Instruction* end = masm.GetBuffer()->GetEndAddress<Instruction*>();
    disasm.DisassembleBuffer(start, end);
  }

  simulator.RunFrom(masm.GetBuffer()->GetStartAddress<Instruction*>());

  fclose(trace_stream);

  // We already traced into the temporary file, so just print the file.
  // Note that these tests need to control the trace flags, so we ignore all
  // --trace-* options here except for --trace-sim.
  if (Test::trace_sim()) PrintFile(trace_stream_filename);

  MaskAddresses(trace_stream_filename);

  bool trace_matched_reference =
      CheckOrGenerateTrace(trace_stream_filename, ref_file);
  remove(trace_stream_filename);  // Clean up before checking the result.
  VIXL_CHECK(trace_matched_reference);

  uint64_t offset_base = simulator.ReadRegister<uint64_t>(0);
  uint64_t index_base = simulator.ReadRegister<uint64_t>(1);

  VIXL_CHECK(index_base >= offset_base);
  VIXL_CHECK((index_base - offset_base) <= kScratchSize);
}


// Test individual options.
TEST(disasm) { TraceTestHelper(false, LOG_DISASM, REF("log-disasm")); }
TEST(regs) { TraceTestHelper(false, LOG_REGS, REF("log-regs")); }
TEST(vregs) { TraceTestHelper(false, LOG_VREGS, REF("log-vregs")); }
TEST(sysregs) { TraceTestHelper(false, LOG_SYSREGS, REF("log-sysregs")); }
TEST(write) { TraceTestHelper(false, LOG_WRITE, REF("log-write")); }
TEST(branch) { TraceTestHelper(false, LOG_WRITE, REF("log-branch")); }

// Test standard combinations.
TEST(none) { TraceTestHelper(false, LOG_NONE, REF("log-none")); }
TEST(state) { TraceTestHelper(false, LOG_STATE, REF("log-state")); }
TEST(all) { TraceTestHelper(false, LOG_ALL, REF("log-all")); }


// Test individual options (with colour).
TEST(disasm_colour) {
  TraceTestHelper(true, LOG_DISASM, REF("log-disasm-colour"));
}
TEST(regs_colour) { TraceTestHelper(true, LOG_REGS, REF("log-regs-colour")); }
TEST(vregs_colour) {
  TraceTestHelper(true, LOG_VREGS, REF("log-vregs-colour"));
}
TEST(sysregs_colour) {
  TraceTestHelper(true, LOG_SYSREGS, REF("log-sysregs-colour"));
}
TEST(write_colour) {
  TraceTestHelper(true, LOG_WRITE, REF("log-write-colour"));
}
TEST(branch_colour) {
  TraceTestHelper(true, LOG_WRITE, REF("log-branch-colour"));
}

// Test standard combinations (with colour).
TEST(none_colour) { TraceTestHelper(true, LOG_NONE, REF("log-none-colour")); }
TEST(state_colour) {
  TraceTestHelper(true, LOG_STATE, REF("log-state-colour"));
}
TEST(all_colour) { TraceTestHelper(true, LOG_ALL, REF("log-all-colour")); }

#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64

static void PrintDisassemblerTestHelper(const char* prefix,
                                        const char* suffix,
                                        const char* ref_file) {
  MacroAssembler masm(12 * KBytes);

  char trace_stream_filename[] = "/tmp/vixl-test-trace-XXXXXX";
  FILE* trace_stream = fdopen(mkstemp(trace_stream_filename), "w");

  // We don't need to execute this code so there's no need for the execution
  // environment setup from TraceTestHelper.

  GenerateTestSequenceBase(&masm);
  GenerateTestSequenceFP(&masm);
  GenerateTestSequenceNEON(&masm);
  GenerateTestSequenceNEONFP(&masm);
  GenerateTestSequenceSVE(&masm);
  masm.FinalizeCode();

  Decoder decoder;
  CPUFeaturesAuditor auditor(&decoder);
  PrintDisassembler disasm(trace_stream);
  if (prefix != NULL) disasm.SetCPUFeaturesPrefix(prefix);
  if (suffix != NULL) disasm.SetCPUFeaturesSuffix(suffix);
  disasm.RegisterCPUFeaturesAuditor(&auditor);
  decoder.AppendVisitor(&disasm);

  Instruction* instruction = masm.GetBuffer()->GetStartAddress<Instruction*>();
  Instruction* end = masm.GetCursorAddress<Instruction*>();
  while (instruction != end) {
    decoder.Decode(instruction);
    instruction += kInstructionSize;
  }

  fclose(trace_stream);

  // We already disassembled into the temporary file, so just print the file.
  if (Test::disassemble()) PrintFile(trace_stream_filename);

  MaskAddresses(trace_stream_filename);

  bool trace_matched_reference =
      CheckOrGenerateTrace(trace_stream_filename, ref_file);
  remove(trace_stream_filename);  // Clean up before checking the result.
  VIXL_CHECK(trace_matched_reference);
}


// Test CPUFeatures disassembly annotations.
TEST(cpufeatures) {
  PrintDisassemblerTestHelper(NULL, NULL, REF("log-cpufeatures"));
}
TEST(cpufeatures_custom) {
  PrintDisassemblerTestHelper("### {", "} ###", REF("log-cpufeatures-custom"));
}
TEST(cpufeatures_colour) {
  // The colour chosen is arbitrary.
  PrintDisassemblerTestHelper("\033[1;35m",  // Prefix: Bold magenta.
                              "\033[0;m",    // Suffix: Reset colour.
                              REF("log-cpufeatures-colour"));
}
}  // namespace aarch64
}  // namespace vixl
