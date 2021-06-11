// Copyright 2019, VIXL authors
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


#include <cstdio>
#include <cstring>
#include <string>

#include "test-runner.h"

#include "aarch64/disasm-aarch64.h"
#include "aarch64/macro-assembler-aarch64.h"
#include "test-disasm-aarch64.h"

namespace vixl {
namespace aarch64 {

TEST(fmov_imm) {
  SETUP();

  COMPARE(fmov(h2, Float16(-5.0)), "fmov h2, #0x94 (-5.0000)");
  COMPARE(fmov(h30, Float16(29.0)), "fmov h30, #0x3d (29.0000)");
  COMPARE(fmov(s0, 1.0f), "fmov s0, #0x70 (1.0000)");
  COMPARE(fmov(s31, -13.0f), "fmov s31, #0xaa (-13.0000)");
  COMPARE(fmov(d1, 1.0), "fmov d1, #0x70 (1.0000)");
  COMPARE(fmov(d29, -13.0), "fmov d29, #0xaa (-13.0000)");

  CLEANUP();
}

TEST(fmov_reg) {
  SETUP();

  COMPARE(fmov(w3, s13), "fmov w3, s13");
  COMPARE(fmov(x6, d26), "fmov x6, d26");
  COMPARE(fmov(s11, w30), "fmov s11, w30");
  COMPARE(fmov(d31, x2), "fmov d31, x2");
  COMPARE(fmov(s12, s13), "fmov s12, s13");
  COMPARE(fmov(d22, d23), "fmov d22, d23");
  COMPARE(fmov(v0.D(), 1, x13), "fmov v0.D[1], x13");
  COMPARE(fmov(x13, v0.D(), 1), "fmov x13, v0.D[1]");

  CLEANUP();
}

TEST(fp_dp1) {
  SETUP();

  COMPARE(fabs(s0, s1), "fabs s0, s1");
  COMPARE(fabs(s31, s30), "fabs s31, s30");
  COMPARE(fabs(d2, d3), "fabs d2, d3");
  COMPARE(fabs(d31, d30), "fabs d31, d30");
  COMPARE(fneg(s4, s5), "fneg s4, s5");
  COMPARE(fneg(s31, s30), "fneg s31, s30");
  COMPARE(fneg(d6, d7), "fneg d6, d7");
  COMPARE(fneg(d31, d30), "fneg d31, d30");
  COMPARE(fsqrt(s8, s9), "fsqrt s8, s9");
  COMPARE(fsqrt(s31, s30), "fsqrt s31, s30");
  COMPARE(fsqrt(d10, d11), "fsqrt d10, d11");
  COMPARE(fsqrt(d31, d30), "fsqrt d31, d30");
  COMPARE(frint32x(s10, s11), "frint32x s10, s11");
  COMPARE(frint32x(s31, s30), "frint32x s31, s30");
  COMPARE(frint32x(d12, d13), "frint32x d12, d13");
  COMPARE(frint32x(d31, d30), "frint32x d31, d30");
  COMPARE(frint32z(s10, s11), "frint32z s10, s11");
  COMPARE(frint32z(s31, s30), "frint32z s31, s30");
  COMPARE(frint32z(d12, d13), "frint32z d12, d13");
  COMPARE(frint32z(d31, d30), "frint32z d31, d30");
  COMPARE(frint64x(s10, s11), "frint64x s10, s11");
  COMPARE(frint64x(s31, s30), "frint64x s31, s30");
  COMPARE(frint64x(d12, d13), "frint64x d12, d13");
  COMPARE(frint64x(d31, d30), "frint64x d31, d30");
  COMPARE(frint64z(s10, s11), "frint64z s10, s11");
  COMPARE(frint64z(s31, s30), "frint64z s31, s30");
  COMPARE(frint64z(d12, d13), "frint64z d12, d13");
  COMPARE(frint64z(d31, d30), "frint64z d31, d30");
  COMPARE(frinta(s10, s11), "frinta s10, s11");
  COMPARE(frinta(s31, s30), "frinta s31, s30");
  COMPARE(frinta(d12, d13), "frinta d12, d13");
  COMPARE(frinta(d31, d30), "frinta d31, d30");
  COMPARE(frinti(s10, s11), "frinti s10, s11");
  COMPARE(frinti(s31, s30), "frinti s31, s30");
  COMPARE(frinti(d12, d13), "frinti d12, d13");
  COMPARE(frinti(d31, d30), "frinti d31, d30");
  COMPARE(frintm(s10, s11), "frintm s10, s11");
  COMPARE(frintm(s31, s30), "frintm s31, s30");
  COMPARE(frintm(d12, d13), "frintm d12, d13");
  COMPARE(frintm(d31, d30), "frintm d31, d30");
  COMPARE(frintn(s10, s11), "frintn s10, s11");
  COMPARE(frintn(s31, s30), "frintn s31, s30");
  COMPARE(frintn(d12, d13), "frintn d12, d13");
  COMPARE(frintn(d31, d30), "frintn d31, d30");
  COMPARE(frintx(s10, s11), "frintx s10, s11");
  COMPARE(frintx(s31, s30), "frintx s31, s30");
  COMPARE(frintx(d12, d13), "frintx d12, d13");
  COMPARE(frintx(d31, d30), "frintx d31, d30");
  COMPARE(frintz(s10, s11), "frintz s10, s11");
  COMPARE(frintz(s31, s30), "frintz s31, s30");
  COMPARE(frintz(d12, d13), "frintz d12, d13");
  COMPARE(frintz(d31, d30), "frintz d31, d30");
  COMPARE(fcvt(d14, s15), "fcvt d14, s15");
  COMPARE(fcvt(d31, s31), "fcvt d31, s31");
  COMPARE(fcvt(s0, d1), "fcvt s0, d1");
  COMPARE(fcvt(s2, h3), "fcvt s2, h3");
  COMPARE(fcvt(d4, h5), "fcvt d4, h5");
  COMPARE(fcvt(h6, s7), "fcvt h6, s7");
  COMPARE(fcvt(h8, d9), "fcvt h8, d9");

  CLEANUP();
}

TEST(fp_dp2) {
  SETUP();

  COMPARE(fadd(h8, h9, h10), "fadd h8, h9, h10");
  COMPARE(fadd(s0, s1, s2), "fadd s0, s1, s2");
  COMPARE(fadd(d3, d4, d5), "fadd d3, d4, d5");
  COMPARE(fsub(h14, h17, h21), "fsub h14, h17, h21");
  COMPARE(fsub(s31, s30, s29), "fsub s31, s30, s29");
  COMPARE(fsub(d31, d30, d29), "fsub d31, d30, d29");
  COMPARE(fmul(h13, h14, h15), "fmul h13, h14, h15");
  COMPARE(fmul(s7, s8, s9), "fmul s7, s8, s9");
  COMPARE(fmul(d10, d11, d12), "fmul d10, d11, d12");
  COMPARE(fnmul(h4, h5, h6), "fnmul h4, h5, h6");
  COMPARE(fnmul(s7, s8, s9), "fnmul s7, s8, s9");
  COMPARE(fnmul(d10, d11, d12), "fnmul d10, d11, d12");
  COMPARE(fdiv(h0, h1, h2), "fdiv h0, h1, h2");
  COMPARE(fdiv(s13, s14, s15), "fdiv s13, s14, s15");
  COMPARE(fdiv(d16, d17, d18), "fdiv d16, d17, d18");
  COMPARE(fmax(h9, h10, h11), "fmax h9, h10, h11");
  COMPARE(fmax(s19, s20, s21), "fmax s19, s20, s21");
  COMPARE(fmax(d22, d23, d24), "fmax d22, d23, d24");
  COMPARE(fmin(h13, h15, h17), "fmin h13, h15, h17");
  COMPARE(fmin(s25, s26, s27), "fmin s25, s26, s27");
  COMPARE(fmin(d28, d29, d30), "fmin d28, d29, d30");
  COMPARE(fmaxnm(h4, h5, h6), "fmaxnm h4, h5, h6");
  COMPARE(fmaxnm(s31, s0, s1), "fmaxnm s31, s0, s1");
  COMPARE(fmaxnm(d2, d3, d4), "fmaxnm d2, d3, d4");
  COMPARE(fminnm(h22, h23, h24), "fminnm h22, h23, h24");
  COMPARE(fminnm(s5, s6, s7), "fminnm s5, s6, s7");
  COMPARE(fminnm(d8, d9, d10), "fminnm d8, d9, d10");

  CLEANUP();
}

TEST(fp_dp3) {
  SETUP();

  COMPARE(fmadd(h4, h5, h6, h10), "fmadd h4, h5, h6, h10");
  COMPARE(fmadd(s7, s8, s9, s10), "fmadd s7, s8, s9, s10");
  COMPARE(fmadd(d10, d11, d12, d10), "fmadd d10, d11, d12, d10");
  COMPARE(fmsub(h4, h5, h6, h10), "fmsub h4, h5, h6, h10");
  COMPARE(fmsub(s7, s8, s9, s10), "fmsub s7, s8, s9, s10");
  COMPARE(fmsub(d10, d11, d12, d10), "fmsub d10, d11, d12, d10");

  COMPARE(fnmadd(h4, h5, h6, h10), "fnmadd h4, h5, h6, h10");
  COMPARE(fnmadd(s7, s8, s9, s10), "fnmadd s7, s8, s9, s10");
  COMPARE(fnmadd(d10, d11, d12, d10), "fnmadd d10, d11, d12, d10");
  COMPARE(fnmsub(h4, h5, h6, h10), "fnmsub h4, h5, h6, h10");
  COMPARE(fnmsub(s7, s8, s9, s10), "fnmsub s7, s8, s9, s10");
  COMPARE(fnmsub(d10, d11, d12, d10), "fnmsub d10, d11, d12, d10");

  CLEANUP();
}

TEST(fp_compare) {
  SETUP();

  COMPARE(fcmp(h0, h1), "fcmp h0, h1");
  COMPARE(fcmp(h31, h30), "fcmp h31, h30");
  COMPARE(fcmp(s0, s1), "fcmp s0, s1");
  COMPARE(fcmp(s31, s30), "fcmp s31, s30");
  COMPARE(fcmp(d0, d1), "fcmp d0, d1");
  COMPARE(fcmp(d31, d30), "fcmp d31, d30");
  COMPARE(fcmp(h12, 0), "fcmp h12, #0.0");
  COMPARE(fcmp(s12, 0), "fcmp s12, #0.0");
  COMPARE(fcmp(d12, 0), "fcmp d12, #0.0");

  COMPARE(fcmpe(h0, h1), "fcmpe h0, h1");
  COMPARE(fcmpe(h31, h30), "fcmpe h31, h30");
  COMPARE(fcmpe(s0, s1), "fcmpe s0, s1");
  COMPARE(fcmpe(s31, s30), "fcmpe s31, s30");
  COMPARE(fcmpe(d0, d1), "fcmpe d0, d1");
  COMPARE(fcmpe(d31, d30), "fcmpe d31, d30");
  COMPARE(fcmpe(h12, 0), "fcmpe h12, #0.0");
  COMPARE(fcmpe(s12, 0), "fcmpe s12, #0.0");
  COMPARE(fcmpe(d12, 0), "fcmpe d12, #0.0");

  CLEANUP();
}

TEST(fp_cond_compare) {
  SETUP();

  COMPARE(fccmp(h8, h9, NoFlag, eq), "fccmp h8, h9, #nzcv, eq");
  COMPARE(fccmp(h10, h11, ZVFlag, ne), "fccmp h10, h11, #nZcV, ne");
  COMPARE(fccmp(h30, h16, NCFlag, pl), "fccmp h30, h16, #NzCv, pl");
  COMPARE(fccmp(h31, h31, NZCVFlag, le), "fccmp h31, h31, #NZCV, le");
  COMPARE(fccmp(s0, s1, NoFlag, eq), "fccmp s0, s1, #nzcv, eq");
  COMPARE(fccmp(s2, s3, ZVFlag, ne), "fccmp s2, s3, #nZcV, ne");
  COMPARE(fccmp(s30, s16, NCFlag, pl), "fccmp s30, s16, #NzCv, pl");
  COMPARE(fccmp(s31, s31, NZCVFlag, le), "fccmp s31, s31, #NZCV, le");
  COMPARE(fccmp(d4, d5, VFlag, gt), "fccmp d4, d5, #nzcV, gt");
  COMPARE(fccmp(d6, d7, NFlag, vs), "fccmp d6, d7, #Nzcv, vs");
  COMPARE(fccmp(d30, d0, NZFlag, vc), "fccmp d30, d0, #NZcv, vc");
  COMPARE(fccmp(d31, d31, ZFlag, hs), "fccmp d31, d31, #nZcv, hs");
  COMPARE(fccmp(h12, h13, CVFlag, al), "fccmp h12, h13, #nzCV, al");
  COMPARE(fccmp(s14, s15, CVFlag, al), "fccmp s14, s15, #nzCV, al");
  COMPARE(fccmp(d16, d17, CFlag, nv), "fccmp d16, d17, #nzCv, nv");

  COMPARE(fccmpe(h8, h9, NoFlag, eq), "fccmpe h8, h9, #nzcv, eq");
  COMPARE(fccmpe(h10, h11, ZVFlag, ne), "fccmpe h10, h11, #nZcV, ne");
  COMPARE(fccmpe(h30, h16, NCFlag, pl), "fccmpe h30, h16, #NzCv, pl");
  COMPARE(fccmpe(h31, h31, NZCVFlag, le), "fccmpe h31, h31, #NZCV, le");
  COMPARE(fccmpe(s0, s1, NoFlag, eq), "fccmpe s0, s1, #nzcv, eq");
  COMPARE(fccmpe(s2, s3, ZVFlag, ne), "fccmpe s2, s3, #nZcV, ne");
  COMPARE(fccmpe(s30, s16, NCFlag, pl), "fccmpe s30, s16, #NzCv, pl");
  COMPARE(fccmpe(s31, s31, NZCVFlag, le), "fccmpe s31, s31, #NZCV, le");
  COMPARE(fccmpe(d4, d5, VFlag, gt), "fccmpe d4, d5, #nzcV, gt");
  COMPARE(fccmpe(d6, d7, NFlag, vs), "fccmpe d6, d7, #Nzcv, vs");
  COMPARE(fccmpe(d30, d0, NZFlag, vc), "fccmpe d30, d0, #NZcv, vc");
  COMPARE(fccmpe(d31, d31, ZFlag, hs), "fccmpe d31, d31, #nZcv, hs");
  COMPARE(fccmpe(h12, h13, CVFlag, al), "fccmpe h12, h13, #nzCV, al");
  COMPARE(fccmpe(s14, s15, CVFlag, al), "fccmpe s14, s15, #nzCV, al");
  COMPARE(fccmpe(d16, d17, CFlag, nv), "fccmpe d16, d17, #nzCv, nv");

  CLEANUP();
}

TEST(fp_select) {
  SETUP();

  COMPARE(fcsel(h0, h1, h2, eq), "fcsel h0, h1, h2, eq");
  COMPARE(fcsel(h31, h31, h30, ne), "fcsel h31, h31, h30, ne");
  COMPARE(fcsel(s0, s1, s2, eq), "fcsel s0, s1, s2, eq");
  COMPARE(fcsel(s31, s31, s30, ne), "fcsel s31, s31, s30, ne");
  COMPARE(fcsel(d0, d1, d2, mi), "fcsel d0, d1, d2, mi");
  COMPARE(fcsel(d31, d30, d31, pl), "fcsel d31, d30, d31, pl");
  COMPARE(fcsel(h11, h12, h13, al), "fcsel h11, h12, h13, al");
  COMPARE(fcsel(s14, s15, s16, al), "fcsel s14, s15, s16, al");
  COMPARE(fcsel(d17, d18, d19, nv), "fcsel d17, d18, d19, nv");

  CLEANUP();
}

TEST(fcvt_scvtf_ucvtf) {
  SETUP();

  COMPARE(fcvtas(w0, s1), "fcvtas w0, s1");
  COMPARE(fcvtas(x2, s3), "fcvtas x2, s3");
  COMPARE(fcvtas(w4, d5), "fcvtas w4, d5");
  COMPARE(fcvtas(x6, d7), "fcvtas x6, d7");
  COMPARE(fcvtau(w8, s9), "fcvtau w8, s9");
  COMPARE(fcvtau(x10, s11), "fcvtau x10, s11");
  COMPARE(fcvtau(w12, d13), "fcvtau w12, d13");
  COMPARE(fcvtau(x14, d15), "fcvtau x14, d15");
  COMPARE(fcvtns(w0, s1), "fcvtns w0, s1");
  COMPARE(fcvtns(x2, s3), "fcvtns x2, s3");
  COMPARE(fcvtns(w4, d5), "fcvtns w4, d5");
  COMPARE(fcvtns(x6, d7), "fcvtns x6, d7");
  COMPARE(fcvtnu(w8, s9), "fcvtnu w8, s9");
  COMPARE(fcvtnu(x10, s11), "fcvtnu x10, s11");
  COMPARE(fcvtnu(w12, d13), "fcvtnu w12, d13");
  COMPARE(fcvtnu(x14, d15), "fcvtnu x14, d15");
  COMPARE(fcvtzu(x16, d17), "fcvtzu x16, d17");
  COMPARE(fcvtzu(w18, d19), "fcvtzu w18, d19");
  COMPARE(fcvtzs(x20, d21), "fcvtzs x20, d21");
  COMPARE(fcvtzs(w22, d23), "fcvtzs w22, d23");
  COMPARE(fcvtzu(x16, s17), "fcvtzu x16, s17");
  COMPARE(fcvtzu(w18, s19), "fcvtzu w18, s19");
  COMPARE(fcvtzs(x20, s21), "fcvtzs x20, s21");
  COMPARE(fcvtzs(w22, s23), "fcvtzs w22, s23");
  COMPARE(fcvtzs(w2, d1, 1), "fcvtzs w2, d1, #1");
  COMPARE(fcvtzs(w2, s1, 1), "fcvtzs w2, s1, #1");
  COMPARE(fcvtzs(x4, d3, 15), "fcvtzs x4, d3, #15");
  COMPARE(fcvtzs(x4, s3, 15), "fcvtzs x4, s3, #15");
  COMPARE(fcvtzs(w6, d5, 32), "fcvtzs w6, d5, #32");
  COMPARE(fcvtzs(w6, s5, 32), "fcvtzs w6, s5, #32");
  COMPARE(fjcvtzs(w0, d1), "fjcvtzs w0, d1");
  COMPARE(fcvtzu(w2, d1, 1), "fcvtzu w2, d1, #1");
  COMPARE(fcvtzu(w2, s1, 1), "fcvtzu w2, s1, #1");
  COMPARE(fcvtzu(x4, d3, 15), "fcvtzu x4, d3, #15");
  COMPARE(fcvtzu(x4, s3, 15), "fcvtzu x4, s3, #15");
  COMPARE(fcvtzu(w6, d5, 32), "fcvtzu w6, d5, #32");
  COMPARE(fcvtzu(w6, s5, 32), "fcvtzu w6, s5, #32");
  COMPARE(fcvtpu(x24, d25), "fcvtpu x24, d25");
  COMPARE(fcvtpu(w26, d27), "fcvtpu w26, d27");
  COMPARE(fcvtps(x28, d29), "fcvtps x28, d29");
  COMPARE(fcvtps(w30, d31), "fcvtps w30, d31");
  COMPARE(fcvtpu(x0, s1), "fcvtpu x0, s1");
  COMPARE(fcvtpu(w2, s3), "fcvtpu w2, s3");
  COMPARE(fcvtps(x4, s5), "fcvtps x4, s5");
  COMPARE(fcvtps(w6, s7), "fcvtps w6, s7");
  COMPARE(scvtf(d24, w25), "scvtf d24, w25");
  COMPARE(scvtf(s24, w25), "scvtf s24, w25");
  COMPARE(scvtf(d26, x0), "scvtf d26, x0");
  COMPARE(scvtf(s26, x0), "scvtf s26, x0");
  COMPARE(ucvtf(d28, w29), "ucvtf d28, w29");
  COMPARE(ucvtf(s28, w29), "ucvtf s28, w29");
  COMPARE(ucvtf(d0, x1), "ucvtf d0, x1");
  COMPARE(ucvtf(s0, x1), "ucvtf s0, x1");
  COMPARE(ucvtf(d0, x1, 0), "ucvtf d0, x1");
  COMPARE(ucvtf(s0, x1, 0), "ucvtf s0, x1");
  COMPARE(scvtf(d1, x2, 1), "scvtf d1, x2, #1");
  COMPARE(scvtf(s1, x2, 1), "scvtf s1, x2, #1");
  COMPARE(scvtf(d3, x4, 15), "scvtf d3, x4, #15");
  COMPARE(scvtf(s3, x4, 15), "scvtf s3, x4, #15");
  COMPARE(scvtf(d5, x6, 32), "scvtf d5, x6, #32");
  COMPARE(scvtf(s5, x6, 32), "scvtf s5, x6, #32");
  COMPARE(ucvtf(d7, x8, 2), "ucvtf d7, x8, #2");
  COMPARE(ucvtf(s7, x8, 2), "ucvtf s7, x8, #2");
  COMPARE(ucvtf(d9, x10, 16), "ucvtf d9, x10, #16");
  COMPARE(ucvtf(s9, x10, 16), "ucvtf s9, x10, #16");
  COMPARE(ucvtf(d11, x12, 33), "ucvtf d11, x12, #33");
  COMPARE(ucvtf(s11, x12, 33), "ucvtf s11, x12, #33");
  COMPARE(fcvtms(w0, s1), "fcvtms w0, s1");
  COMPARE(fcvtms(x2, s3), "fcvtms x2, s3");
  COMPARE(fcvtms(w4, d5), "fcvtms w4, d5");
  COMPARE(fcvtms(x6, d7), "fcvtms x6, d7");
  COMPARE(fcvtmu(w8, s9), "fcvtmu w8, s9");
  COMPARE(fcvtmu(x10, s11), "fcvtmu x10, s11");
  COMPARE(fcvtmu(w12, d13), "fcvtmu w12, d13");
  COMPARE(fcvtmu(x14, d15), "fcvtmu x14, d15");

  COMPARE(fcvtas(w0, h1), "fcvtas w0, h1");
  COMPARE(fcvtas(x2, h3), "fcvtas x2, h3");
  COMPARE(fcvtau(w8, h9), "fcvtau w8, h9");
  COMPARE(fcvtau(x10, h11), "fcvtau x10, h11");
  COMPARE(fcvtns(w0, h1), "fcvtns w0, h1");
  COMPARE(fcvtns(x2, h3), "fcvtns x2, h3");
  COMPARE(fcvtnu(w8, h9), "fcvtnu w8, h9");
  COMPARE(fcvtnu(x10, h11), "fcvtnu x10, h11");
  COMPARE(fcvtzu(x16, h17), "fcvtzu x16, h17");
  COMPARE(fcvtzu(w18, h19), "fcvtzu w18, h19");
  COMPARE(fcvtzs(x20, h21), "fcvtzs x20, h21");
  COMPARE(fcvtzs(w22, h23), "fcvtzs w22, h23");
  COMPARE(fcvtzs(w2, h1, 1), "fcvtzs w2, h1, #1");
  COMPARE(fcvtzs(x4, h3, 15), "fcvtzs x4, h3, #15");
  COMPARE(fcvtzs(w6, h5, 32), "fcvtzs w6, h5, #32");
  COMPARE(fcvtzu(w2, h1, 1), "fcvtzu w2, h1, #1");
  COMPARE(fcvtzu(x4, h3, 15), "fcvtzu x4, h3, #15");
  COMPARE(fcvtzu(w6, h5, 32), "fcvtzu w6, h5, #32");
  COMPARE(fcvtpu(x0, h1), "fcvtpu x0, h1");
  COMPARE(fcvtpu(w2, h3), "fcvtpu w2, h3");
  COMPARE(fcvtps(x4, h5), "fcvtps x4, h5");
  COMPARE(fcvtps(w6, h7), "fcvtps w6, h7");
  COMPARE(scvtf(h24, w25), "scvtf h24, w25");
  COMPARE(scvtf(h26, x0), "scvtf h26, x0");
  COMPARE(ucvtf(h28, w29), "ucvtf h28, w29");
  COMPARE(ucvtf(h0, x1), "ucvtf h0, x1");
  COMPARE(ucvtf(h0, x1, 0), "ucvtf h0, x1");
  COMPARE(scvtf(h1, x2, 1), "scvtf h1, x2, #1");
  COMPARE(scvtf(h3, x4, 15), "scvtf h3, x4, #15");
  COMPARE(scvtf(h5, x6, 32), "scvtf h5, x6, #32");
  COMPARE(ucvtf(h7, x8, 2), "ucvtf h7, x8, #2");
  COMPARE(ucvtf(h9, x10, 16), "ucvtf h9, x10, #16");
  COMPARE(ucvtf(h11, x12, 33), "ucvtf h11, x12, #33");
  COMPARE(fcvtms(w0, h1), "fcvtms w0, h1");
  COMPARE(fcvtms(x2, h3), "fcvtms x2, h3");
  COMPARE(fcvtmu(w8, h9), "fcvtmu w8, h9");
  COMPARE(fcvtmu(x10, h11), "fcvtmu x10, h11");

  CLEANUP();
}

}  // namespace aarch64
}  // namespace vixl
