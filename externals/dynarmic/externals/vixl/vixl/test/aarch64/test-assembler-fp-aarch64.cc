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

#include <sys/mman.h>

#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "test-runner.h"
#include "test-utils.h"
#include "aarch64/test-utils-aarch64.h"

#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/macro-assembler-aarch64.h"
#include "aarch64/simulator-aarch64.h"
#include "test-assembler-aarch64.h"

namespace vixl {
namespace aarch64 {

TEST(load_store_float) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  float src[3] = {1.0, 2.0, 3.0};
  float dst[3] = {0.0, 0.0, 0.0};
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, dst_base);
  __ Mov(x19, src_base);
  __ Mov(x20, dst_base);
  __ Mov(x21, src_base);
  __ Mov(x22, dst_base);
  __ Ldr(s0, MemOperand(x17, sizeof(src[0])));
  __ Str(s0, MemOperand(x18, sizeof(dst[0]), PostIndex));
  __ Ldr(s1, MemOperand(x19, sizeof(src[0]), PostIndex));
  __ Str(s1, MemOperand(x20, 2 * sizeof(dst[0]), PreIndex));
  __ Ldr(s2, MemOperand(x21, 2 * sizeof(src[0]), PreIndex));
  __ Str(s2, MemOperand(x22, sizeof(dst[0])));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(2.0, s0);
    ASSERT_EQUAL_FP32(2.0, dst[0]);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(1.0, dst[2]);
    ASSERT_EQUAL_FP32(3.0, s2);
    ASSERT_EQUAL_FP32(3.0, dst[1]);
    ASSERT_EQUAL_64(src_base, x17);
    ASSERT_EQUAL_64(dst_base + sizeof(dst[0]), x18);
    ASSERT_EQUAL_64(src_base + sizeof(src[0]), x19);
    ASSERT_EQUAL_64(dst_base + 2 * sizeof(dst[0]), x20);
    ASSERT_EQUAL_64(src_base + 2 * sizeof(src[0]), x21);
    ASSERT_EQUAL_64(dst_base, x22);
  }
}


TEST(load_store_double) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  double src[3] = {1.0, 2.0, 3.0};
  double dst[3] = {0.0, 0.0, 0.0};
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, dst_base);
  __ Mov(x19, src_base);
  __ Mov(x20, dst_base);
  __ Mov(x21, src_base);
  __ Mov(x22, dst_base);
  __ Ldr(d0, MemOperand(x17, sizeof(src[0])));
  __ Str(d0, MemOperand(x18, sizeof(dst[0]), PostIndex));
  __ Ldr(d1, MemOperand(x19, sizeof(src[0]), PostIndex));
  __ Str(d1, MemOperand(x20, 2 * sizeof(dst[0]), PreIndex));
  __ Ldr(d2, MemOperand(x21, 2 * sizeof(src[0]), PreIndex));
  __ Str(d2, MemOperand(x22, sizeof(dst[0])));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP64(2.0, d0);
    ASSERT_EQUAL_FP64(2.0, dst[0]);
    ASSERT_EQUAL_FP64(1.0, d1);
    ASSERT_EQUAL_FP64(1.0, dst[2]);
    ASSERT_EQUAL_FP64(3.0, d2);
    ASSERT_EQUAL_FP64(3.0, dst[1]);
    ASSERT_EQUAL_64(src_base, x17);
    ASSERT_EQUAL_64(dst_base + sizeof(dst[0]), x18);
    ASSERT_EQUAL_64(src_base + sizeof(src[0]), x19);
    ASSERT_EQUAL_64(dst_base + 2 * sizeof(dst[0]), x20);
    ASSERT_EQUAL_64(src_base + 2 * sizeof(src[0]), x21);
    ASSERT_EQUAL_64(dst_base, x22);
  }
}

TEST(ldp_stp_float) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  float src[2] = {1.0, 2.0};
  float dst[3] = {0.0, 0.0, 0.0};
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x16, src_base);
  __ Mov(x17, dst_base);
  __ Ldp(s31, s0, MemOperand(x16, 2 * sizeof(src[0]), PostIndex));
  __ Stp(s0, s31, MemOperand(x17, sizeof(dst[1]), PreIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s31);
    ASSERT_EQUAL_FP32(2.0, s0);
    ASSERT_EQUAL_FP32(0.0, dst[0]);
    ASSERT_EQUAL_FP32(2.0, dst[1]);
    ASSERT_EQUAL_FP32(1.0, dst[2]);
    ASSERT_EQUAL_64(src_base + 2 * sizeof(src[0]), x16);
    ASSERT_EQUAL_64(dst_base + sizeof(dst[1]), x17);
  }
}


TEST(ldp_stp_double) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  double src[2] = {1.0, 2.0};
  double dst[3] = {0.0, 0.0, 0.0};
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x16, src_base);
  __ Mov(x17, dst_base);
  __ Ldp(d31, d0, MemOperand(x16, 2 * sizeof(src[0]), PostIndex));
  __ Stp(d0, d31, MemOperand(x17, sizeof(dst[1]), PreIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP64(1.0, d31);
    ASSERT_EQUAL_FP64(2.0, d0);
    ASSERT_EQUAL_FP64(0.0, dst[0]);
    ASSERT_EQUAL_FP64(2.0, dst[1]);
    ASSERT_EQUAL_FP64(1.0, dst[2]);
    ASSERT_EQUAL_64(src_base + 2 * sizeof(src[0]), x16);
    ASSERT_EQUAL_64(dst_base + sizeof(dst[1]), x17);
  }
}

TEST(ldnp_stnp_offset_float) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  float src[3] = {1.2, 2.3, 3.4};
  float dst[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x16, src_base);
  __ Mov(x17, dst_base);
  __ Mov(x18, src_base + 12);
  __ Mov(x19, dst_base + 24);

  // Ensure address set up has happened before executing non-temporal ops.
  __ Dmb(InnerShareable, BarrierAll);

  __ Ldnp(s0, s1, MemOperand(x16));
  __ Ldnp(s2, s3, MemOperand(x16, 4));
  __ Ldnp(s5, s4, MemOperand(x18, -8));
  __ Stnp(s1, s0, MemOperand(x17));
  __ Stnp(s3, s2, MemOperand(x17, 8));
  __ Stnp(s4, s5, MemOperand(x19, -8));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.2, s0);
    ASSERT_EQUAL_FP32(2.3, s1);
    ASSERT_EQUAL_FP32(2.3, dst[0]);
    ASSERT_EQUAL_FP32(1.2, dst[1]);
    ASSERT_EQUAL_FP32(2.3, s2);
    ASSERT_EQUAL_FP32(3.4, s3);
    ASSERT_EQUAL_FP32(3.4, dst[2]);
    ASSERT_EQUAL_FP32(2.3, dst[3]);
    ASSERT_EQUAL_FP32(3.4, s4);
    ASSERT_EQUAL_FP32(2.3, s5);
    ASSERT_EQUAL_FP32(3.4, dst[4]);
    ASSERT_EQUAL_FP32(2.3, dst[5]);
    ASSERT_EQUAL_64(src_base, x16);
    ASSERT_EQUAL_64(dst_base, x17);
    ASSERT_EQUAL_64(src_base + 12, x18);
    ASSERT_EQUAL_64(dst_base + 24, x19);
  }
}


TEST(ldnp_stnp_offset_double) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  double src[3] = {1.2, 2.3, 3.4};
  double dst[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x16, src_base);
  __ Mov(x17, dst_base);
  __ Mov(x18, src_base + 24);
  __ Mov(x19, dst_base + 48);

  // Ensure address set up has happened before executing non-temporal ops.
  __ Dmb(InnerShareable, BarrierAll);

  __ Ldnp(d0, d1, MemOperand(x16));
  __ Ldnp(d2, d3, MemOperand(x16, 8));
  __ Ldnp(d5, d4, MemOperand(x18, -16));
  __ Stnp(d1, d0, MemOperand(x17));
  __ Stnp(d3, d2, MemOperand(x17, 16));
  __ Stnp(d4, d5, MemOperand(x19, -16));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP64(1.2, d0);
    ASSERT_EQUAL_FP64(2.3, d1);
    ASSERT_EQUAL_FP64(2.3, dst[0]);
    ASSERT_EQUAL_FP64(1.2, dst[1]);
    ASSERT_EQUAL_FP64(2.3, d2);
    ASSERT_EQUAL_FP64(3.4, d3);
    ASSERT_EQUAL_FP64(3.4, dst[2]);
    ASSERT_EQUAL_FP64(2.3, dst[3]);
    ASSERT_EQUAL_FP64(3.4, d4);
    ASSERT_EQUAL_FP64(2.3, d5);
    ASSERT_EQUAL_FP64(3.4, dst[4]);
    ASSERT_EQUAL_FP64(2.3, dst[5]);
    ASSERT_EQUAL_64(src_base, x16);
    ASSERT_EQUAL_64(dst_base, x17);
    ASSERT_EQUAL_64(src_base + 24, x18);
    ASSERT_EQUAL_64(dst_base + 48, x19);
  }
}

template <typename T>
void LoadFPValueHelper(T values[], int card) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  const bool is_32bits = (sizeof(T) == 4);
  const VRegister& fp_tgt = is_32bits ? VRegister(s2) : VRegister(d2);
  const Register& tgt1 = is_32bits ? Register(w1) : Register(x1);
  const Register& tgt2 = is_32bits ? Register(w2) : Register(x2);

  START();
  __ Mov(x0, 0);

  // If one of the values differ then x0 will be one.
  for (int i = 0; i < card; ++i) {
    __ Mov(tgt1,
           is_32bits ? FloatToRawbits(values[i]) : DoubleToRawbits(values[i]));
    __ Ldr(fp_tgt, values[i]);
    __ Fmov(tgt2, fp_tgt);
    __ Cmp(tgt1, tgt2);
    __ Cset(x0, ne);
  }
  END();

  if (CAN_RUN()) {
    RUN();

    // If one of the values differs, the trace can be used to identify which
    // one.
    ASSERT_EQUAL_64(0, x0);
  }
}

TEST(ldr_literal_values_d) {
  static const double kValues[] = {-0.0, 0.0, -1.0, 1.0, -1e10, 1e10};

  LoadFPValueHelper(kValues, sizeof(kValues) / sizeof(kValues[0]));
}


TEST(ldr_literal_values_s) {
  static const float kValues[] = {-0.0, 0.0, -1.0, 1.0, -1e10, 1e10};

  LoadFPValueHelper(kValues, sizeof(kValues) / sizeof(kValues[0]));
}

TEST(fmov_imm) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFPHalf);

  START();
  __ Fmov(s1, 255.0);
  __ Fmov(d2, 12.34567);
  __ Fmov(s3, 0.0);
  __ Fmov(d4, 0.0);
  __ Fmov(s5, kFP32PositiveInfinity);
  __ Fmov(d6, kFP64NegativeInfinity);
  __ Fmov(h7, RawbitsToFloat16(0x6400U));
  __ Fmov(h8, kFP16PositiveInfinity);
  __ Fmov(s11, 1.0);
  __ Fmov(h12, RawbitsToFloat16(0x7BFF));
  __ Fmov(h13, RawbitsToFloat16(0x57F2));
  __ Fmov(d22, -13.0);
  __ Fmov(h23, RawbitsToFloat16(0xC500U));
  __ Fmov(h24, Float16(-5.0));
  __ Fmov(h25, Float16(2049.0));
  __ Fmov(h21, RawbitsToFloat16(0x6404U));
  __ Fmov(h26, RawbitsToFloat16(0x0U));
  __ Fmov(h27, RawbitsToFloat16(0x7e00U));
  END();
  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(255.0, s1);
    ASSERT_EQUAL_FP64(12.34567, d2);
    ASSERT_EQUAL_FP32(0.0, s3);
    ASSERT_EQUAL_FP64(0.0, d4);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s5);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d6);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x6400U), h7);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h8);
    ASSERT_EQUAL_FP32(1.0, s11);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x7BFF), h12);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x57F2U), h13);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x6404), h21);
    ASSERT_EQUAL_FP64(-13.0, d22);
    ASSERT_EQUAL_FP16(Float16(-5.0), h23);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xC500), h24);
    // 2049 is unpresentable.
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x6800), h25);
    ASSERT_EQUAL_FP16(kFP16PositiveZero, h26);
    // NaN check.
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x7e00), h27);
  }
}

TEST(fmov_reg) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kFPHalf);

  START();

  __ Fmov(h3, RawbitsToFloat16(0xCA80U));
  __ Fmov(h7, h3);
  __ Fmov(h8, -5.0);
  __ Fmov(w3, h8);
  __ Fmov(h9, w3);
  __ Fmov(h8, Float16(1024.0));
  __ Fmov(x4, h8);
  __ Fmov(h10, x4);
  __ Fmov(s20, 1.0);
  __ Fmov(w10, s20);
  __ Fmov(s30, w10);
  __ Fmov(s5, s20);
  __ Fmov(d1, -13.0);
  __ Fmov(x1, d1);
  __ Fmov(d2, x1);
  __ Fmov(d4, d1);
  __ Fmov(d6, RawbitsToDouble(0x0123456789abcdef));
  __ Fmov(s6, s6);
  __ Fmov(d0, 0.0);
  __ Fmov(v0.D(), 1, x1);
  __ Fmov(x2, v0.D(), 1);
  __ Fmov(v3.D(), 1, x4);
  __ Fmov(v3.D(), 0, x1);
  __ Fmov(x5, v1.D(), 0);

  END();
  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xCA80U), h7);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xC500U), h9);
    ASSERT_EQUAL_32(0x0000C500, w3);
    ASSERT_EQUAL_64(0x0000000000006400, x4);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x6400), h10);
    ASSERT_EQUAL_32(FloatToRawbits(1.0), w10);
    ASSERT_EQUAL_FP32(1.0, s30);
    ASSERT_EQUAL_FP32(1.0, s5);
    ASSERT_EQUAL_64(DoubleToRawbits(-13.0), x1);
    ASSERT_EQUAL_FP64(-13.0, d2);
    ASSERT_EQUAL_FP64(-13.0, d4);
    ASSERT_EQUAL_FP32(RawbitsToFloat(0x89abcdef), s6);
    ASSERT_EQUAL_128(DoubleToRawbits(-13.0), 0x0000000000000000, q0);
    ASSERT_EQUAL_64(DoubleToRawbits(-13.0), x2);
    ASSERT_EQUAL_128(0x0000000000006400, DoubleToRawbits(-13.0), q3);
    ASSERT_EQUAL_64(DoubleToRawbits(-13.0), x5);
  }
}


TEST(fadd) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s14, -0.0f);
  __ Fmov(s15, kFP32PositiveInfinity);
  __ Fmov(s16, kFP32NegativeInfinity);
  __ Fmov(s17, 3.25f);
  __ Fmov(s18, 1.0f);
  __ Fmov(s19, 0.0f);

  __ Fmov(d26, -0.0);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0.0);
  __ Fmov(d30, -2.0);
  __ Fmov(d31, 2.25);

  __ Fadd(s0, s17, s18);
  __ Fadd(s1, s18, s19);
  __ Fadd(s2, s14, s18);
  __ Fadd(s3, s15, s18);
  __ Fadd(s4, s16, s18);
  __ Fadd(s5, s15, s16);
  __ Fadd(s6, s16, s15);

  __ Fadd(d7, d30, d31);
  __ Fadd(d8, d29, d31);
  __ Fadd(d9, d26, d31);
  __ Fadd(d10, d27, d31);
  __ Fadd(d11, d28, d31);
  __ Fadd(d12, d27, d28);
  __ Fadd(d13, d28, d27);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(4.25, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(1.0, s2);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s3);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s4);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s5);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s6);
    ASSERT_EQUAL_FP64(0.25, d7);
    ASSERT_EQUAL_FP64(2.25, d8);
    ASSERT_EQUAL_FP64(2.25, d9);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d10);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d11);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d12);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d13);
  }
}


TEST(fadd_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFPHalf);

  START();
  __ Fmov(h14, -0.0f);
  __ Fmov(h15, kFP16PositiveInfinity);
  __ Fmov(h16, kFP16NegativeInfinity);
  __ Fmov(h17, 3.25f);
  __ Fmov(h18, 1.0);
  __ Fmov(h19, 0.0f);
  __ Fmov(h20, 5.0f);

  __ Fadd(h0, h17, h18);
  __ Fadd(h1, h18, h19);
  __ Fadd(h2, h14, h18);
  __ Fadd(h3, h15, h18);
  __ Fadd(h4, h16, h18);
  __ Fadd(h5, h15, h16);
  __ Fadd(h6, h16, h15);
  __ Fadd(h7, h20, h20);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(4.25), h0);
    ASSERT_EQUAL_FP16(Float16(1.0), h1);
    ASSERT_EQUAL_FP16(Float16(1.0), h2);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h3);
    ASSERT_EQUAL_FP16(kFP16NegativeInfinity, h4);
    ASSERT_EQUAL_FP16(kFP16DefaultNaN, h5);
    ASSERT_EQUAL_FP16(kFP16DefaultNaN, h6);
    ASSERT_EQUAL_FP16(Float16(10.0), h7);
  }
}

TEST(fsub) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s14, -0.0f);
  __ Fmov(s15, kFP32PositiveInfinity);
  __ Fmov(s16, kFP32NegativeInfinity);
  __ Fmov(s17, 3.25f);
  __ Fmov(s18, 1.0f);
  __ Fmov(s19, 0.0f);

  __ Fmov(d26, -0.0);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0.0);
  __ Fmov(d30, -2.0);
  __ Fmov(d31, 2.25);

  __ Fsub(s0, s17, s18);
  __ Fsub(s1, s18, s19);
  __ Fsub(s2, s14, s18);
  __ Fsub(s3, s18, s15);
  __ Fsub(s4, s18, s16);
  __ Fsub(s5, s15, s15);
  __ Fsub(s6, s16, s16);

  __ Fsub(d7, d30, d31);
  __ Fsub(d8, d29, d31);
  __ Fsub(d9, d26, d31);
  __ Fsub(d10, d31, d27);
  __ Fsub(d11, d31, d28);
  __ Fsub(d12, d27, d27);
  __ Fsub(d13, d28, d28);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(2.25, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(-1.0, s2);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s3);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s4);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s5);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s6);
    ASSERT_EQUAL_FP64(-4.25, d7);
    ASSERT_EQUAL_FP64(-2.25, d8);
    ASSERT_EQUAL_FP64(-2.25, d9);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d10);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d11);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d12);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d13);
  }
}


TEST(fsub_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFPHalf);

  START();
  __ Fmov(h14, -0.0f);
  __ Fmov(h15, kFP16PositiveInfinity);
  __ Fmov(h16, kFP16NegativeInfinity);
  __ Fmov(h17, 3.25f);
  __ Fmov(h18, 1.0f);
  __ Fmov(h19, 0.0f);

  __ Fsub(h0, h17, h18);
  __ Fsub(h1, h18, h19);
  __ Fsub(h2, h14, h18);
  __ Fsub(h3, h18, h15);
  __ Fsub(h4, h18, h16);
  __ Fsub(h5, h15, h15);
  __ Fsub(h6, h16, h16);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(2.25), h0);
    ASSERT_EQUAL_FP16(Float16(1.0), h1);
    ASSERT_EQUAL_FP16(Float16(-1.0), h2);
    ASSERT_EQUAL_FP16(kFP16NegativeInfinity, h3);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h4);
    ASSERT_EQUAL_FP16(kFP16DefaultNaN, h5);
    ASSERT_EQUAL_FP16(kFP16DefaultNaN, h6);
  }
}


TEST(fmul) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s14, -0.0f);
  __ Fmov(s15, kFP32PositiveInfinity);
  __ Fmov(s16, kFP32NegativeInfinity);
  __ Fmov(s17, 3.25f);
  __ Fmov(s18, 2.0f);
  __ Fmov(s19, 0.0f);
  __ Fmov(s20, -2.0f);

  __ Fmov(d26, -0.0);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0.0);
  __ Fmov(d30, -2.0);
  __ Fmov(d31, 2.25);

  __ Fmul(s0, s17, s18);
  __ Fmul(s1, s18, s19);
  __ Fmul(s2, s14, s14);
  __ Fmul(s3, s15, s20);
  __ Fmul(s4, s16, s20);
  __ Fmul(s5, s15, s19);
  __ Fmul(s6, s19, s16);

  __ Fmul(d7, d30, d31);
  __ Fmul(d8, d29, d31);
  __ Fmul(d9, d26, d26);
  __ Fmul(d10, d27, d30);
  __ Fmul(d11, d28, d30);
  __ Fmul(d12, d27, d29);
  __ Fmul(d13, d29, d28);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(6.5, s0);
    ASSERT_EQUAL_FP32(0.0, s1);
    ASSERT_EQUAL_FP32(0.0, s2);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s3);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s4);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s5);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s6);
    ASSERT_EQUAL_FP64(-4.5, d7);
    ASSERT_EQUAL_FP64(0.0, d8);
    ASSERT_EQUAL_FP64(0.0, d9);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d10);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d11);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d12);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d13);
  }
}


TEST(fmul_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFPHalf);

  START();
  __ Fmov(h14, -0.0f);
  __ Fmov(h15, kFP16PositiveInfinity);
  __ Fmov(h16, kFP16NegativeInfinity);
  __ Fmov(h17, 3.25f);
  __ Fmov(h18, 2.0f);
  __ Fmov(h19, 0.0f);
  __ Fmov(h20, -2.0f);

  __ Fmul(h0, h17, h18);
  __ Fmul(h1, h18, h19);
  __ Fmul(h2, h14, h14);
  __ Fmul(h3, h15, h20);
  __ Fmul(h4, h16, h20);
  __ Fmul(h5, h15, h19);
  __ Fmul(h6, h19, h16);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(6.5), h0);
    ASSERT_EQUAL_FP16(Float16(0.0), h1);
    ASSERT_EQUAL_FP16(Float16(0.0), h2);
    ASSERT_EQUAL_FP16(kFP16NegativeInfinity, h3);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h4);
    ASSERT_EQUAL_FP16(kFP16DefaultNaN, h5);
    ASSERT_EQUAL_FP16(kFP16DefaultNaN, h6);
  }
}


TEST(fnmul_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFPHalf);

  START();
  __ Fmov(h14, -0.0f);
  __ Fmov(h15, kFP16PositiveInfinity);
  __ Fmov(h16, kFP16NegativeInfinity);
  __ Fmov(h17, 3.25f);
  __ Fmov(h18, 2.0f);
  __ Fmov(h19, 0.0f);
  __ Fmov(h20, -2.0f);

  __ Fnmul(h0, h17, h18);
  __ Fnmul(h1, h18, h19);
  __ Fnmul(h2, h14, h14);
  __ Fnmul(h3, h15, h20);
  __ Fnmul(h4, h16, h20);
  __ Fnmul(h5, h15, h19);
  __ Fnmul(h6, h19, h16);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(-6.5), h0);
    ASSERT_EQUAL_FP16(Float16(-0.0), h1);
    ASSERT_EQUAL_FP16(Float16(-0.0), h2);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h3);
    ASSERT_EQUAL_FP16(kFP16NegativeInfinity, h4);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xfe00), h5);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xfe00), h6);
  }
}


static void FmaddFmsubHelper(double n,
                             double m,
                             double a,
                             double fmadd,
                             double fmsub,
                             double fnmadd,
                             double fnmsub) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();

  __ Fmov(d0, n);
  __ Fmov(d1, m);
  __ Fmov(d2, a);
  __ Fmadd(d28, d0, d1, d2);
  __ Fmsub(d29, d0, d1, d2);
  __ Fnmadd(d30, d0, d1, d2);
  __ Fnmsub(d31, d0, d1, d2);

  END();
  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP64(fmadd, d28);
    ASSERT_EQUAL_FP64(fmsub, d29);
    ASSERT_EQUAL_FP64(fnmadd, d30);
    ASSERT_EQUAL_FP64(fnmsub, d31);
  }
}


TEST(fmadd_fmsub_double) {
  // It's hard to check the result of fused operations because the only way to
  // calculate the result is using fma, which is what the Simulator uses anyway.

  // Basic operation.
  FmaddFmsubHelper(1.0, 2.0, 3.0, 5.0, 1.0, -5.0, -1.0);
  FmaddFmsubHelper(-1.0, 2.0, 3.0, 1.0, 5.0, -1.0, -5.0);

  // Check the sign of exact zeroes.
  //               n     m     a     fmadd  fmsub  fnmadd fnmsub
  FmaddFmsubHelper(-0.0, +0.0, -0.0, -0.0, +0.0, +0.0, +0.0);
  FmaddFmsubHelper(+0.0, +0.0, -0.0, +0.0, -0.0, +0.0, +0.0);
  FmaddFmsubHelper(+0.0, +0.0, +0.0, +0.0, +0.0, -0.0, +0.0);
  FmaddFmsubHelper(-0.0, +0.0, +0.0, +0.0, +0.0, +0.0, -0.0);
  FmaddFmsubHelper(+0.0, -0.0, -0.0, -0.0, +0.0, +0.0, +0.0);
  FmaddFmsubHelper(-0.0, -0.0, -0.0, +0.0, -0.0, +0.0, +0.0);
  FmaddFmsubHelper(-0.0, -0.0, +0.0, +0.0, +0.0, -0.0, +0.0);
  FmaddFmsubHelper(+0.0, -0.0, +0.0, +0.0, +0.0, +0.0, -0.0);

  // Check NaN generation.
  FmaddFmsubHelper(kFP64PositiveInfinity,
                   0.0,
                   42.0,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN);
  FmaddFmsubHelper(0.0,
                   kFP64PositiveInfinity,
                   42.0,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN);
  FmaddFmsubHelper(kFP64PositiveInfinity,
                   1.0,
                   kFP64PositiveInfinity,
                   kFP64PositiveInfinity,  //  inf + ( inf * 1) = inf
                   kFP64DefaultNaN,        //  inf + (-inf * 1) = NaN
                   kFP64NegativeInfinity,  // -inf + (-inf * 1) = -inf
                   kFP64DefaultNaN);       // -inf + ( inf * 1) = NaN
  FmaddFmsubHelper(kFP64NegativeInfinity,
                   1.0,
                   kFP64PositiveInfinity,
                   kFP64DefaultNaN,         //  inf + (-inf * 1) = NaN
                   kFP64PositiveInfinity,   //  inf + ( inf * 1) = inf
                   kFP64DefaultNaN,         // -inf + ( inf * 1) = NaN
                   kFP64NegativeInfinity);  // -inf + (-inf * 1) = -inf
}


static void FmaddFmsubHelper(float n,
                             float m,
                             float a,
                             float fmadd,
                             float fmsub,
                             float fnmadd,
                             float fnmsub) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();

  __ Fmov(s0, n);
  __ Fmov(s1, m);
  __ Fmov(s2, a);
  __ Fmadd(s28, s0, s1, s2);
  __ Fmsub(s29, s0, s1, s2);
  __ Fnmadd(s30, s0, s1, s2);
  __ Fnmsub(s31, s0, s1, s2);

  END();
  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(fmadd, s28);
    ASSERT_EQUAL_FP32(fmsub, s29);
    ASSERT_EQUAL_FP32(fnmadd, s30);
    ASSERT_EQUAL_FP32(fnmsub, s31);
  }
}


TEST(fmadd_fmsub_float) {
  // It's hard to check the result of fused operations because the only way to
  // calculate the result is using fma, which is what the simulator uses anyway.

  // Basic operation.
  FmaddFmsubHelper(1.0f, 2.0f, 3.0f, 5.0f, 1.0f, -5.0f, -1.0f);
  FmaddFmsubHelper(-1.0f, 2.0f, 3.0f, 1.0f, 5.0f, -1.0f, -5.0f);

  // Check the sign of exact zeroes.
  //               n      m      a      fmadd  fmsub  fnmadd fnmsub
  FmaddFmsubHelper(-0.0f, +0.0f, -0.0f, -0.0f, +0.0f, +0.0f, +0.0f);
  FmaddFmsubHelper(+0.0f, +0.0f, -0.0f, +0.0f, -0.0f, +0.0f, +0.0f);
  FmaddFmsubHelper(+0.0f, +0.0f, +0.0f, +0.0f, +0.0f, -0.0f, +0.0f);
  FmaddFmsubHelper(-0.0f, +0.0f, +0.0f, +0.0f, +0.0f, +0.0f, -0.0f);
  FmaddFmsubHelper(+0.0f, -0.0f, -0.0f, -0.0f, +0.0f, +0.0f, +0.0f);
  FmaddFmsubHelper(-0.0f, -0.0f, -0.0f, +0.0f, -0.0f, +0.0f, +0.0f);
  FmaddFmsubHelper(-0.0f, -0.0f, +0.0f, +0.0f, +0.0f, -0.0f, +0.0f);
  FmaddFmsubHelper(+0.0f, -0.0f, +0.0f, +0.0f, +0.0f, +0.0f, -0.0f);

  // Check NaN generation.
  FmaddFmsubHelper(kFP32PositiveInfinity,
                   0.0f,
                   42.0f,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN);
  FmaddFmsubHelper(0.0f,
                   kFP32PositiveInfinity,
                   42.0f,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN);
  FmaddFmsubHelper(kFP32PositiveInfinity,
                   1.0f,
                   kFP32PositiveInfinity,
                   kFP32PositiveInfinity,  //  inf + ( inf * 1) = inf
                   kFP32DefaultNaN,        //  inf + (-inf * 1) = NaN
                   kFP32NegativeInfinity,  // -inf + (-inf * 1) = -inf
                   kFP32DefaultNaN);       // -inf + ( inf * 1) = NaN
  FmaddFmsubHelper(kFP32NegativeInfinity,
                   1.0f,
                   kFP32PositiveInfinity,
                   kFP32DefaultNaN,         //  inf + (-inf * 1) = NaN
                   kFP32PositiveInfinity,   //  inf + ( inf * 1) = inf
                   kFP32DefaultNaN,         // -inf + ( inf * 1) = NaN
                   kFP32NegativeInfinity);  // -inf + (-inf * 1) = -inf
}


TEST(fmadd_fmsub_double_nans) {
  // Make sure that NaN propagation works correctly.
  double s1 = RawbitsToDouble(0x7ff5555511111111);
  double s2 = RawbitsToDouble(0x7ff5555522222222);
  double sa = RawbitsToDouble(0x7ff55555aaaaaaaa);
  double q1 = RawbitsToDouble(0x7ffaaaaa11111111);
  double q2 = RawbitsToDouble(0x7ffaaaaa22222222);
  double qa = RawbitsToDouble(0x7ffaaaaaaaaaaaaa);
  VIXL_ASSERT(IsSignallingNaN(s1));
  VIXL_ASSERT(IsSignallingNaN(s2));
  VIXL_ASSERT(IsSignallingNaN(sa));
  VIXL_ASSERT(IsQuietNaN(q1));
  VIXL_ASSERT(IsQuietNaN(q2));
  VIXL_ASSERT(IsQuietNaN(qa));

  // The input NaNs after passing through ProcessNaN.
  double s1_proc = RawbitsToDouble(0x7ffd555511111111);
  double s2_proc = RawbitsToDouble(0x7ffd555522222222);
  double sa_proc = RawbitsToDouble(0x7ffd5555aaaaaaaa);
  double q1_proc = q1;
  double q2_proc = q2;
  double qa_proc = qa;
  VIXL_ASSERT(IsQuietNaN(s1_proc));
  VIXL_ASSERT(IsQuietNaN(s2_proc));
  VIXL_ASSERT(IsQuietNaN(sa_proc));
  VIXL_ASSERT(IsQuietNaN(q1_proc));
  VIXL_ASSERT(IsQuietNaN(q2_proc));
  VIXL_ASSERT(IsQuietNaN(qa_proc));

  // Negated NaNs as it would be done on ARMv8 hardware.
  double s1_proc_neg = RawbitsToDouble(0xfffd555511111111);
  double sa_proc_neg = RawbitsToDouble(0xfffd5555aaaaaaaa);
  double q1_proc_neg = RawbitsToDouble(0xfffaaaaa11111111);
  double qa_proc_neg = RawbitsToDouble(0xfffaaaaaaaaaaaaa);
  VIXL_ASSERT(IsQuietNaN(s1_proc_neg));
  VIXL_ASSERT(IsQuietNaN(sa_proc_neg));
  VIXL_ASSERT(IsQuietNaN(q1_proc_neg));
  VIXL_ASSERT(IsQuietNaN(qa_proc_neg));

  // Quiet NaNs are propagated.
  FmaddFmsubHelper(q1, 0, 0, q1_proc, q1_proc_neg, q1_proc_neg, q1_proc);
  FmaddFmsubHelper(0, q2, 0, q2_proc, q2_proc, q2_proc, q2_proc);
  FmaddFmsubHelper(0, 0, qa, qa_proc, qa_proc, qa_proc_neg, qa_proc_neg);
  FmaddFmsubHelper(q1, q2, 0, q1_proc, q1_proc_neg, q1_proc_neg, q1_proc);
  FmaddFmsubHelper(0, q2, qa, qa_proc, qa_proc, qa_proc_neg, qa_proc_neg);
  FmaddFmsubHelper(q1, 0, qa, qa_proc, qa_proc, qa_proc_neg, qa_proc_neg);
  FmaddFmsubHelper(q1, q2, qa, qa_proc, qa_proc, qa_proc_neg, qa_proc_neg);

  // Signalling NaNs are propagated, and made quiet.
  FmaddFmsubHelper(s1, 0, 0, s1_proc, s1_proc_neg, s1_proc_neg, s1_proc);
  FmaddFmsubHelper(0, s2, 0, s2_proc, s2_proc, s2_proc, s2_proc);
  FmaddFmsubHelper(0, 0, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, s2, 0, s1_proc, s1_proc_neg, s1_proc_neg, s1_proc);
  FmaddFmsubHelper(0, s2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, 0, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, s2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);

  // Signalling NaNs take precedence over quiet NaNs.
  FmaddFmsubHelper(s1, q2, qa, s1_proc, s1_proc_neg, s1_proc_neg, s1_proc);
  FmaddFmsubHelper(q1, s2, qa, s2_proc, s2_proc, s2_proc, s2_proc);
  FmaddFmsubHelper(q1, q2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, s2, qa, s1_proc, s1_proc_neg, s1_proc_neg, s1_proc);
  FmaddFmsubHelper(q1, s2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, q2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, s2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);

  // A NaN generated by the intermediate op1 * op2 overrides a quiet NaN in a.
  FmaddFmsubHelper(0,
                   kFP64PositiveInfinity,
                   qa,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN);
  FmaddFmsubHelper(kFP64PositiveInfinity,
                   0,
                   qa,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN);
  FmaddFmsubHelper(0,
                   kFP64NegativeInfinity,
                   qa,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN);
  FmaddFmsubHelper(kFP64NegativeInfinity,
                   0,
                   qa,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN,
                   kFP64DefaultNaN);
}


TEST(fmadd_fmsub_float_nans) {
  // Make sure that NaN propagation works correctly.
  float s1 = RawbitsToFloat(0x7f951111);
  float s2 = RawbitsToFloat(0x7f952222);
  float sa = RawbitsToFloat(0x7f95aaaa);
  float q1 = RawbitsToFloat(0x7fea1111);
  float q2 = RawbitsToFloat(0x7fea2222);
  float qa = RawbitsToFloat(0x7feaaaaa);
  VIXL_ASSERT(IsSignallingNaN(s1));
  VIXL_ASSERT(IsSignallingNaN(s2));
  VIXL_ASSERT(IsSignallingNaN(sa));
  VIXL_ASSERT(IsQuietNaN(q1));
  VIXL_ASSERT(IsQuietNaN(q2));
  VIXL_ASSERT(IsQuietNaN(qa));

  // The input NaNs after passing through ProcessNaN.
  float s1_proc = RawbitsToFloat(0x7fd51111);
  float s2_proc = RawbitsToFloat(0x7fd52222);
  float sa_proc = RawbitsToFloat(0x7fd5aaaa);
  float q1_proc = q1;
  float q2_proc = q2;
  float qa_proc = qa;
  VIXL_ASSERT(IsQuietNaN(s1_proc));
  VIXL_ASSERT(IsQuietNaN(s2_proc));
  VIXL_ASSERT(IsQuietNaN(sa_proc));
  VIXL_ASSERT(IsQuietNaN(q1_proc));
  VIXL_ASSERT(IsQuietNaN(q2_proc));
  VIXL_ASSERT(IsQuietNaN(qa_proc));

  // Negated NaNs as it would be done on ARMv8 hardware.
  float s1_proc_neg = RawbitsToFloat(0xffd51111);
  float sa_proc_neg = RawbitsToFloat(0xffd5aaaa);
  float q1_proc_neg = RawbitsToFloat(0xffea1111);
  float qa_proc_neg = RawbitsToFloat(0xffeaaaaa);
  VIXL_ASSERT(IsQuietNaN(s1_proc_neg));
  VIXL_ASSERT(IsQuietNaN(sa_proc_neg));
  VIXL_ASSERT(IsQuietNaN(q1_proc_neg));
  VIXL_ASSERT(IsQuietNaN(qa_proc_neg));

  // Quiet NaNs are propagated.
  FmaddFmsubHelper(q1, 0, 0, q1_proc, q1_proc_neg, q1_proc_neg, q1_proc);
  FmaddFmsubHelper(0, q2, 0, q2_proc, q2_proc, q2_proc, q2_proc);
  FmaddFmsubHelper(0, 0, qa, qa_proc, qa_proc, qa_proc_neg, qa_proc_neg);
  FmaddFmsubHelper(q1, q2, 0, q1_proc, q1_proc_neg, q1_proc_neg, q1_proc);
  FmaddFmsubHelper(0, q2, qa, qa_proc, qa_proc, qa_proc_neg, qa_proc_neg);
  FmaddFmsubHelper(q1, 0, qa, qa_proc, qa_proc, qa_proc_neg, qa_proc_neg);
  FmaddFmsubHelper(q1, q2, qa, qa_proc, qa_proc, qa_proc_neg, qa_proc_neg);

  // Signalling NaNs are propagated, and made quiet.
  FmaddFmsubHelper(s1, 0, 0, s1_proc, s1_proc_neg, s1_proc_neg, s1_proc);
  FmaddFmsubHelper(0, s2, 0, s2_proc, s2_proc, s2_proc, s2_proc);
  FmaddFmsubHelper(0, 0, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, s2, 0, s1_proc, s1_proc_neg, s1_proc_neg, s1_proc);
  FmaddFmsubHelper(0, s2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, 0, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, s2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);

  // Signalling NaNs take precedence over quiet NaNs.
  FmaddFmsubHelper(s1, q2, qa, s1_proc, s1_proc_neg, s1_proc_neg, s1_proc);
  FmaddFmsubHelper(q1, s2, qa, s2_proc, s2_proc, s2_proc, s2_proc);
  FmaddFmsubHelper(q1, q2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, s2, qa, s1_proc, s1_proc_neg, s1_proc_neg, s1_proc);
  FmaddFmsubHelper(q1, s2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, q2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);
  FmaddFmsubHelper(s1, s2, sa, sa_proc, sa_proc, sa_proc_neg, sa_proc_neg);

  // A NaN generated by the intermediate op1 * op2 overrides a quiet NaN in a.
  FmaddFmsubHelper(0,
                   kFP32PositiveInfinity,
                   qa,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN);
  FmaddFmsubHelper(kFP32PositiveInfinity,
                   0,
                   qa,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN);
  FmaddFmsubHelper(0,
                   kFP32NegativeInfinity,
                   qa,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN);
  FmaddFmsubHelper(kFP32NegativeInfinity,
                   0,
                   qa,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN,
                   kFP32DefaultNaN);
}


TEST(fdiv) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s14, -0.0f);
  __ Fmov(s15, kFP32PositiveInfinity);
  __ Fmov(s16, kFP32NegativeInfinity);
  __ Fmov(s17, 3.25f);
  __ Fmov(s18, 2.0f);
  __ Fmov(s19, 2.0f);
  __ Fmov(s20, -2.0f);

  __ Fmov(d26, -0.0);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0.0);
  __ Fmov(d30, -2.0);
  __ Fmov(d31, 2.25);

  __ Fdiv(s0, s17, s18);
  __ Fdiv(s1, s18, s19);
  __ Fdiv(s2, s14, s18);
  __ Fdiv(s3, s18, s15);
  __ Fdiv(s4, s18, s16);
  __ Fdiv(s5, s15, s16);
  __ Fdiv(s6, s14, s14);

  __ Fdiv(d7, d31, d30);
  __ Fdiv(d8, d29, d31);
  __ Fdiv(d9, d26, d31);
  __ Fdiv(d10, d31, d27);
  __ Fdiv(d11, d31, d28);
  __ Fdiv(d12, d28, d27);
  __ Fdiv(d13, d29, d29);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.625f, s0);
    ASSERT_EQUAL_FP32(1.0f, s1);
    ASSERT_EQUAL_FP32(-0.0f, s2);
    ASSERT_EQUAL_FP32(0.0f, s3);
    ASSERT_EQUAL_FP32(-0.0f, s4);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s5);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s6);
    ASSERT_EQUAL_FP64(-1.125, d7);
    ASSERT_EQUAL_FP64(0.0, d8);
    ASSERT_EQUAL_FP64(-0.0, d9);
    ASSERT_EQUAL_FP64(0.0, d10);
    ASSERT_EQUAL_FP64(-0.0, d11);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d12);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d13);
  }
}


TEST(fdiv_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFPHalf);

  START();
  __ Fmov(h14, -0.0f);
  __ Fmov(h15, kFP16PositiveInfinity);
  __ Fmov(h16, kFP16NegativeInfinity);
  __ Fmov(h17, 3.25f);
  __ Fmov(h18, 2.0f);
  __ Fmov(h19, 2.0f);
  __ Fmov(h20, -2.0f);

  __ Fdiv(h0, h17, h18);
  __ Fdiv(h1, h18, h19);
  __ Fdiv(h2, h14, h18);
  __ Fdiv(h3, h18, h15);
  __ Fdiv(h4, h18, h16);
  __ Fdiv(h5, h15, h16);
  __ Fdiv(h6, h14, h14);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(1.625f), h0);
    ASSERT_EQUAL_FP16(Float16(1.0f), h1);
    ASSERT_EQUAL_FP16(Float16(-0.0f), h2);
    ASSERT_EQUAL_FP16(Float16(0.0f), h3);
    ASSERT_EQUAL_FP16(Float16(-0.0f), h4);
    ASSERT_EQUAL_FP16(kFP16DefaultNaN, h5);
    ASSERT_EQUAL_FP16(kFP16DefaultNaN, h6);
  }
}

static float MinMaxHelper(float n,
                          float m,
                          bool min,
                          float quiet_nan_substitute = 0.0) {
  const uint64_t kFP32QuietNaNMask = 0x00400000;
  uint32_t raw_n = FloatToRawbits(n);
  uint32_t raw_m = FloatToRawbits(m);

  if (IsNaN(n) && ((raw_n & kFP32QuietNaNMask) == 0)) {
    // n is signalling NaN.
    return RawbitsToFloat(raw_n | kFP32QuietNaNMask);
  } else if (IsNaN(m) && ((raw_m & kFP32QuietNaNMask) == 0)) {
    // m is signalling NaN.
    return RawbitsToFloat(raw_m | kFP32QuietNaNMask);
  } else if (quiet_nan_substitute == 0.0) {
    if (IsNaN(n)) {
      // n is quiet NaN.
      return n;
    } else if (IsNaN(m)) {
      // m is quiet NaN.
      return m;
    }
  } else {
    // Substitute n or m if one is quiet, but not both.
    if (IsNaN(n) && !IsNaN(m)) {
      // n is quiet NaN: replace with substitute.
      n = quiet_nan_substitute;
    } else if (!IsNaN(n) && IsNaN(m)) {
      // m is quiet NaN: replace with substitute.
      m = quiet_nan_substitute;
    }
  }

  if ((n == 0.0) && (m == 0.0) && (copysign(1.0, n) != copysign(1.0, m))) {
    return min ? -0.0 : 0.0;
  }

  return min ? fminf(n, m) : fmaxf(n, m);
}


static double MinMaxHelper(double n,
                           double m,
                           bool min,
                           double quiet_nan_substitute = 0.0) {
  const uint64_t kFP64QuietNaNMask = 0x0008000000000000;
  uint64_t raw_n = DoubleToRawbits(n);
  uint64_t raw_m = DoubleToRawbits(m);

  if (IsNaN(n) && ((raw_n & kFP64QuietNaNMask) == 0)) {
    // n is signalling NaN.
    return RawbitsToDouble(raw_n | kFP64QuietNaNMask);
  } else if (IsNaN(m) && ((raw_m & kFP64QuietNaNMask) == 0)) {
    // m is signalling NaN.
    return RawbitsToDouble(raw_m | kFP64QuietNaNMask);
  } else if (quiet_nan_substitute == 0.0) {
    if (IsNaN(n)) {
      // n is quiet NaN.
      return n;
    } else if (IsNaN(m)) {
      // m is quiet NaN.
      return m;
    }
  } else {
    // Substitute n or m if one is quiet, but not both.
    if (IsNaN(n) && !IsNaN(m)) {
      // n is quiet NaN: replace with substitute.
      n = quiet_nan_substitute;
    } else if (!IsNaN(n) && IsNaN(m)) {
      // m is quiet NaN: replace with substitute.
      m = quiet_nan_substitute;
    }
  }

  if ((n == 0.0) && (m == 0.0) && (copysign(1.0, n) != copysign(1.0, m))) {
    return min ? -0.0 : 0.0;
  }

  return min ? fmin(n, m) : fmax(n, m);
}


static void FminFmaxDoubleHelper(
    double n, double m, double min, double max, double minnm, double maxnm) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(d0, n);
  __ Fmov(d1, m);
  __ Fmin(d28, d0, d1);
  __ Fmax(d29, d0, d1);
  __ Fminnm(d30, d0, d1);
  __ Fmaxnm(d31, d0, d1);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP64(min, d28);
    ASSERT_EQUAL_FP64(max, d29);
    ASSERT_EQUAL_FP64(minnm, d30);
    ASSERT_EQUAL_FP64(maxnm, d31);
  }
}


TEST(fmax_fmin_d) {
  // Use non-standard NaNs to check that the payload bits are preserved.
  double snan = RawbitsToDouble(0x7ff5555512345678);
  double qnan = RawbitsToDouble(0x7ffaaaaa87654321);

  double snan_processed = RawbitsToDouble(0x7ffd555512345678);
  double qnan_processed = qnan;

  VIXL_ASSERT(IsSignallingNaN(snan));
  VIXL_ASSERT(IsQuietNaN(qnan));
  VIXL_ASSERT(IsQuietNaN(snan_processed));
  VIXL_ASSERT(IsQuietNaN(qnan_processed));

  // Bootstrap tests.
  FminFmaxDoubleHelper(0, 0, 0, 0, 0, 0);
  FminFmaxDoubleHelper(0, 1, 0, 1, 0, 1);
  FminFmaxDoubleHelper(kFP64PositiveInfinity,
                       kFP64NegativeInfinity,
                       kFP64NegativeInfinity,
                       kFP64PositiveInfinity,
                       kFP64NegativeInfinity,
                       kFP64PositiveInfinity);
  FminFmaxDoubleHelper(snan,
                       0,
                       snan_processed,
                       snan_processed,
                       snan_processed,
                       snan_processed);
  FminFmaxDoubleHelper(0,
                       snan,
                       snan_processed,
                       snan_processed,
                       snan_processed,
                       snan_processed);
  FminFmaxDoubleHelper(qnan, 0, qnan_processed, qnan_processed, 0, 0);
  FminFmaxDoubleHelper(0, qnan, qnan_processed, qnan_processed, 0, 0);
  FminFmaxDoubleHelper(qnan,
                       snan,
                       snan_processed,
                       snan_processed,
                       snan_processed,
                       snan_processed);
  FminFmaxDoubleHelper(snan,
                       qnan,
                       snan_processed,
                       snan_processed,
                       snan_processed,
                       snan_processed);

  // Iterate over all combinations of inputs.
  double inputs[] = {DBL_MAX,
                     DBL_MIN,
                     1.0,
                     0.0,
                     -DBL_MAX,
                     -DBL_MIN,
                     -1.0,
                     -0.0,
                     kFP64PositiveInfinity,
                     kFP64NegativeInfinity,
                     kFP64QuietNaN,
                     kFP64SignallingNaN};

  const int count = sizeof(inputs) / sizeof(inputs[0]);

  for (int in = 0; in < count; in++) {
    double n = inputs[in];
    for (int im = 0; im < count; im++) {
      double m = inputs[im];
      FminFmaxDoubleHelper(n,
                           m,
                           MinMaxHelper(n, m, true),
                           MinMaxHelper(n, m, false),
                           MinMaxHelper(n, m, true, kFP64PositiveInfinity),
                           MinMaxHelper(n, m, false, kFP64NegativeInfinity));
    }
  }
}


static void FminFmaxFloatHelper(
    float n, float m, float min, float max, float minnm, float maxnm) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s0, n);
  __ Fmov(s1, m);
  __ Fmin(s28, s0, s1);
  __ Fmax(s29, s0, s1);
  __ Fminnm(s30, s0, s1);
  __ Fmaxnm(s31, s0, s1);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(min, s28);
    ASSERT_EQUAL_FP32(max, s29);
    ASSERT_EQUAL_FP32(minnm, s30);
    ASSERT_EQUAL_FP32(maxnm, s31);
  }
}


TEST(fmax_fmin_s) {
  // Use non-standard NaNs to check that the payload bits are preserved.
  float snan = RawbitsToFloat(0x7f951234);
  float qnan = RawbitsToFloat(0x7fea8765);

  float snan_processed = RawbitsToFloat(0x7fd51234);
  float qnan_processed = qnan;

  VIXL_ASSERT(IsSignallingNaN(snan));
  VIXL_ASSERT(IsQuietNaN(qnan));
  VIXL_ASSERT(IsQuietNaN(snan_processed));
  VIXL_ASSERT(IsQuietNaN(qnan_processed));

  // Bootstrap tests.
  FminFmaxFloatHelper(0, 0, 0, 0, 0, 0);
  FminFmaxFloatHelper(0, 1, 0, 1, 0, 1);
  FminFmaxFloatHelper(kFP32PositiveInfinity,
                      kFP32NegativeInfinity,
                      kFP32NegativeInfinity,
                      kFP32PositiveInfinity,
                      kFP32NegativeInfinity,
                      kFP32PositiveInfinity);
  FminFmaxFloatHelper(snan,
                      0,
                      snan_processed,
                      snan_processed,
                      snan_processed,
                      snan_processed);
  FminFmaxFloatHelper(0,
                      snan,
                      snan_processed,
                      snan_processed,
                      snan_processed,
                      snan_processed);
  FminFmaxFloatHelper(qnan, 0, qnan_processed, qnan_processed, 0, 0);
  FminFmaxFloatHelper(0, qnan, qnan_processed, qnan_processed, 0, 0);
  FminFmaxFloatHelper(qnan,
                      snan,
                      snan_processed,
                      snan_processed,
                      snan_processed,
                      snan_processed);
  FminFmaxFloatHelper(snan,
                      qnan,
                      snan_processed,
                      snan_processed,
                      snan_processed,
                      snan_processed);

  // Iterate over all combinations of inputs.
  float inputs[] = {FLT_MAX,
                    FLT_MIN,
                    1.0,
                    0.0,
                    -FLT_MAX,
                    -FLT_MIN,
                    -1.0,
                    -0.0,
                    kFP32PositiveInfinity,
                    kFP32NegativeInfinity,
                    kFP32QuietNaN,
                    kFP32SignallingNaN};

  const int count = sizeof(inputs) / sizeof(inputs[0]);

  for (int in = 0; in < count; in++) {
    float n = inputs[in];
    for (int im = 0; im < count; im++) {
      float m = inputs[im];
      FminFmaxFloatHelper(n,
                          m,
                          MinMaxHelper(n, m, true),
                          MinMaxHelper(n, m, false),
                          MinMaxHelper(n, m, true, kFP32PositiveInfinity),
                          MinMaxHelper(n, m, false, kFP32NegativeInfinity));
    }
  }
}

TEST(fccmp) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, 0.0);
  __ Fmov(s17, 0.5);
  __ Fmov(d18, -0.5);
  __ Fmov(d19, -1.0);
  __ Mov(x20, 0);
  __ Mov(x21, 0x7ff0000000000001);  // Double precision NaN.
  __ Fmov(d21, x21);
  __ Mov(w22, 0x7f800001);  // Single precision NaN.
  __ Fmov(s22, w22);

  __ Cmp(x20, 0);
  __ Fccmp(s16, s16, NoFlag, eq);
  __ Mrs(x0, NZCV);

  __ Cmp(x20, 0);
  __ Fccmp(s16, s16, VFlag, ne);
  __ Mrs(x1, NZCV);

  __ Cmp(x20, 0);
  __ Fccmp(s16, s17, CFlag, ge);
  __ Mrs(x2, NZCV);

  __ Cmp(x20, 0);
  __ Fccmp(s16, s17, CVFlag, lt);
  __ Mrs(x3, NZCV);

  __ Cmp(x20, 0);
  __ Fccmp(d18, d18, ZFlag, le);
  __ Mrs(x4, NZCV);

  __ Cmp(x20, 0);
  __ Fccmp(d18, d18, ZVFlag, gt);
  __ Mrs(x5, NZCV);

  __ Cmp(x20, 0);
  __ Fccmp(d18, d19, ZCVFlag, ls);
  __ Mrs(x6, NZCV);

  __ Cmp(x20, 0);
  __ Fccmp(d18, d19, NFlag, hi);
  __ Mrs(x7, NZCV);

  // The Macro Assembler does not allow al or nv as condition.
  {
    ExactAssemblyScope scope(&masm, kInstructionSize);
    __ fccmp(s16, s16, NFlag, al);
  }
  __ Mrs(x8, NZCV);

  {
    ExactAssemblyScope scope(&masm, kInstructionSize);
    __ fccmp(d18, d18, NFlag, nv);
  }
  __ Mrs(x9, NZCV);

  __ Cmp(x20, 0);
  __ Fccmpe(s16, s16, NoFlag, eq);
  __ Mrs(x10, NZCV);

  __ Cmp(x20, 0);
  __ Fccmpe(d18, d19, ZCVFlag, ls);
  __ Mrs(x11, NZCV);

  __ Cmp(x20, 0);
  __ Fccmpe(d21, d21, NoFlag, eq);
  __ Mrs(x12, NZCV);

  __ Cmp(x20, 0);
  __ Fccmpe(s22, s22, NoFlag, eq);
  __ Mrs(x13, NZCV);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_32(ZCFlag, w0);
    ASSERT_EQUAL_32(VFlag, w1);
    ASSERT_EQUAL_32(NFlag, w2);
    ASSERT_EQUAL_32(CVFlag, w3);
    ASSERT_EQUAL_32(ZCFlag, w4);
    ASSERT_EQUAL_32(ZVFlag, w5);
    ASSERT_EQUAL_32(CFlag, w6);
    ASSERT_EQUAL_32(NFlag, w7);
    ASSERT_EQUAL_32(ZCFlag, w8);
    ASSERT_EQUAL_32(ZCFlag, w9);
    ASSERT_EQUAL_32(ZCFlag, w10);
    ASSERT_EQUAL_32(CFlag, w11);
    ASSERT_EQUAL_32(CVFlag, w12);
    ASSERT_EQUAL_32(CVFlag, w13);
  }
}


TEST(fccmp_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFPHalf);

  START();
  __ Fmov(h16, Float16(0.0));
  __ Fmov(h17, Float16(0.5));
  __ Mov(x20, 0);
  __ Fmov(h21, kFP16DefaultNaN);

  __ Cmp(x20, 0);
  __ Fccmp(h16, h16, NoFlag, eq);
  __ Mrs(x0, NZCV);

  __ Cmp(x20, 0);
  __ Fccmp(h16, h16, VFlag, ne);
  __ Mrs(x1, NZCV);

  __ Cmp(x20, 0);
  __ Fccmp(h16, h17, CFlag, ge);
  __ Mrs(x2, NZCV);

  __ Cmp(x20, 0);
  __ Fccmp(h16, h17, CVFlag, lt);
  __ Mrs(x3, NZCV);

  // The Macro Assembler does not allow al or nv as condition.
  {
    ExactAssemblyScope scope(&masm, kInstructionSize);
    __ fccmp(h16, h16, NFlag, al);
  }
  __ Mrs(x4, NZCV);
  {
    ExactAssemblyScope scope(&masm, kInstructionSize);
    __ fccmp(h16, h16, NFlag, nv);
  }
  __ Mrs(x5, NZCV);

  __ Cmp(x20, 0);
  __ Fccmpe(h16, h16, NoFlag, eq);
  __ Mrs(x6, NZCV);

  __ Cmp(x20, 0);
  __ Fccmpe(h16, h21, NoFlag, eq);
  __ Mrs(x7, NZCV);

  __ Cmp(x20, 0);
  __ Fccmpe(h21, h16, NoFlag, eq);
  __ Mrs(x8, NZCV);

  __ Cmp(x20, 0);
  __ Fccmpe(h21, h21, NoFlag, eq);
  __ Mrs(x9, NZCV);
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_32(ZCFlag, w0);
    ASSERT_EQUAL_32(VFlag, w1);
    ASSERT_EQUAL_32(NFlag, w2);
    ASSERT_EQUAL_32(CVFlag, w3);
    ASSERT_EQUAL_32(ZCFlag, w4);
    ASSERT_EQUAL_32(ZCFlag, w5);
    ASSERT_EQUAL_32(ZCFlag, w6);
    ASSERT_EQUAL_32(CVFlag, w7);
    ASSERT_EQUAL_32(CVFlag, w8);
    ASSERT_EQUAL_32(CVFlag, w9);
  }
}


TEST(fcmp) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();

  // Some of these tests require a floating-point scratch register assigned to
  // the macro assembler, but most do not.
  {
    UseScratchRegisterScope temps(&masm);
    temps.ExcludeAll();
    temps.Include(ip0, ip1);

    __ Fmov(s8, 0.0);
    __ Fmov(s9, 0.5);
    __ Mov(w18, 0x7f800001);  // Single precision NaN.
    __ Fmov(s18, w18);

    __ Fcmp(s8, s8);
    __ Mrs(x0, NZCV);
    __ Fcmp(s8, s9);
    __ Mrs(x1, NZCV);
    __ Fcmp(s9, s8);
    __ Mrs(x2, NZCV);
    __ Fcmp(s8, s18);
    __ Mrs(x3, NZCV);
    __ Fcmp(s18, s18);
    __ Mrs(x4, NZCV);
    __ Fcmp(s8, 0.0);
    __ Mrs(x5, NZCV);
    temps.Include(d0);
    __ Fcmp(s8, 255.0);
    temps.Exclude(d0);
    __ Mrs(x6, NZCV);

    __ Fmov(d19, 0.0);
    __ Fmov(d20, 0.5);
    __ Mov(x21, 0x7ff0000000000001);  // Double precision NaN.
    __ Fmov(d21, x21);

    __ Fcmp(d19, d19);
    __ Mrs(x10, NZCV);
    __ Fcmp(d19, d20);
    __ Mrs(x11, NZCV);
    __ Fcmp(d20, d19);
    __ Mrs(x12, NZCV);
    __ Fcmp(d19, d21);
    __ Mrs(x13, NZCV);
    __ Fcmp(d21, d21);
    __ Mrs(x14, NZCV);
    __ Fcmp(d19, 0.0);
    __ Mrs(x15, NZCV);
    temps.Include(d0);
    __ Fcmp(d19, 12.3456);
    temps.Exclude(d0);
    __ Mrs(x16, NZCV);

    __ Fcmpe(s8, s8);
    __ Mrs(x22, NZCV);
    __ Fcmpe(s8, 0.0);
    __ Mrs(x23, NZCV);
    __ Fcmpe(d19, d19);
    __ Mrs(x24, NZCV);
    __ Fcmpe(d19, 0.0);
    __ Mrs(x25, NZCV);
    __ Fcmpe(s18, s18);
    __ Mrs(x26, NZCV);
    __ Fcmpe(d21, d21);
    __ Mrs(x27, NZCV);
  }

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_32(ZCFlag, w0);
    ASSERT_EQUAL_32(NFlag, w1);
    ASSERT_EQUAL_32(CFlag, w2);
    ASSERT_EQUAL_32(CVFlag, w3);
    ASSERT_EQUAL_32(CVFlag, w4);
    ASSERT_EQUAL_32(ZCFlag, w5);
    ASSERT_EQUAL_32(NFlag, w6);
    ASSERT_EQUAL_32(ZCFlag, w10);
    ASSERT_EQUAL_32(NFlag, w11);
    ASSERT_EQUAL_32(CFlag, w12);
    ASSERT_EQUAL_32(CVFlag, w13);
    ASSERT_EQUAL_32(CVFlag, w14);
    ASSERT_EQUAL_32(ZCFlag, w15);
    ASSERT_EQUAL_32(NFlag, w16);
    ASSERT_EQUAL_32(ZCFlag, w22);
    ASSERT_EQUAL_32(ZCFlag, w23);
    ASSERT_EQUAL_32(ZCFlag, w24);
    ASSERT_EQUAL_32(ZCFlag, w25);
    ASSERT_EQUAL_32(CVFlag, w26);
    ASSERT_EQUAL_32(CVFlag, w27);
  }
}


TEST(fcmp_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFPHalf);

  START();

  // Some of these tests require a floating-point scratch register assigned to
  // the macro assembler, but most do not.
  {
    UseScratchRegisterScope temps(&masm);
    temps.ExcludeAll();
    temps.Include(ip0, ip1);

    __ Fmov(h8, Float16(0.0));
    __ Fmov(h9, Float16(0.5));
    __ Fmov(h18, kFP16DefaultNaN);

    __ Fcmp(h8, h8);
    __ Mrs(x0, NZCV);
    __ Fcmp(h8, h9);
    __ Mrs(x1, NZCV);
    __ Fcmp(h9, h8);
    __ Mrs(x2, NZCV);
    __ Fcmp(h8, h18);
    __ Mrs(x3, NZCV);
    __ Fcmp(h18, h18);
    __ Mrs(x4, NZCV);
    __ Fcmp(h8, 0.0);
    __ Mrs(x5, NZCV);
    temps.Include(d0);
    __ Fcmp(h8, 255.0);
    temps.Exclude(d0);
    __ Mrs(x6, NZCV);

    __ Fcmpe(h8, h8);
    __ Mrs(x22, NZCV);
    __ Fcmpe(h8, 0.0);
    __ Mrs(x23, NZCV);
    __ Fcmpe(h8, h18);
    __ Mrs(x24, NZCV);
    __ Fcmpe(h18, h8);
    __ Mrs(x25, NZCV);
    __ Fcmpe(h18, h18);
    __ Mrs(x26, NZCV);
  }

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_32(ZCFlag, w0);
    ASSERT_EQUAL_32(NFlag, w1);
    ASSERT_EQUAL_32(CFlag, w2);
    ASSERT_EQUAL_32(CVFlag, w3);
    ASSERT_EQUAL_32(CVFlag, w4);
    ASSERT_EQUAL_32(ZCFlag, w5);
    ASSERT_EQUAL_32(NFlag, w6);
    ASSERT_EQUAL_32(ZCFlag, w22);
    ASSERT_EQUAL_32(ZCFlag, w23);
    ASSERT_EQUAL_32(CVFlag, w24);
    ASSERT_EQUAL_32(CVFlag, w25);
    ASSERT_EQUAL_32(CVFlag, w26);
  }
}


TEST(fcsel) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Mov(x16, 0);
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 2.0);
  __ Fmov(d18, 3.0);
  __ Fmov(d19, 4.0);

  __ Cmp(x16, 0);
  __ Fcsel(s0, s16, s17, eq);
  __ Fcsel(s1, s16, s17, ne);
  __ Fcsel(d2, d18, d19, eq);
  __ Fcsel(d3, d18, d19, ne);
  // The Macro Assembler does not allow al or nv as condition.
  {
    ExactAssemblyScope scope(&masm, 2 * kInstructionSize);
    __ fcsel(s4, s16, s17, al);
    __ fcsel(d5, d18, d19, nv);
  }
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(2.0, s1);
    ASSERT_EQUAL_FP64(3.0, d2);
    ASSERT_EQUAL_FP64(4.0, d3);
    ASSERT_EQUAL_FP32(1.0, s4);
    ASSERT_EQUAL_FP64(3.0, d5);
  }
}


TEST(fcsel_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFPHalf);

  START();
  __ Mov(x16, 0);
  __ Fmov(h16, Float16(1.0));
  __ Fmov(h17, Float16(2.0));

  __ Cmp(x16, 0);
  __ Fcsel(h0, h16, h17, eq);
  __ Fcsel(h1, h16, h17, ne);
  // The Macro Assembler does not allow al or nv as condition.
  {
    ExactAssemblyScope scope(&masm, 2 * kInstructionSize);
    __ fcsel(h4, h16, h17, al);
    __ fcsel(h5, h16, h17, nv);
  }
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_FP16(Float16(1.0), h0);
    ASSERT_EQUAL_FP16(Float16(2.0), h1);
    ASSERT_EQUAL_FP16(Float16(1.0), h4);
    ASSERT_EQUAL_FP16(Float16(1.0), h5);
  }
}


TEST(fneg) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 0.0);
  __ Fmov(s18, kFP32PositiveInfinity);
  __ Fmov(d19, 1.0);
  __ Fmov(d20, 0.0);
  __ Fmov(d21, kFP64PositiveInfinity);

  __ Fneg(s0, s16);
  __ Fneg(s1, s0);
  __ Fneg(s2, s17);
  __ Fneg(s3, s2);
  __ Fneg(s4, s18);
  __ Fneg(s5, s4);
  __ Fneg(d6, d19);
  __ Fneg(d7, d6);
  __ Fneg(d8, d20);
  __ Fneg(d9, d8);
  __ Fneg(d10, d21);
  __ Fneg(d11, d10);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(-1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(-0.0, s2);
    ASSERT_EQUAL_FP32(0.0, s3);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s4);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s5);
    ASSERT_EQUAL_FP64(-1.0, d6);
    ASSERT_EQUAL_FP64(1.0, d7);
    ASSERT_EQUAL_FP64(-0.0, d8);
    ASSERT_EQUAL_FP64(0.0, d9);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d10);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d11);
  }
}


TEST(fabs) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, -1.0);
  __ Fmov(s17, -0.0);
  __ Fmov(s18, kFP32NegativeInfinity);
  __ Fmov(d19, -1.0);
  __ Fmov(d20, -0.0);
  __ Fmov(d21, kFP64NegativeInfinity);

  __ Fabs(s0, s16);
  __ Fabs(s1, s0);
  __ Fabs(s2, s17);
  __ Fabs(s3, s18);
  __ Fabs(d4, d19);
  __ Fabs(d5, d4);
  __ Fabs(d6, d20);
  __ Fabs(d7, d21);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(0.0, s2);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s3);
    ASSERT_EQUAL_FP64(1.0, d4);
    ASSERT_EQUAL_FP64(1.0, d5);
    ASSERT_EQUAL_FP64(0.0, d6);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d7);
  }
}


TEST(fsqrt) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, 0.0);
  __ Fmov(s17, 1.0);
  __ Fmov(s18, 0.25);
  __ Fmov(s19, 65536.0);
  __ Fmov(s20, -0.0);
  __ Fmov(s21, kFP32PositiveInfinity);
  __ Fmov(s22, -1.0);
  __ Fmov(d23, 0.0);
  __ Fmov(d24, 1.0);
  __ Fmov(d25, 0.25);
  __ Fmov(d26, 4294967296.0);
  __ Fmov(d27, -0.0);
  __ Fmov(d28, kFP64PositiveInfinity);
  __ Fmov(d29, -1.0);

  __ Fsqrt(s0, s16);
  __ Fsqrt(s1, s17);
  __ Fsqrt(s2, s18);
  __ Fsqrt(s3, s19);
  __ Fsqrt(s4, s20);
  __ Fsqrt(s5, s21);
  __ Fsqrt(s6, s22);
  __ Fsqrt(d7, d23);
  __ Fsqrt(d8, d24);
  __ Fsqrt(d9, d25);
  __ Fsqrt(d10, d26);
  __ Fsqrt(d11, d27);
  __ Fsqrt(d12, d28);
  __ Fsqrt(d13, d29);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(0.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(0.5, s2);
    ASSERT_EQUAL_FP32(256.0, s3);
    ASSERT_EQUAL_FP32(-0.0, s4);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s5);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s6);
    ASSERT_EQUAL_FP64(0.0, d7);
    ASSERT_EQUAL_FP64(1.0, d8);
    ASSERT_EQUAL_FP64(0.5, d9);
    ASSERT_EQUAL_FP64(65536.0, d10);
    ASSERT_EQUAL_FP64(-0.0, d11);
    ASSERT_EQUAL_FP64(kFP32PositiveInfinity, d12);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d13);
  }
}

TEST(frint32x_s) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFrintToFixedSizedInt);

  START();

  __ Fmov(s13, 1.0);
  __ Fmov(s14, 1.1);
  __ Fmov(s15, 1.5);
  __ Fmov(s16, 1.9);
  __ Fmov(s17, 2.5);
  __ Fmov(s18, -1.5);
  __ Fmov(s19, -2.5);
  __ Fmov(s20, kFP32PositiveInfinity);
  __ Fmov(s21, kFP32NegativeInfinity);
  __ Fmov(s22, 0.0);
  __ Fmov(s23, -0.0);
  __ Fmov(s24, -0.2);
  __ Fmov(s25, kFP32DefaultNaN);
  __ Fmov(s26, INT32_MIN);
  __ Fmov(s27, INT32_MIN + 0x80);  // The next representable FP32.
  __ Fmov(s28, 0x80000000);
  __ Fmov(s29, 0x7fffff80);  // The largest int32_t representable as FP32.
  __ Fmov(s30, FLT_MIN);
  __ Fmov(s31, FLT_MAX);

  __ Frint32x(s0, s13);
  __ Frint32x(s1, s14);
  __ Frint32x(s2, s15);
  __ Frint32x(s3, s16);
  __ Frint32x(s4, s17);
  __ Frint32x(s5, s18);
  __ Frint32x(s6, s19);
  __ Frint32x(s7, s20);
  __ Frint32x(s8, s21);
  __ Frint32x(s9, s22);
  __ Frint32x(s10, s23);
  __ Frint32x(s11, s24);
  __ Frint32x(s12, s25);
  __ Frint32x(s13, s26);
  __ Frint32x(s14, s27);
  __ Frint32x(s15, s28);
  __ Frint32x(s16, s29);
  __ Frint32x(s17, s30);
  __ Frint32x(s18, s31);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(2.0, s2);
    ASSERT_EQUAL_FP32(2.0, s3);
    ASSERT_EQUAL_FP32(2.0, s4);
    ASSERT_EQUAL_FP32(-2.0, s5);
    ASSERT_EQUAL_FP32(-2.0, s6);
    ASSERT_EQUAL_FP32(INT32_MIN, s7);
    ASSERT_EQUAL_FP32(INT32_MIN, s8);
    ASSERT_EQUAL_FP32(0.0, s9);
    ASSERT_EQUAL_FP32(-0.0, s10);
    ASSERT_EQUAL_FP32(-0.0, s11);
    ASSERT_EQUAL_FP32(INT32_MIN, s12);  // NaN.
    ASSERT_EQUAL_FP32(INT32_MIN, s13);
    ASSERT_EQUAL_FP32(INT32_MIN + 0x80, s14);
    ASSERT_EQUAL_FP32(INT32_MIN, s15);  // Out of range.
    ASSERT_EQUAL_FP32(0x7fffff80, s16);
    ASSERT_EQUAL_FP32(0, s17);
    ASSERT_EQUAL_FP32(INT32_MIN, s18);
  }
}

TEST(frint32x_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFrintToFixedSizedInt);

  START();

  __ Fmov(d13, 1.0);
  __ Fmov(d14, 1.1);
  __ Fmov(d15, 1.5);
  __ Fmov(d16, 1.9);
  __ Fmov(d17, 2.5);
  __ Fmov(d18, -1.5);
  __ Fmov(d19, -2.5);
  __ Fmov(d20, kFP64PositiveInfinity);
  __ Fmov(d21, kFP64NegativeInfinity);
  __ Fmov(d22, 0.0);
  __ Fmov(d23, -0.0);
  __ Fmov(d24, -0.2);
  __ Fmov(d25, kFP64DefaultNaN);
  __ Fmov(d26, INT32_MIN);
  __ Fmov(d27, INT32_MIN + 1);
  __ Fmov(d28, INT32_MAX);
  __ Fmov(d29, INT32_MAX - 1);
  __ Fmov(d30, FLT_MIN);
  __ Fmov(d31, FLT_MAX);

  __ Frint32x(d0, d13);
  __ Frint32x(d1, d14);
  __ Frint32x(d2, d15);
  __ Frint32x(d3, d16);
  __ Frint32x(d4, d17);
  __ Frint32x(d5, d18);
  __ Frint32x(d6, d19);
  __ Frint32x(d7, d20);
  __ Frint32x(d8, d21);
  __ Frint32x(d9, d22);
  __ Frint32x(d10, d23);
  __ Frint32x(d11, d24);
  __ Frint32x(d12, d25);
  __ Frint32x(d13, d26);
  __ Frint32x(d14, d27);
  __ Frint32x(d15, d28);
  __ Frint32x(d16, d29);
  __ Frint32x(d17, d30);
  __ Frint32x(d18, d31);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP64(1.0, d0);
    ASSERT_EQUAL_FP64(1.0, d1);
    ASSERT_EQUAL_FP64(2.0, d2);
    ASSERT_EQUAL_FP64(2.0, d3);
    ASSERT_EQUAL_FP64(2.0, d4);
    ASSERT_EQUAL_FP64(-2.0, d5);
    ASSERT_EQUAL_FP64(-2.0, d6);
    ASSERT_EQUAL_FP64(INT32_MIN, d7);
    ASSERT_EQUAL_FP64(INT32_MIN, d8);
    ASSERT_EQUAL_FP64(0.0, d9);
    ASSERT_EQUAL_FP64(-0.0, d10);
    ASSERT_EQUAL_FP64(-0.0, d11);
    ASSERT_EQUAL_FP64(INT32_MIN, d12);
    ASSERT_EQUAL_FP64(INT32_MIN, d13);
    ASSERT_EQUAL_FP64(INT32_MIN + 1, d14);
    ASSERT_EQUAL_FP64(INT32_MAX, d15);
    ASSERT_EQUAL_FP64(INT32_MAX - 1, d16);
    ASSERT_EQUAL_FP64(0, d17);
    ASSERT_EQUAL_FP64(INT32_MIN, d18);
  }
}

TEST(frint32z_s) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFrintToFixedSizedInt);

  START();

  __ Fmov(s13, 1.0);
  __ Fmov(s14, 1.1);
  __ Fmov(s15, 1.5);
  __ Fmov(s16, 1.9);
  __ Fmov(s17, 2.5);
  __ Fmov(s18, -1.5);
  __ Fmov(s19, -2.5);
  __ Fmov(s20, kFP32PositiveInfinity);
  __ Fmov(s21, kFP32NegativeInfinity);
  __ Fmov(s22, 0.0);
  __ Fmov(s23, -0.0);
  __ Fmov(s24, -0.2);
  __ Fmov(s25, kFP32DefaultNaN);
  __ Fmov(s26, INT32_MIN);
  __ Fmov(s27, INT32_MIN + 0x80);  // The next representable FP32.
  __ Fmov(s28, 0x80000000);
  __ Fmov(s29, 0x7fffff80);  // The largest int32_t representable as FP32.
  __ Fmov(s30, FLT_MIN);
  __ Fmov(s31, FLT_MAX);

  __ Frint32z(s0, s13);
  __ Frint32z(s1, s14);
  __ Frint32z(s2, s15);
  __ Frint32z(s3, s16);
  __ Frint32z(s4, s17);
  __ Frint32z(s5, s18);
  __ Frint32z(s6, s19);
  __ Frint32z(s7, s20);
  __ Frint32z(s8, s21);
  __ Frint32z(s9, s22);
  __ Frint32z(s10, s23);
  __ Frint32z(s11, s24);
  __ Frint32z(s12, s25);
  __ Frint32z(s13, s26);
  __ Frint32z(s14, s27);
  __ Frint32z(s15, s28);
  __ Frint32z(s16, s29);
  __ Frint32z(s17, s30);
  __ Frint32z(s18, s31);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(1.0, s2);
    ASSERT_EQUAL_FP32(1.0, s3);
    ASSERT_EQUAL_FP32(2.0, s4);
    ASSERT_EQUAL_FP32(-1.0, s5);
    ASSERT_EQUAL_FP32(-2.0, s6);
    ASSERT_EQUAL_FP32(INT32_MIN, s7);
    ASSERT_EQUAL_FP32(INT32_MIN, s8);
    ASSERT_EQUAL_FP32(0.0, s9);
    ASSERT_EQUAL_FP32(-0.0, s10);
    ASSERT_EQUAL_FP32(-0.0, s11);
    ASSERT_EQUAL_FP32(INT32_MIN, s12);  // NaN.
    ASSERT_EQUAL_FP32(INT32_MIN, s13);
    ASSERT_EQUAL_FP32(INT32_MIN + 0x80, s14);
    ASSERT_EQUAL_FP32(INT32_MIN, s15);  // Out of range.
    ASSERT_EQUAL_FP32(0x7fffff80, s16);
    ASSERT_EQUAL_FP32(0, s17);
    ASSERT_EQUAL_FP32(INT32_MIN, s18);
  }
}

TEST(frint32z_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFrintToFixedSizedInt);

  START();

  __ Fmov(d13, 1.0);
  __ Fmov(d14, 1.1);
  __ Fmov(d15, 1.5);
  __ Fmov(d16, 1.9);
  __ Fmov(d17, 2.5);
  __ Fmov(d18, -1.5);
  __ Fmov(d19, -2.5);
  __ Fmov(d20, kFP64PositiveInfinity);
  __ Fmov(d21, kFP64NegativeInfinity);
  __ Fmov(d22, 0.0);
  __ Fmov(d23, -0.0);
  __ Fmov(d24, -0.2);
  __ Fmov(d25, kFP64DefaultNaN);
  __ Fmov(d26, INT32_MIN);
  __ Fmov(d27, INT32_MIN + 1);
  __ Fmov(d28, INT32_MAX);
  __ Fmov(d29, INT32_MAX - 1);
  __ Fmov(d30, FLT_MIN);
  __ Fmov(d31, FLT_MAX);

  __ Frint32z(d0, d13);
  __ Frint32z(d1, d14);
  __ Frint32z(d2, d15);
  __ Frint32z(d3, d16);
  __ Frint32z(d4, d17);
  __ Frint32z(d5, d18);
  __ Frint32z(d6, d19);
  __ Frint32z(d7, d20);
  __ Frint32z(d8, d21);
  __ Frint32z(d9, d22);
  __ Frint32z(d10, d23);
  __ Frint32z(d11, d24);
  __ Frint32z(d12, d25);
  __ Frint32z(d13, d26);
  __ Frint32z(d14, d27);
  __ Frint32z(d15, d28);
  __ Frint32z(d16, d29);
  __ Frint32z(d17, d30);
  __ Frint32z(d18, d31);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP64(1.0, d0);
    ASSERT_EQUAL_FP64(1.0, d1);
    ASSERT_EQUAL_FP64(1.0, d2);
    ASSERT_EQUAL_FP64(1.0, d3);
    ASSERT_EQUAL_FP64(2.0, d4);
    ASSERT_EQUAL_FP64(-1.0, d5);
    ASSERT_EQUAL_FP64(-2.0, d6);
    ASSERT_EQUAL_FP64(INT32_MIN, d7);
    ASSERT_EQUAL_FP64(INT32_MIN, d8);
    ASSERT_EQUAL_FP64(0.0, d9);
    ASSERT_EQUAL_FP64(-0.0, d10);
    ASSERT_EQUAL_FP64(-0.0, d11);
    ASSERT_EQUAL_FP64(INT32_MIN, d12);
    ASSERT_EQUAL_FP64(INT32_MIN, d13);
    ASSERT_EQUAL_FP64(INT32_MIN + 1, d14);
    ASSERT_EQUAL_FP64(INT32_MAX, d15);
    ASSERT_EQUAL_FP64(INT32_MAX - 1, d16);
    ASSERT_EQUAL_FP64(0, d17);
    ASSERT_EQUAL_FP64(INT32_MIN, d18);
  }
}

TEST(frint64x_s) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFrintToFixedSizedInt);

  START();

  __ Fmov(s13, 1.0);
  __ Fmov(s14, 1.1);
  __ Fmov(s15, 1.5);
  __ Fmov(s16, 1.9);
  __ Fmov(s17, 2.5);
  __ Fmov(s18, -1.5);
  __ Fmov(s19, -2.5);
  __ Fmov(s20, kFP64PositiveInfinity);
  __ Fmov(s21, kFP64NegativeInfinity);
  __ Fmov(s22, 0.0);
  __ Fmov(s23, -0.0);
  __ Fmov(s24, -0.2);
  __ Fmov(s25, kFP64DefaultNaN);
  __ Fmov(s26, INT64_MIN);
  __ Fmov(s27, INT64_MIN + 0x80'00000000);  // The next representable FP32.
  __ Fmov(s28, 0x80000000'00000000);
  // The largest int64_t representable as FP32.
  __ Fmov(s29, 0x7fffff80'00000000);
  __ Fmov(s30, FLT_MIN);
  __ Fmov(s31, FLT_MAX);

  __ Frint64x(s0, s13);
  __ Frint64x(s1, s14);
  __ Frint64x(s2, s15);
  __ Frint64x(s3, s16);
  __ Frint64x(s4, s17);
  __ Frint64x(s5, s18);
  __ Frint64x(s6, s19);
  __ Frint64x(s7, s20);
  __ Frint64x(s8, s21);
  __ Frint64x(s9, s22);
  __ Frint64x(s10, s23);
  __ Frint64x(s11, s24);
  __ Frint64x(s12, s25);
  __ Frint64x(s13, s26);
  __ Frint64x(s14, s27);
  __ Frint64x(s15, s28);
  __ Frint64x(s16, s29);
  __ Frint64x(s17, s30);
  __ Frint64x(s18, s31);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(2.0, s2);
    ASSERT_EQUAL_FP32(2.0, s3);
    ASSERT_EQUAL_FP32(2.0, s4);
    ASSERT_EQUAL_FP32(-2.0, s5);
    ASSERT_EQUAL_FP32(-2.0, s6);
    ASSERT_EQUAL_FP32(INT64_MIN, s7);
    ASSERT_EQUAL_FP32(INT64_MIN, s8);
    ASSERT_EQUAL_FP32(0.0, s9);
    ASSERT_EQUAL_FP32(-0.0, s10);
    ASSERT_EQUAL_FP32(-0.0, s11);
    ASSERT_EQUAL_FP32(INT64_MIN, s12);  // Nan.
    ASSERT_EQUAL_FP32(INT64_MIN, s13);
    ASSERT_EQUAL_FP32(INT64_MIN + 0x80'00000000, s14);
    ASSERT_EQUAL_FP32(INT64_MIN, s15);  // Out of range.
    ASSERT_EQUAL_FP32(0x7fffff80'00000000, s16);
    ASSERT_EQUAL_FP32(0, s17);
    ASSERT_EQUAL_FP32(INT64_MIN, s18);
  }
}

TEST(frint64x_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFrintToFixedSizedInt);

  START();

  __ Fmov(d13, 1.0);
  __ Fmov(d14, 1.1);
  __ Fmov(d15, 1.5);
  __ Fmov(d16, 1.9);
  __ Fmov(d17, 2.5);
  __ Fmov(d18, -1.5);
  __ Fmov(d19, -2.5);
  __ Fmov(d20, kFP64PositiveInfinity);
  __ Fmov(d21, kFP64NegativeInfinity);
  __ Fmov(d22, 0.0);
  __ Fmov(d23, -0.0);
  __ Fmov(d24, -0.2);
  __ Fmov(d25, kFP64DefaultNaN);
  __ Fmov(d26, INT64_MIN);
  __ Fmov(d27, INT64_MIN + 0x400);  // The next representable FP64.
  __ Fmov(d28, 0x80000000'00000000);
  // The largest int64_t representable as FP64.
  __ Fmov(d29, 0x7fffffff'fffffc00);
  __ Fmov(d30, FLT_MIN);
  __ Fmov(d31, FLT_MAX);

  __ Frint64x(d0, d13);
  __ Frint64x(d1, d14);
  __ Frint64x(d2, d15);
  __ Frint64x(d3, d16);
  __ Frint64x(d4, d17);
  __ Frint64x(d5, d18);
  __ Frint64x(d6, d19);
  __ Frint64x(d7, d20);
  __ Frint64x(d8, d21);
  __ Frint64x(d9, d22);
  __ Frint64x(d10, d23);
  __ Frint64x(d11, d24);
  __ Frint64x(d12, d25);
  __ Frint64x(d13, d26);
  __ Frint64x(d14, d27);
  __ Frint64x(d15, d28);
  __ Frint64x(d16, d29);
  __ Frint64x(d17, d30);
  __ Frint64x(d18, d31);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP64(1.0, d0);
    ASSERT_EQUAL_FP64(1.0, d1);
    ASSERT_EQUAL_FP64(2.0, d2);
    ASSERT_EQUAL_FP64(2.0, d3);
    ASSERT_EQUAL_FP64(2.0, d4);
    ASSERT_EQUAL_FP64(-2.0, d5);
    ASSERT_EQUAL_FP64(-2.0, d6);
    ASSERT_EQUAL_FP64(INT64_MIN, d7);
    ASSERT_EQUAL_FP64(INT64_MIN, d8);
    ASSERT_EQUAL_FP64(0.0, d9);
    ASSERT_EQUAL_FP64(-0.0, d10);
    ASSERT_EQUAL_FP64(-0.0, d11);
    ASSERT_EQUAL_FP64(INT64_MIN, d12);  // NaN.
    ASSERT_EQUAL_FP64(INT64_MIN, d13);
    ASSERT_EQUAL_FP64(INT64_MIN + 0x400, d14);
    ASSERT_EQUAL_FP64(INT64_MIN, d15);  // Out of range.
    ASSERT_EQUAL_FP64(0x7fffffff'fffffc00, d16);
    ASSERT_EQUAL_FP64(0, d17);
    ASSERT_EQUAL_FP64(INT64_MIN, d18);
  }
}

TEST(frint64z_s) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFrintToFixedSizedInt);

  START();

  __ Fmov(s13, 1.0);
  __ Fmov(s14, 1.1);
  __ Fmov(s15, 1.5);
  __ Fmov(s16, 1.9);
  __ Fmov(s17, 2.5);
  __ Fmov(s18, -1.5);
  __ Fmov(s19, -2.5);
  __ Fmov(s20, kFP64PositiveInfinity);
  __ Fmov(s21, kFP64NegativeInfinity);
  __ Fmov(s22, 0.0);
  __ Fmov(s23, -0.0);
  __ Fmov(s24, -0.2);
  __ Fmov(s25, kFP64DefaultNaN);
  __ Fmov(s26, INT64_MIN);
  __ Fmov(s27, INT64_MIN + 0x80'00000000);  // The next representable FP32.
  __ Fmov(s28, 0x80000000'00000000);
  // The largest int64_t representable as FP32.
  __ Fmov(s29, 0x7fffff80'00000000);
  __ Fmov(s30, FLT_MIN);
  __ Fmov(s31, FLT_MAX);

  __ Frint64z(s0, s13);
  __ Frint64z(s1, s14);
  __ Frint64z(s2, s15);
  __ Frint64z(s3, s16);
  __ Frint64z(s4, s17);
  __ Frint64z(s5, s18);
  __ Frint64z(s6, s19);
  __ Frint64z(s7, s20);
  __ Frint64z(s8, s21);
  __ Frint64z(s9, s22);
  __ Frint64z(s10, s23);
  __ Frint64z(s11, s24);
  __ Frint64z(s12, s25);
  __ Frint64z(s13, s26);
  __ Frint64z(s14, s27);
  __ Frint64z(s15, s28);
  __ Frint64z(s16, s29);
  __ Frint64z(s17, s30);
  __ Frint64z(s18, s31);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(1.0, s2);
    ASSERT_EQUAL_FP32(1.0, s3);
    ASSERT_EQUAL_FP32(2.0, s4);
    ASSERT_EQUAL_FP32(-1.0, s5);
    ASSERT_EQUAL_FP32(-2.0, s6);
    ASSERT_EQUAL_FP32(INT64_MIN, s7);
    ASSERT_EQUAL_FP32(INT64_MIN, s8);
    ASSERT_EQUAL_FP32(0.0, s9);
    ASSERT_EQUAL_FP32(-0.0, s10);
    ASSERT_EQUAL_FP32(-0.0, s11);
    ASSERT_EQUAL_FP32(INT64_MIN, s12);  // Nan.
    ASSERT_EQUAL_FP32(INT64_MIN, s13);
    ASSERT_EQUAL_FP32(INT64_MIN + 0x80'00000000, s14);
    ASSERT_EQUAL_FP32(INT64_MIN, s15);  // Out of range.
    ASSERT_EQUAL_FP32(0x7fffff80'00000000, s16);
    ASSERT_EQUAL_FP32(0, s17);
    ASSERT_EQUAL_FP32(INT64_MIN, s18);
  }
}

TEST(frint64z_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFrintToFixedSizedInt);

  START();

  __ Fmov(d13, 1.0);
  __ Fmov(d14, 1.1);
  __ Fmov(d15, 1.5);
  __ Fmov(d16, 1.9);
  __ Fmov(d17, 2.5);
  __ Fmov(d18, -1.5);
  __ Fmov(d19, -2.5);
  __ Fmov(d20, kFP64PositiveInfinity);
  __ Fmov(d21, kFP64NegativeInfinity);
  __ Fmov(d22, 0.0);
  __ Fmov(d23, -0.0);
  __ Fmov(d24, -0.2);
  __ Fmov(d25, kFP64DefaultNaN);
  __ Fmov(d26, INT64_MIN);
  __ Fmov(d27, INT64_MIN + 0x400);  // The next representable FP64.
  __ Fmov(d28, 0x80000000'00000000);
  // The largest int64_t representable as FP64.
  __ Fmov(d29, 0x7fffffff'fffffc00);
  __ Fmov(d30, FLT_MIN);
  __ Fmov(d31, FLT_MAX);

  __ Frint64z(d0, d13);
  __ Frint64z(d1, d14);
  __ Frint64z(d2, d15);
  __ Frint64z(d3, d16);
  __ Frint64z(d4, d17);
  __ Frint64z(d5, d18);
  __ Frint64z(d6, d19);
  __ Frint64z(d7, d20);
  __ Frint64z(d8, d21);
  __ Frint64z(d9, d22);
  __ Frint64z(d10, d23);
  __ Frint64z(d11, d24);
  __ Frint64z(d12, d25);
  __ Frint64z(d13, d26);
  __ Frint64z(d14, d27);
  __ Frint64z(d15, d28);
  __ Frint64z(d16, d29);
  __ Frint64z(d17, d30);
  __ Frint64z(d18, d31);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP64(1.0, d0);
    ASSERT_EQUAL_FP64(1.0, d1);
    ASSERT_EQUAL_FP64(1.0, d2);
    ASSERT_EQUAL_FP64(1.0, d3);
    ASSERT_EQUAL_FP64(2.0, d4);
    ASSERT_EQUAL_FP64(-1.0, d5);
    ASSERT_EQUAL_FP64(-2.0, d6);
    ASSERT_EQUAL_FP64(INT64_MIN, d7);
    ASSERT_EQUAL_FP64(INT64_MIN, d8);
    ASSERT_EQUAL_FP64(0.0, d9);
    ASSERT_EQUAL_FP64(-0.0, d10);
    ASSERT_EQUAL_FP64(-0.0, d11);
    ASSERT_EQUAL_FP64(INT64_MIN, d12);  // NaN.
    ASSERT_EQUAL_FP64(INT64_MIN, d13);
    ASSERT_EQUAL_FP64(INT64_MIN + 0x400, d14);
    ASSERT_EQUAL_FP64(INT64_MIN, d15);  // Out of range.
    ASSERT_EQUAL_FP64(0x7fffffff'fffffc00, d16);
    ASSERT_EQUAL_FP64(0, d17);
    ASSERT_EQUAL_FP64(INT64_MIN, d18);
  }
}

TEST(frinta) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, 1.9);
  __ Fmov(s20, 2.5);
  __ Fmov(s21, -1.5);
  __ Fmov(s22, -2.5);
  __ Fmov(s23, kFP32PositiveInfinity);
  __ Fmov(s24, kFP32NegativeInfinity);
  __ Fmov(s25, 0.0);
  __ Fmov(s26, -0.0);
  __ Fmov(s27, -0.2);

  __ Frinta(s0, s16);
  __ Frinta(s1, s17);
  __ Frinta(s2, s18);
  __ Frinta(s3, s19);
  __ Frinta(s4, s20);
  __ Frinta(s5, s21);
  __ Frinta(s6, s22);
  __ Frinta(s7, s23);
  __ Frinta(s8, s24);
  __ Frinta(s9, s25);
  __ Frinta(s10, s26);
  __ Frinta(s11, s27);

  __ Fmov(d16, 1.0);
  __ Fmov(d17, 1.1);
  __ Fmov(d18, 1.5);
  __ Fmov(d19, 1.9);
  __ Fmov(d20, 2.5);
  __ Fmov(d21, -1.5);
  __ Fmov(d22, -2.5);
  __ Fmov(d23, kFP32PositiveInfinity);
  __ Fmov(d24, kFP32NegativeInfinity);
  __ Fmov(d25, 0.0);
  __ Fmov(d26, -0.0);
  __ Fmov(d27, -0.2);

  __ Frinta(d12, d16);
  __ Frinta(d13, d17);
  __ Frinta(d14, d18);
  __ Frinta(d15, d19);
  __ Frinta(d16, d20);
  __ Frinta(d17, d21);
  __ Frinta(d18, d22);
  __ Frinta(d19, d23);
  __ Frinta(d20, d24);
  __ Frinta(d21, d25);
  __ Frinta(d22, d26);
  __ Frinta(d23, d27);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(2.0, s2);
    ASSERT_EQUAL_FP32(2.0, s3);
    ASSERT_EQUAL_FP32(3.0, s4);
    ASSERT_EQUAL_FP32(-2.0, s5);
    ASSERT_EQUAL_FP32(-3.0, s6);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s7);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s8);
    ASSERT_EQUAL_FP32(0.0, s9);
    ASSERT_EQUAL_FP32(-0.0, s10);
    ASSERT_EQUAL_FP32(-0.0, s11);
    ASSERT_EQUAL_FP64(1.0, d12);
    ASSERT_EQUAL_FP64(1.0, d13);
    ASSERT_EQUAL_FP64(2.0, d14);
    ASSERT_EQUAL_FP64(2.0, d15);
    ASSERT_EQUAL_FP64(3.0, d16);
    ASSERT_EQUAL_FP64(-2.0, d17);
    ASSERT_EQUAL_FP64(-3.0, d18);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d19);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d20);
    ASSERT_EQUAL_FP64(0.0, d21);
    ASSERT_EQUAL_FP64(-0.0, d22);
    ASSERT_EQUAL_FP64(-0.0, d23);
  }
}


TEST(frinti) {
  // VIXL only supports the round-to-nearest FPCR mode, so this test has the
  // same results as frintn.
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, 1.9);
  __ Fmov(s20, 2.5);
  __ Fmov(s21, -1.5);
  __ Fmov(s22, -2.5);
  __ Fmov(s23, kFP32PositiveInfinity);
  __ Fmov(s24, kFP32NegativeInfinity);
  __ Fmov(s25, 0.0);
  __ Fmov(s26, -0.0);
  __ Fmov(s27, -0.2);

  __ Frinti(s0, s16);
  __ Frinti(s1, s17);
  __ Frinti(s2, s18);
  __ Frinti(s3, s19);
  __ Frinti(s4, s20);
  __ Frinti(s5, s21);
  __ Frinti(s6, s22);
  __ Frinti(s7, s23);
  __ Frinti(s8, s24);
  __ Frinti(s9, s25);
  __ Frinti(s10, s26);
  __ Frinti(s11, s27);

  __ Fmov(d16, 1.0);
  __ Fmov(d17, 1.1);
  __ Fmov(d18, 1.5);
  __ Fmov(d19, 1.9);
  __ Fmov(d20, 2.5);
  __ Fmov(d21, -1.5);
  __ Fmov(d22, -2.5);
  __ Fmov(d23, kFP32PositiveInfinity);
  __ Fmov(d24, kFP32NegativeInfinity);
  __ Fmov(d25, 0.0);
  __ Fmov(d26, -0.0);
  __ Fmov(d27, -0.2);

  __ Frinti(d12, d16);
  __ Frinti(d13, d17);
  __ Frinti(d14, d18);
  __ Frinti(d15, d19);
  __ Frinti(d16, d20);
  __ Frinti(d17, d21);
  __ Frinti(d18, d22);
  __ Frinti(d19, d23);
  __ Frinti(d20, d24);
  __ Frinti(d21, d25);
  __ Frinti(d22, d26);
  __ Frinti(d23, d27);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(2.0, s2);
    ASSERT_EQUAL_FP32(2.0, s3);
    ASSERT_EQUAL_FP32(2.0, s4);
    ASSERT_EQUAL_FP32(-2.0, s5);
    ASSERT_EQUAL_FP32(-2.0, s6);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s7);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s8);
    ASSERT_EQUAL_FP32(0.0, s9);
    ASSERT_EQUAL_FP32(-0.0, s10);
    ASSERT_EQUAL_FP32(-0.0, s11);
    ASSERT_EQUAL_FP64(1.0, d12);
    ASSERT_EQUAL_FP64(1.0, d13);
    ASSERT_EQUAL_FP64(2.0, d14);
    ASSERT_EQUAL_FP64(2.0, d15);
    ASSERT_EQUAL_FP64(2.0, d16);
    ASSERT_EQUAL_FP64(-2.0, d17);
    ASSERT_EQUAL_FP64(-2.0, d18);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d19);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d20);
    ASSERT_EQUAL_FP64(0.0, d21);
    ASSERT_EQUAL_FP64(-0.0, d22);
    ASSERT_EQUAL_FP64(-0.0, d23);
  }
}


TEST(frintm) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, 1.9);
  __ Fmov(s20, 2.5);
  __ Fmov(s21, -1.5);
  __ Fmov(s22, -2.5);
  __ Fmov(s23, kFP32PositiveInfinity);
  __ Fmov(s24, kFP32NegativeInfinity);
  __ Fmov(s25, 0.0);
  __ Fmov(s26, -0.0);
  __ Fmov(s27, -0.2);

  __ Frintm(s0, s16);
  __ Frintm(s1, s17);
  __ Frintm(s2, s18);
  __ Frintm(s3, s19);
  __ Frintm(s4, s20);
  __ Frintm(s5, s21);
  __ Frintm(s6, s22);
  __ Frintm(s7, s23);
  __ Frintm(s8, s24);
  __ Frintm(s9, s25);
  __ Frintm(s10, s26);
  __ Frintm(s11, s27);

  __ Fmov(d16, 1.0);
  __ Fmov(d17, 1.1);
  __ Fmov(d18, 1.5);
  __ Fmov(d19, 1.9);
  __ Fmov(d20, 2.5);
  __ Fmov(d21, -1.5);
  __ Fmov(d22, -2.5);
  __ Fmov(d23, kFP32PositiveInfinity);
  __ Fmov(d24, kFP32NegativeInfinity);
  __ Fmov(d25, 0.0);
  __ Fmov(d26, -0.0);
  __ Fmov(d27, -0.2);

  __ Frintm(d12, d16);
  __ Frintm(d13, d17);
  __ Frintm(d14, d18);
  __ Frintm(d15, d19);
  __ Frintm(d16, d20);
  __ Frintm(d17, d21);
  __ Frintm(d18, d22);
  __ Frintm(d19, d23);
  __ Frintm(d20, d24);
  __ Frintm(d21, d25);
  __ Frintm(d22, d26);
  __ Frintm(d23, d27);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(1.0, s2);
    ASSERT_EQUAL_FP32(1.0, s3);
    ASSERT_EQUAL_FP32(2.0, s4);
    ASSERT_EQUAL_FP32(-2.0, s5);
    ASSERT_EQUAL_FP32(-3.0, s6);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s7);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s8);
    ASSERT_EQUAL_FP32(0.0, s9);
    ASSERT_EQUAL_FP32(-0.0, s10);
    ASSERT_EQUAL_FP32(-1.0, s11);
    ASSERT_EQUAL_FP64(1.0, d12);
    ASSERT_EQUAL_FP64(1.0, d13);
    ASSERT_EQUAL_FP64(1.0, d14);
    ASSERT_EQUAL_FP64(1.0, d15);
    ASSERT_EQUAL_FP64(2.0, d16);
    ASSERT_EQUAL_FP64(-2.0, d17);
    ASSERT_EQUAL_FP64(-3.0, d18);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d19);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d20);
    ASSERT_EQUAL_FP64(0.0, d21);
    ASSERT_EQUAL_FP64(-0.0, d22);
    ASSERT_EQUAL_FP64(-1.0, d23);
  }
}


TEST(frintn) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, 1.9);
  __ Fmov(s20, 2.5);
  __ Fmov(s21, -1.5);
  __ Fmov(s22, -2.5);
  __ Fmov(s23, kFP32PositiveInfinity);
  __ Fmov(s24, kFP32NegativeInfinity);
  __ Fmov(s25, 0.0);
  __ Fmov(s26, -0.0);
  __ Fmov(s27, -0.2);

  __ Frintn(s0, s16);
  __ Frintn(s1, s17);
  __ Frintn(s2, s18);
  __ Frintn(s3, s19);
  __ Frintn(s4, s20);
  __ Frintn(s5, s21);
  __ Frintn(s6, s22);
  __ Frintn(s7, s23);
  __ Frintn(s8, s24);
  __ Frintn(s9, s25);
  __ Frintn(s10, s26);
  __ Frintn(s11, s27);

  __ Fmov(d16, 1.0);
  __ Fmov(d17, 1.1);
  __ Fmov(d18, 1.5);
  __ Fmov(d19, 1.9);
  __ Fmov(d20, 2.5);
  __ Fmov(d21, -1.5);
  __ Fmov(d22, -2.5);
  __ Fmov(d23, kFP32PositiveInfinity);
  __ Fmov(d24, kFP32NegativeInfinity);
  __ Fmov(d25, 0.0);
  __ Fmov(d26, -0.0);
  __ Fmov(d27, -0.2);

  __ Frintn(d12, d16);
  __ Frintn(d13, d17);
  __ Frintn(d14, d18);
  __ Frintn(d15, d19);
  __ Frintn(d16, d20);
  __ Frintn(d17, d21);
  __ Frintn(d18, d22);
  __ Frintn(d19, d23);
  __ Frintn(d20, d24);
  __ Frintn(d21, d25);
  __ Frintn(d22, d26);
  __ Frintn(d23, d27);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(2.0, s2);
    ASSERT_EQUAL_FP32(2.0, s3);
    ASSERT_EQUAL_FP32(2.0, s4);
    ASSERT_EQUAL_FP32(-2.0, s5);
    ASSERT_EQUAL_FP32(-2.0, s6);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s7);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s8);
    ASSERT_EQUAL_FP32(0.0, s9);
    ASSERT_EQUAL_FP32(-0.0, s10);
    ASSERT_EQUAL_FP32(-0.0, s11);
    ASSERT_EQUAL_FP64(1.0, d12);
    ASSERT_EQUAL_FP64(1.0, d13);
    ASSERT_EQUAL_FP64(2.0, d14);
    ASSERT_EQUAL_FP64(2.0, d15);
    ASSERT_EQUAL_FP64(2.0, d16);
    ASSERT_EQUAL_FP64(-2.0, d17);
    ASSERT_EQUAL_FP64(-2.0, d18);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d19);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d20);
    ASSERT_EQUAL_FP64(0.0, d21);
    ASSERT_EQUAL_FP64(-0.0, d22);
    ASSERT_EQUAL_FP64(-0.0, d23);
  }
}


TEST(frintp) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, 1.9);
  __ Fmov(s20, 2.5);
  __ Fmov(s21, -1.5);
  __ Fmov(s22, -2.5);
  __ Fmov(s23, kFP32PositiveInfinity);
  __ Fmov(s24, kFP32NegativeInfinity);
  __ Fmov(s25, 0.0);
  __ Fmov(s26, -0.0);
  __ Fmov(s27, -0.2);

  __ Frintp(s0, s16);
  __ Frintp(s1, s17);
  __ Frintp(s2, s18);
  __ Frintp(s3, s19);
  __ Frintp(s4, s20);
  __ Frintp(s5, s21);
  __ Frintp(s6, s22);
  __ Frintp(s7, s23);
  __ Frintp(s8, s24);
  __ Frintp(s9, s25);
  __ Frintp(s10, s26);
  __ Frintp(s11, s27);

  __ Fmov(d16, 1.0);
  __ Fmov(d17, 1.1);
  __ Fmov(d18, 1.5);
  __ Fmov(d19, 1.9);
  __ Fmov(d20, 2.5);
  __ Fmov(d21, -1.5);
  __ Fmov(d22, -2.5);
  __ Fmov(d23, kFP32PositiveInfinity);
  __ Fmov(d24, kFP32NegativeInfinity);
  __ Fmov(d25, 0.0);
  __ Fmov(d26, -0.0);
  __ Fmov(d27, -0.2);

  __ Frintp(d12, d16);
  __ Frintp(d13, d17);
  __ Frintp(d14, d18);
  __ Frintp(d15, d19);
  __ Frintp(d16, d20);
  __ Frintp(d17, d21);
  __ Frintp(d18, d22);
  __ Frintp(d19, d23);
  __ Frintp(d20, d24);
  __ Frintp(d21, d25);
  __ Frintp(d22, d26);
  __ Frintp(d23, d27);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(2.0, s1);
    ASSERT_EQUAL_FP32(2.0, s2);
    ASSERT_EQUAL_FP32(2.0, s3);
    ASSERT_EQUAL_FP32(3.0, s4);
    ASSERT_EQUAL_FP32(-1.0, s5);
    ASSERT_EQUAL_FP32(-2.0, s6);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s7);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s8);
    ASSERT_EQUAL_FP32(0.0, s9);
    ASSERT_EQUAL_FP32(-0.0, s10);
    ASSERT_EQUAL_FP32(-0.0, s11);
    ASSERT_EQUAL_FP64(1.0, d12);
    ASSERT_EQUAL_FP64(2.0, d13);
    ASSERT_EQUAL_FP64(2.0, d14);
    ASSERT_EQUAL_FP64(2.0, d15);
    ASSERT_EQUAL_FP64(3.0, d16);
    ASSERT_EQUAL_FP64(-1.0, d17);
    ASSERT_EQUAL_FP64(-2.0, d18);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d19);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d20);
    ASSERT_EQUAL_FP64(0.0, d21);
    ASSERT_EQUAL_FP64(-0.0, d22);
    ASSERT_EQUAL_FP64(-0.0, d23);
  }
}


TEST(frintx) {
  // VIXL only supports the round-to-nearest FPCR mode, and it doesn't support
  // FP exceptions, so this test has the same results as frintn (and frinti).
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, 1.9);
  __ Fmov(s20, 2.5);
  __ Fmov(s21, -1.5);
  __ Fmov(s22, -2.5);
  __ Fmov(s23, kFP32PositiveInfinity);
  __ Fmov(s24, kFP32NegativeInfinity);
  __ Fmov(s25, 0.0);
  __ Fmov(s26, -0.0);
  __ Fmov(s27, -0.2);

  __ Frintx(s0, s16);
  __ Frintx(s1, s17);
  __ Frintx(s2, s18);
  __ Frintx(s3, s19);
  __ Frintx(s4, s20);
  __ Frintx(s5, s21);
  __ Frintx(s6, s22);
  __ Frintx(s7, s23);
  __ Frintx(s8, s24);
  __ Frintx(s9, s25);
  __ Frintx(s10, s26);
  __ Frintx(s11, s27);

  __ Fmov(d16, 1.0);
  __ Fmov(d17, 1.1);
  __ Fmov(d18, 1.5);
  __ Fmov(d19, 1.9);
  __ Fmov(d20, 2.5);
  __ Fmov(d21, -1.5);
  __ Fmov(d22, -2.5);
  __ Fmov(d23, kFP32PositiveInfinity);
  __ Fmov(d24, kFP32NegativeInfinity);
  __ Fmov(d25, 0.0);
  __ Fmov(d26, -0.0);
  __ Fmov(d27, -0.2);

  __ Frintx(d12, d16);
  __ Frintx(d13, d17);
  __ Frintx(d14, d18);
  __ Frintx(d15, d19);
  __ Frintx(d16, d20);
  __ Frintx(d17, d21);
  __ Frintx(d18, d22);
  __ Frintx(d19, d23);
  __ Frintx(d20, d24);
  __ Frintx(d21, d25);
  __ Frintx(d22, d26);
  __ Frintx(d23, d27);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(2.0, s2);
    ASSERT_EQUAL_FP32(2.0, s3);
    ASSERT_EQUAL_FP32(2.0, s4);
    ASSERT_EQUAL_FP32(-2.0, s5);
    ASSERT_EQUAL_FP32(-2.0, s6);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s7);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s8);
    ASSERT_EQUAL_FP32(0.0, s9);
    ASSERT_EQUAL_FP32(-0.0, s10);
    ASSERT_EQUAL_FP32(-0.0, s11);
    ASSERT_EQUAL_FP64(1.0, d12);
    ASSERT_EQUAL_FP64(1.0, d13);
    ASSERT_EQUAL_FP64(2.0, d14);
    ASSERT_EQUAL_FP64(2.0, d15);
    ASSERT_EQUAL_FP64(2.0, d16);
    ASSERT_EQUAL_FP64(-2.0, d17);
    ASSERT_EQUAL_FP64(-2.0, d18);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d19);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d20);
    ASSERT_EQUAL_FP64(0.0, d21);
    ASSERT_EQUAL_FP64(-0.0, d22);
    ASSERT_EQUAL_FP64(-0.0, d23);
  }
}


TEST(frintz) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, 1.9);
  __ Fmov(s20, 2.5);
  __ Fmov(s21, -1.5);
  __ Fmov(s22, -2.5);
  __ Fmov(s23, kFP32PositiveInfinity);
  __ Fmov(s24, kFP32NegativeInfinity);
  __ Fmov(s25, 0.0);
  __ Fmov(s26, -0.0);

  __ Frintz(s0, s16);
  __ Frintz(s1, s17);
  __ Frintz(s2, s18);
  __ Frintz(s3, s19);
  __ Frintz(s4, s20);
  __ Frintz(s5, s21);
  __ Frintz(s6, s22);
  __ Frintz(s7, s23);
  __ Frintz(s8, s24);
  __ Frintz(s9, s25);
  __ Frintz(s10, s26);

  __ Fmov(d16, 1.0);
  __ Fmov(d17, 1.1);
  __ Fmov(d18, 1.5);
  __ Fmov(d19, 1.9);
  __ Fmov(d20, 2.5);
  __ Fmov(d21, -1.5);
  __ Fmov(d22, -2.5);
  __ Fmov(d23, kFP32PositiveInfinity);
  __ Fmov(d24, kFP32NegativeInfinity);
  __ Fmov(d25, 0.0);
  __ Fmov(d26, -0.0);

  __ Frintz(d11, d16);
  __ Frintz(d12, d17);
  __ Frintz(d13, d18);
  __ Frintz(d14, d19);
  __ Frintz(d15, d20);
  __ Frintz(d16, d21);
  __ Frintz(d17, d22);
  __ Frintz(d18, d23);
  __ Frintz(d19, d24);
  __ Frintz(d20, d25);
  __ Frintz(d21, d26);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(1.0, s1);
    ASSERT_EQUAL_FP32(1.0, s2);
    ASSERT_EQUAL_FP32(1.0, s3);
    ASSERT_EQUAL_FP32(2.0, s4);
    ASSERT_EQUAL_FP32(-1.0, s5);
    ASSERT_EQUAL_FP32(-2.0, s6);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s7);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s8);
    ASSERT_EQUAL_FP32(0.0, s9);
    ASSERT_EQUAL_FP32(-0.0, s10);
    ASSERT_EQUAL_FP64(1.0, d11);
    ASSERT_EQUAL_FP64(1.0, d12);
    ASSERT_EQUAL_FP64(1.0, d13);
    ASSERT_EQUAL_FP64(1.0, d14);
    ASSERT_EQUAL_FP64(2.0, d15);
    ASSERT_EQUAL_FP64(-1.0, d16);
    ASSERT_EQUAL_FP64(-2.0, d17);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d18);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d19);
    ASSERT_EQUAL_FP64(0.0, d20);
    ASSERT_EQUAL_FP64(-0.0, d21);
  }
}


TEST(fcvt_ds) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, 1.9);
  __ Fmov(s20, 2.5);
  __ Fmov(s21, -1.5);
  __ Fmov(s22, -2.5);
  __ Fmov(s23, kFP32PositiveInfinity);
  __ Fmov(s24, kFP32NegativeInfinity);
  __ Fmov(s25, 0.0);
  __ Fmov(s26, -0.0);
  __ Fmov(s27, FLT_MAX);
  __ Fmov(s28, FLT_MIN);
  __ Fmov(s29, RawbitsToFloat(0x7fc12345));  // Quiet NaN.
  __ Fmov(s30, RawbitsToFloat(0x7f812345));  // Signalling NaN.

  __ Fcvt(d0, s16);
  __ Fcvt(d1, s17);
  __ Fcvt(d2, s18);
  __ Fcvt(d3, s19);
  __ Fcvt(d4, s20);
  __ Fcvt(d5, s21);
  __ Fcvt(d6, s22);
  __ Fcvt(d7, s23);
  __ Fcvt(d8, s24);
  __ Fcvt(d9, s25);
  __ Fcvt(d10, s26);
  __ Fcvt(d11, s27);
  __ Fcvt(d12, s28);
  __ Fcvt(d13, s29);
  __ Fcvt(d14, s30);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP64(1.0f, d0);
    ASSERT_EQUAL_FP64(1.1f, d1);
    ASSERT_EQUAL_FP64(1.5f, d2);
    ASSERT_EQUAL_FP64(1.9f, d3);
    ASSERT_EQUAL_FP64(2.5f, d4);
    ASSERT_EQUAL_FP64(-1.5f, d5);
    ASSERT_EQUAL_FP64(-2.5f, d6);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d7);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d8);
    ASSERT_EQUAL_FP64(0.0f, d9);
    ASSERT_EQUAL_FP64(-0.0f, d10);
    ASSERT_EQUAL_FP64(FLT_MAX, d11);
    ASSERT_EQUAL_FP64(FLT_MIN, d12);

    // Check that the NaN payload is preserved according to Aarch64 conversion
    // rules:
    //  - The sign bit is preserved.
    //  - The top bit of the mantissa is forced to 1 (making it a quiet NaN).
    //  - The remaining mantissa bits are copied until they run out.
    //  - The low-order bits that haven't already been assigned are set to 0.
    ASSERT_EQUAL_FP64(RawbitsToDouble(0x7ff82468a0000000), d13);
    ASSERT_EQUAL_FP64(RawbitsToDouble(0x7ff82468a0000000), d14);
  }
}


TEST(fcvt_sd) {
  // Test simple conversions here. Complex behaviour (such as rounding
  // specifics) are tested in the simulator tests.

  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(d16, 1.0);
  __ Fmov(d17, 1.1);
  __ Fmov(d18, 1.5);
  __ Fmov(d19, 1.9);
  __ Fmov(d20, 2.5);
  __ Fmov(d21, -1.5);
  __ Fmov(d22, -2.5);
  __ Fmov(d23, kFP32PositiveInfinity);
  __ Fmov(d24, kFP32NegativeInfinity);
  __ Fmov(d25, 0.0);
  __ Fmov(d26, -0.0);
  __ Fmov(d27, FLT_MAX);
  __ Fmov(d28, FLT_MIN);
  __ Fmov(d29, RawbitsToDouble(0x7ff82468a0000000));  // Quiet NaN.
  __ Fmov(d30, RawbitsToDouble(0x7ff02468a0000000));  // Signalling NaN.

  __ Fcvt(s0, d16);
  __ Fcvt(s1, d17);
  __ Fcvt(s2, d18);
  __ Fcvt(s3, d19);
  __ Fcvt(s4, d20);
  __ Fcvt(s5, d21);
  __ Fcvt(s6, d22);
  __ Fcvt(s7, d23);
  __ Fcvt(s8, d24);
  __ Fcvt(s9, d25);
  __ Fcvt(s10, d26);
  __ Fcvt(s11, d27);
  __ Fcvt(s12, d28);
  __ Fcvt(s13, d29);
  __ Fcvt(s14, d30);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0f, s0);
    ASSERT_EQUAL_FP32(1.1f, s1);
    ASSERT_EQUAL_FP32(1.5f, s2);
    ASSERT_EQUAL_FP32(1.9f, s3);
    ASSERT_EQUAL_FP32(2.5f, s4);
    ASSERT_EQUAL_FP32(-1.5f, s5);
    ASSERT_EQUAL_FP32(-2.5f, s6);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s7);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s8);
    ASSERT_EQUAL_FP32(0.0f, s9);
    ASSERT_EQUAL_FP32(-0.0f, s10);
    ASSERT_EQUAL_FP32(FLT_MAX, s11);
    ASSERT_EQUAL_FP32(FLT_MIN, s12);

    // Check that the NaN payload is preserved according to Aarch64 conversion
    // rules:
    //  - The sign bit is preserved.
    //  - The top bit of the mantissa is forced to 1 (making it a quiet NaN).
    //  - The remaining mantissa bits are copied until they run out.
    //  - The low-order bits that haven't already been assigned are set to 0.
    ASSERT_EQUAL_FP32(RawbitsToFloat(0x7fc12345), s13);
    ASSERT_EQUAL_FP32(RawbitsToFloat(0x7fc12345), s14);
  }
}


TEST(fcvt_half) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  Label done;
  {
    // Check all exact conversions from half to float and back.
    Label ok, fail;
    __ Mov(w0, 0);
    for (int i = 0; i < 0xffff; i += 3) {
      if ((i & 0x7c00) == 0x7c00) continue;
      __ Mov(w1, i);
      __ Fmov(s1, w1);
      __ Fcvt(s2, h1);
      __ Fcvt(h2, s2);
      __ Fmov(w2, s2);
      __ Cmp(w1, w2);
      __ B(&fail, ne);
    }
    __ B(&ok);
    __ Bind(&fail);
    __ Mov(w0, 1);
    __ B(&done);
    __ Bind(&ok);
  }
  {
    // Check all exact conversions from half to double and back.
    Label ok, fail;
    for (int i = 0; i < 0xffff; i += 3) {
      if ((i & 0x7c00) == 0x7c00) continue;
      __ Mov(w1, i);
      __ Fmov(s1, w1);
      __ Fcvt(d2, h1);
      __ Fcvt(h2, d2);
      __ Fmov(w2, s2);
      __ Cmp(w1, w2);
      __ B(&fail, ne);
    }
    __ B(&ok);
    __ Bind(&fail);
    __ Mov(w0, 2);
    __ Bind(&ok);
  }
  __ Bind(&done);

  // Check some other interesting values.
  __ Fmov(s0, kFP32PositiveInfinity);
  __ Fmov(s1, kFP32NegativeInfinity);
  __ Fmov(s2, 65504);       // Max half precision.
  __ Fmov(s3, 6.10352e-5);  // Min positive normal.
  __ Fmov(s4, 6.09756e-5);  // Max subnormal.
  __ Fmov(s5, 5.96046e-8);  // Min positive subnormal.
  __ Fmov(s6, 5e-9);        // Not representable -> zero.
  __ Fmov(s7, -0.0);
  __ Fcvt(h0, s0);
  __ Fcvt(h1, s1);
  __ Fcvt(h2, s2);
  __ Fcvt(h3, s3);
  __ Fcvt(h4, s4);
  __ Fcvt(h5, s5);
  __ Fcvt(h6, s6);
  __ Fcvt(h7, s7);

  __ Fmov(d20, kFP64PositiveInfinity);
  __ Fmov(d21, kFP64NegativeInfinity);
  __ Fmov(d22, 65504);       // Max half precision.
  __ Fmov(d23, 6.10352e-5);  // Min positive normal.
  __ Fmov(d24, 6.09756e-5);  // Max subnormal.
  __ Fmov(d25, 5.96046e-8);  // Min positive subnormal.
  __ Fmov(d26, 5e-9);        // Not representable -> zero.
  __ Fmov(d27, -0.0);
  __ Fcvt(h20, d20);
  __ Fcvt(h21, d21);
  __ Fcvt(h22, d22);
  __ Fcvt(h23, d23);
  __ Fcvt(h24, d24);
  __ Fcvt(h25, d25);
  __ Fcvt(h26, d26);
  __ Fcvt(h27, d27);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_32(0, w0);  // 1 => float failed, 2 => double failed.
    ASSERT_EQUAL_128(0, Float16ToRawbits(kFP16PositiveInfinity), q0);
    ASSERT_EQUAL_128(0, Float16ToRawbits(kFP16NegativeInfinity), q1);
    ASSERT_EQUAL_128(0, 0x7bff, q2);
    ASSERT_EQUAL_128(0, 0x0400, q3);
    ASSERT_EQUAL_128(0, 0x03ff, q4);
    ASSERT_EQUAL_128(0, 0x0001, q5);
    ASSERT_EQUAL_128(0, 0, q6);
    ASSERT_EQUAL_128(0, 0x8000, q7);
    ASSERT_EQUAL_128(0, Float16ToRawbits(kFP16PositiveInfinity), q20);
    ASSERT_EQUAL_128(0, Float16ToRawbits(kFP16NegativeInfinity), q21);
    ASSERT_EQUAL_128(0, 0x7bff, q22);
    ASSERT_EQUAL_128(0, 0x0400, q23);
    ASSERT_EQUAL_128(0, 0x03ff, q24);
    ASSERT_EQUAL_128(0, 0x0001, q25);
    ASSERT_EQUAL_128(0, 0, q26);
    ASSERT_EQUAL_128(0, 0x8000, q27);
  }
}


TEST(fcvtas) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s0, 1.0);
  __ Fmov(s1, 1.1);
  __ Fmov(s2, 2.5);
  __ Fmov(s3, -2.5);
  __ Fmov(s4, kFP32PositiveInfinity);
  __ Fmov(s5, kFP32NegativeInfinity);
  __ Fmov(s6, 0x7fffff80);  // Largest float < INT32_MAX.
  __ Fneg(s7, s6);          // Smallest float > INT32_MIN.
  __ Fmov(d8, 1.0);
  __ Fmov(d9, 1.1);
  __ Fmov(d10, 2.5);
  __ Fmov(d11, -2.5);
  __ Fmov(d12, kFP64PositiveInfinity);
  __ Fmov(d13, kFP64NegativeInfinity);
  __ Fmov(d14, kWMaxInt - 1);
  __ Fmov(d15, kWMinInt + 1);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 2.5);
  __ Fmov(s19, -2.5);
  __ Fmov(s20, kFP32PositiveInfinity);
  __ Fmov(s21, kFP32NegativeInfinity);
  __ Fmov(s22, 0x7fffff8000000000);  // Largest float < INT64_MAX.
  __ Fneg(s23, s22);                 // Smallest float > INT64_MIN.
  __ Fmov(d24, 1.1);
  __ Fmov(d25, 2.5);
  __ Fmov(d26, -2.5);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0x7ffffffffffffc00);  // Largest double < INT64_MAX.
  __ Fneg(d30, d29);                 // Smallest double > INT64_MIN.

  __ Fcvtas(w0, s0);
  __ Fcvtas(w1, s1);
  __ Fcvtas(w2, s2);
  __ Fcvtas(w3, s3);
  __ Fcvtas(w4, s4);
  __ Fcvtas(w5, s5);
  __ Fcvtas(w6, s6);
  __ Fcvtas(w7, s7);
  __ Fcvtas(w8, d8);
  __ Fcvtas(w9, d9);
  __ Fcvtas(w10, d10);
  __ Fcvtas(w11, d11);
  __ Fcvtas(w12, d12);
  __ Fcvtas(w13, d13);
  __ Fcvtas(w14, d14);
  __ Fcvtas(w15, d15);
  __ Fcvtas(x17, s17);
  __ Fcvtas(x18, s18);
  __ Fcvtas(x19, s19);
  __ Fcvtas(x20, s20);
  __ Fcvtas(x21, s21);
  __ Fcvtas(x22, s22);
  __ Fcvtas(x23, s23);
  __ Fcvtas(x24, d24);
  __ Fcvtas(x25, d25);
  __ Fcvtas(x26, d26);
  __ Fcvtas(x27, d27);
  __ Fcvtas(x28, d28);
  __ Fcvtas(x29, d29);
  __ Fcvtas(x30, d30);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_64(1, x0);
    ASSERT_EQUAL_64(1, x1);
    ASSERT_EQUAL_64(3, x2);
    ASSERT_EQUAL_64(0xfffffffd, x3);
    ASSERT_EQUAL_64(0x7fffffff, x4);
    ASSERT_EQUAL_64(0x80000000, x5);
    ASSERT_EQUAL_64(0x7fffff80, x6);
    ASSERT_EQUAL_64(0x80000080, x7);
    ASSERT_EQUAL_64(1, x8);
    ASSERT_EQUAL_64(1, x9);
    ASSERT_EQUAL_64(3, x10);
    ASSERT_EQUAL_64(0xfffffffd, x11);
    ASSERT_EQUAL_64(0x7fffffff, x12);
    ASSERT_EQUAL_64(0x80000000, x13);
    ASSERT_EQUAL_64(0x7ffffffe, x14);
    ASSERT_EQUAL_64(0x80000001, x15);
    ASSERT_EQUAL_64(1, x17);
    ASSERT_EQUAL_64(3, x18);
    ASSERT_EQUAL_64(0xfffffffffffffffd, x19);
    ASSERT_EQUAL_64(0x7fffffffffffffff, x20);
    ASSERT_EQUAL_64(0x8000000000000000, x21);
    ASSERT_EQUAL_64(0x7fffff8000000000, x22);
    ASSERT_EQUAL_64(0x8000008000000000, x23);
    ASSERT_EQUAL_64(1, x24);
    ASSERT_EQUAL_64(3, x25);
    ASSERT_EQUAL_64(0xfffffffffffffffd, x26);
    ASSERT_EQUAL_64(0x7fffffffffffffff, x27);
    ASSERT_EQUAL_64(0x8000000000000000, x28);
    ASSERT_EQUAL_64(0x7ffffffffffffc00, x29);
    ASSERT_EQUAL_64(0x8000000000000400, x30);
  }
}


TEST(fcvtau) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s0, 1.0);
  __ Fmov(s1, 1.1);
  __ Fmov(s2, 2.5);
  __ Fmov(s3, -2.5);
  __ Fmov(s4, kFP32PositiveInfinity);
  __ Fmov(s5, kFP32NegativeInfinity);
  __ Fmov(s6, 0xffffff00);  // Largest float < UINT32_MAX.
  __ Fmov(d8, 1.0);
  __ Fmov(d9, 1.1);
  __ Fmov(d10, 2.5);
  __ Fmov(d11, -2.5);
  __ Fmov(d12, kFP64PositiveInfinity);
  __ Fmov(d13, kFP64NegativeInfinity);
  __ Fmov(d14, 0xfffffffe);
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 2.5);
  __ Fmov(s19, -2.5);
  __ Fmov(s20, kFP32PositiveInfinity);
  __ Fmov(s21, kFP32NegativeInfinity);
  __ Fmov(s22, 0xffffff0000000000);  // Largest float < UINT64_MAX.
  __ Fmov(d24, 1.1);
  __ Fmov(d25, 2.5);
  __ Fmov(d26, -2.5);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0xfffffffffffff800);  // Largest double < UINT64_MAX.
  __ Fmov(s30, 0x100000000);

  __ Fcvtau(w0, s0);
  __ Fcvtau(w1, s1);
  __ Fcvtau(w2, s2);
  __ Fcvtau(w3, s3);
  __ Fcvtau(w4, s4);
  __ Fcvtau(w5, s5);
  __ Fcvtau(w6, s6);
  __ Fcvtau(w8, d8);
  __ Fcvtau(w9, d9);
  __ Fcvtau(w10, d10);
  __ Fcvtau(w11, d11);
  __ Fcvtau(w12, d12);
  __ Fcvtau(w13, d13);
  __ Fcvtau(w14, d14);
  __ Fcvtau(w15, d15);
  __ Fcvtau(x16, s16);
  __ Fcvtau(x17, s17);
  __ Fcvtau(x18, s18);
  __ Fcvtau(x19, s19);
  __ Fcvtau(x20, s20);
  __ Fcvtau(x21, s21);
  __ Fcvtau(x22, s22);
  __ Fcvtau(x24, d24);
  __ Fcvtau(x25, d25);
  __ Fcvtau(x26, d26);
  __ Fcvtau(x27, d27);
  __ Fcvtau(x28, d28);
  __ Fcvtau(x29, d29);
  __ Fcvtau(w30, s30);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_64(1, x0);
    ASSERT_EQUAL_64(1, x1);
    ASSERT_EQUAL_64(3, x2);
    ASSERT_EQUAL_64(0, x3);
    ASSERT_EQUAL_64(0xffffffff, x4);
    ASSERT_EQUAL_64(0, x5);
    ASSERT_EQUAL_64(0xffffff00, x6);
    ASSERT_EQUAL_64(1, x8);
    ASSERT_EQUAL_64(1, x9);
    ASSERT_EQUAL_64(3, x10);
    ASSERT_EQUAL_64(0, x11);
    ASSERT_EQUAL_64(0xffffffff, x12);
    ASSERT_EQUAL_64(0, x13);
    ASSERT_EQUAL_64(0xfffffffe, x14);
    ASSERT_EQUAL_64(1, x16);
    ASSERT_EQUAL_64(1, x17);
    ASSERT_EQUAL_64(3, x18);
    ASSERT_EQUAL_64(0, x19);
    ASSERT_EQUAL_64(0xffffffffffffffff, x20);
    ASSERT_EQUAL_64(0, x21);
    ASSERT_EQUAL_64(0xffffff0000000000, x22);
    ASSERT_EQUAL_64(1, x24);
    ASSERT_EQUAL_64(3, x25);
    ASSERT_EQUAL_64(0, x26);
    ASSERT_EQUAL_64(0xffffffffffffffff, x27);
    ASSERT_EQUAL_64(0, x28);
    ASSERT_EQUAL_64(0xfffffffffffff800, x29);
    ASSERT_EQUAL_64(0xffffffff, x30);
  }
}


TEST(fcvtms) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s0, 1.0);
  __ Fmov(s1, 1.1);
  __ Fmov(s2, 1.5);
  __ Fmov(s3, -1.5);
  __ Fmov(s4, kFP32PositiveInfinity);
  __ Fmov(s5, kFP32NegativeInfinity);
  __ Fmov(s6, 0x7fffff80);  // Largest float < INT32_MAX.
  __ Fneg(s7, s6);          // Smallest float > INT32_MIN.
  __ Fmov(d8, 1.0);
  __ Fmov(d9, 1.1);
  __ Fmov(d10, 1.5);
  __ Fmov(d11, -1.5);
  __ Fmov(d12, kFP64PositiveInfinity);
  __ Fmov(d13, kFP64NegativeInfinity);
  __ Fmov(d14, kWMaxInt - 1);
  __ Fmov(d15, kWMinInt + 1);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, -1.5);
  __ Fmov(s20, kFP32PositiveInfinity);
  __ Fmov(s21, kFP32NegativeInfinity);
  __ Fmov(s22, 0x7fffff8000000000);  // Largest float < INT64_MAX.
  __ Fneg(s23, s22);                 // Smallest float > INT64_MIN.
  __ Fmov(d24, 1.1);
  __ Fmov(d25, 1.5);
  __ Fmov(d26, -1.5);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0x7ffffffffffffc00);  // Largest double < INT64_MAX.
  __ Fneg(d30, d29);                 // Smallest double > INT64_MIN.

  __ Fcvtms(w0, s0);
  __ Fcvtms(w1, s1);
  __ Fcvtms(w2, s2);
  __ Fcvtms(w3, s3);
  __ Fcvtms(w4, s4);
  __ Fcvtms(w5, s5);
  __ Fcvtms(w6, s6);
  __ Fcvtms(w7, s7);
  __ Fcvtms(w8, d8);
  __ Fcvtms(w9, d9);
  __ Fcvtms(w10, d10);
  __ Fcvtms(w11, d11);
  __ Fcvtms(w12, d12);
  __ Fcvtms(w13, d13);
  __ Fcvtms(w14, d14);
  __ Fcvtms(w15, d15);
  __ Fcvtms(x17, s17);
  __ Fcvtms(x18, s18);
  __ Fcvtms(x19, s19);
  __ Fcvtms(x20, s20);
  __ Fcvtms(x21, s21);
  __ Fcvtms(x22, s22);
  __ Fcvtms(x23, s23);
  __ Fcvtms(x24, d24);
  __ Fcvtms(x25, d25);
  __ Fcvtms(x26, d26);
  __ Fcvtms(x27, d27);
  __ Fcvtms(x28, d28);
  __ Fcvtms(x29, d29);
  __ Fcvtms(x30, d30);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_64(1, x0);
    ASSERT_EQUAL_64(1, x1);
    ASSERT_EQUAL_64(1, x2);
    ASSERT_EQUAL_64(0xfffffffe, x3);
    ASSERT_EQUAL_64(0x7fffffff, x4);
    ASSERT_EQUAL_64(0x80000000, x5);
    ASSERT_EQUAL_64(0x7fffff80, x6);
    ASSERT_EQUAL_64(0x80000080, x7);
    ASSERT_EQUAL_64(1, x8);
    ASSERT_EQUAL_64(1, x9);
    ASSERT_EQUAL_64(1, x10);
    ASSERT_EQUAL_64(0xfffffffe, x11);
    ASSERT_EQUAL_64(0x7fffffff, x12);
    ASSERT_EQUAL_64(0x80000000, x13);
    ASSERT_EQUAL_64(0x7ffffffe, x14);
    ASSERT_EQUAL_64(0x80000001, x15);
    ASSERT_EQUAL_64(1, x17);
    ASSERT_EQUAL_64(1, x18);
    ASSERT_EQUAL_64(0xfffffffffffffffe, x19);
    ASSERT_EQUAL_64(0x7fffffffffffffff, x20);
    ASSERT_EQUAL_64(0x8000000000000000, x21);
    ASSERT_EQUAL_64(0x7fffff8000000000, x22);
    ASSERT_EQUAL_64(0x8000008000000000, x23);
    ASSERT_EQUAL_64(1, x24);
    ASSERT_EQUAL_64(1, x25);
    ASSERT_EQUAL_64(0xfffffffffffffffe, x26);
    ASSERT_EQUAL_64(0x7fffffffffffffff, x27);
    ASSERT_EQUAL_64(0x8000000000000000, x28);
    ASSERT_EQUAL_64(0x7ffffffffffffc00, x29);
    ASSERT_EQUAL_64(0x8000000000000400, x30);
  }
}


TEST(fcvtmu) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s0, 1.0);
  __ Fmov(s1, 1.1);
  __ Fmov(s2, 1.5);
  __ Fmov(s3, -1.5);
  __ Fmov(s4, kFP32PositiveInfinity);
  __ Fmov(s5, kFP32NegativeInfinity);
  __ Fmov(s6, 0x7fffff80);  // Largest float < INT32_MAX.
  __ Fneg(s7, s6);          // Smallest float > INT32_MIN.
  __ Fmov(d8, 1.0);
  __ Fmov(d9, 1.1);
  __ Fmov(d10, 1.5);
  __ Fmov(d11, -1.5);
  __ Fmov(d12, kFP64PositiveInfinity);
  __ Fmov(d13, kFP64NegativeInfinity);
  __ Fmov(d14, kWMaxInt - 1);
  __ Fmov(d15, kWMinInt + 1);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, -1.5);
  __ Fmov(s20, kFP32PositiveInfinity);
  __ Fmov(s21, kFP32NegativeInfinity);
  __ Fmov(s22, 0x7fffff8000000000);  // Largest float < INT64_MAX.
  __ Fneg(s23, s22);                 // Smallest float > INT64_MIN.
  __ Fmov(d24, 1.1);
  __ Fmov(d25, 1.5);
  __ Fmov(d26, -1.5);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0x7ffffffffffffc00);  // Largest double < INT64_MAX.
  __ Fneg(d30, d29);                 // Smallest double > INT64_MIN.

  __ Fcvtmu(w0, s0);
  __ Fcvtmu(w1, s1);
  __ Fcvtmu(w2, s2);
  __ Fcvtmu(w3, s3);
  __ Fcvtmu(w4, s4);
  __ Fcvtmu(w5, s5);
  __ Fcvtmu(w6, s6);
  __ Fcvtmu(w7, s7);
  __ Fcvtmu(w8, d8);
  __ Fcvtmu(w9, d9);
  __ Fcvtmu(w10, d10);
  __ Fcvtmu(w11, d11);
  __ Fcvtmu(w12, d12);
  __ Fcvtmu(w13, d13);
  __ Fcvtmu(w14, d14);
  __ Fcvtmu(x17, s17);
  __ Fcvtmu(x18, s18);
  __ Fcvtmu(x19, s19);
  __ Fcvtmu(x20, s20);
  __ Fcvtmu(x21, s21);
  __ Fcvtmu(x22, s22);
  __ Fcvtmu(x23, s23);
  __ Fcvtmu(x24, d24);
  __ Fcvtmu(x25, d25);
  __ Fcvtmu(x26, d26);
  __ Fcvtmu(x27, d27);
  __ Fcvtmu(x28, d28);
  __ Fcvtmu(x29, d29);
  __ Fcvtmu(x30, d30);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_64(1, x0);
    ASSERT_EQUAL_64(1, x1);
    ASSERT_EQUAL_64(1, x2);
    ASSERT_EQUAL_64(0, x3);
    ASSERT_EQUAL_64(0xffffffff, x4);
    ASSERT_EQUAL_64(0, x5);
    ASSERT_EQUAL_64(0x7fffff80, x6);
    ASSERT_EQUAL_64(0, x7);
    ASSERT_EQUAL_64(1, x8);
    ASSERT_EQUAL_64(1, x9);
    ASSERT_EQUAL_64(1, x10);
    ASSERT_EQUAL_64(0, x11);
    ASSERT_EQUAL_64(0xffffffff, x12);
    ASSERT_EQUAL_64(0, x13);
    ASSERT_EQUAL_64(0x7ffffffe, x14);
    ASSERT_EQUAL_64(1, x17);
    ASSERT_EQUAL_64(1, x18);
    ASSERT_EQUAL_64(0, x19);
    ASSERT_EQUAL_64(0xffffffffffffffff, x20);
    ASSERT_EQUAL_64(0, x21);
    ASSERT_EQUAL_64(0x7fffff8000000000, x22);
    ASSERT_EQUAL_64(0, x23);
    ASSERT_EQUAL_64(1, x24);
    ASSERT_EQUAL_64(1, x25);
    ASSERT_EQUAL_64(0, x26);
    ASSERT_EQUAL_64(0xffffffffffffffff, x27);
    ASSERT_EQUAL_64(0, x28);
    ASSERT_EQUAL_64(0x7ffffffffffffc00, x29);
    ASSERT_EQUAL_64(0, x30);
  }
}


TEST(fcvtns) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s0, 1.0);
  __ Fmov(s1, 1.1);
  __ Fmov(s2, 1.5);
  __ Fmov(s3, -1.5);
  __ Fmov(s4, kFP32PositiveInfinity);
  __ Fmov(s5, kFP32NegativeInfinity);
  __ Fmov(s6, 0x7fffff80);  // Largest float < INT32_MAX.
  __ Fneg(s7, s6);          // Smallest float > INT32_MIN.
  __ Fmov(d8, 1.0);
  __ Fmov(d9, 1.1);
  __ Fmov(d10, 1.5);
  __ Fmov(d11, -1.5);
  __ Fmov(d12, kFP64PositiveInfinity);
  __ Fmov(d13, kFP64NegativeInfinity);
  __ Fmov(d14, kWMaxInt - 1);
  __ Fmov(d15, kWMinInt + 1);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, -1.5);
  __ Fmov(s20, kFP32PositiveInfinity);
  __ Fmov(s21, kFP32NegativeInfinity);
  __ Fmov(s22, 0x7fffff8000000000);  // Largest float < INT64_MAX.
  __ Fneg(s23, s22);                 // Smallest float > INT64_MIN.
  __ Fmov(d24, 1.1);
  __ Fmov(d25, 1.5);
  __ Fmov(d26, -1.5);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0x7ffffffffffffc00);  // Largest double < INT64_MAX.
  __ Fneg(d30, d29);                 // Smallest double > INT64_MIN.

  __ Fcvtns(w0, s0);
  __ Fcvtns(w1, s1);
  __ Fcvtns(w2, s2);
  __ Fcvtns(w3, s3);
  __ Fcvtns(w4, s4);
  __ Fcvtns(w5, s5);
  __ Fcvtns(w6, s6);
  __ Fcvtns(w7, s7);
  __ Fcvtns(w8, d8);
  __ Fcvtns(w9, d9);
  __ Fcvtns(w10, d10);
  __ Fcvtns(w11, d11);
  __ Fcvtns(w12, d12);
  __ Fcvtns(w13, d13);
  __ Fcvtns(w14, d14);
  __ Fcvtns(w15, d15);
  __ Fcvtns(x17, s17);
  __ Fcvtns(x18, s18);
  __ Fcvtns(x19, s19);
  __ Fcvtns(x20, s20);
  __ Fcvtns(x21, s21);
  __ Fcvtns(x22, s22);
  __ Fcvtns(x23, s23);
  __ Fcvtns(x24, d24);
  __ Fcvtns(x25, d25);
  __ Fcvtns(x26, d26);
  __ Fcvtns(x27, d27);
  __ Fcvtns(x28, d28);
  __ Fcvtns(x29, d29);
  __ Fcvtns(x30, d30);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_64(1, x0);
    ASSERT_EQUAL_64(1, x1);
    ASSERT_EQUAL_64(2, x2);
    ASSERT_EQUAL_64(0xfffffffe, x3);
    ASSERT_EQUAL_64(0x7fffffff, x4);
    ASSERT_EQUAL_64(0x80000000, x5);
    ASSERT_EQUAL_64(0x7fffff80, x6);
    ASSERT_EQUAL_64(0x80000080, x7);
    ASSERT_EQUAL_64(1, x8);
    ASSERT_EQUAL_64(1, x9);
    ASSERT_EQUAL_64(2, x10);
    ASSERT_EQUAL_64(0xfffffffe, x11);
    ASSERT_EQUAL_64(0x7fffffff, x12);
    ASSERT_EQUAL_64(0x80000000, x13);
    ASSERT_EQUAL_64(0x7ffffffe, x14);
    ASSERT_EQUAL_64(0x80000001, x15);
    ASSERT_EQUAL_64(1, x17);
    ASSERT_EQUAL_64(2, x18);
    ASSERT_EQUAL_64(0xfffffffffffffffe, x19);
    ASSERT_EQUAL_64(0x7fffffffffffffff, x20);
    ASSERT_EQUAL_64(0x8000000000000000, x21);
    ASSERT_EQUAL_64(0x7fffff8000000000, x22);
    ASSERT_EQUAL_64(0x8000008000000000, x23);
    ASSERT_EQUAL_64(1, x24);
    ASSERT_EQUAL_64(2, x25);
    ASSERT_EQUAL_64(0xfffffffffffffffe, x26);
    ASSERT_EQUAL_64(0x7fffffffffffffff, x27);
    ASSERT_EQUAL_64(0x8000000000000000, x28);
    ASSERT_EQUAL_64(0x7ffffffffffffc00, x29);
    ASSERT_EQUAL_64(0x8000000000000400, x30);
  }
}


TEST(fcvtnu) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s0, 1.0);
  __ Fmov(s1, 1.1);
  __ Fmov(s2, 1.5);
  __ Fmov(s3, -1.5);
  __ Fmov(s4, kFP32PositiveInfinity);
  __ Fmov(s5, kFP32NegativeInfinity);
  __ Fmov(s6, 0xffffff00);  // Largest float < UINT32_MAX.
  __ Fmov(d8, 1.0);
  __ Fmov(d9, 1.1);
  __ Fmov(d10, 1.5);
  __ Fmov(d11, -1.5);
  __ Fmov(d12, kFP64PositiveInfinity);
  __ Fmov(d13, kFP64NegativeInfinity);
  __ Fmov(d14, 0xfffffffe);
  __ Fmov(s16, 1.0);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, -1.5);
  __ Fmov(s20, kFP32PositiveInfinity);
  __ Fmov(s21, kFP32NegativeInfinity);
  __ Fmov(s22, 0xffffff0000000000);  // Largest float < UINT64_MAX.
  __ Fmov(d24, 1.1);
  __ Fmov(d25, 1.5);
  __ Fmov(d26, -1.5);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0xfffffffffffff800);  // Largest double < UINT64_MAX.
  __ Fmov(s30, 0x100000000);

  __ Fcvtnu(w0, s0);
  __ Fcvtnu(w1, s1);
  __ Fcvtnu(w2, s2);
  __ Fcvtnu(w3, s3);
  __ Fcvtnu(w4, s4);
  __ Fcvtnu(w5, s5);
  __ Fcvtnu(w6, s6);
  __ Fcvtnu(w8, d8);
  __ Fcvtnu(w9, d9);
  __ Fcvtnu(w10, d10);
  __ Fcvtnu(w11, d11);
  __ Fcvtnu(w12, d12);
  __ Fcvtnu(w13, d13);
  __ Fcvtnu(w14, d14);
  __ Fcvtnu(w15, d15);
  __ Fcvtnu(x16, s16);
  __ Fcvtnu(x17, s17);
  __ Fcvtnu(x18, s18);
  __ Fcvtnu(x19, s19);
  __ Fcvtnu(x20, s20);
  __ Fcvtnu(x21, s21);
  __ Fcvtnu(x22, s22);
  __ Fcvtnu(x24, d24);
  __ Fcvtnu(x25, d25);
  __ Fcvtnu(x26, d26);
  __ Fcvtnu(x27, d27);
  __ Fcvtnu(x28, d28);
  __ Fcvtnu(x29, d29);
  __ Fcvtnu(w30, s30);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_64(1, x0);
    ASSERT_EQUAL_64(1, x1);
    ASSERT_EQUAL_64(2, x2);
    ASSERT_EQUAL_64(0, x3);
    ASSERT_EQUAL_64(0xffffffff, x4);
    ASSERT_EQUAL_64(0, x5);
    ASSERT_EQUAL_64(0xffffff00, x6);
    ASSERT_EQUAL_64(1, x8);
    ASSERT_EQUAL_64(1, x9);
    ASSERT_EQUAL_64(2, x10);
    ASSERT_EQUAL_64(0, x11);
    ASSERT_EQUAL_64(0xffffffff, x12);
    ASSERT_EQUAL_64(0, x13);
    ASSERT_EQUAL_64(0xfffffffe, x14);
    ASSERT_EQUAL_64(1, x16);
    ASSERT_EQUAL_64(1, x17);
    ASSERT_EQUAL_64(2, x18);
    ASSERT_EQUAL_64(0, x19);
    ASSERT_EQUAL_64(0xffffffffffffffff, x20);
    ASSERT_EQUAL_64(0, x21);
    ASSERT_EQUAL_64(0xffffff0000000000, x22);
    ASSERT_EQUAL_64(1, x24);
    ASSERT_EQUAL_64(2, x25);
    ASSERT_EQUAL_64(0, x26);
    ASSERT_EQUAL_64(0xffffffffffffffff, x27);
    ASSERT_EQUAL_64(0, x28);
    ASSERT_EQUAL_64(0xfffffffffffff800, x29);
    ASSERT_EQUAL_64(0xffffffff, x30);
  }
}


TEST(fcvtzs) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s0, 1.0);
  __ Fmov(s1, 1.1);
  __ Fmov(s2, 1.5);
  __ Fmov(s3, -1.5);
  __ Fmov(s4, kFP32PositiveInfinity);
  __ Fmov(s5, kFP32NegativeInfinity);
  __ Fmov(s6, 0x7fffff80);  // Largest float < INT32_MAX.
  __ Fneg(s7, s6);          // Smallest float > INT32_MIN.
  __ Fmov(d8, 1.0);
  __ Fmov(d9, 1.1);
  __ Fmov(d10, 1.5);
  __ Fmov(d11, -1.5);
  __ Fmov(d12, kFP64PositiveInfinity);
  __ Fmov(d13, kFP64NegativeInfinity);
  __ Fmov(d14, kWMaxInt - 1);
  __ Fmov(d15, kWMinInt + 1);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, -1.5);
  __ Fmov(s20, kFP32PositiveInfinity);
  __ Fmov(s21, kFP32NegativeInfinity);
  __ Fmov(s22, 0x7fffff8000000000);  // Largest float < INT64_MAX.
  __ Fneg(s23, s22);                 // Smallest float > INT64_MIN.
  __ Fmov(d24, 1.1);
  __ Fmov(d25, 1.5);
  __ Fmov(d26, -1.5);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0x7ffffffffffffc00);  // Largest double < INT64_MAX.
  __ Fneg(d30, d29);                 // Smallest double > INT64_MIN.

  __ Fcvtzs(w0, s0);
  __ Fcvtzs(w1, s1);
  __ Fcvtzs(w2, s2);
  __ Fcvtzs(w3, s3);
  __ Fcvtzs(w4, s4);
  __ Fcvtzs(w5, s5);
  __ Fcvtzs(w6, s6);
  __ Fcvtzs(w7, s7);
  __ Fcvtzs(w8, d8);
  __ Fcvtzs(w9, d9);
  __ Fcvtzs(w10, d10);
  __ Fcvtzs(w11, d11);
  __ Fcvtzs(w12, d12);
  __ Fcvtzs(w13, d13);
  __ Fcvtzs(w14, d14);
  __ Fcvtzs(w15, d15);
  __ Fcvtzs(x17, s17);
  __ Fcvtzs(x18, s18);
  __ Fcvtzs(x19, s19);
  __ Fcvtzs(x20, s20);
  __ Fcvtzs(x21, s21);
  __ Fcvtzs(x22, s22);
  __ Fcvtzs(x23, s23);
  __ Fcvtzs(x24, d24);
  __ Fcvtzs(x25, d25);
  __ Fcvtzs(x26, d26);
  __ Fcvtzs(x27, d27);
  __ Fcvtzs(x28, d28);
  __ Fcvtzs(x29, d29);
  __ Fcvtzs(x30, d30);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_64(1, x0);
    ASSERT_EQUAL_64(1, x1);
    ASSERT_EQUAL_64(1, x2);
    ASSERT_EQUAL_64(0xffffffff, x3);
    ASSERT_EQUAL_64(0x7fffffff, x4);
    ASSERT_EQUAL_64(0x80000000, x5);
    ASSERT_EQUAL_64(0x7fffff80, x6);
    ASSERT_EQUAL_64(0x80000080, x7);
    ASSERT_EQUAL_64(1, x8);
    ASSERT_EQUAL_64(1, x9);
    ASSERT_EQUAL_64(1, x10);
    ASSERT_EQUAL_64(0xffffffff, x11);
    ASSERT_EQUAL_64(0x7fffffff, x12);
    ASSERT_EQUAL_64(0x80000000, x13);
    ASSERT_EQUAL_64(0x7ffffffe, x14);
    ASSERT_EQUAL_64(0x80000001, x15);
    ASSERT_EQUAL_64(1, x17);
    ASSERT_EQUAL_64(1, x18);
    ASSERT_EQUAL_64(0xffffffffffffffff, x19);
    ASSERT_EQUAL_64(0x7fffffffffffffff, x20);
    ASSERT_EQUAL_64(0x8000000000000000, x21);
    ASSERT_EQUAL_64(0x7fffff8000000000, x22);
    ASSERT_EQUAL_64(0x8000008000000000, x23);
    ASSERT_EQUAL_64(1, x24);
    ASSERT_EQUAL_64(1, x25);
    ASSERT_EQUAL_64(0xffffffffffffffff, x26);
    ASSERT_EQUAL_64(0x7fffffffffffffff, x27);
    ASSERT_EQUAL_64(0x8000000000000000, x28);
    ASSERT_EQUAL_64(0x7ffffffffffffc00, x29);
    ASSERT_EQUAL_64(0x8000000000000400, x30);
  }
}

void FjcvtzsHelper(uint64_t value, uint64_t expected, uint32_t expected_z) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kJSCVT);
  START();
  __ Fmov(d0, RawbitsToDouble(value));
  __ Fjcvtzs(w0, d0);
  __ Mrs(x1, NZCV);
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_64(expected, x0);
    ASSERT_EQUAL_32(expected_z, w1);
  }
}

TEST(fjcvtzs) {
  /* Simple values. */
  FjcvtzsHelper(0x0000000000000000, 0, ZFlag);   // 0.0
  FjcvtzsHelper(0x0010000000000000, 0, NoFlag);  // The smallest normal value.
  FjcvtzsHelper(0x3fdfffffffffffff, 0, NoFlag);  // The value just below 0.5.
  FjcvtzsHelper(0x3fe0000000000000, 0, NoFlag);  // 0.5
  FjcvtzsHelper(0x3fe0000000000001, 0, NoFlag);  // The value just above 0.5.
  FjcvtzsHelper(0x3fefffffffffffff, 0, NoFlag);  // The value just below 1.0.
  FjcvtzsHelper(0x3ff0000000000000, 1, ZFlag);   // 1.0
  FjcvtzsHelper(0x3ff0000000000001, 1, NoFlag);  // The value just above 1.0.
  FjcvtzsHelper(0x3ff8000000000000, 1, NoFlag);  // 1.5
  FjcvtzsHelper(0x4024000000000000, 10, ZFlag);  // 10
  FjcvtzsHelper(0x7fefffffffffffff, 0, NoFlag);  // The largest finite value.

  /* Infinity. */
  FjcvtzsHelper(0x7ff0000000000000, 0, NoFlag);

  /* NaNs. */
  /*  - Quiet NaNs */
  FjcvtzsHelper(0x7ff923456789abcd, 0, NoFlag);
  FjcvtzsHelper(0x7ff8000000000000, 0, NoFlag);
  /*  - Signalling NaNs */
  FjcvtzsHelper(0x7ff123456789abcd, 0, NoFlag);
  FjcvtzsHelper(0x7ff0000000000001, 0, NoFlag);

  /* Subnormals. */
  /*  - A recognisable bit pattern. */
  FjcvtzsHelper(0x000123456789abcd, 0, NoFlag);
  /*  - The largest subnormal value. */
  FjcvtzsHelper(0x000fffffffffffff, 0, NoFlag);
  /*  - The smallest subnormal value. */
  FjcvtzsHelper(0x0000000000000001, 0, NoFlag);

  /* The same values again, but negated. */
  FjcvtzsHelper(0x8000000000000000, 0, NoFlag);
  FjcvtzsHelper(0x8010000000000000, 0, NoFlag);
  FjcvtzsHelper(0xbfdfffffffffffff, 0, NoFlag);
  FjcvtzsHelper(0xbfe0000000000000, 0, NoFlag);
  FjcvtzsHelper(0xbfe0000000000001, 0, NoFlag);
  FjcvtzsHelper(0xbfefffffffffffff, 0, NoFlag);
  FjcvtzsHelper(0xbff0000000000000, 0xffffffff, ZFlag);
  FjcvtzsHelper(0xbff0000000000001, 0xffffffff, NoFlag);
  FjcvtzsHelper(0xbff8000000000000, 0xffffffff, NoFlag);
  FjcvtzsHelper(0xc024000000000000, 0xfffffff6, ZFlag);
  FjcvtzsHelper(0xffefffffffffffff, 0, NoFlag);
  FjcvtzsHelper(0xfff0000000000000, 0, NoFlag);
  FjcvtzsHelper(0xfff923456789abcd, 0, NoFlag);
  FjcvtzsHelper(0xfff8000000000000, 0, NoFlag);
  FjcvtzsHelper(0xfff123456789abcd, 0, NoFlag);
  FjcvtzsHelper(0xfff0000000000001, 0, NoFlag);
  FjcvtzsHelper(0x800123456789abcd, 0, NoFlag);
  FjcvtzsHelper(0x800fffffffffffff, 0, NoFlag);
  FjcvtzsHelper(0x8000000000000001, 0, NoFlag);

  // Test floating-point numbers of every possible exponent, most of the
  // expected values are zero but there is a range of exponents where the
  // results are shifted parts of this mantissa.
  uint64_t mantissa = 0x0001234567890abc;

  // Between an exponent of 0 and 52, only some of the top bits of the
  // mantissa are above the decimal position of doubles so the mantissa is
  // shifted to the right down to just those top bits. Above 52, all bits
  // of the mantissa are shifted left above the decimal position until it
  // reaches 52 + 64 where all the bits are shifted out of the range of 64-bit
  // integers.
  int first_exp_boundary = 52;
  int second_exp_boundary = first_exp_boundary + 64;
  for (int exponent = 0; exponent < 2048; exponent += 8) {
    int e = exponent - 1023;

    uint64_t expected = 0;
    if (e < 0) {
      expected = 0;
    } else if (e <= first_exp_boundary) {
      expected = (UINT64_C(1) << e) | (mantissa >> (52 - e));
      expected &= 0xffffffff;
    } else if (e < second_exp_boundary) {
      expected = (mantissa << (e - 52)) & 0xffffffff;
    } else {
      expected = 0;
    }

    uint64_t value = (static_cast<uint64_t>(exponent) << 52) | mantissa;
    FjcvtzsHelper(value, expected, NoFlag);
    FjcvtzsHelper(value | kDSignMask, (-expected) & 0xffffffff, NoFlag);
  }
}

TEST(fcvtzu) {
  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();
  __ Fmov(s0, 1.0);
  __ Fmov(s1, 1.1);
  __ Fmov(s2, 1.5);
  __ Fmov(s3, -1.5);
  __ Fmov(s4, kFP32PositiveInfinity);
  __ Fmov(s5, kFP32NegativeInfinity);
  __ Fmov(s6, 0x7fffff80);  // Largest float < INT32_MAX.
  __ Fneg(s7, s6);          // Smallest float > INT32_MIN.
  __ Fmov(d8, 1.0);
  __ Fmov(d9, 1.1);
  __ Fmov(d10, 1.5);
  __ Fmov(d11, -1.5);
  __ Fmov(d12, kFP64PositiveInfinity);
  __ Fmov(d13, kFP64NegativeInfinity);
  __ Fmov(d14, kWMaxInt - 1);
  __ Fmov(d15, kWMinInt + 1);
  __ Fmov(s17, 1.1);
  __ Fmov(s18, 1.5);
  __ Fmov(s19, -1.5);
  __ Fmov(s20, kFP32PositiveInfinity);
  __ Fmov(s21, kFP32NegativeInfinity);
  __ Fmov(s22, 0x7fffff8000000000);  // Largest float < INT64_MAX.
  __ Fneg(s23, s22);                 // Smallest float > INT64_MIN.
  __ Fmov(d24, 1.1);
  __ Fmov(d25, 1.5);
  __ Fmov(d26, -1.5);
  __ Fmov(d27, kFP64PositiveInfinity);
  __ Fmov(d28, kFP64NegativeInfinity);
  __ Fmov(d29, 0x7ffffffffffffc00);  // Largest double < INT64_MAX.
  __ Fneg(d30, d29);                 // Smallest double > INT64_MIN.

  __ Fcvtzu(w0, s0);
  __ Fcvtzu(w1, s1);
  __ Fcvtzu(w2, s2);
  __ Fcvtzu(w3, s3);
  __ Fcvtzu(w4, s4);
  __ Fcvtzu(w5, s5);
  __ Fcvtzu(w6, s6);
  __ Fcvtzu(w7, s7);
  __ Fcvtzu(w8, d8);
  __ Fcvtzu(w9, d9);
  __ Fcvtzu(w10, d10);
  __ Fcvtzu(w11, d11);
  __ Fcvtzu(w12, d12);
  __ Fcvtzu(w13, d13);
  __ Fcvtzu(w14, d14);
  __ Fcvtzu(x17, s17);
  __ Fcvtzu(x18, s18);
  __ Fcvtzu(x19, s19);
  __ Fcvtzu(x20, s20);
  __ Fcvtzu(x21, s21);
  __ Fcvtzu(x22, s22);
  __ Fcvtzu(x23, s23);
  __ Fcvtzu(x24, d24);
  __ Fcvtzu(x25, d25);
  __ Fcvtzu(x26, d26);
  __ Fcvtzu(x27, d27);
  __ Fcvtzu(x28, d28);
  __ Fcvtzu(x29, d29);
  __ Fcvtzu(x30, d30);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_64(1, x0);
    ASSERT_EQUAL_64(1, x1);
    ASSERT_EQUAL_64(1, x2);
    ASSERT_EQUAL_64(0, x3);
    ASSERT_EQUAL_64(0xffffffff, x4);
    ASSERT_EQUAL_64(0, x5);
    ASSERT_EQUAL_64(0x7fffff80, x6);
    ASSERT_EQUAL_64(0, x7);
    ASSERT_EQUAL_64(1, x8);
    ASSERT_EQUAL_64(1, x9);
    ASSERT_EQUAL_64(1, x10);
    ASSERT_EQUAL_64(0, x11);
    ASSERT_EQUAL_64(0xffffffff, x12);
    ASSERT_EQUAL_64(0, x13);
    ASSERT_EQUAL_64(0x7ffffffe, x14);
    ASSERT_EQUAL_64(1, x17);
    ASSERT_EQUAL_64(1, x18);
    ASSERT_EQUAL_64(0, x19);
    ASSERT_EQUAL_64(0xffffffffffffffff, x20);
    ASSERT_EQUAL_64(0, x21);
    ASSERT_EQUAL_64(0x7fffff8000000000, x22);
    ASSERT_EQUAL_64(0, x23);
    ASSERT_EQUAL_64(1, x24);
    ASSERT_EQUAL_64(1, x25);
    ASSERT_EQUAL_64(0, x26);
    ASSERT_EQUAL_64(0xffffffffffffffff, x27);
    ASSERT_EQUAL_64(0, x28);
    ASSERT_EQUAL_64(0x7ffffffffffffc00, x29);
    ASSERT_EQUAL_64(0, x30);
  }
}

// Test that scvtf and ucvtf can convert the 64-bit input into the expected
// value. All possible values of 'fbits' are tested. The expected value is
// modified accordingly in each case.
//
// The expected value is specified as the bit encoding of the expected double
// produced by scvtf (expected_scvtf_bits) as well as ucvtf
// (expected_ucvtf_bits).
//
// Where the input value is representable by int32_t or uint32_t, conversions
// from W registers will also be tested.
static void TestUScvtfHelper(uint64_t in,
                             uint64_t expected_scvtf_bits,
                             uint64_t expected_ucvtf_bits) {
  uint64_t u64 = in;
  uint32_t u32 = u64 & 0xffffffff;
  int64_t s64 = static_cast<int64_t>(in);
  int32_t s32 = s64 & 0x7fffffff;

  bool cvtf_s32 = (s64 == s32);
  bool cvtf_u32 = (u64 == u32);

  double results_scvtf_x[65];
  double results_ucvtf_x[65];
  double results_scvtf_w[33];
  double results_ucvtf_w[33];

  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();

  __ Mov(x0, reinterpret_cast<uintptr_t>(results_scvtf_x));
  __ Mov(x1, reinterpret_cast<uintptr_t>(results_ucvtf_x));
  __ Mov(x2, reinterpret_cast<uintptr_t>(results_scvtf_w));
  __ Mov(x3, reinterpret_cast<uintptr_t>(results_ucvtf_w));

  __ Mov(x10, s64);

  // Corrupt the top word, in case it is accidentally used during W-register
  // conversions.
  __ Mov(x11, 0x5555555555555555);
  __ Bfi(x11, x10, 0, kWRegSize);

  // Test integer conversions.
  __ Scvtf(d0, x10);
  __ Ucvtf(d1, x10);
  __ Scvtf(d2, w11);
  __ Ucvtf(d3, w11);
  __ Str(d0, MemOperand(x0));
  __ Str(d1, MemOperand(x1));
  __ Str(d2, MemOperand(x2));
  __ Str(d3, MemOperand(x3));

  // Test all possible values of fbits.
  for (int fbits = 1; fbits <= 32; fbits++) {
    __ Scvtf(d0, x10, fbits);
    __ Ucvtf(d1, x10, fbits);
    __ Scvtf(d2, w11, fbits);
    __ Ucvtf(d3, w11, fbits);
    __ Str(d0, MemOperand(x0, fbits * kDRegSizeInBytes));
    __ Str(d1, MemOperand(x1, fbits * kDRegSizeInBytes));
    __ Str(d2, MemOperand(x2, fbits * kDRegSizeInBytes));
    __ Str(d3, MemOperand(x3, fbits * kDRegSizeInBytes));
  }

  // Conversions from W registers can only handle fbits values <= 32, so just
  // test conversions from X registers for 32 < fbits <= 64.
  for (int fbits = 33; fbits <= 64; fbits++) {
    __ Scvtf(d0, x10, fbits);
    __ Ucvtf(d1, x10, fbits);
    __ Str(d0, MemOperand(x0, fbits * kDRegSizeInBytes));
    __ Str(d1, MemOperand(x1, fbits * kDRegSizeInBytes));
  }

  END();
  if (CAN_RUN()) {
    RUN();

    // Check the results.
    double expected_scvtf_base = RawbitsToDouble(expected_scvtf_bits);
    double expected_ucvtf_base = RawbitsToDouble(expected_ucvtf_bits);

    for (int fbits = 0; fbits <= 32; fbits++) {
      double expected_scvtf = expected_scvtf_base / std::pow(2, fbits);
      double expected_ucvtf = expected_ucvtf_base / std::pow(2, fbits);
      ASSERT_EQUAL_FP64(expected_scvtf, results_scvtf_x[fbits]);
      ASSERT_EQUAL_FP64(expected_ucvtf, results_ucvtf_x[fbits]);
      if (cvtf_s32) ASSERT_EQUAL_FP64(expected_scvtf, results_scvtf_w[fbits]);
      if (cvtf_u32) ASSERT_EQUAL_FP64(expected_ucvtf, results_ucvtf_w[fbits]);
    }
    for (int fbits = 33; fbits <= 64; fbits++) {
      double expected_scvtf = expected_scvtf_base / std::pow(2, fbits);
      double expected_ucvtf = expected_ucvtf_base / std::pow(2, fbits);
      ASSERT_EQUAL_FP64(expected_scvtf, results_scvtf_x[fbits]);
      ASSERT_EQUAL_FP64(expected_ucvtf, results_ucvtf_x[fbits]);
    }
  }
}


TEST(scvtf_ucvtf_double) {
  // Simple conversions of positive numbers which require no rounding; the
  // results should not depened on the rounding mode, and ucvtf and scvtf should
  // produce the same result.
  TestUScvtfHelper(0x0000000000000000, 0x0000000000000000, 0x0000000000000000);
  TestUScvtfHelper(0x0000000000000001, 0x3ff0000000000000, 0x3ff0000000000000);
  TestUScvtfHelper(0x0000000040000000, 0x41d0000000000000, 0x41d0000000000000);
  TestUScvtfHelper(0x0000000100000000, 0x41f0000000000000, 0x41f0000000000000);
  TestUScvtfHelper(0x4000000000000000, 0x43d0000000000000, 0x43d0000000000000);
  // Test mantissa extremities.
  TestUScvtfHelper(0x4000000000000400, 0x43d0000000000001, 0x43d0000000000001);
  // The largest int32_t that fits in a double.
  TestUScvtfHelper(0x000000007fffffff, 0x41dfffffffc00000, 0x41dfffffffc00000);
  // Values that would be negative if treated as an int32_t.
  TestUScvtfHelper(0x00000000ffffffff, 0x41efffffffe00000, 0x41efffffffe00000);
  TestUScvtfHelper(0x0000000080000000, 0x41e0000000000000, 0x41e0000000000000);
  TestUScvtfHelper(0x0000000080000001, 0x41e0000000200000, 0x41e0000000200000);
  // The largest int64_t that fits in a double.
  TestUScvtfHelper(0x7ffffffffffffc00, 0x43dfffffffffffff, 0x43dfffffffffffff);
  // Check for bit pattern reproduction.
  TestUScvtfHelper(0x0123456789abcde0, 0x43723456789abcde, 0x43723456789abcde);
  TestUScvtfHelper(0x0000000012345678, 0x41b2345678000000, 0x41b2345678000000);

  // Simple conversions of negative int64_t values. These require no rounding,
  // and the results should not depend on the rounding mode.
  TestUScvtfHelper(0xffffffffc0000000, 0xc1d0000000000000, 0x43effffffff80000);
  TestUScvtfHelper(0xffffffff00000000, 0xc1f0000000000000, 0x43efffffffe00000);
  TestUScvtfHelper(0xc000000000000000, 0xc3d0000000000000, 0x43e8000000000000);

  // Conversions which require rounding.
  TestUScvtfHelper(0x1000000000000000, 0x43b0000000000000, 0x43b0000000000000);
  TestUScvtfHelper(0x1000000000000001, 0x43b0000000000000, 0x43b0000000000000);
  TestUScvtfHelper(0x1000000000000080, 0x43b0000000000000, 0x43b0000000000000);
  TestUScvtfHelper(0x1000000000000081, 0x43b0000000000001, 0x43b0000000000001);
  TestUScvtfHelper(0x1000000000000100, 0x43b0000000000001, 0x43b0000000000001);
  TestUScvtfHelper(0x1000000000000101, 0x43b0000000000001, 0x43b0000000000001);
  TestUScvtfHelper(0x1000000000000180, 0x43b0000000000002, 0x43b0000000000002);
  TestUScvtfHelper(0x1000000000000181, 0x43b0000000000002, 0x43b0000000000002);
  TestUScvtfHelper(0x1000000000000200, 0x43b0000000000002, 0x43b0000000000002);
  TestUScvtfHelper(0x1000000000000201, 0x43b0000000000002, 0x43b0000000000002);
  TestUScvtfHelper(0x1000000000000280, 0x43b0000000000002, 0x43b0000000000002);
  TestUScvtfHelper(0x1000000000000281, 0x43b0000000000003, 0x43b0000000000003);
  TestUScvtfHelper(0x1000000000000300, 0x43b0000000000003, 0x43b0000000000003);
  // Check rounding of negative int64_t values (and large uint64_t values).
  TestUScvtfHelper(0x8000000000000000, 0xc3e0000000000000, 0x43e0000000000000);
  TestUScvtfHelper(0x8000000000000001, 0xc3e0000000000000, 0x43e0000000000000);
  TestUScvtfHelper(0x8000000000000200, 0xc3e0000000000000, 0x43e0000000000000);
  TestUScvtfHelper(0x8000000000000201, 0xc3dfffffffffffff, 0x43e0000000000000);
  TestUScvtfHelper(0x8000000000000400, 0xc3dfffffffffffff, 0x43e0000000000000);
  TestUScvtfHelper(0x8000000000000401, 0xc3dfffffffffffff, 0x43e0000000000001);
  TestUScvtfHelper(0x8000000000000600, 0xc3dffffffffffffe, 0x43e0000000000001);
  TestUScvtfHelper(0x8000000000000601, 0xc3dffffffffffffe, 0x43e0000000000001);
  TestUScvtfHelper(0x8000000000000800, 0xc3dffffffffffffe, 0x43e0000000000001);
  TestUScvtfHelper(0x8000000000000801, 0xc3dffffffffffffe, 0x43e0000000000001);
  TestUScvtfHelper(0x8000000000000a00, 0xc3dffffffffffffe, 0x43e0000000000001);
  TestUScvtfHelper(0x8000000000000a01, 0xc3dffffffffffffd, 0x43e0000000000001);
  TestUScvtfHelper(0x8000000000000c00, 0xc3dffffffffffffd, 0x43e0000000000002);
  // Round up to produce a result that's too big for the input to represent.
  TestUScvtfHelper(0x7ffffffffffffe00, 0x43e0000000000000, 0x43e0000000000000);
  TestUScvtfHelper(0x7fffffffffffffff, 0x43e0000000000000, 0x43e0000000000000);
  TestUScvtfHelper(0xfffffffffffffc00, 0xc090000000000000, 0x43f0000000000000);
  TestUScvtfHelper(0xffffffffffffffff, 0xbff0000000000000, 0x43f0000000000000);
}


// The same as TestUScvtfHelper, but convert to floats.
static void TestUScvtf32Helper(uint64_t in,
                               uint32_t expected_scvtf_bits,
                               uint32_t expected_ucvtf_bits) {
  uint64_t u64 = in;
  uint32_t u32 = u64 & 0xffffffff;
  int64_t s64 = static_cast<int64_t>(in);
  int32_t s32 = s64 & 0x7fffffff;

  bool cvtf_s32 = (s64 == s32);
  bool cvtf_u32 = (u64 == u32);

  float results_scvtf_x[65];
  float results_ucvtf_x[65];
  float results_scvtf_w[33];
  float results_ucvtf_w[33];

  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();

  __ Mov(x0, reinterpret_cast<uintptr_t>(results_scvtf_x));
  __ Mov(x1, reinterpret_cast<uintptr_t>(results_ucvtf_x));
  __ Mov(x2, reinterpret_cast<uintptr_t>(results_scvtf_w));
  __ Mov(x3, reinterpret_cast<uintptr_t>(results_ucvtf_w));

  __ Mov(x10, s64);

  // Corrupt the top word, in case it is accidentally used during W-register
  // conversions.
  __ Mov(x11, 0x5555555555555555);
  __ Bfi(x11, x10, 0, kWRegSize);

  // Test integer conversions.
  __ Scvtf(s0, x10);
  __ Ucvtf(s1, x10);
  __ Scvtf(s2, w11);
  __ Ucvtf(s3, w11);
  __ Str(s0, MemOperand(x0));
  __ Str(s1, MemOperand(x1));
  __ Str(s2, MemOperand(x2));
  __ Str(s3, MemOperand(x3));

  // Test all possible values of fbits.
  for (int fbits = 1; fbits <= 32; fbits++) {
    __ Scvtf(s0, x10, fbits);
    __ Ucvtf(s1, x10, fbits);
    __ Scvtf(s2, w11, fbits);
    __ Ucvtf(s3, w11, fbits);
    __ Str(s0, MemOperand(x0, fbits * kSRegSizeInBytes));
    __ Str(s1, MemOperand(x1, fbits * kSRegSizeInBytes));
    __ Str(s2, MemOperand(x2, fbits * kSRegSizeInBytes));
    __ Str(s3, MemOperand(x3, fbits * kSRegSizeInBytes));
  }

  // Conversions from W registers can only handle fbits values <= 32, so just
  // test conversions from X registers for 32 < fbits <= 64.
  for (int fbits = 33; fbits <= 64; fbits++) {
    __ Scvtf(s0, x10, fbits);
    __ Ucvtf(s1, x10, fbits);
    __ Str(s0, MemOperand(x0, fbits * kSRegSizeInBytes));
    __ Str(s1, MemOperand(x1, fbits * kSRegSizeInBytes));
  }

  END();
  if (CAN_RUN()) {
    RUN();

    // Check the results.
    float expected_scvtf_base = RawbitsToFloat(expected_scvtf_bits);
    float expected_ucvtf_base = RawbitsToFloat(expected_ucvtf_bits);

    for (int fbits = 0; fbits <= 32; fbits++) {
      float expected_scvtf = expected_scvtf_base / std::pow(2.0f, fbits);
      float expected_ucvtf = expected_ucvtf_base / std::pow(2.0f, fbits);
      ASSERT_EQUAL_FP32(expected_scvtf, results_scvtf_x[fbits]);
      ASSERT_EQUAL_FP32(expected_ucvtf, results_ucvtf_x[fbits]);
      if (cvtf_s32) ASSERT_EQUAL_FP32(expected_scvtf, results_scvtf_w[fbits]);
      if (cvtf_u32) ASSERT_EQUAL_FP32(expected_ucvtf, results_ucvtf_w[fbits]);
    }
    for (int fbits = 33; fbits <= 64; fbits++) {
      float expected_scvtf = expected_scvtf_base / std::pow(2.0f, fbits);
      float expected_ucvtf = expected_ucvtf_base / std::pow(2.0f, fbits);
      ASSERT_EQUAL_FP32(expected_scvtf, results_scvtf_x[fbits]);
      ASSERT_EQUAL_FP32(expected_ucvtf, results_ucvtf_x[fbits]);
    }
  }
}


TEST(scvtf_ucvtf_float) {
  // Simple conversions of positive numbers which require no rounding; the
  // results should not depened on the rounding mode, and ucvtf and scvtf should
  // produce the same result.
  TestUScvtf32Helper(0x0000000000000000, 0x00000000, 0x00000000);
  TestUScvtf32Helper(0x0000000000000001, 0x3f800000, 0x3f800000);
  TestUScvtf32Helper(0x0000000040000000, 0x4e800000, 0x4e800000);
  TestUScvtf32Helper(0x0000000100000000, 0x4f800000, 0x4f800000);
  TestUScvtf32Helper(0x4000000000000000, 0x5e800000, 0x5e800000);
  // Test mantissa extremities.
  TestUScvtf32Helper(0x0000000000800001, 0x4b000001, 0x4b000001);
  TestUScvtf32Helper(0x4000008000000000, 0x5e800001, 0x5e800001);
  // The largest int32_t that fits in a float.
  TestUScvtf32Helper(0x000000007fffff80, 0x4effffff, 0x4effffff);
  // Values that would be negative if treated as an int32_t.
  TestUScvtf32Helper(0x00000000ffffff00, 0x4f7fffff, 0x4f7fffff);
  TestUScvtf32Helper(0x0000000080000000, 0x4f000000, 0x4f000000);
  TestUScvtf32Helper(0x0000000080000100, 0x4f000001, 0x4f000001);
  // The largest int64_t that fits in a float.
  TestUScvtf32Helper(0x7fffff8000000000, 0x5effffff, 0x5effffff);
  // Check for bit pattern reproduction.
  TestUScvtf32Helper(0x0000000000876543, 0x4b076543, 0x4b076543);

  // Simple conversions of negative int64_t values. These require no rounding,
  // and the results should not depend on the rounding mode.
  TestUScvtf32Helper(0xfffffc0000000000, 0xd4800000, 0x5f7ffffc);
  TestUScvtf32Helper(0xc000000000000000, 0xde800000, 0x5f400000);

  // Conversions which require rounding.
  TestUScvtf32Helper(0x0000800000000000, 0x57000000, 0x57000000);
  TestUScvtf32Helper(0x0000800000000001, 0x57000000, 0x57000000);
  TestUScvtf32Helper(0x0000800000800000, 0x57000000, 0x57000000);
  TestUScvtf32Helper(0x0000800000800001, 0x57000001, 0x57000001);
  TestUScvtf32Helper(0x0000800001000000, 0x57000001, 0x57000001);
  TestUScvtf32Helper(0x0000800001000001, 0x57000001, 0x57000001);
  TestUScvtf32Helper(0x0000800001800000, 0x57000002, 0x57000002);
  TestUScvtf32Helper(0x0000800001800001, 0x57000002, 0x57000002);
  TestUScvtf32Helper(0x0000800002000000, 0x57000002, 0x57000002);
  TestUScvtf32Helper(0x0000800002000001, 0x57000002, 0x57000002);
  TestUScvtf32Helper(0x0000800002800000, 0x57000002, 0x57000002);
  TestUScvtf32Helper(0x0000800002800001, 0x57000003, 0x57000003);
  TestUScvtf32Helper(0x0000800003000000, 0x57000003, 0x57000003);
  // Check rounding of negative int64_t values (and large uint64_t values).
  TestUScvtf32Helper(0x8000000000000000, 0xdf000000, 0x5f000000);
  TestUScvtf32Helper(0x8000000000000001, 0xdf000000, 0x5f000000);
  TestUScvtf32Helper(0x8000004000000000, 0xdf000000, 0x5f000000);
  TestUScvtf32Helper(0x8000004000000001, 0xdeffffff, 0x5f000000);
  TestUScvtf32Helper(0x8000008000000000, 0xdeffffff, 0x5f000000);
  TestUScvtf32Helper(0x8000008000000001, 0xdeffffff, 0x5f000001);
  TestUScvtf32Helper(0x800000c000000000, 0xdefffffe, 0x5f000001);
  TestUScvtf32Helper(0x800000c000000001, 0xdefffffe, 0x5f000001);
  TestUScvtf32Helper(0x8000010000000000, 0xdefffffe, 0x5f000001);
  TestUScvtf32Helper(0x8000010000000001, 0xdefffffe, 0x5f000001);
  TestUScvtf32Helper(0x8000014000000000, 0xdefffffe, 0x5f000001);
  TestUScvtf32Helper(0x8000014000000001, 0xdefffffd, 0x5f000001);
  TestUScvtf32Helper(0x8000018000000000, 0xdefffffd, 0x5f000002);
  // Round up to produce a result that's too big for the input to represent.
  TestUScvtf32Helper(0x000000007fffffc0, 0x4f000000, 0x4f000000);
  TestUScvtf32Helper(0x000000007fffffff, 0x4f000000, 0x4f000000);
  TestUScvtf32Helper(0x00000000ffffff80, 0x4f800000, 0x4f800000);
  TestUScvtf32Helper(0x00000000ffffffff, 0x4f800000, 0x4f800000);
  TestUScvtf32Helper(0x7fffffc000000000, 0x5f000000, 0x5f000000);
  TestUScvtf32Helper(0x7fffffffffffffff, 0x5f000000, 0x5f000000);
  TestUScvtf32Helper(0xffffff8000000000, 0xd3000000, 0x5f800000);
  TestUScvtf32Helper(0xffffffffffffffff, 0xbf800000, 0x5f800000);
}

TEST(process_nan_double) {
  // Make sure that NaN propagation works correctly.
  double sn = RawbitsToDouble(0x7ff5555511111111);
  double qn = RawbitsToDouble(0x7ffaaaaa11111111);
  VIXL_ASSERT(IsSignallingNaN(sn));
  VIXL_ASSERT(IsQuietNaN(qn));

  // The input NaNs after passing through ProcessNaN.
  double sn_proc = RawbitsToDouble(0x7ffd555511111111);
  double qn_proc = qn;
  VIXL_ASSERT(IsQuietNaN(sn_proc));
  VIXL_ASSERT(IsQuietNaN(qn_proc));

  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();

  // Execute a number of instructions which all use ProcessNaN, and check that
  // they all handle the NaN correctly.
  __ Fmov(d0, sn);
  __ Fmov(d10, qn);

  // Operations that always propagate NaNs unchanged, even signalling NaNs.
  //   - Signalling NaN
  __ Fmov(d1, d0);
  __ Fabs(d2, d0);
  __ Fneg(d3, d0);
  //   - Quiet NaN
  __ Fmov(d11, d10);
  __ Fabs(d12, d10);
  __ Fneg(d13, d10);

  // Operations that use ProcessNaN.
  //   - Signalling NaN
  __ Fsqrt(d4, d0);
  __ Frinta(d5, d0);
  __ Frintn(d6, d0);
  __ Frintz(d7, d0);
  //   - Quiet NaN
  __ Fsqrt(d14, d10);
  __ Frinta(d15, d10);
  __ Frintn(d16, d10);
  __ Frintz(d17, d10);

  // The behaviour of fcvt is checked in TEST(fcvt_sd).

  END();
  if (CAN_RUN()) {
    RUN();

    uint64_t qn_raw = DoubleToRawbits(qn);
    uint64_t sn_raw = DoubleToRawbits(sn);

    //   - Signalling NaN
    ASSERT_EQUAL_FP64(sn, d1);
    ASSERT_EQUAL_FP64(RawbitsToDouble(sn_raw & ~kDSignMask), d2);
    ASSERT_EQUAL_FP64(RawbitsToDouble(sn_raw ^ kDSignMask), d3);
    //   - Quiet NaN
    ASSERT_EQUAL_FP64(qn, d11);
    ASSERT_EQUAL_FP64(RawbitsToDouble(qn_raw & ~kDSignMask), d12);
    ASSERT_EQUAL_FP64(RawbitsToDouble(qn_raw ^ kDSignMask), d13);

    //   - Signalling NaN
    ASSERT_EQUAL_FP64(sn_proc, d4);
    ASSERT_EQUAL_FP64(sn_proc, d5);
    ASSERT_EQUAL_FP64(sn_proc, d6);
    ASSERT_EQUAL_FP64(sn_proc, d7);
    //   - Quiet NaN
    ASSERT_EQUAL_FP64(qn_proc, d14);
    ASSERT_EQUAL_FP64(qn_proc, d15);
    ASSERT_EQUAL_FP64(qn_proc, d16);
    ASSERT_EQUAL_FP64(qn_proc, d17);
  }
}


TEST(process_nan_float) {
  // Make sure that NaN propagation works correctly.
  float sn = RawbitsToFloat(0x7f951111);
  float qn = RawbitsToFloat(0x7fea1111);
  VIXL_ASSERT(IsSignallingNaN(sn));
  VIXL_ASSERT(IsQuietNaN(qn));

  // The input NaNs after passing through ProcessNaN.
  float sn_proc = RawbitsToFloat(0x7fd51111);
  float qn_proc = qn;
  VIXL_ASSERT(IsQuietNaN(sn_proc));
  VIXL_ASSERT(IsQuietNaN(qn_proc));

  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();

  // Execute a number of instructions which all use ProcessNaN, and check that
  // they all handle the NaN correctly.
  __ Fmov(s0, sn);
  __ Fmov(s10, qn);

  // Operations that always propagate NaNs unchanged, even signalling NaNs.
  //   - Signalling NaN
  __ Fmov(s1, s0);
  __ Fabs(s2, s0);
  __ Fneg(s3, s0);
  //   - Quiet NaN
  __ Fmov(s11, s10);
  __ Fabs(s12, s10);
  __ Fneg(s13, s10);

  // Operations that use ProcessNaN.
  //   - Signalling NaN
  __ Fsqrt(s4, s0);
  __ Frinta(s5, s0);
  __ Frintn(s6, s0);
  __ Frintz(s7, s0);
  //   - Quiet NaN
  __ Fsqrt(s14, s10);
  __ Frinta(s15, s10);
  __ Frintn(s16, s10);
  __ Frintz(s17, s10);

  // The behaviour of fcvt is checked in TEST(fcvt_sd).

  END();
  if (CAN_RUN()) {
    RUN();

    uint32_t qn_raw = FloatToRawbits(qn);
    uint32_t sn_raw = FloatToRawbits(sn);

    //   - Signalling NaN
    ASSERT_EQUAL_FP32(sn, s1);
    ASSERT_EQUAL_FP32(RawbitsToFloat(sn_raw & ~kSSignMask), s2);
    ASSERT_EQUAL_FP32(RawbitsToFloat(sn_raw ^ kSSignMask), s3);
    //   - Quiet NaN
    ASSERT_EQUAL_FP32(qn, s11);
    ASSERT_EQUAL_FP32(RawbitsToFloat(qn_raw & ~kSSignMask), s12);
    ASSERT_EQUAL_FP32(RawbitsToFloat(qn_raw ^ kSSignMask), s13);

    //   - Signalling NaN
    ASSERT_EQUAL_FP32(sn_proc, s4);
    ASSERT_EQUAL_FP32(sn_proc, s5);
    ASSERT_EQUAL_FP32(sn_proc, s6);
    ASSERT_EQUAL_FP32(sn_proc, s7);
    //   - Quiet NaN
    ASSERT_EQUAL_FP32(qn_proc, s14);
    ASSERT_EQUAL_FP32(qn_proc, s15);
    ASSERT_EQUAL_FP32(qn_proc, s16);
    ASSERT_EQUAL_FP32(qn_proc, s17);
  }
}

// TODO: TEST(process_nan_half) {}

static void ProcessNaNsHelper(double n, double m, double expected) {
  VIXL_ASSERT(IsNaN(n) || IsNaN(m));
  VIXL_ASSERT(IsNaN(expected));

  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();

  // Execute a number of instructions which all use ProcessNaNs, and check that
  // they all propagate NaNs correctly.
  __ Fmov(d0, n);
  __ Fmov(d1, m);

  __ Fadd(d2, d0, d1);
  __ Fsub(d3, d0, d1);
  __ Fmul(d4, d0, d1);
  __ Fdiv(d5, d0, d1);
  __ Fmax(d6, d0, d1);
  __ Fmin(d7, d0, d1);

  END();
  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP64(expected, d2);
    ASSERT_EQUAL_FP64(expected, d3);
    ASSERT_EQUAL_FP64(expected, d4);
    ASSERT_EQUAL_FP64(expected, d5);
    ASSERT_EQUAL_FP64(expected, d6);
    ASSERT_EQUAL_FP64(expected, d7);
  }
}


TEST(process_nans_double) {
  // Make sure that NaN propagation works correctly.
  double sn = RawbitsToDouble(0x7ff5555511111111);
  double sm = RawbitsToDouble(0x7ff5555522222222);
  double qn = RawbitsToDouble(0x7ffaaaaa11111111);
  double qm = RawbitsToDouble(0x7ffaaaaa22222222);
  VIXL_ASSERT(IsSignallingNaN(sn));
  VIXL_ASSERT(IsSignallingNaN(sm));
  VIXL_ASSERT(IsQuietNaN(qn));
  VIXL_ASSERT(IsQuietNaN(qm));

  // The input NaNs after passing through ProcessNaN.
  double sn_proc = RawbitsToDouble(0x7ffd555511111111);
  double sm_proc = RawbitsToDouble(0x7ffd555522222222);
  double qn_proc = qn;
  double qm_proc = qm;
  VIXL_ASSERT(IsQuietNaN(sn_proc));
  VIXL_ASSERT(IsQuietNaN(sm_proc));
  VIXL_ASSERT(IsQuietNaN(qn_proc));
  VIXL_ASSERT(IsQuietNaN(qm_proc));

  // Quiet NaNs are propagated.
  ProcessNaNsHelper(qn, 0, qn_proc);
  ProcessNaNsHelper(0, qm, qm_proc);
  ProcessNaNsHelper(qn, qm, qn_proc);

  // Signalling NaNs are propagated, and made quiet.
  ProcessNaNsHelper(sn, 0, sn_proc);
  ProcessNaNsHelper(0, sm, sm_proc);
  ProcessNaNsHelper(sn, sm, sn_proc);

  // Signalling NaNs take precedence over quiet NaNs.
  ProcessNaNsHelper(sn, qm, sn_proc);
  ProcessNaNsHelper(qn, sm, sm_proc);
  ProcessNaNsHelper(sn, sm, sn_proc);
}


static void ProcessNaNsHelper(float n, float m, float expected) {
  VIXL_ASSERT(IsNaN(n) || IsNaN(m));
  VIXL_ASSERT(IsNaN(expected));

  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();

  // Execute a number of instructions which all use ProcessNaNs, and check that
  // they all propagate NaNs correctly.
  __ Fmov(s0, n);
  __ Fmov(s1, m);

  __ Fadd(s2, s0, s1);
  __ Fsub(s3, s0, s1);
  __ Fmul(s4, s0, s1);
  __ Fdiv(s5, s0, s1);
  __ Fmax(s6, s0, s1);
  __ Fmin(s7, s0, s1);

  END();
  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(expected, s2);
    ASSERT_EQUAL_FP32(expected, s3);
    ASSERT_EQUAL_FP32(expected, s4);
    ASSERT_EQUAL_FP32(expected, s5);
    ASSERT_EQUAL_FP32(expected, s6);
    ASSERT_EQUAL_FP32(expected, s7);
  }
}


TEST(process_nans_float) {
  // Make sure that NaN propagation works correctly.
  float sn = RawbitsToFloat(0x7f951111);
  float sm = RawbitsToFloat(0x7f952222);
  float qn = RawbitsToFloat(0x7fea1111);
  float qm = RawbitsToFloat(0x7fea2222);
  VIXL_ASSERT(IsSignallingNaN(sn));
  VIXL_ASSERT(IsSignallingNaN(sm));
  VIXL_ASSERT(IsQuietNaN(qn));
  VIXL_ASSERT(IsQuietNaN(qm));

  // The input NaNs after passing through ProcessNaN.
  float sn_proc = RawbitsToFloat(0x7fd51111);
  float sm_proc = RawbitsToFloat(0x7fd52222);
  float qn_proc = qn;
  float qm_proc = qm;
  VIXL_ASSERT(IsQuietNaN(sn_proc));
  VIXL_ASSERT(IsQuietNaN(sm_proc));
  VIXL_ASSERT(IsQuietNaN(qn_proc));
  VIXL_ASSERT(IsQuietNaN(qm_proc));

  // Quiet NaNs are propagated.
  ProcessNaNsHelper(qn, 0, qn_proc);
  ProcessNaNsHelper(0, qm, qm_proc);
  ProcessNaNsHelper(qn, qm, qn_proc);

  // Signalling NaNs are propagated, and made quiet.
  ProcessNaNsHelper(sn, 0, sn_proc);
  ProcessNaNsHelper(0, sm, sm_proc);
  ProcessNaNsHelper(sn, sm, sn_proc);

  // Signalling NaNs take precedence over quiet NaNs.
  ProcessNaNsHelper(sn, qm, sn_proc);
  ProcessNaNsHelper(qn, sm, sm_proc);
  ProcessNaNsHelper(sn, sm, sn_proc);
}


static void ProcessNaNsHelper(Float16 n, Float16 m, Float16 expected) {
  VIXL_ASSERT(IsNaN(n) || IsNaN(m));
  VIXL_ASSERT(IsNaN(expected));

  SETUP_WITH_FEATURES(CPUFeatures::kFP, CPUFeatures::kFPHalf);

  START();

  // Execute a number of instructions which all use ProcessNaNs, and check that
  // they all propagate NaNs correctly.
  __ Fmov(h0, n);
  __ Fmov(h1, m);

  __ Fadd(h2, h0, h1);
  __ Fsub(h3, h0, h1);
  __ Fmul(h4, h0, h1);
  __ Fdiv(h5, h0, h1);
  __ Fmax(h6, h0, h1);
  __ Fmin(h7, h0, h1);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_FP16(expected, h2);
    ASSERT_EQUAL_FP16(expected, h3);
    ASSERT_EQUAL_FP16(expected, h4);
    ASSERT_EQUAL_FP16(expected, h5);
    ASSERT_EQUAL_FP16(expected, h6);
    ASSERT_EQUAL_FP16(expected, h7);
  }
}


TEST(process_nans_half) {
  // Make sure that NaN propagation works correctly.
  Float16 sn(RawbitsToFloat16(0x7c11));
  Float16 sm(RawbitsToFloat16(0xfc22));
  Float16 qn(RawbitsToFloat16(0x7e33));
  Float16 qm(RawbitsToFloat16(0xfe44));
  VIXL_ASSERT(IsSignallingNaN(sn));
  VIXL_ASSERT(IsSignallingNaN(sm));
  VIXL_ASSERT(IsQuietNaN(qn));
  VIXL_ASSERT(IsQuietNaN(qm));

  // The input NaNs after passing through ProcessNaN.
  Float16 sn_proc(RawbitsToFloat16(0x7e11));
  Float16 sm_proc(RawbitsToFloat16(0xfe22));
  Float16 qn_proc = qn;
  Float16 qm_proc = qm;
  VIXL_ASSERT(IsQuietNaN(sn_proc));
  VIXL_ASSERT(IsQuietNaN(sm_proc));
  VIXL_ASSERT(IsQuietNaN(qn_proc));
  VIXL_ASSERT(IsQuietNaN(qm_proc));

  // Quiet NaNs are propagated.
  ProcessNaNsHelper(qn, Float16(), qn_proc);
  ProcessNaNsHelper(Float16(), qm, qm_proc);
  ProcessNaNsHelper(qn, qm, qn_proc);

  // Signalling NaNs are propagated, and made quiet.
  ProcessNaNsHelper(sn, Float16(), sn_proc);
  ProcessNaNsHelper(Float16(), sm, sm_proc);
  ProcessNaNsHelper(sn, sm, sn_proc);

  // Signalling NaNs take precedence over quiet NaNs.
  ProcessNaNsHelper(sn, qm, sn_proc);
  ProcessNaNsHelper(qn, sm, sm_proc);
  ProcessNaNsHelper(sn, sm, sn_proc);
}


static void DefaultNaNHelper(float n, float m, float a) {
  VIXL_ASSERT(IsNaN(n) || IsNaN(m) || IsNaN(a));

  bool test_1op = IsNaN(n);
  bool test_2op = IsNaN(n) || IsNaN(m);

  SETUP_WITH_FEATURES(CPUFeatures::kFP);
  START();

  // Enable Default-NaN mode in the FPCR.
  __ Mrs(x0, FPCR);
  __ Orr(x1, x0, DN_mask);
  __ Msr(FPCR, x1);

  // Execute a number of instructions which all use ProcessNaNs, and check that
  // they all produce the default NaN.
  __ Fmov(s0, n);
  __ Fmov(s1, m);
  __ Fmov(s2, a);

  if (test_1op) {
    // Operations that always propagate NaNs unchanged, even signalling NaNs.
    __ Fmov(s10, s0);
    __ Fabs(s11, s0);
    __ Fneg(s12, s0);

    // Operations that use ProcessNaN.
    __ Fsqrt(s13, s0);
    __ Frinta(s14, s0);
    __ Frintn(s15, s0);
    __ Frintz(s16, s0);

    // Fcvt usually has special NaN handling, but it respects default-NaN mode.
    __ Fcvt(d17, s0);
  }

  if (test_2op) {
    __ Fadd(s18, s0, s1);
    __ Fsub(s19, s0, s1);
    __ Fmul(s20, s0, s1);
    __ Fdiv(s21, s0, s1);
    __ Fmax(s22, s0, s1);
    __ Fmin(s23, s0, s1);
  }

  __ Fmadd(s24, s0, s1, s2);
  __ Fmsub(s25, s0, s1, s2);
  __ Fnmadd(s26, s0, s1, s2);
  __ Fnmsub(s27, s0, s1, s2);

  // Restore FPCR.
  __ Msr(FPCR, x0);

  END();
  if (CAN_RUN()) {
    RUN();

    if (test_1op) {
      uint32_t n_raw = FloatToRawbits(n);
      ASSERT_EQUAL_FP32(n, s10);
      ASSERT_EQUAL_FP32(RawbitsToFloat(n_raw & ~kSSignMask), s11);
      ASSERT_EQUAL_FP32(RawbitsToFloat(n_raw ^ kSSignMask), s12);
      ASSERT_EQUAL_FP32(kFP32DefaultNaN, s13);
      ASSERT_EQUAL_FP32(kFP32DefaultNaN, s14);
      ASSERT_EQUAL_FP32(kFP32DefaultNaN, s15);
      ASSERT_EQUAL_FP32(kFP32DefaultNaN, s16);
      ASSERT_EQUAL_FP64(kFP64DefaultNaN, d17);
    }

    if (test_2op) {
      ASSERT_EQUAL_FP32(kFP32DefaultNaN, s18);
      ASSERT_EQUAL_FP32(kFP32DefaultNaN, s19);
      ASSERT_EQUAL_FP32(kFP32DefaultNaN, s20);
      ASSERT_EQUAL_FP32(kFP32DefaultNaN, s21);
      ASSERT_EQUAL_FP32(kFP32DefaultNaN, s22);
      ASSERT_EQUAL_FP32(kFP32DefaultNaN, s23);
    }

    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s24);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s25);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s26);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s27);
  }
}


TEST(default_nan_float) {
  float sn = RawbitsToFloat(0x7f951111);
  float sm = RawbitsToFloat(0x7f952222);
  float sa = RawbitsToFloat(0x7f95aaaa);
  float qn = RawbitsToFloat(0x7fea1111);
  float qm = RawbitsToFloat(0x7fea2222);
  float qa = RawbitsToFloat(0x7feaaaaa);
  VIXL_ASSERT(IsSignallingNaN(sn));
  VIXL_ASSERT(IsSignallingNaN(sm));
  VIXL_ASSERT(IsSignallingNaN(sa));
  VIXL_ASSERT(IsQuietNaN(qn));
  VIXL_ASSERT(IsQuietNaN(qm));
  VIXL_ASSERT(IsQuietNaN(qa));

  //   - Signalling NaNs
  DefaultNaNHelper(sn, 0.0f, 0.0f);
  DefaultNaNHelper(0.0f, sm, 0.0f);
  DefaultNaNHelper(0.0f, 0.0f, sa);
  DefaultNaNHelper(sn, sm, 0.0f);
  DefaultNaNHelper(0.0f, sm, sa);
  DefaultNaNHelper(sn, 0.0f, sa);
  DefaultNaNHelper(sn, sm, sa);
  //   - Quiet NaNs
  DefaultNaNHelper(qn, 0.0f, 0.0f);
  DefaultNaNHelper(0.0f, qm, 0.0f);
  DefaultNaNHelper(0.0f, 0.0f, qa);
  DefaultNaNHelper(qn, qm, 0.0f);
  DefaultNaNHelper(0.0f, qm, qa);
  DefaultNaNHelper(qn, 0.0f, qa);
  DefaultNaNHelper(qn, qm, qa);
  //   - Mixed NaNs
  DefaultNaNHelper(qn, sm, sa);
  DefaultNaNHelper(sn, qm, sa);
  DefaultNaNHelper(sn, sm, qa);
  DefaultNaNHelper(qn, qm, sa);
  DefaultNaNHelper(sn, qm, qa);
  DefaultNaNHelper(qn, sm, qa);
  DefaultNaNHelper(qn, qm, qa);
}


static void DefaultNaNHelper(double n, double m, double a) {
  VIXL_ASSERT(IsNaN(n) || IsNaN(m) || IsNaN(a));

  bool test_1op = IsNaN(n);
  bool test_2op = IsNaN(n) || IsNaN(m);

  SETUP_WITH_FEATURES(CPUFeatures::kFP);

  START();

  // Enable Default-NaN mode in the FPCR.
  __ Mrs(x0, FPCR);
  __ Orr(x1, x0, DN_mask);
  __ Msr(FPCR, x1);

  // Execute a number of instructions which all use ProcessNaNs, and check that
  // they all produce the default NaN.
  __ Fmov(d0, n);
  __ Fmov(d1, m);
  __ Fmov(d2, a);

  if (test_1op) {
    // Operations that always propagate NaNs unchanged, even signalling NaNs.
    __ Fmov(d10, d0);
    __ Fabs(d11, d0);
    __ Fneg(d12, d0);

    // Operations that use ProcessNaN.
    __ Fsqrt(d13, d0);
    __ Frinta(d14, d0);
    __ Frintn(d15, d0);
    __ Frintz(d16, d0);

    // Fcvt usually has special NaN handling, but it respects default-NaN mode.
    __ Fcvt(s17, d0);
  }

  if (test_2op) {
    __ Fadd(d18, d0, d1);
    __ Fsub(d19, d0, d1);
    __ Fmul(d20, d0, d1);
    __ Fdiv(d21, d0, d1);
    __ Fmax(d22, d0, d1);
    __ Fmin(d23, d0, d1);
  }

  __ Fmadd(d24, d0, d1, d2);
  __ Fmsub(d25, d0, d1, d2);
  __ Fnmadd(d26, d0, d1, d2);
  __ Fnmsub(d27, d0, d1, d2);

  // Restore FPCR.
  __ Msr(FPCR, x0);

  END();
  if (CAN_RUN()) {
    RUN();

    if (test_1op) {
      uint64_t n_raw = DoubleToRawbits(n);
      ASSERT_EQUAL_FP64(n, d10);
      ASSERT_EQUAL_FP64(RawbitsToDouble(n_raw & ~kDSignMask), d11);
      ASSERT_EQUAL_FP64(RawbitsToDouble(n_raw ^ kDSignMask), d12);
      ASSERT_EQUAL_FP64(kFP64DefaultNaN, d13);
      ASSERT_EQUAL_FP64(kFP64DefaultNaN, d14);
      ASSERT_EQUAL_FP64(kFP64DefaultNaN, d15);
      ASSERT_EQUAL_FP64(kFP64DefaultNaN, d16);
      ASSERT_EQUAL_FP32(kFP32DefaultNaN, s17);
    }

    if (test_2op) {
      ASSERT_EQUAL_FP64(kFP64DefaultNaN, d18);
      ASSERT_EQUAL_FP64(kFP64DefaultNaN, d19);
      ASSERT_EQUAL_FP64(kFP64DefaultNaN, d20);
      ASSERT_EQUAL_FP64(kFP64DefaultNaN, d21);
      ASSERT_EQUAL_FP64(kFP64DefaultNaN, d22);
      ASSERT_EQUAL_FP64(kFP64DefaultNaN, d23);
    }

    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d24);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d25);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d26);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d27);
  }
}


TEST(default_nan_double) {
  double sn = RawbitsToDouble(0x7ff5555511111111);
  double sm = RawbitsToDouble(0x7ff5555522222222);
  double sa = RawbitsToDouble(0x7ff55555aaaaaaaa);
  double qn = RawbitsToDouble(0x7ffaaaaa11111111);
  double qm = RawbitsToDouble(0x7ffaaaaa22222222);
  double qa = RawbitsToDouble(0x7ffaaaaaaaaaaaaa);
  VIXL_ASSERT(IsSignallingNaN(sn));
  VIXL_ASSERT(IsSignallingNaN(sm));
  VIXL_ASSERT(IsSignallingNaN(sa));
  VIXL_ASSERT(IsQuietNaN(qn));
  VIXL_ASSERT(IsQuietNaN(qm));
  VIXL_ASSERT(IsQuietNaN(qa));

  //   - Signalling NaNs
  DefaultNaNHelper(sn, 0.0, 0.0);
  DefaultNaNHelper(0.0, sm, 0.0);
  DefaultNaNHelper(0.0, 0.0, sa);
  DefaultNaNHelper(sn, sm, 0.0);
  DefaultNaNHelper(0.0, sm, sa);
  DefaultNaNHelper(sn, 0.0, sa);
  DefaultNaNHelper(sn, sm, sa);
  //   - Quiet NaNs
  DefaultNaNHelper(qn, 0.0, 0.0);
  DefaultNaNHelper(0.0, qm, 0.0);
  DefaultNaNHelper(0.0, 0.0, qa);
  DefaultNaNHelper(qn, qm, 0.0);
  DefaultNaNHelper(0.0, qm, qa);
  DefaultNaNHelper(qn, 0.0, qa);
  DefaultNaNHelper(qn, qm, qa);
  //   - Mixed NaNs
  DefaultNaNHelper(qn, sm, sa);
  DefaultNaNHelper(sn, qm, sa);
  DefaultNaNHelper(sn, sm, qa);
  DefaultNaNHelper(qn, qm, sa);
  DefaultNaNHelper(sn, qm, qa);
  DefaultNaNHelper(qn, sm, qa);
  DefaultNaNHelper(qn, qm, qa);
}

}  // namespace aarch64
}  // namespace vixl
