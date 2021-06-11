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

TEST(load_store_b) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[3] = {0x12, 0x23, 0x34};
  uint8_t dst[3] = {0, 0, 0};
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, dst_base);
  __ Mov(x19, src_base);
  __ Mov(x20, dst_base);
  __ Mov(x21, src_base);
  __ Mov(x22, dst_base);
  __ Ldr(b0, MemOperand(x17, sizeof(src[0])));
  __ Str(b0, MemOperand(x18, sizeof(dst[0]), PostIndex));
  __ Ldr(b1, MemOperand(x19, sizeof(src[0]), PostIndex));
  __ Str(b1, MemOperand(x20, 2 * sizeof(dst[0]), PreIndex));
  __ Ldr(b2, MemOperand(x21, 2 * sizeof(src[0]), PreIndex));
  __ Str(b2, MemOperand(x22, sizeof(dst[0])));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x23, q0);
    ASSERT_EQUAL_64(0x23, dst[0]);
    ASSERT_EQUAL_128(0, 0x12, q1);
    ASSERT_EQUAL_64(0x12, dst[2]);
    ASSERT_EQUAL_128(0, 0x34, q2);
    ASSERT_EQUAL_64(0x34, dst[1]);
    ASSERT_EQUAL_64(src_base, x17);
    ASSERT_EQUAL_64(dst_base + sizeof(dst[0]), x18);
    ASSERT_EQUAL_64(src_base + sizeof(src[0]), x19);
    ASSERT_EQUAL_64(dst_base + 2 * sizeof(dst[0]), x20);
    ASSERT_EQUAL_64(src_base + 2 * sizeof(src[0]), x21);
    ASSERT_EQUAL_64(dst_base, x22);
  }
}


TEST(load_store_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint16_t src[3] = {0x1234, 0x2345, 0x3456};
  uint16_t dst[3] = {0, 0, 0};
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, dst_base);
  __ Mov(x19, src_base);
  __ Mov(x20, dst_base);
  __ Mov(x21, src_base);
  __ Mov(x22, dst_base);
  __ Ldr(h0, MemOperand(x17, sizeof(src[0])));
  __ Str(h0, MemOperand(x18, sizeof(dst[0]), PostIndex));
  __ Ldr(h1, MemOperand(x19, sizeof(src[0]), PostIndex));
  __ Str(h1, MemOperand(x20, 2 * sizeof(dst[0]), PreIndex));
  __ Ldr(h2, MemOperand(x21, 2 * sizeof(src[0]), PreIndex));
  __ Str(h2, MemOperand(x22, sizeof(dst[0])));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x2345, q0);
    ASSERT_EQUAL_64(0x2345, dst[0]);
    ASSERT_EQUAL_128(0, 0x1234, q1);
    ASSERT_EQUAL_64(0x1234, dst[2]);
    ASSERT_EQUAL_128(0, 0x3456, q2);
    ASSERT_EQUAL_64(0x3456, dst[1]);
    ASSERT_EQUAL_64(src_base, x17);
    ASSERT_EQUAL_64(dst_base + sizeof(dst[0]), x18);
    ASSERT_EQUAL_64(src_base + sizeof(src[0]), x19);
    ASSERT_EQUAL_64(dst_base + 2 * sizeof(dst[0]), x20);
    ASSERT_EQUAL_64(src_base + 2 * sizeof(src[0]), x21);
    ASSERT_EQUAL_64(dst_base, x22);
  }
}


TEST(load_store_q) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[48] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x01, 0x23,
                     0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x21, 0x43, 0x65, 0x87,
                     0xa9, 0xcb, 0xed, 0x0f, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc,
                     0xde, 0xf0, 0x24, 0x46, 0x68, 0x8a, 0xac, 0xce, 0xe0, 0x02,
                     0x42, 0x64, 0x86, 0xa8, 0xca, 0xec, 0x0e, 0x20};

  uint64_t dst[6] = {0, 0, 0, 0, 0, 0};
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, dst_base);
  __ Mov(x19, src_base);
  __ Mov(x20, dst_base);
  __ Mov(x21, src_base);
  __ Mov(x22, dst_base);
  __ Ldr(q0, MemOperand(x17, 16));
  __ Str(q0, MemOperand(x18, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Str(q1, MemOperand(x20, 32, PreIndex));
  __ Ldr(q2, MemOperand(x21, 32, PreIndex));
  __ Str(q2, MemOperand(x22, 16));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xf0debc9a78563412, 0x0fedcba987654321, q0);
    ASSERT_EQUAL_64(0x0fedcba987654321, dst[0]);
    ASSERT_EQUAL_64(0xf0debc9a78563412, dst[1]);
    ASSERT_EQUAL_128(0xefcdab8967452301, 0xfedcba9876543210, q1);
    ASSERT_EQUAL_64(0xfedcba9876543210, dst[4]);
    ASSERT_EQUAL_64(0xefcdab8967452301, dst[5]);
    ASSERT_EQUAL_128(0x200eeccaa8866442, 0x02e0ceac8a684624, q2);
    ASSERT_EQUAL_64(0x02e0ceac8a684624, dst[2]);
    ASSERT_EQUAL_64(0x200eeccaa8866442, dst[3]);
    ASSERT_EQUAL_64(src_base, x17);
    ASSERT_EQUAL_64(dst_base + 16, x18);
    ASSERT_EQUAL_64(src_base + 16, x19);
    ASSERT_EQUAL_64(dst_base + 32, x20);
    ASSERT_EQUAL_64(src_base + 32, x21);
    ASSERT_EQUAL_64(dst_base, x22);
  }
}


TEST(load_store_v_regoffset) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uint8_t dst[64];
  memset(dst, 0, sizeof(dst));

  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x17, src_base + 16);
  __ Mov(x18, 1);
  __ Mov(w19, -1);
  __ Mov(x20, dst_base - 1);

  __ Ldr(b0, MemOperand(x17, x18));
  __ Ldr(b1, MemOperand(x17, x19, SXTW));

  __ Ldr(h2, MemOperand(x17, x18));
  __ Ldr(h3, MemOperand(x17, x18, UXTW, 1));
  __ Ldr(h4, MemOperand(x17, x19, SXTW, 1));
  __ Ldr(h5, MemOperand(x17, x18, LSL, 1));

  __ Ldr(s16, MemOperand(x17, x18));
  __ Ldr(s17, MemOperand(x17, x18, UXTW, 2));
  __ Ldr(s18, MemOperand(x17, x19, SXTW, 2));
  __ Ldr(s19, MemOperand(x17, x18, LSL, 2));

  __ Ldr(d20, MemOperand(x17, x18));
  __ Ldr(d21, MemOperand(x17, x18, UXTW, 3));
  __ Ldr(d22, MemOperand(x17, x19, SXTW, 3));
  __ Ldr(d23, MemOperand(x17, x18, LSL, 3));

  __ Ldr(q24, MemOperand(x17, x18));
  __ Ldr(q25, MemOperand(x17, x18, UXTW, 4));
  __ Ldr(q26, MemOperand(x17, x19, SXTW, 4));
  __ Ldr(q27, MemOperand(x17, x18, LSL, 4));

  // Store [bhsdq]27 to adjacent memory locations, then load again to check.
  __ Str(b27, MemOperand(x20, x18));
  __ Str(h27, MemOperand(x20, x18, UXTW, 1));
  __ Add(x20, x20, 8);
  __ Str(s27, MemOperand(x20, x19, SXTW, 2));
  __ Sub(x20, x20, 8);
  __ Str(d27, MemOperand(x20, x18, LSL, 3));
  __ Add(x20, x20, 32);
  __ Str(q27, MemOperand(x20, x19, SXTW, 4));

  __ Sub(x20, x20, 32);
  __ Ldr(q6, MemOperand(x20, x18));
  __ Ldr(q7, MemOperand(x20, x18, LSL, 4));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x11, q0);
    ASSERT_EQUAL_128(0, 0x0f, q1);
    ASSERT_EQUAL_128(0, 0x1211, q2);
    ASSERT_EQUAL_128(0, 0x1312, q3);
    ASSERT_EQUAL_128(0, 0x0f0e, q4);
    ASSERT_EQUAL_128(0, 0x1312, q5);
    ASSERT_EQUAL_128(0, 0x14131211, q16);
    ASSERT_EQUAL_128(0, 0x17161514, q17);
    ASSERT_EQUAL_128(0, 0x0f0e0d0c, q18);
    ASSERT_EQUAL_128(0, 0x17161514, q19);
    ASSERT_EQUAL_128(0, 0x1817161514131211, q20);
    ASSERT_EQUAL_128(0, 0x1f1e1d1c1b1a1918, q21);
    ASSERT_EQUAL_128(0, 0x0f0e0d0c0b0a0908, q22);
    ASSERT_EQUAL_128(0, 0x1f1e1d1c1b1a1918, q23);
    ASSERT_EQUAL_128(0x201f1e1d1c1b1a19, 0x1817161514131211, q24);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726252423222120, q25);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q26);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726252423222120, q27);
    ASSERT_EQUAL_128(0x2027262524232221, 0x2023222120212020, q6);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726252423222120, q7);
  }
}

TEST(ldp_stp_quad) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint64_t src[4] = {0x0123456789abcdef,
                     0xaaaaaaaa55555555,
                     0xfedcba9876543210,
                     0x55555555aaaaaaaa};
  uint64_t dst[6] = {0, 0, 0, 0, 0, 0};
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x16, src_base);
  __ Mov(x17, dst_base);
  __ Ldp(q31, q0, MemOperand(x16, 4 * sizeof(src[0]), PostIndex));
  __ Stp(q0, q31, MemOperand(x17, 2 * sizeof(dst[1]), PreIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xaaaaaaaa55555555, 0x0123456789abcdef, q31);
    ASSERT_EQUAL_128(0x55555555aaaaaaaa, 0xfedcba9876543210, q0);
    ASSERT_EQUAL_64(0, dst[0]);
    ASSERT_EQUAL_64(0, dst[1]);
    ASSERT_EQUAL_64(0xfedcba9876543210, dst[2]);
    ASSERT_EQUAL_64(0x55555555aaaaaaaa, dst[3]);
    ASSERT_EQUAL_64(0x0123456789abcdef, dst[4]);
    ASSERT_EQUAL_64(0xaaaaaaaa55555555, dst[5]);
    ASSERT_EQUAL_64(src_base + 4 * sizeof(src[0]), x16);
    ASSERT_EQUAL_64(dst_base + 2 * sizeof(dst[1]), x17);
  }
}

TEST(neon_ld1_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[32 + 5];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Ldr(q2, MemOperand(x17));  // Initialise top 64-bits of Q register.
  __ Ld1(v2.V8B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1(v3.V8B(), v4.V8B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1(v5.V4H(), v6.V4H(), v7.V4H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1(v16.V2S(), v17.V2S(), v18.V2S(), v19.V2S(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1(v30.V2S(), v31.V2S(), v0.V2S(), v1.V2S(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1(v20.V1D(), v21.V1D(), v22.V1D(), v23.V1D(), MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x0706050403020100, q2);
    ASSERT_EQUAL_128(0, 0x0807060504030201, q3);
    ASSERT_EQUAL_128(0, 0x100f0e0d0c0b0a09, q4);
    ASSERT_EQUAL_128(0, 0x0908070605040302, q5);
    ASSERT_EQUAL_128(0, 0x11100f0e0d0c0b0a, q6);
    ASSERT_EQUAL_128(0, 0x1918171615141312, q7);
    ASSERT_EQUAL_128(0, 0x0a09080706050403, q16);
    ASSERT_EQUAL_128(0, 0x1211100f0e0d0c0b, q17);
    ASSERT_EQUAL_128(0, 0x1a19181716151413, q18);
    ASSERT_EQUAL_128(0, 0x2221201f1e1d1c1b, q19);
    ASSERT_EQUAL_128(0, 0x0b0a090807060504, q30);
    ASSERT_EQUAL_128(0, 0x131211100f0e0d0c, q31);
    ASSERT_EQUAL_128(0, 0x1b1a191817161514, q0);
    ASSERT_EQUAL_128(0, 0x232221201f1e1d1c, q1);
    ASSERT_EQUAL_128(0, 0x0c0b0a0908070605, q20);
    ASSERT_EQUAL_128(0, 0x14131211100f0e0d, q21);
    ASSERT_EQUAL_128(0, 0x1c1b1a1918171615, q22);
    ASSERT_EQUAL_128(0, 0x24232221201f1e1d, q23);
  }
}


TEST(neon_ld1_d_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[32 + 5];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base + 1);
  __ Mov(x19, src_base + 2);
  __ Mov(x20, src_base + 3);
  __ Mov(x21, src_base + 4);
  __ Mov(x22, src_base + 5);
  __ Mov(x23, 1);
  __ Ldr(q2, MemOperand(x17));  // Initialise top 64-bits of Q register.
  __ Ld1(v2.V8B(), MemOperand(x17, x23, PostIndex));
  __ Ld1(v3.V8B(), v4.V8B(), MemOperand(x18, 16, PostIndex));
  __ Ld1(v5.V4H(), v6.V4H(), v7.V4H(), MemOperand(x19, 24, PostIndex));
  __ Ld1(v16.V2S(),
         v17.V2S(),
         v18.V2S(),
         v19.V2S(),
         MemOperand(x20, 32, PostIndex));
  __ Ld1(v30.V2S(),
         v31.V2S(),
         v0.V2S(),
         v1.V2S(),
         MemOperand(x21, 32, PostIndex));
  __ Ld1(v20.V1D(),
         v21.V1D(),
         v22.V1D(),
         v23.V1D(),
         MemOperand(x22, 32, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x0706050403020100, q2);
    ASSERT_EQUAL_128(0, 0x0807060504030201, q3);
    ASSERT_EQUAL_128(0, 0x100f0e0d0c0b0a09, q4);
    ASSERT_EQUAL_128(0, 0x0908070605040302, q5);
    ASSERT_EQUAL_128(0, 0x11100f0e0d0c0b0a, q6);
    ASSERT_EQUAL_128(0, 0x1918171615141312, q7);
    ASSERT_EQUAL_128(0, 0x0a09080706050403, q16);
    ASSERT_EQUAL_128(0, 0x1211100f0e0d0c0b, q17);
    ASSERT_EQUAL_128(0, 0x1a19181716151413, q18);
    ASSERT_EQUAL_128(0, 0x2221201f1e1d1c1b, q19);
    ASSERT_EQUAL_128(0, 0x0b0a090807060504, q30);
    ASSERT_EQUAL_128(0, 0x131211100f0e0d0c, q31);
    ASSERT_EQUAL_128(0, 0x1b1a191817161514, q0);
    ASSERT_EQUAL_128(0, 0x232221201f1e1d1c, q1);
    ASSERT_EQUAL_128(0, 0x0c0b0a0908070605, q20);
    ASSERT_EQUAL_128(0, 0x14131211100f0e0d, q21);
    ASSERT_EQUAL_128(0, 0x1c1b1a1918171615, q22);
    ASSERT_EQUAL_128(0, 0x24232221201f1e1d, q23);
    ASSERT_EQUAL_64(src_base + 1, x17);
    ASSERT_EQUAL_64(src_base + 1 + 16, x18);
    ASSERT_EQUAL_64(src_base + 2 + 24, x19);
    ASSERT_EQUAL_64(src_base + 3 + 32, x20);
    ASSERT_EQUAL_64(src_base + 4 + 32, x21);
    ASSERT_EQUAL_64(src_base + 5 + 32, x22);
  }
}


TEST(neon_ld1_q) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Ld1(v2.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1(v3.V16B(), v4.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1(v5.V8H(), v6.V8H(), v7.V8H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1(v16.V4S(), v17.V4S(), v18.V4S(), v19.V4S(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1(v30.V2D(), v31.V2D(), v0.V2D(), v1.V2D(), MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q2);
    ASSERT_EQUAL_128(0x100f0e0d0c0b0a09, 0x0807060504030201, q3);
    ASSERT_EQUAL_128(0x201f1e1d1c1b1a19, 0x1817161514131211, q4);
    ASSERT_EQUAL_128(0x11100f0e0d0c0b0a, 0x0908070605040302, q5);
    ASSERT_EQUAL_128(0x21201f1e1d1c1b1a, 0x1918171615141312, q6);
    ASSERT_EQUAL_128(0x31302f2e2d2c2b2a, 0x2928272625242322, q7);
    ASSERT_EQUAL_128(0x1211100f0e0d0c0b, 0x0a09080706050403, q16);
    ASSERT_EQUAL_128(0x2221201f1e1d1c1b, 0x1a19181716151413, q17);
    ASSERT_EQUAL_128(0x3231302f2e2d2c2b, 0x2a29282726252423, q18);
    ASSERT_EQUAL_128(0x4241403f3e3d3c3b, 0x3a39383736353433, q19);
    ASSERT_EQUAL_128(0x131211100f0e0d0c, 0x0b0a090807060504, q30);
    ASSERT_EQUAL_128(0x232221201f1e1d1c, 0x1b1a191817161514, q31);
    ASSERT_EQUAL_128(0x333231302f2e2d2c, 0x2b2a292827262524, q0);
    ASSERT_EQUAL_128(0x434241403f3e3d3c, 0x3b3a393837363534, q1);
  }
}


TEST(neon_ld1_q_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base + 1);
  __ Mov(x19, src_base + 2);
  __ Mov(x20, src_base + 3);
  __ Mov(x21, src_base + 4);
  __ Mov(x22, 1);
  __ Ld1(v2.V16B(), MemOperand(x17, x22, PostIndex));
  __ Ld1(v3.V16B(), v4.V16B(), MemOperand(x18, 32, PostIndex));
  __ Ld1(v5.V8H(), v6.V8H(), v7.V8H(), MemOperand(x19, 48, PostIndex));
  __ Ld1(v16.V4S(),
         v17.V4S(),
         v18.V4S(),
         v19.V4S(),
         MemOperand(x20, 64, PostIndex));
  __ Ld1(v30.V2D(),
         v31.V2D(),
         v0.V2D(),
         v1.V2D(),
         MemOperand(x21, 64, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q2);
    ASSERT_EQUAL_128(0x100f0e0d0c0b0a09, 0x0807060504030201, q3);
    ASSERT_EQUAL_128(0x201f1e1d1c1b1a19, 0x1817161514131211, q4);
    ASSERT_EQUAL_128(0x11100f0e0d0c0b0a, 0x0908070605040302, q5);
    ASSERT_EQUAL_128(0x21201f1e1d1c1b1a, 0x1918171615141312, q6);
    ASSERT_EQUAL_128(0x31302f2e2d2c2b2a, 0x2928272625242322, q7);
    ASSERT_EQUAL_128(0x1211100f0e0d0c0b, 0x0a09080706050403, q16);
    ASSERT_EQUAL_128(0x2221201f1e1d1c1b, 0x1a19181716151413, q17);
    ASSERT_EQUAL_128(0x3231302f2e2d2c2b, 0x2a29282726252423, q18);
    ASSERT_EQUAL_128(0x4241403f3e3d3c3b, 0x3a39383736353433, q19);
    ASSERT_EQUAL_128(0x131211100f0e0d0c, 0x0b0a090807060504, q30);
    ASSERT_EQUAL_128(0x232221201f1e1d1c, 0x1b1a191817161514, q31);
    ASSERT_EQUAL_128(0x333231302f2e2d2c, 0x2b2a292827262524, q0);
    ASSERT_EQUAL_128(0x434241403f3e3d3c, 0x3b3a393837363534, q1);
    ASSERT_EQUAL_64(src_base + 1, x17);
    ASSERT_EQUAL_64(src_base + 1 + 32, x18);
    ASSERT_EQUAL_64(src_base + 2 + 48, x19);
    ASSERT_EQUAL_64(src_base + 3 + 64, x20);
    ASSERT_EQUAL_64(src_base + 4 + 64, x21);
  }
}


TEST(neon_ld1_lane) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();

  // Test loading whole register by element.
  __ Mov(x17, src_base);
  for (int i = 15; i >= 0; i--) {
    __ Ld1(v0.B(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 7; i >= 0; i--) {
    __ Ld1(v1.H(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 3; i >= 0; i--) {
    __ Ld1(v2.S(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 1; i >= 0; i--) {
    __ Ld1(v3.D(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  // Test loading a single element into an initialised register.
  __ Mov(x17, src_base);
  __ Ldr(q4, MemOperand(x17));
  __ Ld1(v4.B(), 4, MemOperand(x17));
  __ Ldr(q5, MemOperand(x17));
  __ Ld1(v5.H(), 3, MemOperand(x17));
  __ Ldr(q6, MemOperand(x17));
  __ Ld1(v6.S(), 2, MemOperand(x17));
  __ Ldr(q7, MemOperand(x17));
  __ Ld1(v7.D(), 1, MemOperand(x17));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0001020304050607, 0x08090a0b0c0d0e0f, q0);
    ASSERT_EQUAL_128(0x0100020103020403, 0x0504060507060807, q1);
    ASSERT_EQUAL_128(0x0302010004030201, 0x0504030206050403, q2);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0807060504030201, q3);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050003020100, q4);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0100050403020100, q5);
    ASSERT_EQUAL_128(0x0f0e0d0c03020100, 0x0706050403020100, q6);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0706050403020100, q7);
  }
}

TEST(neon_ld2_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Ld2(v2.V8B(), v3.V8B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld2(v4.V8B(), v5.V8B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld2(v6.V4H(), v7.V4H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld2(v31.V2S(), v0.V2S(), MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x0e0c0a0806040200, q2);
    ASSERT_EQUAL_128(0, 0x0f0d0b0907050301, q3);
    ASSERT_EQUAL_128(0, 0x0f0d0b0907050301, q4);
    ASSERT_EQUAL_128(0, 0x100e0c0a08060402, q5);
    ASSERT_EQUAL_128(0, 0x0f0e0b0a07060302, q6);
    ASSERT_EQUAL_128(0, 0x11100d0c09080504, q7);
    ASSERT_EQUAL_128(0, 0x0e0d0c0b06050403, q31);
    ASSERT_EQUAL_128(0, 0x1211100f0a090807, q0);
  }
}

TEST(neon_ld2_d_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[32 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base + 1);
  __ Mov(x19, src_base + 2);
  __ Mov(x20, src_base + 3);
  __ Mov(x21, src_base + 4);
  __ Mov(x22, 1);
  __ Ld2(v2.V8B(), v3.V8B(), MemOperand(x17, x22, PostIndex));
  __ Ld2(v4.V8B(), v5.V8B(), MemOperand(x18, 16, PostIndex));
  __ Ld2(v5.V4H(), v6.V4H(), MemOperand(x19, 16, PostIndex));
  __ Ld2(v16.V2S(), v17.V2S(), MemOperand(x20, 16, PostIndex));
  __ Ld2(v31.V2S(), v0.V2S(), MemOperand(x21, 16, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x0e0c0a0806040200, q2);
    ASSERT_EQUAL_128(0, 0x0f0d0b0907050301, q3);
    ASSERT_EQUAL_128(0, 0x0f0d0b0907050301, q4);
    ASSERT_EQUAL_128(0, 0x0f0e0b0a07060302, q5);
    ASSERT_EQUAL_128(0, 0x11100d0c09080504, q6);
    ASSERT_EQUAL_128(0, 0x0e0d0c0b06050403, q16);
    ASSERT_EQUAL_128(0, 0x1211100f0a090807, q17);
    ASSERT_EQUAL_128(0, 0x0f0e0d0c07060504, q31);
    ASSERT_EQUAL_128(0, 0x131211100b0a0908, q0);

    ASSERT_EQUAL_64(src_base + 1, x17);
    ASSERT_EQUAL_64(src_base + 1 + 16, x18);
    ASSERT_EQUAL_64(src_base + 2 + 16, x19);
    ASSERT_EQUAL_64(src_base + 3 + 16, x20);
    ASSERT_EQUAL_64(src_base + 4 + 16, x21);
  }
}


TEST(neon_ld2_q) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Ld2(v2.V16B(), v3.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld2(v4.V16B(), v5.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld2(v6.V8H(), v7.V8H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld2(v16.V4S(), v17.V4S(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld2(v31.V2D(), v0.V2D(), MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1e1c1a1816141210, 0x0e0c0a0806040200, q2);
    ASSERT_EQUAL_128(0x1f1d1b1917151311, 0x0f0d0b0907050301, q3);
    ASSERT_EQUAL_128(0x1f1d1b1917151311, 0x0f0d0b0907050301, q4);
    ASSERT_EQUAL_128(0x201e1c1a18161412, 0x100e0c0a08060402, q5);
    ASSERT_EQUAL_128(0x1f1e1b1a17161312, 0x0f0e0b0a07060302, q6);
    ASSERT_EQUAL_128(0x21201d1c19181514, 0x11100d0c09080504, q7);
    ASSERT_EQUAL_128(0x1e1d1c1b16151413, 0x0e0d0c0b06050403, q16);
    ASSERT_EQUAL_128(0x2221201f1a191817, 0x1211100f0a090807, q17);
    ASSERT_EQUAL_128(0x1b1a191817161514, 0x0b0a090807060504, q31);
    ASSERT_EQUAL_128(0x232221201f1e1d1c, 0x131211100f0e0d0c, q0);
  }
}


TEST(neon_ld2_q_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base + 1);
  __ Mov(x19, src_base + 2);
  __ Mov(x20, src_base + 3);
  __ Mov(x21, src_base + 4);
  __ Mov(x22, 1);
  __ Ld2(v2.V16B(), v3.V16B(), MemOperand(x17, x22, PostIndex));
  __ Ld2(v4.V16B(), v5.V16B(), MemOperand(x18, 32, PostIndex));
  __ Ld2(v6.V8H(), v7.V8H(), MemOperand(x19, 32, PostIndex));
  __ Ld2(v16.V4S(), v17.V4S(), MemOperand(x20, 32, PostIndex));
  __ Ld2(v31.V2D(), v0.V2D(), MemOperand(x21, 32, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1e1c1a1816141210, 0x0e0c0a0806040200, q2);
    ASSERT_EQUAL_128(0x1f1d1b1917151311, 0x0f0d0b0907050301, q3);
    ASSERT_EQUAL_128(0x1f1d1b1917151311, 0x0f0d0b0907050301, q4);
    ASSERT_EQUAL_128(0x201e1c1a18161412, 0x100e0c0a08060402, q5);
    ASSERT_EQUAL_128(0x1f1e1b1a17161312, 0x0f0e0b0a07060302, q6);
    ASSERT_EQUAL_128(0x21201d1c19181514, 0x11100d0c09080504, q7);
    ASSERT_EQUAL_128(0x1e1d1c1b16151413, 0x0e0d0c0b06050403, q16);
    ASSERT_EQUAL_128(0x2221201f1a191817, 0x1211100f0a090807, q17);
    ASSERT_EQUAL_128(0x1b1a191817161514, 0x0b0a090807060504, q31);
    ASSERT_EQUAL_128(0x232221201f1e1d1c, 0x131211100f0e0d0c, q0);


    ASSERT_EQUAL_64(src_base + 1, x17);
    ASSERT_EQUAL_64(src_base + 1 + 32, x18);
    ASSERT_EQUAL_64(src_base + 2 + 32, x19);
    ASSERT_EQUAL_64(src_base + 3 + 32, x20);
    ASSERT_EQUAL_64(src_base + 4 + 32, x21);
  }
}


TEST(neon_ld2_lane) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();

  // Test loading whole register by element.
  __ Mov(x17, src_base);
  for (int i = 15; i >= 0; i--) {
    __ Ld2(v0.B(), v1.B(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 7; i >= 0; i--) {
    __ Ld2(v2.H(), v3.H(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 3; i >= 0; i--) {
    __ Ld2(v4.S(), v5.S(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 1; i >= 0; i--) {
    __ Ld2(v6.D(), v7.D(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  // Test loading a single element into an initialised register.
  __ Mov(x17, src_base);
  __ Mov(x4, x17);
  __ Ldr(q8, MemOperand(x4, 16, PostIndex));
  __ Ldr(q9, MemOperand(x4));
  __ Ld2(v8.B(), v9.B(), 4, MemOperand(x17));
  __ Mov(x5, x17);
  __ Ldr(q10, MemOperand(x5, 16, PostIndex));
  __ Ldr(q11, MemOperand(x5));
  __ Ld2(v10.H(), v11.H(), 3, MemOperand(x17));
  __ Mov(x6, x17);
  __ Ldr(q12, MemOperand(x6, 16, PostIndex));
  __ Ldr(q13, MemOperand(x6));
  __ Ld2(v12.S(), v13.S(), 2, MemOperand(x17));
  __ Mov(x7, x17);
  __ Ldr(q14, MemOperand(x7, 16, PostIndex));
  __ Ldr(q15, MemOperand(x7));
  __ Ld2(v14.D(), v15.D(), 1, MemOperand(x17));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0001020304050607, 0x08090a0b0c0d0e0f, q0);
    ASSERT_EQUAL_128(0x0102030405060708, 0x090a0b0c0d0e0f10, q1);
    ASSERT_EQUAL_128(0x0100020103020403, 0x0504060507060807, q2);
    ASSERT_EQUAL_128(0x0302040305040605, 0x0706080709080a09, q3);
    ASSERT_EQUAL_128(0x0302010004030201, 0x0504030206050403, q4);
    ASSERT_EQUAL_128(0x0706050408070605, 0x090807060a090807, q5);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0807060504030201, q6);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x100f0e0d0c0b0a09, q7);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050003020100, q8);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716150113121110, q9);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0100050403020100, q10);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x0302151413121110, q11);
    ASSERT_EQUAL_128(0x0f0e0d0c03020100, 0x0706050403020100, q12);
    ASSERT_EQUAL_128(0x1f1e1d1c07060504, 0x1716151413121110, q13);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0706050403020100, q14);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x1716151413121110, q15);
  }
}


TEST(neon_ld2_lane_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Mov(x19, src_base);
  __ Mov(x20, src_base);
  __ Mov(x21, src_base);
  __ Mov(x22, src_base);
  __ Mov(x23, src_base);
  __ Mov(x24, src_base);

  // Test loading whole register by element.
  for (int i = 15; i >= 0; i--) {
    __ Ld2(v0.B(), v1.B(), i, MemOperand(x17, 2, PostIndex));
  }

  for (int i = 7; i >= 0; i--) {
    __ Ld2(v2.H(), v3.H(), i, MemOperand(x18, 4, PostIndex));
  }

  for (int i = 3; i >= 0; i--) {
    __ Ld2(v4.S(), v5.S(), i, MemOperand(x19, 8, PostIndex));
  }

  for (int i = 1; i >= 0; i--) {
    __ Ld2(v6.D(), v7.D(), i, MemOperand(x20, 16, PostIndex));
  }

  // Test loading a single element into an initialised register.
  __ Mov(x25, 1);
  __ Mov(x4, x21);
  __ Ldr(q8, MemOperand(x4, 16, PostIndex));
  __ Ldr(q9, MemOperand(x4));
  __ Ld2(v8.B(), v9.B(), 4, MemOperand(x21, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Mov(x5, x22);
  __ Ldr(q10, MemOperand(x5, 16, PostIndex));
  __ Ldr(q11, MemOperand(x5));
  __ Ld2(v10.H(), v11.H(), 3, MemOperand(x22, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Mov(x6, x23);
  __ Ldr(q12, MemOperand(x6, 16, PostIndex));
  __ Ldr(q13, MemOperand(x6));
  __ Ld2(v12.S(), v13.S(), 2, MemOperand(x23, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Mov(x7, x24);
  __ Ldr(q14, MemOperand(x7, 16, PostIndex));
  __ Ldr(q15, MemOperand(x7));
  __ Ld2(v14.D(), v15.D(), 1, MemOperand(x24, x25, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x00020406080a0c0e, 0x10121416181a1c1e, q0);
    ASSERT_EQUAL_128(0x01030507090b0d0f, 0x11131517191b1d1f, q1);
    ASSERT_EQUAL_128(0x0100050409080d0c, 0x1110151419181d1c, q2);
    ASSERT_EQUAL_128(0x030207060b0a0f0e, 0x131217161b1a1f1e, q3);
    ASSERT_EQUAL_128(0x030201000b0a0908, 0x131211101b1a1918, q4);
    ASSERT_EQUAL_128(0x070605040f0e0d0c, 0x171615141f1e1d1c, q5);
    ASSERT_EQUAL_128(0x0706050403020100, 0x1716151413121110, q6);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x1f1e1d1c1b1a1918, q7);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050003020100, q8);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716150113121110, q9);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0100050403020100, q10);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x0302151413121110, q11);
    ASSERT_EQUAL_128(0x0f0e0d0c03020100, 0x0706050403020100, q12);
    ASSERT_EQUAL_128(0x1f1e1d1c07060504, 0x1716151413121110, q13);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0706050403020100, q14);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x1716151413121110, q15);


    ASSERT_EQUAL_64(src_base + 32, x17);
    ASSERT_EQUAL_64(src_base + 32, x18);
    ASSERT_EQUAL_64(src_base + 32, x19);
    ASSERT_EQUAL_64(src_base + 32, x20);
    ASSERT_EQUAL_64(src_base + 1, x21);
    ASSERT_EQUAL_64(src_base + 2, x22);
    ASSERT_EQUAL_64(src_base + 3, x23);
    ASSERT_EQUAL_64(src_base + 4, x24);
  }
}


TEST(neon_ld2_alllanes) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base + 1);
  __ Mov(x18, 1);
  __ Ld2r(v0.V8B(), v1.V8B(), MemOperand(x17));
  __ Add(x17, x17, 2);
  __ Ld2r(v2.V16B(), v3.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld2r(v4.V4H(), v5.V4H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld2r(v6.V8H(), v7.V8H(), MemOperand(x17));
  __ Add(x17, x17, 4);
  __ Ld2r(v8.V2S(), v9.V2S(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld2r(v10.V4S(), v11.V4S(), MemOperand(x17));
  __ Add(x17, x17, 8);
  __ Ld2r(v12.V2D(), v13.V2D(), MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x0101010101010101, q0);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0202020202020202, q1);
    ASSERT_EQUAL_128(0x0303030303030303, 0x0303030303030303, q2);
    ASSERT_EQUAL_128(0x0404040404040404, 0x0404040404040404, q3);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0504050405040504, q4);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0706070607060706, q5);
    ASSERT_EQUAL_128(0x0605060506050605, 0x0605060506050605, q6);
    ASSERT_EQUAL_128(0x0807080708070807, 0x0807080708070807, q7);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0c0b0a090c0b0a09, q8);
    ASSERT_EQUAL_128(0x0000000000000000, 0x100f0e0d100f0e0d, q9);
    ASSERT_EQUAL_128(0x0d0c0b0a0d0c0b0a, 0x0d0c0b0a0d0c0b0a, q10);
    ASSERT_EQUAL_128(0x11100f0e11100f0e, 0x11100f0e11100f0e, q11);
    ASSERT_EQUAL_128(0x1918171615141312, 0x1918171615141312, q12);
    ASSERT_EQUAL_128(0x21201f1e1d1c1b1a, 0x21201f1e1d1c1b1a, q13);
  }
}


TEST(neon_ld2_alllanes_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base + 1);
  __ Mov(x18, 1);
  __ Ld2r(v0.V8B(), v1.V8B(), MemOperand(x17, 2, PostIndex));
  __ Ld2r(v2.V16B(), v3.V16B(), MemOperand(x17, x18, PostIndex));
  __ Ld2r(v4.V4H(), v5.V4H(), MemOperand(x17, x18, PostIndex));
  __ Ld2r(v6.V8H(), v7.V8H(), MemOperand(x17, 4, PostIndex));
  __ Ld2r(v8.V2S(), v9.V2S(), MemOperand(x17, x18, PostIndex));
  __ Ld2r(v10.V4S(), v11.V4S(), MemOperand(x17, 8, PostIndex));
  __ Ld2r(v12.V2D(), v13.V2D(), MemOperand(x17, 16, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x0101010101010101, q0);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0202020202020202, q1);
    ASSERT_EQUAL_128(0x0303030303030303, 0x0303030303030303, q2);
    ASSERT_EQUAL_128(0x0404040404040404, 0x0404040404040404, q3);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0504050405040504, q4);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0706070607060706, q5);
    ASSERT_EQUAL_128(0x0605060506050605, 0x0605060506050605, q6);
    ASSERT_EQUAL_128(0x0807080708070807, 0x0807080708070807, q7);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0c0b0a090c0b0a09, q8);
    ASSERT_EQUAL_128(0x0000000000000000, 0x100f0e0d100f0e0d, q9);
    ASSERT_EQUAL_128(0x0d0c0b0a0d0c0b0a, 0x0d0c0b0a0d0c0b0a, q10);
    ASSERT_EQUAL_128(0x11100f0e11100f0e, 0x11100f0e11100f0e, q11);
    ASSERT_EQUAL_128(0x1918171615141312, 0x1918171615141312, q12);
    ASSERT_EQUAL_128(0x21201f1e1d1c1b1a, 0x21201f1e1d1c1b1a, q13);
    ASSERT_EQUAL_64(src_base + 34, x17);
  }
}


TEST(neon_ld3_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Ld3(v2.V8B(), v3.V8B(), v4.V8B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld3(v5.V8B(), v6.V8B(), v7.V8B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld3(v8.V4H(), v9.V4H(), v10.V4H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld3(v31.V2S(), v0.V2S(), v1.V2S(), MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x15120f0c09060300, q2);
    ASSERT_EQUAL_128(0, 0x1613100d0a070401, q3);
    ASSERT_EQUAL_128(0, 0x1714110e0b080502, q4);
    ASSERT_EQUAL_128(0, 0x1613100d0a070401, q5);
    ASSERT_EQUAL_128(0, 0x1714110e0b080502, q6);
    ASSERT_EQUAL_128(0, 0x1815120f0c090603, q7);
    ASSERT_EQUAL_128(0, 0x15140f0e09080302, q8);
    ASSERT_EQUAL_128(0, 0x171611100b0a0504, q9);
    ASSERT_EQUAL_128(0, 0x191813120d0c0706, q10);
    ASSERT_EQUAL_128(0, 0x1211100f06050403, q31);
    ASSERT_EQUAL_128(0, 0x161514130a090807, q0);
    ASSERT_EQUAL_128(0, 0x1a1918170e0d0c0b, q1);
  }
}


TEST(neon_ld3_d_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[32 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base + 1);
  __ Mov(x19, src_base + 2);
  __ Mov(x20, src_base + 3);
  __ Mov(x21, src_base + 4);
  __ Mov(x22, 1);
  __ Ld3(v2.V8B(), v3.V8B(), v4.V8B(), MemOperand(x17, x22, PostIndex));
  __ Ld3(v5.V8B(), v6.V8B(), v7.V8B(), MemOperand(x18, 24, PostIndex));
  __ Ld3(v8.V4H(), v9.V4H(), v10.V4H(), MemOperand(x19, 24, PostIndex));
  __ Ld3(v11.V2S(), v12.V2S(), v13.V2S(), MemOperand(x20, 24, PostIndex));
  __ Ld3(v31.V2S(), v0.V2S(), v1.V2S(), MemOperand(x21, 24, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x15120f0c09060300, q2);
    ASSERT_EQUAL_128(0, 0x1613100d0a070401, q3);
    ASSERT_EQUAL_128(0, 0x1714110e0b080502, q4);
    ASSERT_EQUAL_128(0, 0x1613100d0a070401, q5);
    ASSERT_EQUAL_128(0, 0x1714110e0b080502, q6);
    ASSERT_EQUAL_128(0, 0x1815120f0c090603, q7);
    ASSERT_EQUAL_128(0, 0x15140f0e09080302, q8);
    ASSERT_EQUAL_128(0, 0x171611100b0a0504, q9);
    ASSERT_EQUAL_128(0, 0x191813120d0c0706, q10);
    ASSERT_EQUAL_128(0, 0x1211100f06050403, q11);
    ASSERT_EQUAL_128(0, 0x161514130a090807, q12);
    ASSERT_EQUAL_128(0, 0x1a1918170e0d0c0b, q13);
    ASSERT_EQUAL_128(0, 0x1312111007060504, q31);
    ASSERT_EQUAL_128(0, 0x171615140b0a0908, q0);
    ASSERT_EQUAL_128(0, 0x1b1a19180f0e0d0c, q1);

    ASSERT_EQUAL_64(src_base + 1, x17);
    ASSERT_EQUAL_64(src_base + 1 + 24, x18);
    ASSERT_EQUAL_64(src_base + 2 + 24, x19);
    ASSERT_EQUAL_64(src_base + 3 + 24, x20);
    ASSERT_EQUAL_64(src_base + 4 + 24, x21);
  }
}


TEST(neon_ld3_q) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Ld3(v2.V16B(), v3.V16B(), v4.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld3(v5.V16B(), v6.V16B(), v7.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld3(v8.V8H(), v9.V8H(), v10.V8H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld3(v11.V4S(), v12.V4S(), v13.V4S(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld3(v31.V2D(), v0.V2D(), v1.V2D(), MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x2d2a2724211e1b18, 0x15120f0c09060300, q2);
    ASSERT_EQUAL_128(0x2e2b2825221f1c19, 0x1613100d0a070401, q3);
    ASSERT_EQUAL_128(0x2f2c292623201d1a, 0x1714110e0b080502, q4);
    ASSERT_EQUAL_128(0x2e2b2825221f1c19, 0x1613100d0a070401, q5);
    ASSERT_EQUAL_128(0x2f2c292623201d1a, 0x1714110e0b080502, q6);
    ASSERT_EQUAL_128(0x302d2a2724211e1b, 0x1815120f0c090603, q7);
    ASSERT_EQUAL_128(0x2d2c272621201b1a, 0x15140f0e09080302, q8);
    ASSERT_EQUAL_128(0x2f2e292823221d1c, 0x171611100b0a0504, q9);
    ASSERT_EQUAL_128(0x31302b2a25241f1e, 0x191813120d0c0706, q10);
    ASSERT_EQUAL_128(0x2a2928271e1d1c1b, 0x1211100f06050403, q11);
    ASSERT_EQUAL_128(0x2e2d2c2b2221201f, 0x161514130a090807, q12);
    ASSERT_EQUAL_128(0x3231302f26252423, 0x1a1918170e0d0c0b, q13);
    ASSERT_EQUAL_128(0x232221201f1e1d1c, 0x0b0a090807060504, q31);
    ASSERT_EQUAL_128(0x2b2a292827262524, 0x131211100f0e0d0c, q0);
    ASSERT_EQUAL_128(0x333231302f2e2d2c, 0x1b1a191817161514, q1);
  }
}


TEST(neon_ld3_q_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base + 1);
  __ Mov(x19, src_base + 2);
  __ Mov(x20, src_base + 3);
  __ Mov(x21, src_base + 4);
  __ Mov(x22, 1);

  __ Ld3(v2.V16B(), v3.V16B(), v4.V16B(), MemOperand(x17, x22, PostIndex));
  __ Ld3(v5.V16B(), v6.V16B(), v7.V16B(), MemOperand(x18, 48, PostIndex));
  __ Ld3(v8.V8H(), v9.V8H(), v10.V8H(), MemOperand(x19, 48, PostIndex));
  __ Ld3(v11.V4S(), v12.V4S(), v13.V4S(), MemOperand(x20, 48, PostIndex));
  __ Ld3(v31.V2D(), v0.V2D(), v1.V2D(), MemOperand(x21, 48, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x2d2a2724211e1b18, 0x15120f0c09060300, q2);
    ASSERT_EQUAL_128(0x2e2b2825221f1c19, 0x1613100d0a070401, q3);
    ASSERT_EQUAL_128(0x2f2c292623201d1a, 0x1714110e0b080502, q4);
    ASSERT_EQUAL_128(0x2e2b2825221f1c19, 0x1613100d0a070401, q5);
    ASSERT_EQUAL_128(0x2f2c292623201d1a, 0x1714110e0b080502, q6);
    ASSERT_EQUAL_128(0x302d2a2724211e1b, 0x1815120f0c090603, q7);
    ASSERT_EQUAL_128(0x2d2c272621201b1a, 0x15140f0e09080302, q8);
    ASSERT_EQUAL_128(0x2f2e292823221d1c, 0x171611100b0a0504, q9);
    ASSERT_EQUAL_128(0x31302b2a25241f1e, 0x191813120d0c0706, q10);
    ASSERT_EQUAL_128(0x2a2928271e1d1c1b, 0x1211100f06050403, q11);
    ASSERT_EQUAL_128(0x2e2d2c2b2221201f, 0x161514130a090807, q12);
    ASSERT_EQUAL_128(0x3231302f26252423, 0x1a1918170e0d0c0b, q13);
    ASSERT_EQUAL_128(0x232221201f1e1d1c, 0x0b0a090807060504, q31);
    ASSERT_EQUAL_128(0x2b2a292827262524, 0x131211100f0e0d0c, q0);
    ASSERT_EQUAL_128(0x333231302f2e2d2c, 0x1b1a191817161514, q1);

    ASSERT_EQUAL_64(src_base + 1, x17);
    ASSERT_EQUAL_64(src_base + 1 + 48, x18);
    ASSERT_EQUAL_64(src_base + 2 + 48, x19);
    ASSERT_EQUAL_64(src_base + 3 + 48, x20);
    ASSERT_EQUAL_64(src_base + 4 + 48, x21);
  }
}


TEST(neon_ld3_lane) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();

  // Test loading whole register by element.
  __ Mov(x17, src_base);
  for (int i = 15; i >= 0; i--) {
    __ Ld3(v0.B(), v1.B(), v2.B(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 7; i >= 0; i--) {
    __ Ld3(v3.H(), v4.H(), v5.H(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 3; i >= 0; i--) {
    __ Ld3(v6.S(), v7.S(), v8.S(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 1; i >= 0; i--) {
    __ Ld3(v9.D(), v10.D(), v11.D(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  // Test loading a single element into an initialised register.
  __ Mov(x17, src_base);
  __ Mov(x4, x17);
  __ Ldr(q12, MemOperand(x4, 16, PostIndex));
  __ Ldr(q13, MemOperand(x4, 16, PostIndex));
  __ Ldr(q14, MemOperand(x4));
  __ Ld3(v12.B(), v13.B(), v14.B(), 4, MemOperand(x17));
  __ Mov(x5, x17);
  __ Ldr(q15, MemOperand(x5, 16, PostIndex));
  __ Ldr(q16, MemOperand(x5, 16, PostIndex));
  __ Ldr(q17, MemOperand(x5));
  __ Ld3(v15.H(), v16.H(), v17.H(), 3, MemOperand(x17));
  __ Mov(x6, x17);
  __ Ldr(q18, MemOperand(x6, 16, PostIndex));
  __ Ldr(q19, MemOperand(x6, 16, PostIndex));
  __ Ldr(q20, MemOperand(x6));
  __ Ld3(v18.S(), v19.S(), v20.S(), 2, MemOperand(x17));
  __ Mov(x7, x17);
  __ Ldr(q21, MemOperand(x7, 16, PostIndex));
  __ Ldr(q22, MemOperand(x7, 16, PostIndex));
  __ Ldr(q23, MemOperand(x7));
  __ Ld3(v21.D(), v22.D(), v23.D(), 1, MemOperand(x17));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0001020304050607, 0x08090a0b0c0d0e0f, q0);
    ASSERT_EQUAL_128(0x0102030405060708, 0x090a0b0c0d0e0f10, q1);
    ASSERT_EQUAL_128(0x0203040506070809, 0x0a0b0c0d0e0f1011, q2);
    ASSERT_EQUAL_128(0x0100020103020403, 0x0504060507060807, q3);
    ASSERT_EQUAL_128(0x0302040305040605, 0x0706080709080a09, q4);
    ASSERT_EQUAL_128(0x0504060507060807, 0x09080a090b0a0c0b, q5);
    ASSERT_EQUAL_128(0x0302010004030201, 0x0504030206050403, q6);
    ASSERT_EQUAL_128(0x0706050408070605, 0x090807060a090807, q7);
    ASSERT_EQUAL_128(0x0b0a09080c0b0a09, 0x0d0c0b0a0e0d0c0b, q8);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0807060504030201, q9);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x100f0e0d0c0b0a09, q10);
    ASSERT_EQUAL_128(0x1716151413121110, 0x1817161514131211, q11);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050003020100, q12);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716150113121110, q13);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726250223222120, q14);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0100050403020100, q15);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x0302151413121110, q16);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x0504252423222120, q17);
  }
}


TEST(neon_ld3_lane_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();

  // Test loading whole register by element.
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Mov(x19, src_base);
  __ Mov(x20, src_base);
  __ Mov(x21, src_base);
  __ Mov(x22, src_base);
  __ Mov(x23, src_base);
  __ Mov(x24, src_base);
  for (int i = 15; i >= 0; i--) {
    __ Ld3(v0.B(), v1.B(), v2.B(), i, MemOperand(x17, 3, PostIndex));
  }

  for (int i = 7; i >= 0; i--) {
    __ Ld3(v3.H(), v4.H(), v5.H(), i, MemOperand(x18, 6, PostIndex));
  }

  for (int i = 3; i >= 0; i--) {
    __ Ld3(v6.S(), v7.S(), v8.S(), i, MemOperand(x19, 12, PostIndex));
  }

  for (int i = 1; i >= 0; i--) {
    __ Ld3(v9.D(), v10.D(), v11.D(), i, MemOperand(x20, 24, PostIndex));
  }


  // Test loading a single element into an initialised register.
  __ Mov(x25, 1);
  __ Mov(x4, x21);
  __ Ldr(q12, MemOperand(x4, 16, PostIndex));
  __ Ldr(q13, MemOperand(x4, 16, PostIndex));
  __ Ldr(q14, MemOperand(x4));
  __ Ld3(v12.B(), v13.B(), v14.B(), 4, MemOperand(x21, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Mov(x5, x22);
  __ Ldr(q15, MemOperand(x5, 16, PostIndex));
  __ Ldr(q16, MemOperand(x5, 16, PostIndex));
  __ Ldr(q17, MemOperand(x5));
  __ Ld3(v15.H(), v16.H(), v17.H(), 3, MemOperand(x22, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Mov(x6, x23);
  __ Ldr(q18, MemOperand(x6, 16, PostIndex));
  __ Ldr(q19, MemOperand(x6, 16, PostIndex));
  __ Ldr(q20, MemOperand(x6));
  __ Ld3(v18.S(), v19.S(), v20.S(), 2, MemOperand(x23, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Mov(x7, x24);
  __ Ldr(q21, MemOperand(x7, 16, PostIndex));
  __ Ldr(q22, MemOperand(x7, 16, PostIndex));
  __ Ldr(q23, MemOperand(x7));
  __ Ld3(v21.D(), v22.D(), v23.D(), 1, MemOperand(x24, x25, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x000306090c0f1215, 0x181b1e2124272a2d, q0);
    ASSERT_EQUAL_128(0x0104070a0d101316, 0x191c1f2225282b2e, q1);
    ASSERT_EQUAL_128(0x0205080b0e111417, 0x1a1d202326292c2f, q2);
    ASSERT_EQUAL_128(0x010007060d0c1312, 0x19181f1e25242b2a, q3);
    ASSERT_EQUAL_128(0x030209080f0e1514, 0x1b1a212027262d2c, q4);
    ASSERT_EQUAL_128(0x05040b0a11101716, 0x1d1c232229282f2e, q5);
    ASSERT_EQUAL_128(0x030201000f0e0d0c, 0x1b1a191827262524, q6);
    ASSERT_EQUAL_128(0x0706050413121110, 0x1f1e1d1c2b2a2928, q7);
    ASSERT_EQUAL_128(0x0b0a090817161514, 0x232221202f2e2d2c, q8);
    ASSERT_EQUAL_128(0x0706050403020100, 0x1f1e1d1c1b1a1918, q9);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x2726252423222120, q10);
    ASSERT_EQUAL_128(0x1716151413121110, 0x2f2e2d2c2b2a2928, q11);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050003020100, q12);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716150113121110, q13);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726250223222120, q14);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0100050403020100, q15);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x0302151413121110, q16);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x0504252423222120, q17);
    ASSERT_EQUAL_128(0x0f0e0d0c03020100, 0x0706050403020100, q18);
    ASSERT_EQUAL_128(0x1f1e1d1c07060504, 0x1716151413121110, q19);
    ASSERT_EQUAL_128(0x2f2e2d2c0b0a0908, 0x2726252423222120, q20);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0706050403020100, q21);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x1716151413121110, q22);
    ASSERT_EQUAL_128(0x1716151413121110, 0x2726252423222120, q23);

    ASSERT_EQUAL_64(src_base + 48, x17);
    ASSERT_EQUAL_64(src_base + 48, x18);
    ASSERT_EQUAL_64(src_base + 48, x19);
    ASSERT_EQUAL_64(src_base + 48, x20);
    ASSERT_EQUAL_64(src_base + 1, x21);
    ASSERT_EQUAL_64(src_base + 2, x22);
    ASSERT_EQUAL_64(src_base + 3, x23);
    ASSERT_EQUAL_64(src_base + 4, x24);
  }
}


TEST(neon_ld3_alllanes) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base + 1);
  __ Mov(x18, 1);
  __ Ld3r(v0.V8B(), v1.V8B(), v2.V8B(), MemOperand(x17));
  __ Add(x17, x17, 3);
  __ Ld3r(v3.V16B(), v4.V16B(), v5.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld3r(v6.V4H(), v7.V4H(), v8.V4H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld3r(v9.V8H(), v10.V8H(), v11.V8H(), MemOperand(x17));
  __ Add(x17, x17, 6);
  __ Ld3r(v12.V2S(), v13.V2S(), v14.V2S(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld3r(v15.V4S(), v16.V4S(), v17.V4S(), MemOperand(x17));
  __ Add(x17, x17, 12);
  __ Ld3r(v18.V2D(), v19.V2D(), v20.V2D(), MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x0101010101010101, q0);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0202020202020202, q1);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0303030303030303, q2);
    ASSERT_EQUAL_128(0x0404040404040404, 0x0404040404040404, q3);
    ASSERT_EQUAL_128(0x0505050505050505, 0x0505050505050505, q4);
    ASSERT_EQUAL_128(0x0606060606060606, 0x0606060606060606, q5);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0605060506050605, q6);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0807080708070807, q7);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0a090a090a090a09, q8);
    ASSERT_EQUAL_128(0x0706070607060706, 0x0706070607060706, q9);
    ASSERT_EQUAL_128(0x0908090809080908, 0x0908090809080908, q10);
    ASSERT_EQUAL_128(0x0b0a0b0a0b0a0b0a, 0x0b0a0b0a0b0a0b0a, q11);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0f0e0d0c0f0e0d0c, q12);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1312111013121110, q13);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1716151417161514, q14);
    ASSERT_EQUAL_128(0x100f0e0d100f0e0d, 0x100f0e0d100f0e0d, q15);
    ASSERT_EQUAL_128(0x1413121114131211, 0x1413121114131211, q16);
    ASSERT_EQUAL_128(0x1817161518171615, 0x1817161518171615, q17);
    ASSERT_EQUAL_128(0x201f1e1d1c1b1a19, 0x201f1e1d1c1b1a19, q18);
    ASSERT_EQUAL_128(0x2827262524232221, 0x2827262524232221, q19);
    ASSERT_EQUAL_128(0x302f2e2d2c2b2a29, 0x302f2e2d2c2b2a29, q20);
  }
}


TEST(neon_ld3_alllanes_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  __ Mov(x17, src_base + 1);
  __ Mov(x18, 1);

  START();
  __ Mov(x17, src_base + 1);
  __ Mov(x18, 1);
  __ Ld3r(v0.V8B(), v1.V8B(), v2.V8B(), MemOperand(x17, 3, PostIndex));
  __ Ld3r(v3.V16B(), v4.V16B(), v5.V16B(), MemOperand(x17, x18, PostIndex));
  __ Ld3r(v6.V4H(), v7.V4H(), v8.V4H(), MemOperand(x17, x18, PostIndex));
  __ Ld3r(v9.V8H(), v10.V8H(), v11.V8H(), MemOperand(x17, 6, PostIndex));
  __ Ld3r(v12.V2S(), v13.V2S(), v14.V2S(), MemOperand(x17, x18, PostIndex));
  __ Ld3r(v15.V4S(), v16.V4S(), v17.V4S(), MemOperand(x17, 12, PostIndex));
  __ Ld3r(v18.V2D(), v19.V2D(), v20.V2D(), MemOperand(x17, 24, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x0101010101010101, q0);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0202020202020202, q1);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0303030303030303, q2);
    ASSERT_EQUAL_128(0x0404040404040404, 0x0404040404040404, q3);
    ASSERT_EQUAL_128(0x0505050505050505, 0x0505050505050505, q4);
    ASSERT_EQUAL_128(0x0606060606060606, 0x0606060606060606, q5);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0605060506050605, q6);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0807080708070807, q7);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0a090a090a090a09, q8);
    ASSERT_EQUAL_128(0x0706070607060706, 0x0706070607060706, q9);
    ASSERT_EQUAL_128(0x0908090809080908, 0x0908090809080908, q10);
    ASSERT_EQUAL_128(0x0b0a0b0a0b0a0b0a, 0x0b0a0b0a0b0a0b0a, q11);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0f0e0d0c0f0e0d0c, q12);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1312111013121110, q13);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1716151417161514, q14);
    ASSERT_EQUAL_128(0x100f0e0d100f0e0d, 0x100f0e0d100f0e0d, q15);
    ASSERT_EQUAL_128(0x1413121114131211, 0x1413121114131211, q16);
    ASSERT_EQUAL_128(0x1817161518171615, 0x1817161518171615, q17);
    ASSERT_EQUAL_128(0x201f1e1d1c1b1a19, 0x201f1e1d1c1b1a19, q18);
    ASSERT_EQUAL_128(0x2827262524232221, 0x2827262524232221, q19);
    ASSERT_EQUAL_128(0x302f2e2d2c2b2a29, 0x302f2e2d2c2b2a29, q20);
  }
}


TEST(neon_ld4_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Ld4(v2.V8B(), v3.V8B(), v4.V8B(), v5.V8B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld4(v6.V8B(), v7.V8B(), v8.V8B(), v9.V8B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld4(v10.V4H(), v11.V4H(), v12.V4H(), v13.V4H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld4(v30.V2S(), v31.V2S(), v0.V2S(), v1.V2S(), MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x1c1814100c080400, q2);
    ASSERT_EQUAL_128(0, 0x1d1915110d090501, q3);
    ASSERT_EQUAL_128(0, 0x1e1a16120e0a0602, q4);
    ASSERT_EQUAL_128(0, 0x1f1b17130f0b0703, q5);
    ASSERT_EQUAL_128(0, 0x1d1915110d090501, q6);
    ASSERT_EQUAL_128(0, 0x1e1a16120e0a0602, q7);
    ASSERT_EQUAL_128(0, 0x1f1b17130f0b0703, q8);
    ASSERT_EQUAL_128(0, 0x201c1814100c0804, q9);
    ASSERT_EQUAL_128(0, 0x1b1a13120b0a0302, q10);
    ASSERT_EQUAL_128(0, 0x1d1c15140d0c0504, q11);
    ASSERT_EQUAL_128(0, 0x1f1e17160f0e0706, q12);
    ASSERT_EQUAL_128(0, 0x2120191811100908, q13);
    ASSERT_EQUAL_128(0, 0x1615141306050403, q30);
    ASSERT_EQUAL_128(0, 0x1a1918170a090807, q31);
    ASSERT_EQUAL_128(0, 0x1e1d1c1b0e0d0c0b, q0);
    ASSERT_EQUAL_128(0, 0x2221201f1211100f, q1);
  }
}


TEST(neon_ld4_d_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[32 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base + 1);
  __ Mov(x19, src_base + 2);
  __ Mov(x20, src_base + 3);
  __ Mov(x21, src_base + 4);
  __ Mov(x22, 1);
  __ Ld4(v2.V8B(),
         v3.V8B(),
         v4.V8B(),
         v5.V8B(),
         MemOperand(x17, x22, PostIndex));
  __ Ld4(v6.V8B(),
         v7.V8B(),
         v8.V8B(),
         v9.V8B(),
         MemOperand(x18, 32, PostIndex));
  __ Ld4(v10.V4H(),
         v11.V4H(),
         v12.V4H(),
         v13.V4H(),
         MemOperand(x19, 32, PostIndex));
  __ Ld4(v14.V2S(),
         v15.V2S(),
         v16.V2S(),
         v17.V2S(),
         MemOperand(x20, 32, PostIndex));
  __ Ld4(v30.V2S(),
         v31.V2S(),
         v0.V2S(),
         v1.V2S(),
         MemOperand(x21, 32, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x1c1814100c080400, q2);
    ASSERT_EQUAL_128(0, 0x1d1915110d090501, q3);
    ASSERT_EQUAL_128(0, 0x1e1a16120e0a0602, q4);
    ASSERT_EQUAL_128(0, 0x1f1b17130f0b0703, q5);
    ASSERT_EQUAL_128(0, 0x1d1915110d090501, q6);
    ASSERT_EQUAL_128(0, 0x1e1a16120e0a0602, q7);
    ASSERT_EQUAL_128(0, 0x1f1b17130f0b0703, q8);
    ASSERT_EQUAL_128(0, 0x201c1814100c0804, q9);
    ASSERT_EQUAL_128(0, 0x1b1a13120b0a0302, q10);
    ASSERT_EQUAL_128(0, 0x1d1c15140d0c0504, q11);
    ASSERT_EQUAL_128(0, 0x1f1e17160f0e0706, q12);
    ASSERT_EQUAL_128(0, 0x2120191811100908, q13);
    ASSERT_EQUAL_128(0, 0x1615141306050403, q14);
    ASSERT_EQUAL_128(0, 0x1a1918170a090807, q15);
    ASSERT_EQUAL_128(0, 0x1e1d1c1b0e0d0c0b, q16);
    ASSERT_EQUAL_128(0, 0x2221201f1211100f, q17);
    ASSERT_EQUAL_128(0, 0x1716151407060504, q30);
    ASSERT_EQUAL_128(0, 0x1b1a19180b0a0908, q31);
    ASSERT_EQUAL_128(0, 0x1f1e1d1c0f0e0d0c, q0);
    ASSERT_EQUAL_128(0, 0x2322212013121110, q1);


    ASSERT_EQUAL_64(src_base + 1, x17);
    ASSERT_EQUAL_64(src_base + 1 + 32, x18);
    ASSERT_EQUAL_64(src_base + 2 + 32, x19);
    ASSERT_EQUAL_64(src_base + 3 + 32, x20);
    ASSERT_EQUAL_64(src_base + 4 + 32, x21);
  }
}


TEST(neon_ld4_q) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Ld4(v2.V16B(), v3.V16B(), v4.V16B(), v5.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld4(v6.V16B(), v7.V16B(), v8.V16B(), v9.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld4(v10.V8H(), v11.V8H(), v12.V8H(), v13.V8H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld4(v14.V4S(), v15.V4S(), v16.V4S(), v17.V4S(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld4(v18.V2D(), v19.V2D(), v20.V2D(), v21.V2D(), MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x3c3834302c282420, 0x1c1814100c080400, q2);
    ASSERT_EQUAL_128(0x3d3935312d292521, 0x1d1915110d090501, q3);
    ASSERT_EQUAL_128(0x3e3a36322e2a2622, 0x1e1a16120e0a0602, q4);
    ASSERT_EQUAL_128(0x3f3b37332f2b2723, 0x1f1b17130f0b0703, q5);
    ASSERT_EQUAL_128(0x3d3935312d292521, 0x1d1915110d090501, q6);
    ASSERT_EQUAL_128(0x3e3a36322e2a2622, 0x1e1a16120e0a0602, q7);
    ASSERT_EQUAL_128(0x3f3b37332f2b2723, 0x1f1b17130f0b0703, q8);
    ASSERT_EQUAL_128(0x403c3834302c2824, 0x201c1814100c0804, q9);
    ASSERT_EQUAL_128(0x3b3a33322b2a2322, 0x1b1a13120b0a0302, q10);
    ASSERT_EQUAL_128(0x3d3c35342d2c2524, 0x1d1c15140d0c0504, q11);
    ASSERT_EQUAL_128(0x3f3e37362f2e2726, 0x1f1e17160f0e0706, q12);
    ASSERT_EQUAL_128(0x4140393831302928, 0x2120191811100908, q13);
    ASSERT_EQUAL_128(0x3635343326252423, 0x1615141306050403, q14);
    ASSERT_EQUAL_128(0x3a3938372a292827, 0x1a1918170a090807, q15);
    ASSERT_EQUAL_128(0x3e3d3c3b2e2d2c2b, 0x1e1d1c1b0e0d0c0b, q16);
    ASSERT_EQUAL_128(0x4241403f3231302f, 0x2221201f1211100f, q17);
    ASSERT_EQUAL_128(0x2b2a292827262524, 0x0b0a090807060504, q18);
    ASSERT_EQUAL_128(0x333231302f2e2d2c, 0x131211100f0e0d0c, q19);
    ASSERT_EQUAL_128(0x3b3a393837363534, 0x1b1a191817161514, q20);
    ASSERT_EQUAL_128(0x434241403f3e3d3c, 0x232221201f1e1d1c, q21);
  }
}


TEST(neon_ld4_q_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 4];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base + 1);
  __ Mov(x19, src_base + 2);
  __ Mov(x20, src_base + 3);
  __ Mov(x21, src_base + 4);
  __ Mov(x22, 1);

  __ Ld4(v2.V16B(),
         v3.V16B(),
         v4.V16B(),
         v5.V16B(),
         MemOperand(x17, x22, PostIndex));
  __ Ld4(v6.V16B(),
         v7.V16B(),
         v8.V16B(),
         v9.V16B(),
         MemOperand(x18, 64, PostIndex));
  __ Ld4(v10.V8H(),
         v11.V8H(),
         v12.V8H(),
         v13.V8H(),
         MemOperand(x19, 64, PostIndex));
  __ Ld4(v14.V4S(),
         v15.V4S(),
         v16.V4S(),
         v17.V4S(),
         MemOperand(x20, 64, PostIndex));
  __ Ld4(v30.V2D(),
         v31.V2D(),
         v0.V2D(),
         v1.V2D(),
         MemOperand(x21, 64, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x3c3834302c282420, 0x1c1814100c080400, q2);
    ASSERT_EQUAL_128(0x3d3935312d292521, 0x1d1915110d090501, q3);
    ASSERT_EQUAL_128(0x3e3a36322e2a2622, 0x1e1a16120e0a0602, q4);
    ASSERT_EQUAL_128(0x3f3b37332f2b2723, 0x1f1b17130f0b0703, q5);
    ASSERT_EQUAL_128(0x3d3935312d292521, 0x1d1915110d090501, q6);
    ASSERT_EQUAL_128(0x3e3a36322e2a2622, 0x1e1a16120e0a0602, q7);
    ASSERT_EQUAL_128(0x3f3b37332f2b2723, 0x1f1b17130f0b0703, q8);
    ASSERT_EQUAL_128(0x403c3834302c2824, 0x201c1814100c0804, q9);
    ASSERT_EQUAL_128(0x3b3a33322b2a2322, 0x1b1a13120b0a0302, q10);
    ASSERT_EQUAL_128(0x3d3c35342d2c2524, 0x1d1c15140d0c0504, q11);
    ASSERT_EQUAL_128(0x3f3e37362f2e2726, 0x1f1e17160f0e0706, q12);
    ASSERT_EQUAL_128(0x4140393831302928, 0x2120191811100908, q13);
    ASSERT_EQUAL_128(0x3635343326252423, 0x1615141306050403, q14);
    ASSERT_EQUAL_128(0x3a3938372a292827, 0x1a1918170a090807, q15);
    ASSERT_EQUAL_128(0x3e3d3c3b2e2d2c2b, 0x1e1d1c1b0e0d0c0b, q16);
    ASSERT_EQUAL_128(0x4241403f3231302f, 0x2221201f1211100f, q17);
    ASSERT_EQUAL_128(0x2b2a292827262524, 0x0b0a090807060504, q30);
    ASSERT_EQUAL_128(0x333231302f2e2d2c, 0x131211100f0e0d0c, q31);
    ASSERT_EQUAL_128(0x3b3a393837363534, 0x1b1a191817161514, q0);
    ASSERT_EQUAL_128(0x434241403f3e3d3c, 0x232221201f1e1d1c, q1);


    ASSERT_EQUAL_64(src_base + 1, x17);
    ASSERT_EQUAL_64(src_base + 1 + 64, x18);
    ASSERT_EQUAL_64(src_base + 2 + 64, x19);
    ASSERT_EQUAL_64(src_base + 3 + 64, x20);
    ASSERT_EQUAL_64(src_base + 4 + 64, x21);
  }
}


TEST(neon_ld4_lane) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();

  // Test loading whole register by element.
  __ Mov(x17, src_base);
  for (int i = 15; i >= 0; i--) {
    __ Ld4(v0.B(), v1.B(), v2.B(), v3.B(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 7; i >= 0; i--) {
    __ Ld4(v4.H(), v5.H(), v6.H(), v7.H(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 3; i >= 0; i--) {
    __ Ld4(v8.S(), v9.S(), v10.S(), v11.S(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  __ Mov(x17, src_base);
  for (int i = 1; i >= 0; i--) {
    __ Ld4(v12.D(), v13.D(), v14.D(), v15.D(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }

  // Test loading a single element into an initialised register.
  __ Mov(x17, src_base);
  __ Mov(x4, x17);
  __ Ldr(q16, MemOperand(x4, 16, PostIndex));
  __ Ldr(q17, MemOperand(x4, 16, PostIndex));
  __ Ldr(q18, MemOperand(x4, 16, PostIndex));
  __ Ldr(q19, MemOperand(x4));
  __ Ld4(v16.B(), v17.B(), v18.B(), v19.B(), 4, MemOperand(x17));

  __ Mov(x5, x17);
  __ Ldr(q20, MemOperand(x5, 16, PostIndex));
  __ Ldr(q21, MemOperand(x5, 16, PostIndex));
  __ Ldr(q22, MemOperand(x5, 16, PostIndex));
  __ Ldr(q23, MemOperand(x5));
  __ Ld4(v20.H(), v21.H(), v22.H(), v23.H(), 3, MemOperand(x17));

  __ Mov(x6, x17);
  __ Ldr(q24, MemOperand(x6, 16, PostIndex));
  __ Ldr(q25, MemOperand(x6, 16, PostIndex));
  __ Ldr(q26, MemOperand(x6, 16, PostIndex));
  __ Ldr(q27, MemOperand(x6));
  __ Ld4(v24.S(), v25.S(), v26.S(), v27.S(), 2, MemOperand(x17));

  __ Mov(x7, x17);
  __ Ldr(q28, MemOperand(x7, 16, PostIndex));
  __ Ldr(q29, MemOperand(x7, 16, PostIndex));
  __ Ldr(q30, MemOperand(x7, 16, PostIndex));
  __ Ldr(q31, MemOperand(x7));
  __ Ld4(v28.D(), v29.D(), v30.D(), v31.D(), 1, MemOperand(x17));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0001020304050607, 0x08090a0b0c0d0e0f, q0);
    ASSERT_EQUAL_128(0x0102030405060708, 0x090a0b0c0d0e0f10, q1);
    ASSERT_EQUAL_128(0x0203040506070809, 0x0a0b0c0d0e0f1011, q2);
    ASSERT_EQUAL_128(0x030405060708090a, 0x0b0c0d0e0f101112, q3);
    ASSERT_EQUAL_128(0x0100020103020403, 0x0504060507060807, q4);
    ASSERT_EQUAL_128(0x0302040305040605, 0x0706080709080a09, q5);
    ASSERT_EQUAL_128(0x0504060507060807, 0x09080a090b0a0c0b, q6);
    ASSERT_EQUAL_128(0x0706080709080a09, 0x0b0a0c0b0d0c0e0d, q7);
    ASSERT_EQUAL_128(0x0302010004030201, 0x0504030206050403, q8);
    ASSERT_EQUAL_128(0x0706050408070605, 0x090807060a090807, q9);
    ASSERT_EQUAL_128(0x0b0a09080c0b0a09, 0x0d0c0b0a0e0d0c0b, q10);
    ASSERT_EQUAL_128(0x0f0e0d0c100f0e0d, 0x11100f0e1211100f, q11);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0807060504030201, q12);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x100f0e0d0c0b0a09, q13);
    ASSERT_EQUAL_128(0x1716151413121110, 0x1817161514131211, q14);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x201f1e1d1c1b1a19, q15);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050003020100, q16);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716150113121110, q17);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726250223222120, q18);
    ASSERT_EQUAL_128(0x3f3e3d3c3b3a3938, 0x3736350333323130, q19);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0100050403020100, q20);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x0302151413121110, q21);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x0504252423222120, q22);
    ASSERT_EQUAL_128(0x3f3e3d3c3b3a3938, 0x0706353433323130, q23);
    ASSERT_EQUAL_128(0x0f0e0d0c03020100, 0x0706050403020100, q24);
    ASSERT_EQUAL_128(0x1f1e1d1c07060504, 0x1716151413121110, q25);
    ASSERT_EQUAL_128(0x2f2e2d2c0b0a0908, 0x2726252423222120, q26);
    ASSERT_EQUAL_128(0x3f3e3d3c0f0e0d0c, 0x3736353433323130, q27);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0706050403020100, q28);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x1716151413121110, q29);
    ASSERT_EQUAL_128(0x1716151413121110, 0x2726252423222120, q30);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x3736353433323130, q31);
  }
}


TEST(neon_ld4_lane_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();

  // Test loading whole register by element.
  __ Mov(x17, src_base);
  for (int i = 15; i >= 0; i--) {
    __ Ld4(v0.B(), v1.B(), v2.B(), v3.B(), i, MemOperand(x17, 4, PostIndex));
  }

  __ Mov(x18, src_base);
  for (int i = 7; i >= 0; i--) {
    __ Ld4(v4.H(), v5.H(), v6.H(), v7.H(), i, MemOperand(x18, 8, PostIndex));
  }

  __ Mov(x19, src_base);
  for (int i = 3; i >= 0; i--) {
    __ Ld4(v8.S(), v9.S(), v10.S(), v11.S(), i, MemOperand(x19, 16, PostIndex));
  }

  __ Mov(x20, src_base);
  for (int i = 1; i >= 0; i--) {
    __ Ld4(v12.D(),
           v13.D(),
           v14.D(),
           v15.D(),
           i,
           MemOperand(x20, 32, PostIndex));
  }

  // Test loading a single element into an initialised register.
  __ Mov(x25, 1);
  __ Mov(x21, src_base);
  __ Mov(x22, src_base);
  __ Mov(x23, src_base);
  __ Mov(x24, src_base);

  __ Mov(x4, x21);
  __ Ldr(q16, MemOperand(x4, 16, PostIndex));
  __ Ldr(q17, MemOperand(x4, 16, PostIndex));
  __ Ldr(q18, MemOperand(x4, 16, PostIndex));
  __ Ldr(q19, MemOperand(x4));
  __ Ld4(v16.B(),
         v17.B(),
         v18.B(),
         v19.B(),
         4,
         MemOperand(x21, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Mov(x5, x22);
  __ Ldr(q20, MemOperand(x5, 16, PostIndex));
  __ Ldr(q21, MemOperand(x5, 16, PostIndex));
  __ Ldr(q22, MemOperand(x5, 16, PostIndex));
  __ Ldr(q23, MemOperand(x5));
  __ Ld4(v20.H(),
         v21.H(),
         v22.H(),
         v23.H(),
         3,
         MemOperand(x22, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Mov(x6, x23);
  __ Ldr(q24, MemOperand(x6, 16, PostIndex));
  __ Ldr(q25, MemOperand(x6, 16, PostIndex));
  __ Ldr(q26, MemOperand(x6, 16, PostIndex));
  __ Ldr(q27, MemOperand(x6));
  __ Ld4(v24.S(),
         v25.S(),
         v26.S(),
         v27.S(),
         2,
         MemOperand(x23, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Mov(x7, x24);
  __ Ldr(q28, MemOperand(x7, 16, PostIndex));
  __ Ldr(q29, MemOperand(x7, 16, PostIndex));
  __ Ldr(q30, MemOperand(x7, 16, PostIndex));
  __ Ldr(q31, MemOperand(x7));
  __ Ld4(v28.D(),
         v29.D(),
         v30.D(),
         v31.D(),
         1,
         MemOperand(x24, x25, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0004080c1014181c, 0x2024282c3034383c, q0);
    ASSERT_EQUAL_128(0x0105090d1115191d, 0x2125292d3135393d, q1);
    ASSERT_EQUAL_128(0x02060a0e12161a1e, 0x22262a2e32363a3e, q2);
    ASSERT_EQUAL_128(0x03070b0f13171b1f, 0x23272b2f33373b3f, q3);
    ASSERT_EQUAL_128(0x0100090811101918, 0x2120292831303938, q4);
    ASSERT_EQUAL_128(0x03020b0a13121b1a, 0x23222b2a33323b3a, q5);
    ASSERT_EQUAL_128(0x05040d0c15141d1c, 0x25242d2c35343d3c, q6);
    ASSERT_EQUAL_128(0x07060f0e17161f1e, 0x27262f2e37363f3e, q7);
    ASSERT_EQUAL_128(0x0302010013121110, 0x2322212033323130, q8);
    ASSERT_EQUAL_128(0x0706050417161514, 0x2726252437363534, q9);
    ASSERT_EQUAL_128(0x0b0a09081b1a1918, 0x2b2a29283b3a3938, q10);
    ASSERT_EQUAL_128(0x0f0e0d0c1f1e1d1c, 0x2f2e2d2c3f3e3d3c, q11);
    ASSERT_EQUAL_128(0x0706050403020100, 0x2726252423222120, q12);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x2f2e2d2c2b2a2928, q13);
    ASSERT_EQUAL_128(0x1716151413121110, 0x3736353433323130, q14);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x3f3e3d3c3b3a3938, q15);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050003020100, q16);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716150113121110, q17);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726250223222120, q18);
    ASSERT_EQUAL_128(0x3f3e3d3c3b3a3938, 0x3736350333323130, q19);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0100050403020100, q20);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x0302151413121110, q21);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x0504252423222120, q22);
    ASSERT_EQUAL_128(0x3f3e3d3c3b3a3938, 0x0706353433323130, q23);
    ASSERT_EQUAL_128(0x0f0e0d0c03020100, 0x0706050403020100, q24);
    ASSERT_EQUAL_128(0x1f1e1d1c07060504, 0x1716151413121110, q25);
    ASSERT_EQUAL_128(0x2f2e2d2c0b0a0908, 0x2726252423222120, q26);
    ASSERT_EQUAL_128(0x3f3e3d3c0f0e0d0c, 0x3736353433323130, q27);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0706050403020100, q28);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x1716151413121110, q29);
    ASSERT_EQUAL_128(0x1716151413121110, 0x2726252423222120, q30);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x3736353433323130, q31);

    ASSERT_EQUAL_64(src_base + 64, x17);
    ASSERT_EQUAL_64(src_base + 64, x18);
    ASSERT_EQUAL_64(src_base + 64, x19);
    ASSERT_EQUAL_64(src_base + 64, x20);
    ASSERT_EQUAL_64(src_base + 1, x21);
    ASSERT_EQUAL_64(src_base + 2, x22);
    ASSERT_EQUAL_64(src_base + 3, x23);
    ASSERT_EQUAL_64(src_base + 4, x24);
  }
}


TEST(neon_ld4_alllanes) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base + 1);
  __ Mov(x18, 1);
  __ Ld4r(v0.V8B(), v1.V8B(), v2.V8B(), v3.V8B(), MemOperand(x17));
  __ Add(x17, x17, 4);
  __ Ld4r(v4.V16B(), v5.V16B(), v6.V16B(), v7.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld4r(v8.V4H(), v9.V4H(), v10.V4H(), v11.V4H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld4r(v12.V8H(), v13.V8H(), v14.V8H(), v15.V8H(), MemOperand(x17));
  __ Add(x17, x17, 8);
  __ Ld4r(v16.V2S(), v17.V2S(), v18.V2S(), v19.V2S(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld4r(v20.V4S(), v21.V4S(), v22.V4S(), v23.V4S(), MemOperand(x17));
  __ Add(x17, x17, 16);
  __ Ld4r(v24.V2D(), v25.V2D(), v26.V2D(), v27.V2D(), MemOperand(x17));


  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x0101010101010101, q0);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0202020202020202, q1);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0303030303030303, q2);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0404040404040404, q3);
    ASSERT_EQUAL_128(0x0505050505050505, 0x0505050505050505, q4);
    ASSERT_EQUAL_128(0x0606060606060606, 0x0606060606060606, q5);
    ASSERT_EQUAL_128(0x0707070707070707, 0x0707070707070707, q6);
    ASSERT_EQUAL_128(0x0808080808080808, 0x0808080808080808, q7);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0706070607060706, q8);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0908090809080908, q9);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0b0a0b0a0b0a0b0a, q10);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0d0c0d0c0d0c0d0c, q11);
    ASSERT_EQUAL_128(0x0807080708070807, 0x0807080708070807, q12);
    ASSERT_EQUAL_128(0x0a090a090a090a09, 0x0a090a090a090a09, q13);
    ASSERT_EQUAL_128(0x0c0b0c0b0c0b0c0b, 0x0c0b0c0b0c0b0c0b, q14);
    ASSERT_EQUAL_128(0x0e0d0e0d0e0d0e0d, 0x0e0d0e0d0e0d0e0d, q15);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1211100f1211100f, q16);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1615141316151413, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1a1918171a191817, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1e1d1c1b1e1d1c1b, q19);
    ASSERT_EQUAL_128(0x1312111013121110, 0x1312111013121110, q20);
    ASSERT_EQUAL_128(0x1716151417161514, 0x1716151417161514, q21);
    ASSERT_EQUAL_128(0x1b1a19181b1a1918, 0x1b1a19181b1a1918, q22);
    ASSERT_EQUAL_128(0x1f1e1d1c1f1e1d1c, 0x1f1e1d1c1f1e1d1c, q23);
    ASSERT_EQUAL_128(0x2726252423222120, 0x2726252423222120, q24);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2f2e2d2c2b2a2928, q25);
    ASSERT_EQUAL_128(0x3736353433323130, 0x3736353433323130, q26);
    ASSERT_EQUAL_128(0x3f3e3d3c3b3a3938, 0x3f3e3d3c3b3a3938, q27);
  }
}


TEST(neon_ld4_alllanes_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);
  __ Mov(x17, src_base + 1);
  __ Mov(x18, 1);

  START();
  __ Mov(x17, src_base + 1);
  __ Mov(x18, 1);
  __ Ld4r(v0.V8B(),
          v1.V8B(),
          v2.V8B(),
          v3.V8B(),
          MemOperand(x17, 4, PostIndex));
  __ Ld4r(v4.V16B(),
          v5.V16B(),
          v6.V16B(),
          v7.V16B(),
          MemOperand(x17, x18, PostIndex));
  __ Ld4r(v8.V4H(),
          v9.V4H(),
          v10.V4H(),
          v11.V4H(),
          MemOperand(x17, x18, PostIndex));
  __ Ld4r(v12.V8H(),
          v13.V8H(),
          v14.V8H(),
          v15.V8H(),
          MemOperand(x17, 8, PostIndex));
  __ Ld4r(v16.V2S(),
          v17.V2S(),
          v18.V2S(),
          v19.V2S(),
          MemOperand(x17, x18, PostIndex));
  __ Ld4r(v20.V4S(),
          v21.V4S(),
          v22.V4S(),
          v23.V4S(),
          MemOperand(x17, 16, PostIndex));
  __ Ld4r(v24.V2D(),
          v25.V2D(),
          v26.V2D(),
          v27.V2D(),
          MemOperand(x17, 32, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x0101010101010101, q0);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0202020202020202, q1);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0303030303030303, q2);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0404040404040404, q3);
    ASSERT_EQUAL_128(0x0505050505050505, 0x0505050505050505, q4);
    ASSERT_EQUAL_128(0x0606060606060606, 0x0606060606060606, q5);
    ASSERT_EQUAL_128(0x0707070707070707, 0x0707070707070707, q6);
    ASSERT_EQUAL_128(0x0808080808080808, 0x0808080808080808, q7);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0706070607060706, q8);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0908090809080908, q9);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0b0a0b0a0b0a0b0a, q10);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0d0c0d0c0d0c0d0c, q11);
    ASSERT_EQUAL_128(0x0807080708070807, 0x0807080708070807, q12);
    ASSERT_EQUAL_128(0x0a090a090a090a09, 0x0a090a090a090a09, q13);
    ASSERT_EQUAL_128(0x0c0b0c0b0c0b0c0b, 0x0c0b0c0b0c0b0c0b, q14);
    ASSERT_EQUAL_128(0x0e0d0e0d0e0d0e0d, 0x0e0d0e0d0e0d0e0d, q15);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1211100f1211100f, q16);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1615141316151413, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1a1918171a191817, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x1e1d1c1b1e1d1c1b, q19);
    ASSERT_EQUAL_128(0x1312111013121110, 0x1312111013121110, q20);
    ASSERT_EQUAL_128(0x1716151417161514, 0x1716151417161514, q21);
    ASSERT_EQUAL_128(0x1b1a19181b1a1918, 0x1b1a19181b1a1918, q22);
    ASSERT_EQUAL_128(0x1f1e1d1c1f1e1d1c, 0x1f1e1d1c1f1e1d1c, q23);
    ASSERT_EQUAL_128(0x2726252423222120, 0x2726252423222120, q24);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2f2e2d2c2b2a2928, q25);
    ASSERT_EQUAL_128(0x3736353433323130, 0x3736353433323130, q26);
    ASSERT_EQUAL_128(0x3f3e3d3c3b3a3938, 0x3f3e3d3c3b3a3938, q27);
    ASSERT_EQUAL_64(src_base + 64, x17);
  }
}


TEST(neon_st1_lane) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, -16);
  __ Ldr(q0, MemOperand(x17));

  for (int i = 15; i >= 0; i--) {
    __ St1(v0.B(), i, MemOperand(x17));
    __ Add(x17, x17, 1);
  }
  __ Ldr(q1, MemOperand(x17, x18));

  for (int i = 7; i >= 0; i--) {
    __ St1(v0.H(), i, MemOperand(x17));
    __ Add(x17, x17, 2);
  }
  __ Ldr(q2, MemOperand(x17, x18));

  for (int i = 3; i >= 0; i--) {
    __ St1(v0.S(), i, MemOperand(x17));
    __ Add(x17, x17, 4);
  }
  __ Ldr(q3, MemOperand(x17, x18));

  for (int i = 1; i >= 0; i--) {
    __ St1(v0.D(), i, MemOperand(x17));
    __ Add(x17, x17, 8);
  }
  __ Ldr(q4, MemOperand(x17, x18));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0001020304050607, 0x08090a0b0c0d0e0f, q1);
    ASSERT_EQUAL_128(0x0100030205040706, 0x09080b0a0d0c0f0e, q2);
    ASSERT_EQUAL_128(0x0302010007060504, 0x0b0a09080f0e0d0c, q3);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0f0e0d0c0b0a0908, q4);
  }
}


TEST(neon_st2_lane) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  // Struct size * addressing modes * element sizes * vector size.
  uint8_t dst[2 * 2 * 4 * 16];
  memset(dst, 0, sizeof(dst));
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x17, dst_base);
  __ Mov(x18, dst_base);
  __ Movi(v0.V2D(), 0x0001020304050607, 0x08090a0b0c0d0e0f);
  __ Movi(v1.V2D(), 0x1011121314151617, 0x18191a1b1c1d1e1f);

  // Test B stores with and without post index.
  for (int i = 15; i >= 0; i--) {
    __ St2(v0.B(), v1.B(), i, MemOperand(x18));
    __ Add(x18, x18, 2);
  }
  for (int i = 15; i >= 0; i--) {
    __ St2(v0.B(), v1.B(), i, MemOperand(x18, 2, PostIndex));
  }
  __ Ldr(q2, MemOperand(x17, 0 * 16));
  __ Ldr(q3, MemOperand(x17, 1 * 16));
  __ Ldr(q4, MemOperand(x17, 2 * 16));
  __ Ldr(q5, MemOperand(x17, 3 * 16));

  // Test H stores with and without post index.
  __ Mov(x0, 4);
  for (int i = 7; i >= 0; i--) {
    __ St2(v0.H(), v1.H(), i, MemOperand(x18));
    __ Add(x18, x18, 4);
  }
  for (int i = 7; i >= 0; i--) {
    __ St2(v0.H(), v1.H(), i, MemOperand(x18, x0, PostIndex));
  }
  __ Ldr(q6, MemOperand(x17, 4 * 16));
  __ Ldr(q7, MemOperand(x17, 5 * 16));
  __ Ldr(q16, MemOperand(x17, 6 * 16));
  __ Ldr(q17, MemOperand(x17, 7 * 16));

  // Test S stores with and without post index.
  for (int i = 3; i >= 0; i--) {
    __ St2(v0.S(), v1.S(), i, MemOperand(x18));
    __ Add(x18, x18, 8);
  }
  for (int i = 3; i >= 0; i--) {
    __ St2(v0.S(), v1.S(), i, MemOperand(x18, 8, PostIndex));
  }
  __ Ldr(q18, MemOperand(x17, 8 * 16));
  __ Ldr(q19, MemOperand(x17, 9 * 16));
  __ Ldr(q20, MemOperand(x17, 10 * 16));
  __ Ldr(q21, MemOperand(x17, 11 * 16));

  // Test D stores with and without post index.
  __ Mov(x0, 16);
  __ St2(v0.D(), v1.D(), 1, MemOperand(x18));
  __ Add(x18, x18, 16);
  __ St2(v0.D(), v1.D(), 0, MemOperand(x18, 16, PostIndex));
  __ St2(v0.D(), v1.D(), 1, MemOperand(x18, x0, PostIndex));
  __ St2(v0.D(), v1.D(), 0, MemOperand(x18, x0, PostIndex));
  __ Ldr(q22, MemOperand(x17, 12 * 16));
  __ Ldr(q23, MemOperand(x17, 13 * 16));
  __ Ldr(q24, MemOperand(x17, 14 * 16));
  __ Ldr(q25, MemOperand(x17, 15 * 16));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1707160615051404, 0x1303120211011000, q2);
    ASSERT_EQUAL_128(0x1f0f1e0e1d0d1c0c, 0x1b0b1a0a19091808, q3);
    ASSERT_EQUAL_128(0x1707160615051404, 0x1303120211011000, q4);
    ASSERT_EQUAL_128(0x1f0f1e0e1d0d1c0c, 0x1b0b1a0a19091808, q5);

    ASSERT_EQUAL_128(0x1617060714150405, 0x1213020310110001, q6);
    ASSERT_EQUAL_128(0x1e1f0e0f1c1d0c0d, 0x1a1b0a0b18190809, q7);
    ASSERT_EQUAL_128(0x1617060714150405, 0x1213020310110001, q16);
    ASSERT_EQUAL_128(0x1e1f0e0f1c1d0c0d, 0x1a1b0a0b18190809, q17);

    ASSERT_EQUAL_128(0x1415161704050607, 0x1011121300010203, q18);
    ASSERT_EQUAL_128(0x1c1d1e1f0c0d0e0f, 0x18191a1b08090a0b, q19);
    ASSERT_EQUAL_128(0x1415161704050607, 0x1011121300010203, q20);
    ASSERT_EQUAL_128(0x1c1d1e1f0c0d0e0f, 0x18191a1b08090a0b, q21);

    ASSERT_EQUAL_128(0x1011121314151617, 0x0001020304050607, q22);
    ASSERT_EQUAL_128(0x18191a1b1c1d1e1f, 0x08090a0b0c0d0e0f, q23);
    ASSERT_EQUAL_128(0x1011121314151617, 0x0001020304050607, q22);
    ASSERT_EQUAL_128(0x18191a1b1c1d1e1f, 0x08090a0b0c0d0e0f, q23);
  }
}


TEST(neon_st3_lane) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  // Struct size * addressing modes * element sizes * vector size.
  uint8_t dst[3 * 2 * 4 * 16];
  memset(dst, 0, sizeof(dst));
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x17, dst_base);
  __ Mov(x18, dst_base);
  __ Movi(v0.V2D(), 0x0001020304050607, 0x08090a0b0c0d0e0f);
  __ Movi(v1.V2D(), 0x1011121314151617, 0x18191a1b1c1d1e1f);
  __ Movi(v2.V2D(), 0x2021222324252627, 0x28292a2b2c2d2e2f);

  // Test B stores with and without post index.
  for (int i = 15; i >= 0; i--) {
    __ St3(v0.B(), v1.B(), v2.B(), i, MemOperand(x18));
    __ Add(x18, x18, 3);
  }
  for (int i = 15; i >= 0; i--) {
    __ St3(v0.B(), v1.B(), v2.B(), i, MemOperand(x18, 3, PostIndex));
  }
  __ Ldr(q3, MemOperand(x17, 0 * 16));
  __ Ldr(q4, MemOperand(x17, 1 * 16));
  __ Ldr(q5, MemOperand(x17, 2 * 16));
  __ Ldr(q6, MemOperand(x17, 3 * 16));
  __ Ldr(q7, MemOperand(x17, 4 * 16));
  __ Ldr(q16, MemOperand(x17, 5 * 16));

  // Test H stores with and without post index.
  __ Mov(x0, 6);
  for (int i = 7; i >= 0; i--) {
    __ St3(v0.H(), v1.H(), v2.H(), i, MemOperand(x18));
    __ Add(x18, x18, 6);
  }
  for (int i = 7; i >= 0; i--) {
    __ St3(v0.H(), v1.H(), v2.H(), i, MemOperand(x18, x0, PostIndex));
  }
  __ Ldr(q17, MemOperand(x17, 6 * 16));
  __ Ldr(q18, MemOperand(x17, 7 * 16));
  __ Ldr(q19, MemOperand(x17, 8 * 16));
  __ Ldr(q20, MemOperand(x17, 9 * 16));
  __ Ldr(q21, MemOperand(x17, 10 * 16));
  __ Ldr(q22, MemOperand(x17, 11 * 16));

  // Test S stores with and without post index.
  for (int i = 3; i >= 0; i--) {
    __ St3(v0.S(), v1.S(), v2.S(), i, MemOperand(x18));
    __ Add(x18, x18, 12);
  }
  for (int i = 3; i >= 0; i--) {
    __ St3(v0.S(), v1.S(), v2.S(), i, MemOperand(x18, 12, PostIndex));
  }
  __ Ldr(q23, MemOperand(x17, 12 * 16));
  __ Ldr(q24, MemOperand(x17, 13 * 16));
  __ Ldr(q25, MemOperand(x17, 14 * 16));
  __ Ldr(q26, MemOperand(x17, 15 * 16));
  __ Ldr(q27, MemOperand(x17, 16 * 16));
  __ Ldr(q28, MemOperand(x17, 17 * 16));

  // Test D stores with and without post index.
  __ Mov(x0, 24);
  __ St3(v0.D(), v1.D(), v2.D(), 1, MemOperand(x18));
  __ Add(x18, x18, 24);
  __ St3(v0.D(), v1.D(), v2.D(), 0, MemOperand(x18, 24, PostIndex));
  __ St3(v0.D(), v1.D(), v2.D(), 1, MemOperand(x18, x0, PostIndex));
  __ Ldr(q29, MemOperand(x17, 18 * 16));
  __ Ldr(q30, MemOperand(x17, 19 * 16));
  __ Ldr(q31, MemOperand(x17, 20 * 16));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0524140423130322, 0x1202211101201000, q3);
    ASSERT_EQUAL_128(0x1a0a291909281808, 0x2717072616062515, q4);
    ASSERT_EQUAL_128(0x2f1f0f2e1e0e2d1d, 0x0d2c1c0c2b1b0b2a, q5);
    ASSERT_EQUAL_128(0x0524140423130322, 0x1202211101201000, q6);
    ASSERT_EQUAL_128(0x1a0a291909281808, 0x2717072616062515, q7);
    ASSERT_EQUAL_128(0x2f1f0f2e1e0e2d1d, 0x0d2c1c0c2b1b0b2a, q16);

    ASSERT_EQUAL_128(0x1415040522231213, 0x0203202110110001, q17);
    ASSERT_EQUAL_128(0x0a0b282918190809, 0x2627161706072425, q18);
    ASSERT_EQUAL_128(0x2e2f1e1f0e0f2c2d, 0x1c1d0c0d2a2b1a1b, q19);
    ASSERT_EQUAL_128(0x1415040522231213, 0x0203202110110001, q20);
    ASSERT_EQUAL_128(0x0a0b282918190809, 0x2627161706072425, q21);
    ASSERT_EQUAL_128(0x2e2f1e1f0e0f2c2d, 0x1c1d0c0d2a2b1a1b, q22);

    ASSERT_EQUAL_128(0x0405060720212223, 0x1011121300010203, q23);
    ASSERT_EQUAL_128(0x18191a1b08090a0b, 0x2425262714151617, q24);
    ASSERT_EQUAL_128(0x2c2d2e2f1c1d1e1f, 0x0c0d0e0f28292a2b, q25);
    ASSERT_EQUAL_128(0x0405060720212223, 0x1011121300010203, q26);
    ASSERT_EQUAL_128(0x18191a1b08090a0b, 0x2425262714151617, q27);
    ASSERT_EQUAL_128(0x2c2d2e2f1c1d1e1f, 0x0c0d0e0f28292a2b, q28);
  }
}


TEST(neon_st4_lane) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  // Struct size * element sizes * vector size.
  uint8_t dst[4 * 4 * 16];
  memset(dst, 0, sizeof(dst));
  uintptr_t dst_base = reinterpret_cast<uintptr_t>(dst);

  START();
  __ Mov(x17, dst_base);
  __ Mov(x18, dst_base);
  __ Movi(v0.V2D(), 0x0001020304050607, 0x08090a0b0c0d0e0f);
  __ Movi(v1.V2D(), 0x1011121314151617, 0x18191a1b1c1d1e1f);
  __ Movi(v2.V2D(), 0x2021222324252627, 0x28292a2b2c2d2e2f);
  __ Movi(v3.V2D(), 0x2021222324252627, 0x28292a2b2c2d2e2f);

  // Test B stores without post index.
  for (int i = 15; i >= 0; i--) {
    __ St4(v0.B(), v1.B(), v2.B(), v3.B(), i, MemOperand(x18));
    __ Add(x18, x18, 4);
  }
  __ Ldr(q4, MemOperand(x17, 0 * 16));
  __ Ldr(q5, MemOperand(x17, 1 * 16));
  __ Ldr(q6, MemOperand(x17, 2 * 16));
  __ Ldr(q7, MemOperand(x17, 3 * 16));

  // Test H stores with post index.
  __ Mov(x0, 8);
  for (int i = 7; i >= 0; i--) {
    __ St4(v0.H(), v1.H(), v2.H(), v3.H(), i, MemOperand(x18, x0, PostIndex));
  }
  __ Ldr(q16, MemOperand(x17, 4 * 16));
  __ Ldr(q17, MemOperand(x17, 5 * 16));
  __ Ldr(q18, MemOperand(x17, 6 * 16));
  __ Ldr(q19, MemOperand(x17, 7 * 16));

  // Test S stores without post index.
  for (int i = 3; i >= 0; i--) {
    __ St4(v0.S(), v1.S(), v2.S(), v3.S(), i, MemOperand(x18));
    __ Add(x18, x18, 16);
  }
  __ Ldr(q20, MemOperand(x17, 8 * 16));
  __ Ldr(q21, MemOperand(x17, 9 * 16));
  __ Ldr(q22, MemOperand(x17, 10 * 16));
  __ Ldr(q23, MemOperand(x17, 11 * 16));

  // Test D stores with post index.
  __ Mov(x0, 32);
  __ St4(v0.D(), v1.D(), v2.D(), v3.D(), 0, MemOperand(x18, 32, PostIndex));
  __ St4(v0.D(), v1.D(), v2.D(), v3.D(), 1, MemOperand(x18, x0, PostIndex));

  __ Ldr(q24, MemOperand(x17, 12 * 16));
  __ Ldr(q25, MemOperand(x17, 13 * 16));
  __ Ldr(q26, MemOperand(x17, 14 * 16));
  __ Ldr(q27, MemOperand(x17, 15 * 16));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x2323130322221202, 0x2121110120201000, q4);
    ASSERT_EQUAL_128(0x2727170726261606, 0x2525150524241404, q5);
    ASSERT_EQUAL_128(0x2b2b1b0b2a2a1a0a, 0x2929190928281808, q6);
    ASSERT_EQUAL_128(0x2f2f1f0f2e2e1e0e, 0x2d2d1d0d2c2c1c0c, q7);

    ASSERT_EQUAL_128(0x2223222312130203, 0x2021202110110001, q16);
    ASSERT_EQUAL_128(0x2627262716170607, 0x2425242514150405, q17);
    ASSERT_EQUAL_128(0x2a2b2a2b1a1b0a0b, 0x2829282918190809, q18);
    ASSERT_EQUAL_128(0x2e2f2e2f1e1f0e0f, 0x2c2d2c2d1c1d0c0d, q19);

    ASSERT_EQUAL_128(0x2021222320212223, 0x1011121300010203, q20);
    ASSERT_EQUAL_128(0x2425262724252627, 0x1415161704050607, q21);
    ASSERT_EQUAL_128(0x28292a2b28292a2b, 0x18191a1b08090a0b, q22);
    ASSERT_EQUAL_128(0x2c2d2e2f2c2d2e2f, 0x1c1d1e1f0c0d0e0f, q23);

    ASSERT_EQUAL_128(0x18191a1b1c1d1e1f, 0x08090a0b0c0d0e0f, q24);
    ASSERT_EQUAL_128(0x28292a2b2c2d2e2f, 0x28292a2b2c2d2e2f, q25);
    ASSERT_EQUAL_128(0x1011121314151617, 0x0001020304050607, q26);
    ASSERT_EQUAL_128(0x2021222324252627, 0x2021222324252627, q27);
  }
}


TEST(neon_ld1_lane_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Mov(x19, src_base);
  __ Mov(x20, src_base);
  __ Mov(x21, src_base);
  __ Mov(x22, src_base);
  __ Mov(x23, src_base);
  __ Mov(x24, src_base);

  // Test loading whole register by element.
  for (int i = 15; i >= 0; i--) {
    __ Ld1(v0.B(), i, MemOperand(x17, 1, PostIndex));
  }

  for (int i = 7; i >= 0; i--) {
    __ Ld1(v1.H(), i, MemOperand(x18, 2, PostIndex));
  }

  for (int i = 3; i >= 0; i--) {
    __ Ld1(v2.S(), i, MemOperand(x19, 4, PostIndex));
  }

  for (int i = 1; i >= 0; i--) {
    __ Ld1(v3.D(), i, MemOperand(x20, 8, PostIndex));
  }

  // Test loading a single element into an initialised register.
  __ Mov(x25, 1);
  __ Ldr(q4, MemOperand(x21));
  __ Ld1(v4.B(), 4, MemOperand(x21, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Ldr(q5, MemOperand(x22));
  __ Ld1(v5.H(), 3, MemOperand(x22, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Ldr(q6, MemOperand(x23));
  __ Ld1(v6.S(), 2, MemOperand(x23, x25, PostIndex));
  __ Add(x25, x25, 1);

  __ Ldr(q7, MemOperand(x24));
  __ Ld1(v7.D(), 1, MemOperand(x24, x25, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0001020304050607, 0x08090a0b0c0d0e0f, q0);
    ASSERT_EQUAL_128(0x0100030205040706, 0x09080b0a0d0c0f0e, q1);
    ASSERT_EQUAL_128(0x0302010007060504, 0x0b0a09080f0e0d0c, q2);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0f0e0d0c0b0a0908, q3);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050003020100, q4);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0100050403020100, q5);
    ASSERT_EQUAL_128(0x0f0e0d0c03020100, 0x0706050403020100, q6);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0706050403020100, q7);
    ASSERT_EQUAL_64(src_base + 16, x17);
    ASSERT_EQUAL_64(src_base + 16, x18);
    ASSERT_EQUAL_64(src_base + 16, x19);
    ASSERT_EQUAL_64(src_base + 16, x20);
    ASSERT_EQUAL_64(src_base + 1, x21);
    ASSERT_EQUAL_64(src_base + 2, x22);
    ASSERT_EQUAL_64(src_base + 3, x23);
    ASSERT_EQUAL_64(src_base + 4, x24);
  }
}


TEST(neon_st1_lane_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, -16);
  __ Ldr(q0, MemOperand(x17));

  for (int i = 15; i >= 0; i--) {
    __ St1(v0.B(), i, MemOperand(x17, 1, PostIndex));
  }
  __ Ldr(q1, MemOperand(x17, x18));

  for (int i = 7; i >= 0; i--) {
    __ St1(v0.H(), i, MemOperand(x17, 2, PostIndex));
  }
  __ Ldr(q2, MemOperand(x17, x18));

  for (int i = 3; i >= 0; i--) {
    __ St1(v0.S(), i, MemOperand(x17, 4, PostIndex));
  }
  __ Ldr(q3, MemOperand(x17, x18));

  for (int i = 1; i >= 0; i--) {
    __ St1(v0.D(), i, MemOperand(x17, 8, PostIndex));
  }
  __ Ldr(q4, MemOperand(x17, x18));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0001020304050607, 0x08090a0b0c0d0e0f, q1);
    ASSERT_EQUAL_128(0x0100030205040706, 0x09080b0a0d0c0f0e, q2);
    ASSERT_EQUAL_128(0x0302010007060504, 0x0b0a09080f0e0d0c, q3);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0f0e0d0c0b0a0908, q4);
  }
}


TEST(neon_ld1_alllanes) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base + 1);
  __ Ld1r(v0.V8B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1r(v1.V16B(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1r(v2.V4H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1r(v3.V8H(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1r(v4.V2S(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1r(v5.V4S(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1r(v6.V1D(), MemOperand(x17));
  __ Add(x17, x17, 1);
  __ Ld1r(v7.V2D(), MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x0101010101010101, q0);
    ASSERT_EQUAL_128(0x0202020202020202, 0x0202020202020202, q1);
    ASSERT_EQUAL_128(0, 0x0403040304030403, q2);
    ASSERT_EQUAL_128(0x0504050405040504, 0x0504050405040504, q3);
    ASSERT_EQUAL_128(0, 0x0807060508070605, q4);
    ASSERT_EQUAL_128(0x0908070609080706, 0x0908070609080706, q5);
    ASSERT_EQUAL_128(0, 0x0e0d0c0b0a090807, q6);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0f0e0d0c0b0a0908, q7);
  }
}


TEST(neon_ld1_alllanes_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base + 1);
  __ Mov(x18, 1);
  __ Ld1r(v0.V8B(), MemOperand(x17, 1, PostIndex));
  __ Ld1r(v1.V16B(), MemOperand(x17, x18, PostIndex));
  __ Ld1r(v2.V4H(), MemOperand(x17, x18, PostIndex));
  __ Ld1r(v3.V8H(), MemOperand(x17, 2, PostIndex));
  __ Ld1r(v4.V2S(), MemOperand(x17, x18, PostIndex));
  __ Ld1r(v5.V4S(), MemOperand(x17, 4, PostIndex));
  __ Ld1r(v6.V2D(), MemOperand(x17, 8, PostIndex));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x0101010101010101, q0);
    ASSERT_EQUAL_128(0x0202020202020202, 0x0202020202020202, q1);
    ASSERT_EQUAL_128(0, 0x0403040304030403, q2);
    ASSERT_EQUAL_128(0x0504050405040504, 0x0504050405040504, q3);
    ASSERT_EQUAL_128(0, 0x0908070609080706, q4);
    ASSERT_EQUAL_128(0x0a0908070a090807, 0x0a0908070a090807, q5);
    ASSERT_EQUAL_128(0x1211100f0e0d0c0b, 0x1211100f0e0d0c0b, q6);
    ASSERT_EQUAL_64(src_base + 19, x17);
  }
}


TEST(neon_st1_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[14 * kDRegSizeInBytes];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));
  __ Ldr(q3, MemOperand(x17, 16, PostIndex));
  __ Mov(x17, src_base);

  __ St1(v0.V8B(), MemOperand(x17));
  __ Ldr(d16, MemOperand(x17, 8, PostIndex));

  __ St1(v0.V8B(), v1.V8B(), MemOperand(x17));
  __ Ldr(q17, MemOperand(x17, 16, PostIndex));

  __ St1(v0.V4H(), v1.V4H(), v2.V4H(), MemOperand(x17));
  __ Ldr(d18, MemOperand(x17, 8, PostIndex));
  __ Ldr(d19, MemOperand(x17, 8, PostIndex));
  __ Ldr(d20, MemOperand(x17, 8, PostIndex));

  __ St1(v0.V2S(), v1.V2S(), v2.V2S(), v3.V2S(), MemOperand(x17));
  __ Ldr(q21, MemOperand(x17, 16, PostIndex));
  __ Ldr(q22, MemOperand(x17, 16, PostIndex));

  __ St1(v0.V1D(), v1.V1D(), v2.V1D(), v3.V1D(), MemOperand(x17));
  __ Ldr(q23, MemOperand(x17, 16, PostIndex));
  __ Ldr(q24, MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q0);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716151413121110, q1);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726252423222120, q2);
    ASSERT_EQUAL_128(0x3f3e3d3c3b3a3938, 0x3736353433323130, q3);
    ASSERT_EQUAL_128(0, 0x0706050403020100, q16);
    ASSERT_EQUAL_128(0x1716151413121110, 0x0706050403020100, q17);
    ASSERT_EQUAL_128(0, 0x0706050403020100, q18);
    ASSERT_EQUAL_128(0, 0x1716151413121110, q19);
    ASSERT_EQUAL_128(0, 0x2726252423222120, q20);
    ASSERT_EQUAL_128(0x1716151413121110, 0x0706050403020100, q21);
    ASSERT_EQUAL_128(0x3736353433323130, 0x2726252423222120, q22);
    ASSERT_EQUAL_128(0x1716151413121110, 0x0706050403020100, q23);
    ASSERT_EQUAL_128(0x3736353433323130, 0x2726252423222120, q24);
  }
}


TEST(neon_st1_d_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 14 * kDRegSizeInBytes];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, -8);
  __ Mov(x19, -16);
  __ Mov(x20, -24);
  __ Mov(x21, -32);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));
  __ Ldr(q3, MemOperand(x17, 16, PostIndex));
  __ Mov(x17, src_base);

  __ St1(v0.V8B(), MemOperand(x17, 8, PostIndex));
  __ Ldr(d16, MemOperand(x17, x18));

  __ St1(v0.V8B(), v1.V8B(), MemOperand(x17, 16, PostIndex));
  __ Ldr(q17, MemOperand(x17, x19));

  __ St1(v0.V4H(), v1.V4H(), v2.V4H(), MemOperand(x17, 24, PostIndex));
  __ Ldr(d18, MemOperand(x17, x20));
  __ Ldr(d19, MemOperand(x17, x19));
  __ Ldr(d20, MemOperand(x17, x18));

  __ St1(v0.V2S(),
         v1.V2S(),
         v2.V2S(),
         v3.V2S(),
         MemOperand(x17, 32, PostIndex));
  __ Ldr(q21, MemOperand(x17, x21));
  __ Ldr(q22, MemOperand(x17, x19));

  __ St1(v0.V1D(),
         v1.V1D(),
         v2.V1D(),
         v3.V1D(),
         MemOperand(x17, 32, PostIndex));
  __ Ldr(q23, MemOperand(x17, x21));
  __ Ldr(q24, MemOperand(x17, x19));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x0706050403020100, q16);
    ASSERT_EQUAL_128(0x1716151413121110, 0x0706050403020100, q17);
    ASSERT_EQUAL_128(0, 0x0706050403020100, q18);
    ASSERT_EQUAL_128(0, 0x1716151413121110, q19);
    ASSERT_EQUAL_128(0, 0x2726252423222120, q20);
    ASSERT_EQUAL_128(0x1716151413121110, 0x0706050403020100, q21);
    ASSERT_EQUAL_128(0x3736353433323130, 0x2726252423222120, q22);
    ASSERT_EQUAL_128(0x1716151413121110, 0x0706050403020100, q23);
    ASSERT_EQUAL_128(0x3736353433323130, 0x2726252423222120, q24);
  }
}


TEST(neon_st1_q) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 160];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));
  __ Ldr(q3, MemOperand(x17, 16, PostIndex));

  __ St1(v0.V16B(), MemOperand(x17));
  __ Ldr(q16, MemOperand(x17, 16, PostIndex));

  __ St1(v0.V8H(), v1.V8H(), MemOperand(x17));
  __ Ldr(q17, MemOperand(x17, 16, PostIndex));
  __ Ldr(q18, MemOperand(x17, 16, PostIndex));

  __ St1(v0.V4S(), v1.V4S(), v2.V4S(), MemOperand(x17));
  __ Ldr(q19, MemOperand(x17, 16, PostIndex));
  __ Ldr(q20, MemOperand(x17, 16, PostIndex));
  __ Ldr(q21, MemOperand(x17, 16, PostIndex));

  __ St1(v0.V2D(), v1.V2D(), v2.V2D(), v3.V2D(), MemOperand(x17));
  __ Ldr(q22, MemOperand(x17, 16, PostIndex));
  __ Ldr(q23, MemOperand(x17, 16, PostIndex));
  __ Ldr(q24, MemOperand(x17, 16, PostIndex));
  __ Ldr(q25, MemOperand(x17));
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q16);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q17);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716151413121110, q18);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q19);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716151413121110, q20);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726252423222120, q21);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q22);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716151413121110, q23);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726252423222120, q24);
    ASSERT_EQUAL_128(0x3f3e3d3c3b3a3938, 0x3736353433323130, q25);
  }
}


TEST(neon_st1_q_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[64 + 160];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, -16);
  __ Mov(x19, -32);
  __ Mov(x20, -48);
  __ Mov(x21, -64);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));
  __ Ldr(q3, MemOperand(x17, 16, PostIndex));

  __ St1(v0.V16B(), MemOperand(x17, 16, PostIndex));
  __ Ldr(q16, MemOperand(x17, x18));

  __ St1(v0.V8H(), v1.V8H(), MemOperand(x17, 32, PostIndex));
  __ Ldr(q17, MemOperand(x17, x19));
  __ Ldr(q18, MemOperand(x17, x18));

  __ St1(v0.V4S(), v1.V4S(), v2.V4S(), MemOperand(x17, 48, PostIndex));
  __ Ldr(q19, MemOperand(x17, x20));
  __ Ldr(q20, MemOperand(x17, x19));
  __ Ldr(q21, MemOperand(x17, x18));

  __ St1(v0.V2D(),
         v1.V2D(),
         v2.V2D(),
         v3.V2D(),
         MemOperand(x17, 64, PostIndex));
  __ Ldr(q22, MemOperand(x17, x21));
  __ Ldr(q23, MemOperand(x17, x20));
  __ Ldr(q24, MemOperand(x17, x19));
  __ Ldr(q25, MemOperand(x17, x18));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q16);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q17);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716151413121110, q18);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q19);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716151413121110, q20);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726252423222120, q21);
    ASSERT_EQUAL_128(0x0f0e0d0c0b0a0908, 0x0706050403020100, q22);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x1716151413121110, q23);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726252423222120, q24);
    ASSERT_EQUAL_128(0x3f3e3d3c3b3a3938, 0x3736353433323130, q25);
  }
}


TEST(neon_st2_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[4 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));

  __ St2(v0.V8B(), v1.V8B(), MemOperand(x18));
  __ Add(x18, x18, 22);
  __ St2(v0.V4H(), v1.V4H(), MemOperand(x18));
  __ Add(x18, x18, 11);
  __ St2(v0.V2S(), v1.V2S(), MemOperand(x18));

  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Ldr(q2, MemOperand(x19, 16, PostIndex));
  __ Ldr(q3, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1707160615051404, 0x1303120211011000, q0);
    ASSERT_EQUAL_128(0x0504131203021110, 0x0100151413121110, q1);
    ASSERT_EQUAL_128(0x1615140706050413, 0x1211100302010014, q2);
    ASSERT_EQUAL_128(0x3f3e3d3c3b3a3938, 0x3736353433323117, q3);
  }
}


TEST(neon_st2_d_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[4 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x22, 5);
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));

  __ St2(v0.V8B(), v1.V8B(), MemOperand(x18, x22, PostIndex));
  __ St2(v0.V4H(), v1.V4H(), MemOperand(x18, 16, PostIndex));
  __ St2(v0.V2S(), v1.V2S(), MemOperand(x18));


  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Ldr(q2, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1405041312030211, 0x1001000211011000, q0);
    ASSERT_EQUAL_128(0x0605041312111003, 0x0201001716070615, q1);
    ASSERT_EQUAL_128(0x2f2e2d2c2b2a2928, 0x2726251716151407, q2);
  }
}


TEST(neon_st2_q) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[5 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));

  __ St2(v0.V16B(), v1.V16B(), MemOperand(x18));
  __ Add(x18, x18, 8);
  __ St2(v0.V8H(), v1.V8H(), MemOperand(x18));
  __ Add(x18, x18, 22);
  __ St2(v0.V4S(), v1.V4S(), MemOperand(x18));
  __ Add(x18, x18, 2);
  __ St2(v0.V2D(), v1.V2D(), MemOperand(x18));

  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Ldr(q2, MemOperand(x19, 16, PostIndex));
  __ Ldr(q3, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1312030211100100, 0x1303120211011000, q0);
    ASSERT_EQUAL_128(0x01000b0a19180908, 0x1716070615140504, q1);
    ASSERT_EQUAL_128(0x1716151413121110, 0x0706050403020100, q2);
    ASSERT_EQUAL_128(0x1f1e1d1c1b1a1918, 0x0f0e0d0c0b0a0908, q3);
  }
}


TEST(neon_st2_q_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[5 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x22, 5);
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));

  __ St2(v0.V16B(), v1.V16B(), MemOperand(x18, x22, PostIndex));
  __ St2(v0.V8H(), v1.V8H(), MemOperand(x18, 32, PostIndex));
  __ St2(v0.V4S(), v1.V4S(), MemOperand(x18, x22, PostIndex));
  __ St2(v0.V2D(), v1.V2D(), MemOperand(x18));

  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Ldr(q2, MemOperand(x19, 16, PostIndex));
  __ Ldr(q3, MemOperand(x19, 16, PostIndex));
  __ Ldr(q4, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1405041312030211, 0x1001000211011000, q0);
    ASSERT_EQUAL_128(0x1c0d0c1b1a0b0a19, 0x1809081716070615, q1);
    ASSERT_EQUAL_128(0x0504030201001003, 0x0201001f1e0f0e1d, q2);
    ASSERT_EQUAL_128(0x0d0c0b0a09081716, 0x1514131211100706, q3);
    ASSERT_EQUAL_128(0x4f4e4d4c4b4a1f1e, 0x1d1c1b1a19180f0e, q4);
  }
}


TEST(neon_st3_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[3 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));

  __ St3(v0.V8B(), v1.V8B(), v2.V8B(), MemOperand(x18));
  __ Add(x18, x18, 3);
  __ St3(v0.V4H(), v1.V4H(), v2.V4H(), MemOperand(x18));
  __ Add(x18, x18, 2);
  __ St3(v0.V2S(), v1.V2S(), v2.V2S(), MemOperand(x18));


  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x2221201312111003, 0x0201000100201000, q0);
    ASSERT_EQUAL_128(0x1f1e1d2726252417, 0x1615140706050423, q1);
  }
}


TEST(neon_st3_d_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[4 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x22, 5);
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));

  __ St3(v0.V8B(), v1.V8B(), v2.V8B(), MemOperand(x18, x22, PostIndex));
  __ St3(v0.V4H(), v1.V4H(), v2.V4H(), MemOperand(x18, 24, PostIndex));
  __ St3(v0.V2S(), v1.V2S(), v2.V2S(), MemOperand(x18));


  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Ldr(q2, MemOperand(x19, 16, PostIndex));
  __ Ldr(q3, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x2213120302212011, 0x1001001101201000, q0);
    ASSERT_EQUAL_128(0x0201002726171607, 0x0625241514050423, q1);
    ASSERT_EQUAL_128(0x1615140706050423, 0x2221201312111003, q2);
    ASSERT_EQUAL_128(0x3f3e3d3c3b3a3938, 0x3736352726252417, q3);
  }
}


TEST(neon_st3_q) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[6 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));

  __ St3(v0.V16B(), v1.V16B(), v2.V16B(), MemOperand(x18));
  __ Add(x18, x18, 5);
  __ St3(v0.V8H(), v1.V8H(), v2.V8H(), MemOperand(x18));
  __ Add(x18, x18, 12);
  __ St3(v0.V4S(), v1.V4S(), v2.V4S(), MemOperand(x18));
  __ Add(x18, x18, 22);
  __ St3(v0.V2D(), v1.V2D(), v2.V2D(), MemOperand(x18));

  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Ldr(q2, MemOperand(x19, 16, PostIndex));
  __ Ldr(q3, MemOperand(x19, 16, PostIndex));
  __ Ldr(q4, MemOperand(x19, 16, PostIndex));
  __ Ldr(q5, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x2213120302212011, 0x1001001101201000, q0);
    ASSERT_EQUAL_128(0x0605042322212013, 0x1211100302010023, q1);
    ASSERT_EQUAL_128(0x1007060504030201, 0x0025241716151407, q2);
    ASSERT_EQUAL_128(0x0827262524232221, 0x2017161514131211, q3);
    ASSERT_EQUAL_128(0x281f1e1d1c1b1a19, 0x180f0e0d0c0b0a09, q4);
    ASSERT_EQUAL_128(0x5f5e5d5c5b5a5958, 0x572f2e2d2c2b2a29, q5);
  }
}


TEST(neon_st3_q_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[7 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x22, 5);
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));

  __ St3(v0.V16B(), v1.V16B(), v2.V16B(), MemOperand(x18, x22, PostIndex));
  __ St3(v0.V8H(), v1.V8H(), v2.V8H(), MemOperand(x18, 48, PostIndex));
  __ St3(v0.V4S(), v1.V4S(), v2.V4S(), MemOperand(x18, x22, PostIndex));
  __ St3(v0.V2D(), v1.V2D(), v2.V2D(), MemOperand(x18));

  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Ldr(q2, MemOperand(x19, 16, PostIndex));
  __ Ldr(q3, MemOperand(x19, 16, PostIndex));
  __ Ldr(q4, MemOperand(x19, 16, PostIndex));
  __ Ldr(q5, MemOperand(x19, 16, PostIndex));
  __ Ldr(q6, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x2213120302212011, 0x1001001101201000, q0);
    ASSERT_EQUAL_128(0x1809082726171607, 0x0625241514050423, q1);
    ASSERT_EQUAL_128(0x0e2d2c1d1c0d0c2b, 0x2a1b1a0b0a292819, q2);
    ASSERT_EQUAL_128(0x0504030201001003, 0x0201002f2e1f1e0f, q3);
    ASSERT_EQUAL_128(0x2524232221201716, 0x1514131211100706, q4);
    ASSERT_EQUAL_128(0x1d1c1b1a19180f0e, 0x0d0c0b0a09082726, q5);
    ASSERT_EQUAL_128(0x6f6e6d6c6b6a2f2e, 0x2d2c2b2a29281f1e, q6);
  }
}


TEST(neon_st4_d) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[4 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));
  __ Ldr(q3, MemOperand(x17, 16, PostIndex));

  __ St4(v0.V8B(), v1.V8B(), v2.V8B(), v3.V8B(), MemOperand(x18));
  __ Add(x18, x18, 12);
  __ St4(v0.V4H(), v1.V4H(), v2.V4H(), v3.V4H(), MemOperand(x18));
  __ Add(x18, x18, 15);
  __ St4(v0.V2S(), v1.V2S(), v2.V2S(), v3.V2S(), MemOperand(x18));


  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Ldr(q2, MemOperand(x19, 16, PostIndex));
  __ Ldr(q3, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1110010032221202, 0X3121110130201000, q0);
    ASSERT_EQUAL_128(0x1003020100322322, 0X1312030231302120, q1);
    ASSERT_EQUAL_128(0x1407060504333231, 0X3023222120131211, q2);
    ASSERT_EQUAL_128(0x3f3e3d3c3b373635, 0x3427262524171615, q3);
  }
}


TEST(neon_st4_d_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[5 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x22, 5);
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));
  __ Ldr(q3, MemOperand(x17, 16, PostIndex));

  __ St4(v0.V8B(),
         v1.V8B(),
         v2.V8B(),
         v3.V8B(),
         MemOperand(x18, x22, PostIndex));
  __ St4(v0.V4H(),
         v1.V4H(),
         v2.V4H(),
         v3.V4H(),
         MemOperand(x18, 32, PostIndex));
  __ St4(v0.V2S(), v1.V2S(), v2.V2S(), v3.V2S(), MemOperand(x18));


  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Ldr(q2, MemOperand(x19, 16, PostIndex));
  __ Ldr(q3, MemOperand(x19, 16, PostIndex));
  __ Ldr(q4, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1203023130212011, 0x1001000130201000, q0);
    ASSERT_EQUAL_128(0x1607063534252415, 0x1405043332232213, q1);
    ASSERT_EQUAL_128(0x2221201312111003, 0x0201003736272617, q2);
    ASSERT_EQUAL_128(0x2625241716151407, 0x0605043332313023, q3);
    ASSERT_EQUAL_128(0x4f4e4d4c4b4a4948, 0x4746453736353427, q4);
  }
}


TEST(neon_st4_q) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[7 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));
  __ Ldr(q3, MemOperand(x17, 16, PostIndex));

  __ St4(v0.V16B(), v1.V16B(), v2.V16B(), v3.V16B(), MemOperand(x18));
  __ Add(x18, x18, 5);
  __ St4(v0.V8H(), v1.V8H(), v2.V8H(), v3.V8H(), MemOperand(x18));
  __ Add(x18, x18, 12);
  __ St4(v0.V4S(), v1.V4S(), v2.V4S(), v3.V4S(), MemOperand(x18));
  __ Add(x18, x18, 22);
  __ St4(v0.V2D(), v1.V2D(), v2.V2D(), v3.V2D(), MemOperand(x18));
  __ Add(x18, x18, 10);

  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Ldr(q2, MemOperand(x19, 16, PostIndex));
  __ Ldr(q3, MemOperand(x19, 16, PostIndex));
  __ Ldr(q4, MemOperand(x19, 16, PostIndex));
  __ Ldr(q5, MemOperand(x19, 16, PostIndex));
  __ Ldr(q6, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1203023130212011, 0x1001000130201000, q0);
    ASSERT_EQUAL_128(0x3231302322212013, 0x1211100302010013, q1);
    ASSERT_EQUAL_128(0x1007060504030201, 0x0015140706050433, q2);
    ASSERT_EQUAL_128(0x3027262524232221, 0x2017161514131211, q3);
    ASSERT_EQUAL_128(0x180f0e0d0c0b0a09, 0x0837363534333231, q4);
    ASSERT_EQUAL_128(0x382f2e2d2c2b2a29, 0x281f1e1d1c1b1a19, q5);
    ASSERT_EQUAL_128(0x6f6e6d6c6b6a6968, 0x673f3e3d3c3b3a39, q6);
  }
}


TEST(neon_st4_q_postindex) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  uint8_t src[9 * 16];
  for (unsigned i = 0; i < sizeof(src); i++) {
    src[i] = i;
  }
  uintptr_t src_base = reinterpret_cast<uintptr_t>(src);

  START();
  __ Mov(x22, 5);
  __ Mov(x17, src_base);
  __ Mov(x18, src_base);
  __ Ldr(q0, MemOperand(x17, 16, PostIndex));
  __ Ldr(q1, MemOperand(x17, 16, PostIndex));
  __ Ldr(q2, MemOperand(x17, 16, PostIndex));
  __ Ldr(q3, MemOperand(x17, 16, PostIndex));

  __ St4(v0.V16B(),
         v1.V16B(),
         v2.V16B(),
         v3.V16B(),
         MemOperand(x18, x22, PostIndex));
  __ St4(v0.V8H(),
         v1.V8H(),
         v2.V8H(),
         v3.V8H(),
         MemOperand(x18, 64, PostIndex));
  __ St4(v0.V4S(),
         v1.V4S(),
         v2.V4S(),
         v3.V4S(),
         MemOperand(x18, x22, PostIndex));
  __ St4(v0.V2D(), v1.V2D(), v2.V2D(), v3.V2D(), MemOperand(x18));

  __ Mov(x19, src_base);
  __ Ldr(q0, MemOperand(x19, 16, PostIndex));
  __ Ldr(q1, MemOperand(x19, 16, PostIndex));
  __ Ldr(q2, MemOperand(x19, 16, PostIndex));
  __ Ldr(q3, MemOperand(x19, 16, PostIndex));
  __ Ldr(q4, MemOperand(x19, 16, PostIndex));
  __ Ldr(q5, MemOperand(x19, 16, PostIndex));
  __ Ldr(q6, MemOperand(x19, 16, PostIndex));
  __ Ldr(q7, MemOperand(x19, 16, PostIndex));
  __ Ldr(q8, MemOperand(x19, 16, PostIndex));

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1203023130212011, 0x1001000130201000, q0);
    ASSERT_EQUAL_128(0x1607063534252415, 0x1405043332232213, q1);
    ASSERT_EQUAL_128(0x1a0b0a3938292819, 0x1809083736272617, q2);
    ASSERT_EQUAL_128(0x1e0f0e3d3c2d2c1d, 0x1c0d0c3b3a2b2a1b, q3);
    ASSERT_EQUAL_128(0x0504030201001003, 0x0201003f3e2f2e1f, q4);
    ASSERT_EQUAL_128(0x2524232221201716, 0x1514131211100706, q5);
    ASSERT_EQUAL_128(0x0d0c0b0a09083736, 0x3534333231302726, q6);
    ASSERT_EQUAL_128(0x2d2c2b2a29281f1e, 0x1d1c1b1a19180f0e, q7);
    ASSERT_EQUAL_128(0x8f8e8d8c8b8a3f3e, 0x3d3c3b3a39382f2e, q8);
  }
}


TEST(neon_destructive_minmaxp) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();
  __ Movi(v0.V2D(), 0, 0x2222222233333333);
  __ Movi(v1.V2D(), 0, 0x0000000011111111);

  __ Sminp(v16.V2S(), v0.V2S(), v1.V2S());
  __ Mov(v17, v0);
  __ Sminp(v17.V2S(), v17.V2S(), v1.V2S());
  __ Mov(v18, v1);
  __ Sminp(v18.V2S(), v0.V2S(), v18.V2S());
  __ Mov(v19, v0);
  __ Sminp(v19.V2S(), v19.V2S(), v19.V2S());

  __ Smaxp(v20.V2S(), v0.V2S(), v1.V2S());
  __ Mov(v21, v0);
  __ Smaxp(v21.V2S(), v21.V2S(), v1.V2S());
  __ Mov(v22, v1);
  __ Smaxp(v22.V2S(), v0.V2S(), v22.V2S());
  __ Mov(v23, v0);
  __ Smaxp(v23.V2S(), v23.V2S(), v23.V2S());

  __ Uminp(v24.V2S(), v0.V2S(), v1.V2S());
  __ Mov(v25, v0);
  __ Uminp(v25.V2S(), v25.V2S(), v1.V2S());
  __ Mov(v26, v1);
  __ Uminp(v26.V2S(), v0.V2S(), v26.V2S());
  __ Mov(v27, v0);
  __ Uminp(v27.V2S(), v27.V2S(), v27.V2S());

  __ Umaxp(v28.V2S(), v0.V2S(), v1.V2S());
  __ Mov(v29, v0);
  __ Umaxp(v29.V2S(), v29.V2S(), v1.V2S());
  __ Mov(v30, v1);
  __ Umaxp(v30.V2S(), v0.V2S(), v30.V2S());
  __ Mov(v31, v0);
  __ Umaxp(v31.V2S(), v31.V2S(), v31.V2S());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x0000000022222222, q16);
    ASSERT_EQUAL_128(0, 0x0000000022222222, q17);
    ASSERT_EQUAL_128(0, 0x0000000022222222, q18);
    ASSERT_EQUAL_128(0, 0x2222222222222222, q19);

    ASSERT_EQUAL_128(0, 0x1111111133333333, q20);
    ASSERT_EQUAL_128(0, 0x1111111133333333, q21);
    ASSERT_EQUAL_128(0, 0x1111111133333333, q22);
    ASSERT_EQUAL_128(0, 0x3333333333333333, q23);

    ASSERT_EQUAL_128(0, 0x0000000022222222, q24);
    ASSERT_EQUAL_128(0, 0x0000000022222222, q25);
    ASSERT_EQUAL_128(0, 0x0000000022222222, q26);
    ASSERT_EQUAL_128(0, 0x2222222222222222, q27);

    ASSERT_EQUAL_128(0, 0x1111111133333333, q28);
    ASSERT_EQUAL_128(0, 0x1111111133333333, q29);
    ASSERT_EQUAL_128(0, 0x1111111133333333, q30);
    ASSERT_EQUAL_128(0, 0x3333333333333333, q31);
  }
}


TEST(neon_destructive_tbl) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();
  __ Movi(v0.V2D(), 0x0041424334353627, 0x28291a1b1c0d0e0f);
  __ Movi(v1.V2D(), 0xafaeadacabaaa9a8, 0xa7a6a5a4a3a2a1a0);
  __ Movi(v2.V2D(), 0xbfbebdbcbbbab9b8, 0xb7b6b5b4b3b2b1b0);
  __ Movi(v3.V2D(), 0xcfcecdcccbcac9c8, 0xc7c6c5c4c3c2c1c0);
  __ Movi(v4.V2D(), 0xdfdedddcdbdad9d8, 0xd7d6d5d4d3d2d1d0);

  __ Movi(v16.V2D(), 0x5555555555555555, 0x5555555555555555);
  __ Tbl(v16.V16B(), v1.V16B(), v0.V16B());
  __ Mov(v17, v0);
  __ Tbl(v17.V16B(), v1.V16B(), v17.V16B());
  __ Mov(v18, v1);
  __ Tbl(v18.V16B(), v18.V16B(), v0.V16B());
  __ Mov(v19, v0);
  __ Tbl(v19.V16B(), v19.V16B(), v19.V16B());

  __ Movi(v20.V2D(), 0x5555555555555555, 0x5555555555555555);
  __ Tbl(v20.V16B(), v1.V16B(), v2.V16B(), v3.V16B(), v4.V16B(), v0.V16B());
  __ Mov(v21, v0);
  __ Tbl(v21.V16B(), v1.V16B(), v2.V16B(), v3.V16B(), v4.V16B(), v21.V16B());
  __ Mov(v22, v1);
  __ Mov(v23, v2);
  __ Mov(v24, v3);
  __ Mov(v25, v4);
  __ Tbl(v22.V16B(), v22.V16B(), v23.V16B(), v24.V16B(), v25.V16B(), v0.V16B());
  __ Mov(v26, v0);
  __ Mov(v27, v1);
  __ Mov(v28, v2);
  __ Mov(v29, v3);
  __ Tbl(v26.V16B(),
         v26.V16B(),
         v27.V16B(),
         v28.V16B(),
         v29.V16B(),
         v26.V16B());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xa000000000000000, 0x0000000000adaeaf, q16);
    ASSERT_EQUAL_128(0xa000000000000000, 0x0000000000adaeaf, q17);
    ASSERT_EQUAL_128(0xa000000000000000, 0x0000000000adaeaf, q18);
    ASSERT_EQUAL_128(0x0f00000000000000, 0x0000000000424100, q19);

    ASSERT_EQUAL_128(0xa0000000d4d5d6c7, 0xc8c9babbbcadaeaf, q20);
    ASSERT_EQUAL_128(0xa0000000d4d5d6c7, 0xc8c9babbbcadaeaf, q21);
    ASSERT_EQUAL_128(0xa0000000d4d5d6c7, 0xc8c9babbbcadaeaf, q22);
    ASSERT_EQUAL_128(0x0f000000c4c5c6b7, 0xb8b9aaabac424100, q26);
  }
}


TEST(neon_destructive_tbx) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();
  __ Movi(v0.V2D(), 0x0041424334353627, 0x28291a1b1c0d0e0f);
  __ Movi(v1.V2D(), 0xafaeadacabaaa9a8, 0xa7a6a5a4a3a2a1a0);
  __ Movi(v2.V2D(), 0xbfbebdbcbbbab9b8, 0xb7b6b5b4b3b2b1b0);
  __ Movi(v3.V2D(), 0xcfcecdcccbcac9c8, 0xc7c6c5c4c3c2c1c0);
  __ Movi(v4.V2D(), 0xdfdedddcdbdad9d8, 0xd7d6d5d4d3d2d1d0);

  __ Movi(v16.V2D(), 0x5555555555555555, 0x5555555555555555);
  __ Tbx(v16.V16B(), v1.V16B(), v0.V16B());
  __ Mov(v17, v0);
  __ Tbx(v17.V16B(), v1.V16B(), v17.V16B());
  __ Mov(v18, v1);
  __ Tbx(v18.V16B(), v18.V16B(), v0.V16B());
  __ Mov(v19, v0);
  __ Tbx(v19.V16B(), v19.V16B(), v19.V16B());

  __ Movi(v20.V2D(), 0x5555555555555555, 0x5555555555555555);
  __ Tbx(v20.V16B(), v1.V16B(), v2.V16B(), v3.V16B(), v4.V16B(), v0.V16B());
  __ Mov(v21, v0);
  __ Tbx(v21.V16B(), v1.V16B(), v2.V16B(), v3.V16B(), v4.V16B(), v21.V16B());
  __ Mov(v22, v1);
  __ Mov(v23, v2);
  __ Mov(v24, v3);
  __ Mov(v25, v4);
  __ Tbx(v22.V16B(), v22.V16B(), v23.V16B(), v24.V16B(), v25.V16B(), v0.V16B());
  __ Mov(v26, v0);
  __ Mov(v27, v1);
  __ Mov(v28, v2);
  __ Mov(v29, v3);
  __ Tbx(v26.V16B(),
         v26.V16B(),
         v27.V16B(),
         v28.V16B(),
         v29.V16B(),
         v26.V16B());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xa055555555555555, 0x5555555555adaeaf, q16);
    ASSERT_EQUAL_128(0xa041424334353627, 0x28291a1b1cadaeaf, q17);
    ASSERT_EQUAL_128(0xa0aeadacabaaa9a8, 0xa7a6a5a4a3adaeaf, q18);
    ASSERT_EQUAL_128(0x0f41424334353627, 0x28291a1b1c424100, q19);

    ASSERT_EQUAL_128(0xa0555555d4d5d6c7, 0xc8c9babbbcadaeaf, q20);
    ASSERT_EQUAL_128(0xa0414243d4d5d6c7, 0xc8c9babbbcadaeaf, q21);
    ASSERT_EQUAL_128(0xa0aeadacd4d5d6c7, 0xc8c9babbbcadaeaf, q22);
    ASSERT_EQUAL_128(0x0f414243c4c5c6b7, 0xb8b9aaabac424100, q26);
  }
}


TEST(neon_destructive_fcvtl) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();
  __ Movi(v0.V2D(), 0x400000003f800000, 0xbf800000c0000000);
  __ Fcvtl(v16.V2D(), v0.V2S());
  __ Fcvtl2(v17.V2D(), v0.V4S());
  __ Mov(v18, v0);
  __ Mov(v19, v0);
  __ Fcvtl(v18.V2D(), v18.V2S());
  __ Fcvtl2(v19.V2D(), v19.V4S());

  __ Movi(v1.V2D(), 0x40003c003c004000, 0xc000bc00bc00c000);
  __ Fcvtl(v20.V4S(), v1.V4H());
  __ Fcvtl2(v21.V4S(), v1.V8H());
  __ Mov(v22, v1);
  __ Mov(v23, v1);
  __ Fcvtl(v22.V4S(), v22.V4H());
  __ Fcvtl2(v23.V4S(), v23.V8H());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xbff0000000000000, 0xc000000000000000, q16);
    ASSERT_EQUAL_128(0x4000000000000000, 0x3ff0000000000000, q17);
    ASSERT_EQUAL_128(0xbff0000000000000, 0xc000000000000000, q18);
    ASSERT_EQUAL_128(0x4000000000000000, 0x3ff0000000000000, q19);

    ASSERT_EQUAL_128(0xc0000000bf800000, 0xbf800000c0000000, q20);
    ASSERT_EQUAL_128(0x400000003f800000, 0x3f80000040000000, q21);
    ASSERT_EQUAL_128(0xc0000000bf800000, 0xbf800000c0000000, q22);
    ASSERT_EQUAL_128(0x400000003f800000, 0x3f80000040000000, q23);
  }
}

TEST(fadd_h_neon) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Fmov(v0.V4H(), 24.0);
  __ Fmov(v1.V4H(), 1024.0);
  __ Fmov(v2.V8H(), 5.5);
  __ Fmov(v3.V8H(), 2048.0);
  __ Fmov(v4.V8H(), kFP16PositiveInfinity);
  __ Fmov(v5.V8H(), kFP16NegativeInfinity);
  __ Fmov(v6.V4H(), RawbitsToFloat16(0x7c2f));
  __ Fmov(v7.V8H(), RawbitsToFloat16(0xfe0f));

  __ Fadd(v8.V4H(), v1.V4H(), v0.V4H());
  __ Fadd(v9.V8H(), v3.V8H(), v2.V8H());
  __ Fadd(v10.V4H(), v4.V4H(), v3.V4H());

  __ Fadd(v11.V4H(), v6.V4H(), v1.V4H());
  __ Fadd(v12.V4H(), v7.V4H(), v7.V4H());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x6418641864186418, q8);
    // 2053.5 is unrepresentable in FP16.
    ASSERT_EQUAL_128(0x6803680368036803, 0x6803680368036803, q9);

    // Note: we test NaNs here as vectors aren't covered by process_nans_half
    // and we don't have traces for half-precision enabled hardware.
    // Default (Signalling NaN)
    ASSERT_EQUAL_128(0x0000000000000000, 0x7c007c007c007c00, q10);
    // Quiet NaN from Signalling.
    ASSERT_EQUAL_128(0x0000000000000000, 0x7e2f7e2f7e2f7e2f, q11);
    // Quiet NaN.
    ASSERT_EQUAL_128(0x0000000000000000, 0xfe0ffe0ffe0ffe0f, q12);
  }
}

TEST(fsub_h_neon) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Fmov(v0.V4H(), 24.0);
  __ Fmov(v1.V4H(), 1024.0);
  __ Fmov(v2.V8H(), 5.5);
  __ Fmov(v3.V8H(), 2048.0);
  __ Fmov(v4.V4H(), kFP16PositiveInfinity);
  __ Fmov(v5.V4H(), kFP16NegativeInfinity);
  __ Fmov(v6.V4H(), RawbitsToFloat16(0x7c22));
  __ Fmov(v7.V8H(), RawbitsToFloat16(0xfe02));

  __ Fsub(v0.V4H(), v1.V4H(), v0.V4H());
  __ Fsub(v8.V8H(), v3.V8H(), v2.V8H());
  __ Fsub(v9.V4H(), v4.V4H(), v3.V4H());
  __ Fsub(v10.V4H(), v0.V4H(), v1.V4H());

  __ Fsub(v11.V4H(), v6.V4H(), v2.V4H());
  __ Fsub(v12.V4H(), v7.V4H(), v7.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x63d063d063d063d0, q0);
    // 2042.5 is unpresentable in FP16:
    ASSERT_EQUAL_128(0x67fa67fa67fa67fa, 0x67fa67fa67fa67fa, q8);

    // Note: we test NaNs here as vectors aren't covered by process_nans_half
    // and we don't have traces for half-precision enabled hardware.
    // Signalling (Default) NaN.
    ASSERT_EQUAL_128(0x0000000000000000, 0x7c007c007c007c00, q9);
    ASSERT_EQUAL_128(0x0000000000000000, 0xce00ce00ce00ce00, q10);
    // Quiet NaN from Signalling.
    ASSERT_EQUAL_128(0x0000000000000000, 0x7e227e227e227e22, q11);
    // Quiet NaN.
    ASSERT_EQUAL_128(0x0000000000000000, 0xfe02fe02fe02fe02, q12);
  }
}

TEST(fmul_h_neon) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Fmov(v0.V4H(), 24.0);
  __ Fmov(v1.V4H(), -2.0);
  __ Fmov(v2.V8H(), 5.5);
  __ Fmov(v3.V8H(), 0.5);
  __ Fmov(v4.V4H(), kFP16PositiveInfinity);
  __ Fmov(v5.V4H(), kFP16NegativeInfinity);

  __ Fmul(v6.V4H(), v1.V4H(), v0.V4H());
  __ Fmul(v7.V8H(), v3.V8H(), v2.V8H());
  __ Fmul(v8.V4H(), v4.V4H(), v3.V4H());
  __ Fmul(v9.V4H(), v0.V4H(), v1.V4H());
  __ Fmul(v10.V4H(), v5.V4H(), v0.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0xd200d200d200d200, q6);
    ASSERT_EQUAL_128(0x4180418041804180, 0x4180418041804180, q7);
    ASSERT_EQUAL_128(0x0000000000000000, 0x7c007c007c007c00, q8);
    ASSERT_EQUAL_128(0x0000000000000000, 0xd200d200d200d200, q9);
    ASSERT_EQUAL_128(0x0000000000000000, 0xfc00fc00fc00fc00, q10);
  }
}

TEST(fdiv_h_neon) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Fmov(v0.V4H(), 24.0);
  __ Fmov(v1.V4H(), -2.0);
  __ Fmov(v2.V8H(), 5.5);
  __ Fmov(v3.V8H(), 0.5);
  __ Fmov(v4.V4H(), kFP16PositiveInfinity);
  __ Fmov(v5.V4H(), kFP16NegativeInfinity);

  __ Fdiv(v6.V4H(), v0.V4H(), v1.V4H());
  __ Fdiv(v7.V8H(), v2.V8H(), v3.V8H());
  __ Fdiv(v8.V4H(), v4.V4H(), v3.V4H());
  __ Fdiv(v9.V4H(), v1.V4H(), v0.V4H());
  __ Fdiv(v10.V4H(), v5.V4H(), v0.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0xca00ca00ca00ca00, q6);
    ASSERT_EQUAL_128(0x4980498049804980, 0x4980498049804980, q7);
    ASSERT_EQUAL_128(0x0000000000000000, 0x7c007c007c007c00, q8);
    // -0.083333... is unrepresentable in FP16:
    ASSERT_EQUAL_128(0x0000000000000000, 0xad55ad55ad55ad55, q9);
    ASSERT_EQUAL_128(0x0000000000000000, 0xfc00fc00fc00fc00, q10);
  }
}

TEST(neon_fcvtl) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();

  __ Movi(v0.V2D(), 0x000080007efffeff, 0x3100b1007c00fc00);
  __ Movi(v1.V2D(), 0x03ff83ff00038003, 0x000180017c01fc01);
  __ Movi(v2.V2D(), 0x3e200000be200000, 0x7f800000ff800000);
  __ Movi(v3.V2D(), 0x0000000080000000, 0x7f8fffffff8fffff);
  __ Movi(v4.V2D(), 0x7fcfffffffcfffff, 0x0000000180000001);
  __ Fcvtl(v16.V4S(), v0.V4H());
  __ Fcvtl2(v17.V4S(), v0.V8H());
  __ Fcvtl(v18.V4S(), v1.V4H());
  __ Fcvtl2(v19.V4S(), v1.V8H());

  __ Fcvtl(v20.V2D(), v2.V2S());
  __ Fcvtl2(v21.V2D(), v2.V4S());
  __ Fcvtl(v22.V2D(), v3.V2S());
  __ Fcvtl2(v23.V2D(), v3.V4S());
  __ Fcvtl(v24.V2D(), v4.V2S());
  __ Fcvtl2(v25.V2D(), v4.V4S());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x3e200000be200000, 0x7f800000ff800000, q16);
    ASSERT_EQUAL_128(0x0000000080000000, 0x7fdfe000ffdfe000, q17);
    ASSERT_EQUAL_128(0x33800000b3800000, 0x7fc02000ffc02000, q18);
    ASSERT_EQUAL_128(0x387fc000b87fc000, 0x34400000b4400000, q19);
    ASSERT_EQUAL_128(0x7ff0000000000000, 0xfff0000000000000, q20);
    ASSERT_EQUAL_128(0x3fc4000000000000, 0xbfc4000000000000, q21);
    ASSERT_EQUAL_128(0x7ff9ffffe0000000, 0xfff9ffffe0000000, q22);
    ASSERT_EQUAL_128(0x0000000000000000, 0x8000000000000000, q23);
    ASSERT_EQUAL_128(0x36a0000000000000, 0xb6a0000000000000, q24);
    ASSERT_EQUAL_128(0x7ff9ffffe0000000, 0xfff9ffffe0000000, q25);
  }
}


TEST(neon_fcvtn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();

  __ Movi(v0.V2D(), 0x3e200000be200000, 0x7f800000ff800000);
  __ Movi(v1.V2D(), 0x0000000080000000, 0x7f8fffffff8fffff);
  __ Movi(v2.V2D(), 0x7fcfffffffcfffff, 0x0000000180000001);
  __ Movi(v3.V2D(), 0x3fc4000000000000, 0xbfc4000000000000);
  __ Movi(v4.V2D(), 0x7ff0000000000000, 0xfff0000000000000);
  __ Movi(v5.V2D(), 0x0000000000000000, 0x8000000000000000);
  __ Movi(v6.V2D(), 0x7ff0ffffffffffff, 0xfff0ffffffffffff);
  __ Movi(v7.V2D(), 0x7ff8ffffffffffff, 0xfff8ffffffffffff);
  __ Movi(v8.V2D(), 0x0000000000000001, 0x8000000000000001);

  __ Fcvtn(v16.V4H(), v0.V4S());
  __ Fcvtn2(v16.V8H(), v1.V4S());
  __ Fcvtn(v17.V4H(), v2.V4S());
  __ Fcvtn(v18.V2S(), v3.V2D());
  __ Fcvtn2(v18.V4S(), v4.V2D());
  __ Fcvtn(v19.V2S(), v5.V2D());
  __ Fcvtn2(v19.V4S(), v6.V2D());
  __ Fcvtn(v20.V2S(), v7.V2D());
  __ Fcvtn2(v20.V4S(), v8.V2D());
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x000080007e7ffe7f, 0x3100b1007c00fc00, q16);
    ASSERT_EQUAL_64(0x7e7ffe7f00008000, d17);
    ASSERT_EQUAL_128(0x7f800000ff800000, 0x3e200000be200000, q18);
    ASSERT_EQUAL_128(0x7fc7ffffffc7ffff, 0x0000000080000000, q19);
    ASSERT_EQUAL_128(0x0000000080000000, 0x7fc7ffffffc7ffff, q20);
  }
}


TEST(neon_fcvtxn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();
  __ Movi(v0.V2D(), 0x3e200000be200000, 0x7f800000ff800000);
  __ Movi(v1.V2D(), 0x0000000080000000, 0x7f8fffffff8fffff);
  __ Movi(v2.V2D(), 0x7fcfffffffcfffff, 0x0000000180000001);
  __ Movi(v3.V2D(), 0x3fc4000000000000, 0xbfc4000000000000);
  __ Movi(v4.V2D(), 0x7ff0000000000000, 0xfff0000000000000);
  __ Movi(v5.V2D(), 0x0000000000000000, 0x8000000000000000);
  __ Movi(v6.V2D(), 0x7ff0ffffffffffff, 0xfff0ffffffffffff);
  __ Movi(v7.V2D(), 0x7ff8ffffffffffff, 0xfff8ffffffffffff);
  __ Movi(v8.V2D(), 0x0000000000000001, 0x8000000000000001);
  __ Movi(v9.V2D(), 0x41ed000000000000, 0x41efffffffefffff);
  __ Fcvtxn(v16.V2S(), v0.V2D());
  __ Fcvtxn2(v16.V4S(), v1.V2D());
  __ Fcvtxn(v17.V2S(), v2.V2D());
  __ Fcvtxn2(v17.V4S(), v3.V2D());
  __ Fcvtxn(v18.V2S(), v4.V2D());
  __ Fcvtxn2(v18.V4S(), v5.V2D());
  __ Fcvtxn(v19.V2S(), v6.V2D());
  __ Fcvtxn2(v19.V4S(), v7.V2D());
  __ Fcvtxn(v20.V2S(), v8.V2D());
  __ Fcvtxn2(v20.V4S(), v9.V2D());
  __ Fcvtxn(s21, d0);
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x000000017f7fffff, 0x310000057f7fffff, q16);
    ASSERT_EQUAL_128(0x3e200000be200000, 0x7f7fffff00000001, q17);
    ASSERT_EQUAL_128(0x0000000080000000, 0x7f800000ff800000, q18);
    ASSERT_EQUAL_128(0x7fc7ffffffc7ffff, 0x7fc7ffffffc7ffff, q19);
    ASSERT_EQUAL_128(0x4f6800004f7fffff, 0x0000000180000001, q20);
    ASSERT_EQUAL_128(0, 0x7f7fffff, q21);
  }
}

TEST(neon_3same_addp) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0xaa55ff55555500ff);
  __ Addp(v16.V16B(), v0.V16B(), v1.V16B());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x00ff54ffff54aaff, 0xffffffffffffffff, q16);
  }
}

TEST(neon_3same_sqdmulh_sqrdmulh) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000040004008000);
  __ Movi(v1.V2D(), 0x0000000000000000, 0x0000002000108000);
  __ Movi(v2.V2D(), 0x0400000080000000, 0x0400000080000000);
  __ Movi(v3.V2D(), 0x0000002080000000, 0x0000001080000000);

  __ Sqdmulh(v16.V4H(), v0.V4H(), v1.V4H());
  __ Sqdmulh(v17.V4S(), v2.V4S(), v3.V4S());
  __ Sqdmulh(h18, h0, h1);
  __ Sqdmulh(s19, s2, s3);

  __ Sqrdmulh(v20.V4H(), v0.V4H(), v1.V4H());
  __ Sqrdmulh(v21.V4S(), v2.V4S(), v3.V4S());
  __ Sqrdmulh(h22, h0, h1);
  __ Sqrdmulh(s23, s2, s3);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000100007fff, q16);
    ASSERT_EQUAL_128(0x000000017fffffff, 0x000000007fffffff, q17);
    ASSERT_EQUAL_128(0, 0x7fff, q18);
    ASSERT_EQUAL_128(0, 0x7fffffff, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000100017fff, q20);
    ASSERT_EQUAL_128(0x000000017fffffff, 0x000000017fffffff, q21);
    ASSERT_EQUAL_128(0, 0x7fff, q22);
    ASSERT_EQUAL_128(0, 0x7fffffff, q23);
  }
}

TEST(neon_byelement_sqdmulh_sqrdmulh) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000040004008000);
  __ Movi(v1.V2D(), 0x0000000000000000, 0x0000002000108000);
  __ Movi(v2.V2D(), 0x0400000080000000, 0x0400000080000000);
  __ Movi(v3.V2D(), 0x0000002080000000, 0x0000001080000000);

  __ Sqdmulh(v16.V4H(), v0.V4H(), v1.H(), 1);
  __ Sqdmulh(v17.V4S(), v2.V4S(), v3.S(), 1);
  __ Sqdmulh(h18, h0, v1.H(), 0);
  __ Sqdmulh(s19, s2, v3.S(), 0);

  __ Sqrdmulh(v20.V4H(), v0.V4H(), v1.H(), 1);
  __ Sqrdmulh(v21.V4S(), v2.V4S(), v3.S(), 1);
  __ Sqrdmulh(h22, h0, v1.H(), 0);
  __ Sqrdmulh(s23, s2, v3.S(), 0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000000000fff0, q16);
    ASSERT_EQUAL_128(0x00000000fffffff0, 0x00000000fffffff0, q17);
    ASSERT_EQUAL_128(0, 0x7fff, q18);
    ASSERT_EQUAL_128(0, 0x7fffffff, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000010001fff0, q20);
    ASSERT_EQUAL_128(0x00000001fffffff0, 0x00000001fffffff0, q21);
    ASSERT_EQUAL_128(0, 0x7fff, q22);
    ASSERT_EQUAL_128(0, 0x7fffffff, q23);
  }
}

TEST(neon_3same_sqrdmlah) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kRDM);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000040004008000);
  __ Movi(v1.V2D(), 0x0000000000000000, 0x0000002000108000);
  __ Movi(v2.V2D(), 0x0400000080000000, 0x0400000080000000);
  __ Movi(v3.V2D(), 0x0000002080000000, 0x0000001080000000);

  __ Movi(v16.V2D(), 0x0000040004008000, 0x0000040004008000);
  __ Movi(v17.V2D(), 0x0000000000000000, 0x0000002000108000);
  __ Movi(v18.V2D(), 0x0400000080000000, 0x0400000080000000);
  __ Movi(v19.V2D(), 0x0000002080000000, 0x0000001080000000);

  __ Sqrdmlah(v16.V4H(), v0.V4H(), v1.V4H());
  __ Sqrdmlah(v17.V4S(), v2.V4S(), v3.V4S());
  __ Sqrdmlah(h18, h0, h1);
  __ Sqrdmlah(s19, s2, s3);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0, 0x0000040104010000, q16);
    ASSERT_EQUAL_128(0x000000017fffffff, 0x000000217fffffff, q17);
    ASSERT_EQUAL_128(0, 0x7fff, q18);
    ASSERT_EQUAL_128(0, 0, q19);
  }
}

TEST(neon_byelement_sqrdmlah) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kRDM);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000040004008000);
  __ Movi(v1.V2D(), 0x0000000000000000, 0x0000002000108000);
  __ Movi(v2.V2D(), 0x0400000080000000, 0x0400000080000000);
  __ Movi(v3.V2D(), 0x0000002080000000, 0x0000001080000000);

  __ Movi(v16.V2D(), 0x0000040004008000, 0x0000040004008000);
  __ Movi(v17.V2D(), 0x0000000000000000, 0x0000002000108000);
  __ Movi(v18.V2D(), 0x0400000080000000, 0x0400000080000000);
  __ Movi(v19.V2D(), 0x0000002080000000, 0x0000001080000000);

  __ Sqrdmlah(v16.V4H(), v0.V4H(), v1.H(), 1);
  __ Sqrdmlah(v17.V4S(), v2.V4S(), v3.S(), 1);
  __ Sqrdmlah(h18, h0, v1.H(), 0);
  __ Sqrdmlah(s19, s2, v3.S(), 0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0, 0x0000040104018000, q16);
    ASSERT_EQUAL_128(0x00000001fffffff0, 0x0000002100107ff0, q17);
    ASSERT_EQUAL_128(0, 0x7fff, q18);
    ASSERT_EQUAL_128(0, 0, q19);
  }
}

TEST(neon_3same_sqrdmlsh) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kRDM);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000040004000500);
  __ Movi(v1.V2D(), 0x0000000000000000, 0x0000002000100080);
  __ Movi(v2.V2D(), 0x0400000080000000, 0x0400000080000000);
  __ Movi(v3.V2D(), 0x0000002080000000, 0x0000001080000000);

  __ Movi(v16.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v17.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v18.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v19.V2D(), 0x4000400040004000, 0x4000400040004000);

  __ Sqrdmlsh(v16.V4H(), v0.V4H(), v1.V4H());
  __ Sqrdmlsh(v17.V4S(), v2.V4S(), v3.V4S());
  __ Sqrdmlsh(h18, h0, h1);
  __ Sqrdmlsh(s19, s2, s3);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0, 0x40003fff40003ffb, q16);
    ASSERT_EQUAL_128(0x40003fffc0004000, 0x40004000c0004000, q17);
    ASSERT_EQUAL_128(0, 0x3ffb, q18);
    ASSERT_EQUAL_128(0, 0xc0004000, q19);
  }
}

TEST(neon_byelement_sqrdmlsh) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kRDM);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000040004008000);
  __ Movi(v1.V2D(), 0x0000000000000000, 0x0000002000108000);
  __ Movi(v2.V2D(), 0x0400000080000000, 0x0400000080000000);
  __ Movi(v3.V2D(), 0x0000002080000000, 0x0000001080000000);

  __ Movi(v16.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v17.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v18.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v19.V2D(), 0x4000400040004000, 0x4000400040004000);

  __ Sqrdmlsh(v16.V4H(), v0.V4H(), v1.H(), 1);
  __ Sqrdmlsh(v17.V4S(), v2.V4S(), v3.S(), 1);
  __ Sqrdmlsh(h18, h0, v1.H(), 0);
  __ Sqrdmlsh(s19, s2, v3.S(), 0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0, 0x4000400040004010, q16);
    ASSERT_EQUAL_128(0x4000400040004010, 0x4000400040004010, q17);
    ASSERT_EQUAL_128(0, 0xc000, q18);
    ASSERT_EQUAL_128(0, 0xc0004000, q19);
  }
}

TEST(neon_3same_sdot_udot) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kDotProduct);

  START();

  __ Movi(v0.V2D(), 0x7122712271227122, 0x7122712271227122);
  __ Movi(v1.V2D(), 0xe245e245f245f245, 0xe245e245f245f245);
  __ Movi(v2.V2D(), 0x3939393900000000, 0x3939393900000000);

  __ Movi(v16.V2D(), 0x0000400000004000, 0x0000400000004000);
  __ Movi(v17.V2D(), 0x0000400000004000, 0x0000400000004000);
  __ Movi(v18.V2D(), 0x0000400000004000, 0x0000400000004000);
  __ Movi(v19.V2D(), 0x0000400000004000, 0x0000400000004000);

  __ Sdot(v16.V4S(), v0.V16B(), v1.V16B());
  __ Sdot(v17.V2S(), v1.V8B(), v2.V8B());

  __ Udot(v18.V4S(), v0.V16B(), v1.V16B());
  __ Udot(v19.V2S(), v1.V8B(), v2.V8B());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x000037d8000045f8, 0x000037d8000045f8, q16);
    ASSERT_EQUAL_128(0, 0x0000515e00004000, q17);
    ASSERT_EQUAL_128(0x000119d8000127f8, 0x000119d8000127f8, q18);
    ASSERT_EQUAL_128(0, 0x0000c35e00004000, q19);
  }
}

TEST(neon_byelement_sdot_udot) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kDotProduct);

  START();

  __ Movi(v0.V2D(), 0x7122712271227122, 0x7122712271227122);
  __ Movi(v1.V2D(), 0xe245e245f245f245, 0xe245e245f245f245);
  __ Movi(v2.V2D(), 0x3939393900000000, 0x3939393900000000);

  __ Movi(v16.V2D(), 0x0000400000004000, 0x0000400000004000);
  __ Movi(v17.V2D(), 0x0000400000004000, 0x0000400000004000);
  __ Movi(v18.V2D(), 0x0000400000004000, 0x0000400000004000);
  __ Movi(v19.V2D(), 0x0000400000004000, 0x0000400000004000);

  __ Sdot(v16.V4S(), v0.V16B(), v1.S4B(), 1);
  __ Sdot(v17.V2S(), v1.V8B(), v2.S4B(), 1);

  __ Udot(v18.V4S(), v0.V16B(), v1.S4B(), 1);
  __ Udot(v19.V2S(), v1.V8B(), v2.S4B(), 1);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x000037d8000037d8, 0x000037d8000037d8, q16);
    ASSERT_EQUAL_128(0, 0x0000515e0000587e, q17);
    ASSERT_EQUAL_128(0x000119d8000119d8, 0x000119d8000119d8, q18);
    ASSERT_EQUAL_128(0, 0x0000c35e0000ca7e, q19);
  }
}


TEST(neon_2regmisc_saddlp) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);

  __ Saddlp(v16.V8H(), v0.V16B());
  __ Saddlp(v17.V4H(), v0.V8B());

  __ Saddlp(v18.V4S(), v0.V8H());
  __ Saddlp(v19.V2S(), v0.V4H());

  __ Saddlp(v20.V2D(), v0.V4S());
  __ Saddlp(v21.V1D(), v0.V2S());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0080ffffff010080, 0xff01ffff0080ff01, q16);
    ASSERT_EQUAL_128(0x0000000000000000, 0xff01ffff0080ff01, q17);
    ASSERT_EQUAL_128(0x0000800000000081, 0xffff7f81ffff8200, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffff7f81ffff8200, q19);
    ASSERT_EQUAL_128(0x0000000000818000, 0xffffffff82017f81, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffff82017f81, q21);
  }
}

TEST(neon_2regmisc_uaddlp) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);

  __ Uaddlp(v16.V8H(), v0.V16B());
  __ Uaddlp(v17.V4H(), v0.V8B());

  __ Uaddlp(v18.V4S(), v0.V8H());
  __ Uaddlp(v19.V2S(), v0.V4H());

  __ Uaddlp(v20.V2D(), v0.V4S());
  __ Uaddlp(v21.V1D(), v0.V2S());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x008000ff01010080, 0x010100ff00800101, q16);
    ASSERT_EQUAL_128(0x0000000000000000, 0x010100ff00800101, q17);
    ASSERT_EQUAL_128(0x0000800000010081, 0x00017f8100008200, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00017f8100008200, q19);
    ASSERT_EQUAL_128(0x0000000100818000, 0x0000000082017f81, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000082017f81, q21);
  }
}

TEST(neon_2regmisc_sadalp) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Mov(v16.V16B(), v1.V16B());
  __ Mov(v17.V16B(), v1.V16B());
  __ Sadalp(v16.V8H(), v0.V16B());
  __ Sadalp(v17.V4H(), v0.V8B());

  __ Mov(v18.V16B(), v2.V16B());
  __ Mov(v19.V16B(), v2.V16B());
  __ Sadalp(v18.V4S(), v1.V8H());
  __ Sadalp(v19.V2S(), v1.V4H());

  __ Mov(v20.V16B(), v3.V16B());
  __ Mov(v21.V16B(), v4.V16B());
  __ Sadalp(v20.V2D(), v2.V4S());
  __ Sadalp(v21.V1D(), v2.V2S());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x80808000ff000080, 0xff00ffff00817f00, q16);
    ASSERT_EQUAL_128(0x0000000000000000, 0xff00ffff00817f00, q17);
    ASSERT_EQUAL_128(0x7fff0001fffffffe, 0xffffffff80007fff, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffff80007fff, q19);
    ASSERT_EQUAL_128(0x7fffffff80000000, 0x800000007ffffffe, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000007fffffff, q21);
  }
}

TEST(neon_2regmisc_uadalp) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Mov(v16.V16B(), v1.V16B());
  __ Mov(v17.V16B(), v1.V16B());
  __ Uadalp(v16.V8H(), v0.V16B());
  __ Uadalp(v17.V4H(), v0.V8B());

  __ Mov(v18.V16B(), v2.V16B());
  __ Mov(v19.V16B(), v2.V16B());
  __ Uadalp(v18.V4S(), v1.V8H());
  __ Uadalp(v19.V2S(), v1.V4H());

  __ Mov(v20.V16B(), v3.V16B());
  __ Mov(v21.V16B(), v4.V16B());
  __ Uadalp(v20.V2D(), v2.V4S());
  __ Uadalp(v21.V1D(), v2.V2S());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x8080810001000080, 0x010000ff00818100, q16);
    ASSERT_EQUAL_128(0x0000000000000000, 0x010000ff00818100, q17);
    ASSERT_EQUAL_128(0x800100010000fffe, 0x0000ffff80007fff, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000ffff80007fff, q19);
    ASSERT_EQUAL_128(0x8000000180000000, 0x800000007ffffffe, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000007fffffff, q21);
  }
}

TEST(neon_3same_mul) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0xaa55ff55555500ff);
  __ Movi(v16.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);
  __ Movi(v17.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);

  __ Mla(v16.V16B(), v0.V16B(), v1.V16B());
  __ Mls(v17.V16B(), v0.V16B(), v1.V16B());
  __ Mul(v18.V16B(), v0.V16B(), v1.V16B());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0102757605b1b208, 0x5f0a61450db90f56, q16);
    ASSERT_EQUAL_128(0x01029192055b5c08, 0xb30ab5d30d630faa, q17);
    ASSERT_EQUAL_128(0x0000727200abab00, 0x5600563900ab0056, q18);
  }
}


TEST(neon_3same_absdiff) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0xaa55ff55555500ff);
  __ Movi(v16.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);
  __ Movi(v17.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);

  __ Saba(v16.V16B(), v0.V16B(), v1.V16B());
  __ Uaba(v17.V16B(), v0.V16B(), v1.V16B());
  __ Sabd(v18.V16B(), v0.V16B(), v1.V16B());
  __ Uabd(v19.V16B(), v0.V16B(), v1.V16B());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0202aeaf065c5d5e, 0x5e5f600c62646455, q16);
    ASSERT_EQUAL_128(0x0002585904b0b1b2, 0x5e5f600c62b86455, q17);
    ASSERT_EQUAL_128(0x0100abab01565656, 0x5555550055565555, q18);
    ASSERT_EQUAL_128(0xff005555ffaaaaaa, 0x5555550055aa5555, q19);
  }
}


TEST(neon_byelement_mul) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x000155aaff55ff00, 0xaa55ff55555500ff);


  __ Mul(v16.V4H(), v0.V4H(), v1.H(), 0);
  __ Mul(v17.V8H(), v0.V8H(), v1.H(), 7);
  __ Mul(v18.V2S(), v0.V2S(), v1.S(), 0);
  __ Mul(v19.V4S(), v0.V4S(), v1.S(), 3);

  __ Movi(v20.V2D(), 0x0000000000000000, 0x0001000200030004);
  __ Movi(v21.V2D(), 0x0005000600070008, 0x0001000200030004);
  __ Mla(v20.V4H(), v0.V4H(), v1.H(), 0);
  __ Mla(v21.V8H(), v0.V8H(), v1.H(), 7);

  __ Movi(v22.V2D(), 0x0000000000000000, 0x0000000200000004);
  __ Movi(v23.V2D(), 0x0000000600000008, 0x0000000200000004);
  __ Mla(v22.V2S(), v0.V2S(), v1.S(), 0);
  __ Mla(v23.V4S(), v0.V4S(), v1.S(), 3);

  __ Movi(v24.V2D(), 0x0000000000000000, 0x0100aaabfe015456);
  __ Movi(v25.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Mls(v24.V4H(), v0.V4H(), v1.H(), 0);
  __ Mls(v25.V8H(), v0.V8H(), v1.H(), 7);

  __ Movi(v26.V2D(), 0x0000000000000000, 0xc8e2aaabe1c85456);
  __ Movi(v27.V2D(), 0x39545572c6aa54e4, 0x39545572c6aa54e4);
  __ Mls(v26.V2S(), v0.V2S(), v1.S(), 0);
  __ Mls(v27.V4S(), v0.V4S(), v1.S(), 3);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x0100aaabfe015456, q16);
    ASSERT_EQUAL_128(0xff00aa5500ff55aa, 0xff00aa5500ff55aa, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0xc8e2aaabe1c85456, q18);
    ASSERT_EQUAL_128(0x39545572c6aa54e4, 0x39545572c6aa54e4, q19);

    ASSERT_EQUAL_128(0x0000000000000000, 0x0101aaadfe04545a, q20);
    ASSERT_EQUAL_128(0xff05aa5b010655b2, 0xff01aa57010255ae, q21);
    ASSERT_EQUAL_128(0x0000000000000000, 0xc8e2aaade1c8545a, q22);
    ASSERT_EQUAL_128(0x39545578c6aa54ec, 0x39545574c6aa54e8, q23);

    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q24);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q25);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q26);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q27);
  }
}


TEST(neon_byelement_mull) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaa55ff55555500ff, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x000155aaff55ff00, 0xaa55ff55555500ff);


  __ Smull(v16.V4S(), v0.V4H(), v1.H(), 7);
  __ Smull2(v17.V4S(), v0.V8H(), v1.H(), 0);
  __ Umull(v18.V4S(), v0.V4H(), v1.H(), 7);
  __ Umull2(v19.V4S(), v0.V8H(), v1.H(), 0);

  __ Movi(v20.V2D(), 0x0000000100000002, 0x0000000200000001);
  __ Movi(v21.V2D(), 0x0000000100000002, 0x0000000200000001);
  __ Movi(v22.V2D(), 0x0000000100000002, 0x0000000200000001);
  __ Movi(v23.V2D(), 0x0000000100000002, 0x0000000200000001);

  __ Smlal(v20.V4S(), v0.V4H(), v1.H(), 7);
  __ Smlal2(v21.V4S(), v0.V8H(), v1.H(), 0);
  __ Umlal(v22.V4S(), v0.V4H(), v1.H(), 7);
  __ Umlal2(v23.V4S(), v0.V8H(), v1.H(), 0);

  __ Movi(v24.V2D(), 0xffffff00ffffaa55, 0x000000ff000055aa);
  __ Movi(v25.V2D(), 0xffaaaaabffff55ab, 0x0054ffab0000fe01);
  __ Movi(v26.V2D(), 0x0000ff000000aa55, 0x000000ff000055aa);
  __ Movi(v27.V2D(), 0x00a9aaab00fe55ab, 0x0054ffab0000fe01);

  __ Smlsl(v24.V4S(), v0.V4H(), v1.H(), 7);
  __ Smlsl2(v25.V4S(), v0.V8H(), v1.H(), 0);
  __ Umlsl(v26.V4S(), v0.V4H(), v1.H(), 7);
  __ Umlsl2(v27.V4S(), v0.V8H(), v1.H(), 0);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xffffff00ffffaa55, 0x000000ff000055aa, q16);
    ASSERT_EQUAL_128(0xffaaaaabffff55ab, 0x0054ffab0000fe01, q17);
    ASSERT_EQUAL_128(0x0000ff000000aa55, 0x000000ff000055aa, q18);
    ASSERT_EQUAL_128(0x00a9aaab00fe55ab, 0x0054ffab0000fe01, q19);

    ASSERT_EQUAL_128(0xffffff01ffffaa57, 0x00000101000055ab, q20);
    ASSERT_EQUAL_128(0xffaaaaacffff55ad, 0x0054ffad0000fe02, q21);
    ASSERT_EQUAL_128(0x0000ff010000aa57, 0x00000101000055ab, q22);
    ASSERT_EQUAL_128(0x00a9aaac00fe55ad, 0x0054ffad0000fe02, q23);

    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q24);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q25);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q26);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q27);
  }
}


TEST(neon_byelement_sqdmull) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaa55ff55555500ff, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x000155aaff55ff00, 0xaa55ff55555500ff);

  __ Sqdmull(v16.V4S(), v0.V4H(), v1.H(), 7);
  __ Sqdmull2(v17.V4S(), v0.V8H(), v1.H(), 0);
  __ Sqdmull(s18, h0, v1.H(), 7);

  __ Movi(v20.V2D(), 0x0000000100000002, 0x0000000200000001);
  __ Movi(v21.V2D(), 0x0000000100000002, 0x0000000200000001);
  __ Movi(v22.V2D(), 0x0000000100000002, 0x0000000200000001);

  __ Sqdmlal(v20.V4S(), v0.V4H(), v1.H(), 7);
  __ Sqdmlal2(v21.V4S(), v0.V8H(), v1.H(), 0);
  __ Sqdmlal(s22, h0, v1.H(), 7);

  __ Movi(v24.V2D(), 0xfffffe00ffff54aa, 0x000001fe0000ab54);
  __ Movi(v25.V2D(), 0xff555556fffeab56, 0x00a9ff560001fc02);
  __ Movi(v26.V2D(), 0x0000000000000000, 0x000000000000ab54);

  __ Sqdmlsl(v24.V4S(), v0.V4H(), v1.H(), 7);
  __ Sqdmlsl2(v25.V4S(), v0.V8H(), v1.H(), 0);
  __ Sqdmlsl(s26, h0, v1.H(), 7);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xfffffe00ffff54aa, 0x000001fe0000ab54, q16);
    ASSERT_EQUAL_128(0xff555556fffeab56, 0x00a9ff560001fc02, q17);
    ASSERT_EQUAL_128(0, 0x0000ab54, q18);

    ASSERT_EQUAL_128(0xfffffe01ffff54ac, 0x000002000000ab55, q20);
    ASSERT_EQUAL_128(0xff555557fffeab58, 0x00a9ff580001fc03, q21);
    ASSERT_EQUAL_128(0, 0x0000ab55, q22);

    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q24);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q25);
    ASSERT_EQUAL_128(0, 0x00000000, q26);
  }
}


TEST(neon_3diff_absdiff) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55ab, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0xaa55ff55555500ff);
  __ Movi(v16.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);
  __ Movi(v17.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);
  __ Movi(v18.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);
  __ Movi(v19.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);

  __ Sabal(v16.V8H(), v0.V8B(), v1.V8B());
  __ Uabal(v17.V8H(), v0.V8B(), v1.V8B());
  __ Sabal2(v18.V8H(), v0.V16B(), v1.V16B());
  __ Uabal2(v19.V8H(), v0.V16B(), v1.V16B());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x01570359055b0708, 0x095f0b620d630f55, q16);
    ASSERT_EQUAL_128(0x01570359055b0708, 0x095f0bb60d630f55, q17);
    ASSERT_EQUAL_128(0x0103030405b107b3, 0x090b0b620d640f55, q18);
    ASSERT_EQUAL_128(0x02010304055b075d, 0x0a090bb60db80fab, q19);
  }
}


TEST(neon_3diff_sqdmull) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7fff7fff80008000, 0x80007fff7fff8000);
  __ Movi(v1.V2D(), 0x80007fff7fff8000, 0x7fff7fff80008000);
  __ Movi(v2.V2D(), 0x800000007fffffff, 0x7fffffff80000000);
  __ Movi(v3.V2D(), 0x8000000080000000, 0x8000000080000000);

  __ Sqdmull(v16.V4S(), v0.V4H(), v1.V4H());
  __ Sqdmull2(v17.V4S(), v0.V8H(), v1.V8H());
  __ Sqdmull(v18.V2D(), v2.V2S(), v3.V2S());
  __ Sqdmull2(v19.V2D(), v2.V4S(), v3.V4S());
  __ Sqdmull(s20, h0, h1);
  __ Sqdmull(d21, s2, s3);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x800100007ffe0002, 0x800100007fffffff, q16);
    ASSERT_EQUAL_128(0x800100007ffe0002, 0x800100007fffffff, q17);
    ASSERT_EQUAL_128(0x8000000100000000, 0x7fffffffffffffff, q18);
    ASSERT_EQUAL_128(0x7fffffffffffffff, 0x8000000100000000, q19);
    ASSERT_EQUAL_128(0, 0x7fffffff, q20);
    ASSERT_EQUAL_128(0, 0x7fffffffffffffff, q21);
  }
}


TEST(neon_3diff_sqdmlal) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7fff7fff80008000, 0x80007fff7fff8000);
  __ Movi(v1.V2D(), 0x80007fff7fff8000, 0x7fff7fff80008000);
  __ Movi(v2.V2D(), 0x800000007fffffff, 0x7fffffff80000000);
  __ Movi(v3.V2D(), 0x8000000080000000, 0x8000000080000000);

  __ Movi(v16.V2D(), 0xffffffff00000001, 0x8fffffff00000001);
  __ Movi(v17.V2D(), 0x00000001ffffffff, 0x00000001ffffffff);
  __ Movi(v18.V2D(), 0x8000000000000001, 0x0000000000000001);
  __ Movi(v19.V2D(), 0xffffffffffffffff, 0x7fffffffffffffff);
  __ Movi(v20.V2D(), 0, 0x00000001);
  __ Movi(v21.V2D(), 0, 0x00000001);

  __ Sqdmlal(v16.V4S(), v0.V4H(), v1.V4H());
  __ Sqdmlal2(v17.V4S(), v0.V8H(), v1.V8H());
  __ Sqdmlal(v18.V2D(), v2.V2S(), v3.V2S());
  __ Sqdmlal2(v19.V2D(), v2.V4S(), v3.V4S());
  __ Sqdmlal(s20, h0, h1);
  __ Sqdmlal(d21, s2, s3);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x8000ffff7ffe0003, 0x800000007fffffff, q16);
    ASSERT_EQUAL_128(0x800100017ffe0001, 0x800100017ffffffe, q17);
    ASSERT_EQUAL_128(0x8000000000000000, 0x7fffffffffffffff, q18);
    ASSERT_EQUAL_128(0x7ffffffffffffffe, 0x00000000ffffffff, q19);
    ASSERT_EQUAL_128(0, 0x7fffffff, q20);
    ASSERT_EQUAL_128(0, 0x7fffffffffffffff, q21);
  }
}


TEST(neon_3diff_sqdmlsl) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7fff7fff80008000, 0x80007fff7fff8000);
  __ Movi(v1.V2D(), 0x80007fff7fff8000, 0x7fff7fff80008000);
  __ Movi(v2.V2D(), 0x800000007fffffff, 0x7fffffff80000000);
  __ Movi(v3.V2D(), 0x8000000080000000, 0x8000000080000000);

  __ Movi(v16.V2D(), 0xffffffff00000001, 0x7ffffffe80000001);
  __ Movi(v17.V2D(), 0x00000001ffffffff, 0x7ffffffe00000001);
  __ Movi(v18.V2D(), 0x8000000000000001, 0x8000000000000001);
  __ Movi(v19.V2D(), 0xfffffffffffffffe, 0x7fffffffffffffff);
  __ Movi(v20.V2D(), 0, 0x00000001);
  __ Movi(v21.V2D(), 0, 0x00000001);

  __ Sqdmlsl(v16.V4S(), v0.V4H(), v1.V4H());
  __ Sqdmlsl2(v17.V4S(), v0.V8H(), v1.V8H());
  __ Sqdmlsl(v18.V2D(), v2.V2S(), v3.V2S());
  __ Sqdmlsl2(v19.V2D(), v2.V4S(), v3.V4S());
  __ Sqdmlsl(s20, h0, h1);
  __ Sqdmlsl(d21, s2, s3);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x7ffeffff8001ffff, 0x7fffffff80000000, q16);
    ASSERT_EQUAL_128(0x7fff00018001fffd, 0x7fffffff80000002, q17);
    ASSERT_EQUAL_128(0xffffffff00000001, 0x8000000000000000, q18);
    ASSERT_EQUAL_128(0x8000000000000000, 0x7fffffffffffffff, q19);
    ASSERT_EQUAL_128(0, 0x80000002, q20);
    ASSERT_EQUAL_128(0, 0x8000000000000002, q21);
  }
}


TEST(neon_3diff_mla) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55ab, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0xaa55ff55555500ff);
  __ Movi(v16.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);
  __ Movi(v17.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);
  __ Movi(v18.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);
  __ Movi(v19.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);

  __ Smlal(v16.V8H(), v0.V8B(), v1.V8B());
  __ Umlal(v17.V8H(), v0.V8B(), v1.V8B());
  __ Smlal2(v18.V8H(), v0.V16B(), v1.V16B());
  __ Umlal2(v19.V8H(), v0.V16B(), v1.V16B());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x01580304055c2341, 0x090a0ab70d0e0f56, q16);
    ASSERT_EQUAL_128(0xaa580304ae5c2341, 0x090a5fb70d0eb856, q17);
    ASSERT_EQUAL_128(0x01020304e878ea7a, 0x090a0ab70cb90f00, q18);
    ASSERT_EQUAL_128(0x010203043d783f7a, 0x090a5fb761b90f00, q19);
  }
}


TEST(neon_3diff_mls) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55ab, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0xaa55ff55555500ff);
  __ Movi(v16.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);
  __ Movi(v17.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);
  __ Movi(v18.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);
  __ Movi(v19.V2D(), 0x0102030405060708, 0x090a0b0c0d0e0f00);

  __ Smlsl(v16.V8H(), v0.V8B(), v1.V8B());
  __ Umlsl(v17.V8H(), v0.V8B(), v1.V8B());
  __ Smlsl2(v18.V8H(), v0.V16B(), v1.V16B());
  __ Umlsl2(v19.V8H(), v0.V16B(), v1.V16B());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x00ac030404b0eacf, 0x090a0b610d0e0eaa, q16);
    ASSERT_EQUAL_128(0x57ac03045bb0eacf, 0x090ab6610d0e65aa, q17);
    ASSERT_EQUAL_128(0x0102030421942396, 0x090a0b610d630f00, q18);
    ASSERT_EQUAL_128(0x01020304cc94ce96, 0x090ab661b8630f00, q19);
  }
}


TEST(neon_3same_compare) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0xaa55ff55555500ff);

  __ Cmeq(v16.V16B(), v0.V16B(), v0.V16B());
  __ Cmeq(v17.V16B(), v0.V16B(), v1.V16B());
  __ Cmge(v18.V16B(), v0.V16B(), v0.V16B());
  __ Cmge(v19.V16B(), v0.V16B(), v1.V16B());
  __ Cmgt(v20.V16B(), v0.V16B(), v0.V16B());
  __ Cmgt(v21.V16B(), v0.V16B(), v1.V16B());
  __ Cmhi(v22.V16B(), v0.V16B(), v0.V16B());
  __ Cmhi(v23.V16B(), v0.V16B(), v1.V16B());
  __ Cmhs(v24.V16B(), v0.V16B(), v0.V16B());
  __ Cmhs(v25.V16B(), v0.V16B(), v1.V16B());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q16);
    ASSERT_EQUAL_128(0x00ff000000000000, 0x000000ff00000000, q17);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q18);
    ASSERT_EQUAL_128(0x00ff00ffff00ff00, 0xff0000ff0000ff00, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q20);
    ASSERT_EQUAL_128(0x000000ffff00ff00, 0xff0000000000ff00, q21);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q22);
    ASSERT_EQUAL_128(0xff00ff0000ff00ff, 0xff00000000ffff00, q23);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q24);
    ASSERT_EQUAL_128(0xffffff0000ff00ff, 0xff0000ff00ffff00, q25);
  }
}


TEST(neon_3same_scalar_compare) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0xaa55ff55555500ff);

  __ Cmeq(d16, d0, d0);
  __ Cmeq(d17, d0, d1);
  __ Cmeq(d18, d1, d0);
  __ Cmge(d19, d0, d0);
  __ Cmge(d20, d0, d1);
  __ Cmge(d21, d1, d0);
  __ Cmgt(d22, d0, d0);
  __ Cmgt(d23, d0, d1);
  __ Cmhi(d24, d0, d0);
  __ Cmhi(d25, d0, d1);
  __ Cmhs(d26, d0, d0);
  __ Cmhs(d27, d0, d1);
  __ Cmhs(d28, d1, d0);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q16);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q17);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q18);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q19);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q20);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q21);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q22);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q23);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q24);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q25);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q26);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q27);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q28);
  }
}

TEST(neon_fcmeq_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000000000000000);  // 0.
  __ Movi(v1.V2D(), 0xffffffffffffffff, 0xffffffffffffffff);  // NaN.
  __ Movi(v2.V2D(), 0xbc00bc00bc00bc00, 0xbc00bc00bc00bc00);  // -1.0.
  __ Movi(v3.V2D(), 0x3c003c003c003c00, 0x3c003c003c003c00);  // 1.0.

  __ Fcmeq(v4.V8H(), v0.V8H(), v0.V8H());
  __ Fcmeq(v5.V8H(), v1.V8H(), v0.V8H());
  __ Fcmeq(v6.V8H(), v2.V8H(), v0.V8H());
  __ Fcmeq(v7.V8H(), v3.V8H(), v0.V8H());
  __ Fcmeq(v8.V4H(), v0.V4H(), v0.V4H());
  __ Fcmeq(v9.V4H(), v1.V4H(), v0.V4H());
  __ Fcmeq(v10.V4H(), v2.V4H(), v0.V4H());
  __ Fcmeq(v11.V4H(), v3.V4H(), v0.V4H());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, v4);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v5);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v6);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v7);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, v8);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v9);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v10);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v11);
  }
}

TEST(neon_fcmeq_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFPHalf);

  START();

  __ Fmov(h0, Float16(0.0));
  __ Fmov(h1, RawbitsToFloat16(0xffff));
  __ Fmov(h2, Float16(-1.0));
  __ Fmov(h3, Float16(1.0));
  __ Fcmeq(h4, h0, h0);
  __ Fcmeq(h5, h1, h0);
  __ Fcmeq(h6, h2, h0);
  __ Fcmeq(h7, h3, h0);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xffff), h4);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x0000), h5);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x0000), h6);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x0000), h7);
  }
}

TEST(neon_fcmge_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000000000000000);  // 0.
  __ Movi(v1.V2D(), 0xffffffffffffffff, 0xffffffffffffffff);  // NaN.
  __ Movi(v2.V2D(), 0xbc00bc00bc00bc00, 0xbc00bc00bc00bc00);  // -1.0.
  __ Movi(v3.V2D(), 0x3c003c003c003c00, 0x3c003c003c003c00);  // 1.0.

  __ Fcmge(v4.V8H(), v0.V8H(), v0.V8H());
  __ Fcmge(v5.V8H(), v1.V8H(), v0.V8H());
  __ Fcmge(v6.V8H(), v2.V8H(), v0.V8H());
  __ Fcmge(v7.V8H(), v3.V8H(), v0.V8H());
  __ Fcmge(v8.V4H(), v0.V4H(), v0.V4H());
  __ Fcmge(v9.V4H(), v1.V4H(), v0.V4H());
  __ Fcmge(v10.V4H(), v2.V4H(), v0.V4H());
  __ Fcmge(v11.V4H(), v3.V4H(), v0.V4H());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, v4);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v5);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v6);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, v7);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, v8);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v9);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v10);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, v11);
  }
}

TEST(neon_fcmge_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFPHalf);

  START();

  __ Fmov(h0, Float16(0.0));
  __ Fmov(h1, RawbitsToFloat16(0xffff));
  __ Fmov(h2, Float16(-1.0));
  __ Fmov(h3, Float16(1.0));
  __ Fcmge(h4, h0, h0);
  __ Fcmge(h5, h1, h0);
  __ Fcmge(h6, h2, h0);
  __ Fcmge(h7, h3, h0);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xffff), h4);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x0000), h5);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x0000), h6);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xffff), h7);
  }
}

TEST(neon_fcmgt_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000000000000000);  // 0.
  __ Movi(v1.V2D(), 0xffffffffffffffff, 0xffffffffffffffff);  // NaN.
  __ Movi(v2.V2D(), 0xbc00bc00bc00bc00, 0xbc00bc00bc00bc00);  // -1.0.
  __ Movi(v3.V2D(), 0x3c003c003c003c00, 0x3c003c003c003c00);  // 1.0.

  __ Fcmgt(v4.V8H(), v0.V8H(), v0.V8H());
  __ Fcmgt(v5.V8H(), v1.V8H(), v0.V8H());
  __ Fcmgt(v6.V8H(), v2.V8H(), v0.V8H());
  __ Fcmgt(v7.V8H(), v3.V8H(), v0.V8H());
  __ Fcmgt(v8.V4H(), v0.V4H(), v0.V4H());
  __ Fcmgt(v9.V4H(), v1.V4H(), v0.V4H());
  __ Fcmgt(v10.V4H(), v2.V4H(), v0.V4H());
  __ Fcmgt(v11.V4H(), v3.V4H(), v0.V4H());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v4);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v5);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v6);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, v7);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v8);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v9);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v10);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, v11);
  }
}

TEST(neon_fcmgt_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFPHalf);

  START();

  __ Fmov(h0, Float16(0.0));
  __ Fmov(h1, RawbitsToFloat16(0xffff));
  __ Fmov(h2, Float16(-1.0));
  __ Fmov(h3, Float16(1.0));
  __ Fcmgt(h4, h0, h0);
  __ Fcmgt(h5, h1, h0);
  __ Fcmgt(h6, h2, h0);
  __ Fcmgt(h7, h3, h0);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x0000), h4);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x0000), h5);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x0000), h6);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xffff), h7);
  }
}

TEST(neon_facge_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000000000000000);  // 0.
  __ Movi(v1.V2D(), 0xffffffffffffffff, 0xffffffffffffffff);  // NaN.
  __ Movi(v2.V2D(), 0xbc00bc00bc00bc00, 0xbc00bc00bc00bc00);  // -1.0.
  __ Movi(v3.V2D(), 0x3c003c003c003c00, 0x3c003c003c003c00);  // 1.0.

  __ Facge(v4.V8H(), v0.V8H(), v0.V8H());
  __ Facge(v5.V8H(), v1.V8H(), v0.V8H());
  __ Facge(v6.V8H(), v2.V8H(), v0.V8H());
  __ Facge(v7.V8H(), v3.V8H(), v0.V8H());
  __ Facge(v8.V4H(), v0.V4H(), v0.V4H());
  __ Facge(v9.V4H(), v1.V4H(), v0.V4H());
  __ Facge(v10.V4H(), v2.V4H(), v0.V4H());
  __ Facge(v11.V4H(), v3.V4H(), v0.V4H());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, v4);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v5);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, v6);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, v7);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, v8);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v9);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, v10);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, v11);
  }
}

TEST(neon_facge_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFPHalf);

  START();

  __ Fmov(h0, Float16(0.0));
  __ Fmov(h1, RawbitsToFloat16(0xffff));
  __ Fmov(h2, Float16(-1.0));
  __ Fmov(h3, Float16(1.0));
  __ Facge(h4, h0, h0);
  __ Facge(h5, h1, h0);
  __ Facge(h6, h2, h0);
  __ Facge(h7, h3, h0);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xffff), h4);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x0000), h5);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xffff), h6);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xffff), h7);
  }
}

TEST(neon_facgt_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000000000000000);  // 0.
  __ Movi(v1.V2D(), 0xffffffffffffffff, 0xffffffffffffffff);  // NaN.
  __ Movi(v2.V2D(), 0xbc00bc00bc00bc00, 0xbc00bc00bc00bc00);  // -1.0.
  __ Movi(v3.V2D(), 0x3c003c003c003c00, 0x3c003c003c003c00);  // 1.0.

  __ Facgt(v4.V8H(), v0.V8H(), v0.V8H());
  __ Facgt(v5.V8H(), v1.V8H(), v0.V8H());
  __ Facgt(v6.V8H(), v2.V8H(), v0.V8H());
  __ Facgt(v7.V8H(), v3.V8H(), v0.V8H());
  __ Facgt(v8.V4H(), v0.V4H(), v0.V4H());
  __ Facgt(v9.V4H(), v1.V4H(), v0.V4H());
  __ Facgt(v10.V4H(), v2.V4H(), v0.V4H());
  __ Facgt(v11.V4H(), v3.V4H(), v0.V4H());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v4);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v5);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, v6);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, v7);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v8);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v9);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, v10);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, v11);
  }
}

TEST(neon_facgt_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFPHalf);

  START();

  __ Fmov(h0, Float16(0.0));
  __ Fmov(h1, RawbitsToFloat16(0xffff));
  __ Fmov(h2, Float16(-1.0));
  __ Fmov(h3, Float16(1.0));
  __ Facgt(h4, h0, h0);
  __ Facgt(h5, h1, h0);
  __ Facgt(h6, h2, h0);
  __ Facgt(h7, h3, h0);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x0000), h4);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0x0000), h5);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xffff), h6);
    ASSERT_EQUAL_FP16(RawbitsToFloat16(0xffff), h7);
  }
}

TEST(neon_2regmisc_fcmeq) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000000000000000);  // Zero.
  __ Movi(v1.V2D(), 0xffffffffffffffff, 0xffffffffffffffff);  // Nan.
  __ Movi(v2.V2D(), 0xbf800000bf800000, 0xbf800000bf800000);  // < 0.
  __ Movi(v3.V2D(), 0x3f8000003f800000, 0x3f8000003f800000);  // > 0.

  __ Fcmeq(s16, s0, 0.0);
  __ Fcmeq(s17, s1, 0.0);
  __ Fcmeq(s18, s2, 0.0);
  __ Fcmeq(d19, d0, 0.0);
  __ Fcmeq(d20, d1, 0.0);
  __ Fcmeq(d21, d2, 0.0);
  __ Fcmeq(v22.V2S(), v0.V2S(), 0.0);
  __ Fcmeq(v23.V4S(), v1.V4S(), 0.0);
  __ Fcmeq(v24.V2D(), v1.V2D(), 0.0);
  __ Fcmeq(v25.V2D(), v2.V2D(), 0.0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0, 0xffffffff, q16);
    ASSERT_EQUAL_128(0, 0x00000000, q17);
    ASSERT_EQUAL_128(0, 0x00000000, q18);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q19);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q20);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q21);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q22);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q24);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q25);
  }
}

TEST(neon_2regmisc_fcmge) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000000000000000);  // Zero.
  __ Movi(v1.V2D(), 0xffffffffffffffff, 0xffffffffffffffff);  // Nan.
  __ Movi(v2.V2D(), 0xbf800000bf800000, 0xbf800000bf800000);  // < 0.
  __ Movi(v3.V2D(), 0x3f8000003f800000, 0x3f8000003f800000);  // > 0.

  __ Fcmge(s16, s0, 0.0);
  __ Fcmge(s17, s1, 0.0);
  __ Fcmge(s18, s2, 0.0);
  __ Fcmge(d19, d0, 0.0);
  __ Fcmge(d20, d1, 0.0);
  __ Fcmge(d21, d3, 0.0);
  __ Fcmge(v22.V2S(), v0.V2S(), 0.0);
  __ Fcmge(v23.V4S(), v1.V4S(), 0.0);
  __ Fcmge(v24.V2D(), v1.V2D(), 0.0);
  __ Fcmge(v25.V2D(), v3.V2D(), 0.0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0, 0xffffffff, q16);
    ASSERT_EQUAL_128(0, 0x00000000, q17);
    ASSERT_EQUAL_128(0, 0x00000000, q18);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q19);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q20);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q21);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q22);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q24);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q25);
  }
}


TEST(neon_2regmisc_fcmgt) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000000000000000);  // Zero.
  __ Movi(v1.V2D(), 0xffffffffffffffff, 0xffffffffffffffff);  // Nan.
  __ Movi(v2.V2D(), 0xbf800000bf800000, 0xbf800000bf800000);  // < 0.
  __ Movi(v3.V2D(), 0x3f8000003f800000, 0x3f8000003f800000);  // > 0.

  __ Fcmgt(s16, s0, 0.0);
  __ Fcmgt(s17, s1, 0.0);
  __ Fcmgt(s18, s2, 0.0);
  __ Fcmgt(d19, d0, 0.0);
  __ Fcmgt(d20, d1, 0.0);
  __ Fcmgt(d21, d3, 0.0);
  __ Fcmgt(v22.V2S(), v0.V2S(), 0.0);
  __ Fcmgt(v23.V4S(), v1.V4S(), 0.0);
  __ Fcmgt(v24.V2D(), v1.V2D(), 0.0);
  __ Fcmgt(v25.V2D(), v3.V2D(), 0.0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0, 0x00000000, q16);
    ASSERT_EQUAL_128(0, 0x00000000, q17);
    ASSERT_EQUAL_128(0, 0x00000000, q18);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q19);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q20);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q21);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q22);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q24);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q25);
  }
}

TEST(neon_2regmisc_fcmle) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000000000000000);  // Zero.
  __ Movi(v1.V2D(), 0xffffffffffffffff, 0xffffffffffffffff);  // Nan.
  __ Movi(v2.V2D(), 0xbf800000bf800000, 0xbf800000bf800000);  // < 0.
  __ Movi(v3.V2D(), 0x3f8000003f800000, 0x3f8000003f800000);  // > 0.

  __ Fcmle(s16, s0, 0.0);
  __ Fcmle(s17, s1, 0.0);
  __ Fcmle(s18, s3, 0.0);
  __ Fcmle(d19, d0, 0.0);
  __ Fcmle(d20, d1, 0.0);
  __ Fcmle(d21, d2, 0.0);
  __ Fcmle(v22.V2S(), v0.V2S(), 0.0);
  __ Fcmle(v23.V4S(), v1.V4S(), 0.0);
  __ Fcmle(v24.V2D(), v1.V2D(), 0.0);
  __ Fcmle(v25.V2D(), v2.V2D(), 0.0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0, 0xffffffff, q16);
    ASSERT_EQUAL_128(0, 0x00000000, q17);
    ASSERT_EQUAL_128(0, 0x00000000, q18);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q19);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q20);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q21);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q22);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q24);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q25);
  }
}


TEST(neon_2regmisc_fcmlt) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000000000000000);  // Zero.
  __ Movi(v1.V2D(), 0xffffffffffffffff, 0xffffffffffffffff);  // Nan.
  __ Movi(v2.V2D(), 0xbf800000bf800000, 0xbf800000bf800000);  // < 0.
  __ Movi(v3.V2D(), 0x3f8000003f800000, 0x3f8000003f800000);  // > 0.

  __ Fcmlt(s16, s0, 0.0);
  __ Fcmlt(s17, s1, 0.0);
  __ Fcmlt(s18, s3, 0.0);
  __ Fcmlt(d19, d0, 0.0);
  __ Fcmlt(d20, d1, 0.0);
  __ Fcmlt(d21, d2, 0.0);
  __ Fcmlt(v22.V2S(), v0.V2S(), 0.0);
  __ Fcmlt(v23.V4S(), v1.V4S(), 0.0);
  __ Fcmlt(v24.V2D(), v1.V2D(), 0.0);
  __ Fcmlt(v25.V2D(), v2.V2D(), 0.0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0, 0x00000000, q16);
    ASSERT_EQUAL_128(0, 0x00000000, q17);
    ASSERT_EQUAL_128(0, 0x00000000, q18);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q19);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q20);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q21);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q22);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q24);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q25);
  }
}

TEST(neon_2regmisc_cmeq) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0001000200030004, 0x0000000000000000);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0x0000ff55555500ff);

  __ Cmeq(v16.V8B(), v1.V8B(), 0);
  __ Cmeq(v17.V16B(), v1.V16B(), 0);
  __ Cmeq(v18.V4H(), v1.V4H(), 0);
  __ Cmeq(v19.V8H(), v1.V8H(), 0);
  __ Cmeq(v20.V2S(), v0.V2S(), 0);
  __ Cmeq(v21.V4S(), v0.V4S(), 0);
  __ Cmeq(d22, d0, 0);
  __ Cmeq(d23, d1, 0);
  __ Cmeq(v24.V2D(), v0.V2D(), 0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0xffff00000000ff00, q16);
    ASSERT_EQUAL_128(0xffff0000000000ff, 0xffff00000000ff00, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffff000000000000, q18);
    ASSERT_EQUAL_128(0xffff000000000000, 0xffff000000000000, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q21);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q22);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q24);
  }
}


TEST(neon_2regmisc_cmge) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff01000200030004, 0x0000000000000000);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0x0000ff55555500ff);

  __ Cmge(v16.V8B(), v1.V8B(), 0);
  __ Cmge(v17.V16B(), v1.V16B(), 0);
  __ Cmge(v18.V4H(), v1.V4H(), 0);
  __ Cmge(v19.V8H(), v1.V8H(), 0);
  __ Cmge(v20.V2S(), v0.V2S(), 0);
  __ Cmge(v21.V4S(), v0.V4S(), 0);
  __ Cmge(d22, d0, 0);
  __ Cmge(d23, d1, 0);
  __ Cmge(v24.V2D(), v0.V2D(), 0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0xffff00ffffffff00, q16);
    ASSERT_EQUAL_128(0xffffff0000ff00ff, 0xffff00ffffffff00, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffff0000ffffffff, q18);
    ASSERT_EQUAL_128(0xffffffff00000000, 0xffff0000ffffffff, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q20);
    ASSERT_EQUAL_128(0x00000000ffffffff, 0xffffffffffffffff, q21);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q22);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q24);
  }
}


TEST(neon_2regmisc_cmlt) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0001000200030004, 0xff00000000000000);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0x0000ff55555500ff);

  __ Cmlt(v16.V8B(), v1.V8B(), 0);
  __ Cmlt(v17.V16B(), v1.V16B(), 0);
  __ Cmlt(v18.V4H(), v1.V4H(), 0);
  __ Cmlt(v19.V8H(), v1.V8H(), 0);
  __ Cmlt(v20.V2S(), v1.V2S(), 0);
  __ Cmlt(v21.V4S(), v1.V4S(), 0);
  __ Cmlt(d22, d0, 0);
  __ Cmlt(d23, d1, 0);
  __ Cmlt(v24.V2D(), v0.V2D(), 0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000ff00000000ff, q16);
    ASSERT_EQUAL_128(0x000000ffff00ff00, 0x0000ff00000000ff, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000ffff00000000, q18);
    ASSERT_EQUAL_128(0x00000000ffffffff, 0x0000ffff00000000, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q20);
    ASSERT_EQUAL_128(0x00000000ffffffff, 0x0000000000000000, q21);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q22);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q24);
  }
}


TEST(neon_2regmisc_cmle) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0001000200030004, 0x0000000000000000);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0x0000ff55555500ff);

  __ Cmle(v16.V8B(), v1.V8B(), 0);
  __ Cmle(v17.V16B(), v1.V16B(), 0);
  __ Cmle(v18.V4H(), v1.V4H(), 0);
  __ Cmle(v19.V8H(), v1.V8H(), 0);
  __ Cmle(v20.V2S(), v1.V2S(), 0);
  __ Cmle(v21.V4S(), v1.V4S(), 0);
  __ Cmle(d22, d0, 0);
  __ Cmle(d23, d1, 0);
  __ Cmle(v24.V2D(), v0.V2D(), 0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffff000000ffff, q16);
    ASSERT_EQUAL_128(0xffff00ffff00ffff, 0xffffff000000ffff, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffff00000000, q18);
    ASSERT_EQUAL_128(0xffff0000ffffffff, 0xffffffff00000000, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q20);
    ASSERT_EQUAL_128(0x00000000ffffffff, 0x0000000000000000, q21);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q22);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q24);
  }
}


TEST(neon_2regmisc_cmgt) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0001000200030004, 0x0000000000000000);
  __ Movi(v1.V2D(), 0x000055aaff55ff00, 0x0000ff55555500ff);

  __ Cmgt(v16.V8B(), v1.V8B(), 0);
  __ Cmgt(v17.V16B(), v1.V16B(), 0);
  __ Cmgt(v18.V4H(), v1.V4H(), 0);
  __ Cmgt(v19.V8H(), v1.V8H(), 0);
  __ Cmgt(v20.V2S(), v0.V2S(), 0);
  __ Cmgt(v21.V4S(), v0.V4S(), 0);
  __ Cmgt(d22, d0, 0);
  __ Cmgt(d23, d1, 0);
  __ Cmgt(v24.V2D(), v0.V2D(), 0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000ffffff0000, q16);
    ASSERT_EQUAL_128(0x0000ff0000ff0000, 0x000000ffffff0000, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00000000ffffffff, q18);
    ASSERT_EQUAL_128(0x0000ffff00000000, 0x00000000ffffffff, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q20);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0x0000000000000000, q21);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q22);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffffffffffffffff, q23);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0x0000000000000000, q24);
  }
}


TEST(neon_2regmisc_neg) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Neg(v16.V8B(), v0.V8B());
  __ Neg(v17.V16B(), v0.V16B());
  __ Neg(v18.V4H(), v1.V4H());
  __ Neg(v19.V8H(), v1.V8H());
  __ Neg(v20.V2S(), v2.V2S());
  __ Neg(v21.V4S(), v2.V4S());
  __ Neg(d22, d3);
  __ Neg(v23.V2D(), v3.V2D());
  __ Neg(v24.V2D(), v4.V2D());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x807f0100ff81807f, q16);
    ASSERT_EQUAL_128(0x81ff00017f8081ff, 0x807f0100ff81807f, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00010000ffff8001, q18);
    ASSERT_EQUAL_128(0x80007fff00010000, 0x00010000ffff8001, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000080000001, q20);
    ASSERT_EQUAL_128(0x8000000000000001, 0x0000000080000001, q21);
    ASSERT_EQUAL_128(0x0000000000000000, 0x8000000000000001, q22);
    ASSERT_EQUAL_128(0x7fffffffffffffff, 0x8000000000000001, q23);
    ASSERT_EQUAL_128(0x8000000000000000, 0x0000000000000000, q24);
  }
}


TEST(neon_2regmisc_sqneg) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Sqneg(v16.V8B(), v0.V8B());
  __ Sqneg(v17.V16B(), v0.V16B());
  __ Sqneg(v18.V4H(), v1.V4H());
  __ Sqneg(v19.V8H(), v1.V8H());
  __ Sqneg(v20.V2S(), v2.V2S());
  __ Sqneg(v21.V4S(), v2.V4S());
  __ Sqneg(v22.V2D(), v3.V2D());
  __ Sqneg(v23.V2D(), v4.V2D());

  __ Sqneg(b24, b0);
  __ Sqneg(h25, h1);
  __ Sqneg(s26, s2);
  __ Sqneg(d27, d3);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x7f7f0100ff817f7f, q16);
    ASSERT_EQUAL_128(0x81ff00017f7f81ff, 0x7f7f0100ff817f7f, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00010000ffff8001, q18);
    ASSERT_EQUAL_128(0x7fff7fff00010000, 0x00010000ffff8001, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000080000001, q20);
    ASSERT_EQUAL_128(0x7fffffff00000001, 0x0000000080000001, q21);
    ASSERT_EQUAL_128(0x7fffffffffffffff, 0x8000000000000001, q22);
    ASSERT_EQUAL_128(0x7fffffffffffffff, 0x0000000000000000, q23);

    ASSERT_EQUAL_128(0, 0x7f, q24);
    ASSERT_EQUAL_128(0, 0x8001, q25);
    ASSERT_EQUAL_128(0, 0x80000001, q26);
    ASSERT_EQUAL_128(0, 0x8000000000000001, q27);
  }
}


TEST(neon_2regmisc_abs) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Abs(v16.V8B(), v0.V8B());
  __ Abs(v17.V16B(), v0.V16B());
  __ Abs(v18.V4H(), v1.V4H());
  __ Abs(v19.V8H(), v1.V8H());
  __ Abs(v20.V2S(), v2.V2S());
  __ Abs(v21.V4S(), v2.V4S());
  __ Abs(d22, d3);
  __ Abs(v23.V2D(), v3.V2D());
  __ Abs(v24.V2D(), v4.V2D());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x807f0100017f807f, q16);
    ASSERT_EQUAL_128(0x7f0100017f807f01, 0x807f0100017f807f, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0001000000017fff, q18);
    ASSERT_EQUAL_128(0x80007fff00010000, 0x0001000000017fff, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000007fffffff, q20);
    ASSERT_EQUAL_128(0x8000000000000001, 0x000000007fffffff, q21);
    ASSERT_EQUAL_128(0x0000000000000000, 0x7fffffffffffffff, q22);
    ASSERT_EQUAL_128(0x7fffffffffffffff, 0x7fffffffffffffff, q23);
    ASSERT_EQUAL_128(0x8000000000000000, 0x0000000000000000, q24);
  }
}


TEST(neon_2regmisc_sqabs) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Sqabs(v16.V8B(), v0.V8B());
  __ Sqabs(v17.V16B(), v0.V16B());
  __ Sqabs(v18.V4H(), v1.V4H());
  __ Sqabs(v19.V8H(), v1.V8H());
  __ Sqabs(v20.V2S(), v2.V2S());
  __ Sqabs(v21.V4S(), v2.V4S());
  __ Sqabs(v22.V2D(), v3.V2D());
  __ Sqabs(v23.V2D(), v4.V2D());

  __ Sqabs(b24, b0);
  __ Sqabs(h25, h1);
  __ Sqabs(s26, s2);
  __ Sqabs(d27, d3);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x7f7f0100017f7f7f, q16);
    ASSERT_EQUAL_128(0x7f0100017f7f7f01, 0x7f7f0100017f7f7f, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0001000000017fff, q18);
    ASSERT_EQUAL_128(0x7fff7fff00010000, 0x0001000000017fff, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000007fffffff, q20);
    ASSERT_EQUAL_128(0x7fffffff00000001, 0x000000007fffffff, q21);
    ASSERT_EQUAL_128(0x7fffffffffffffff, 0x7fffffffffffffff, q22);
    ASSERT_EQUAL_128(0x7fffffffffffffff, 0x0000000000000000, q23);

    ASSERT_EQUAL_128(0, 0x7f, q24);
    ASSERT_EQUAL_128(0, 0x7fff, q25);
    ASSERT_EQUAL_128(0, 0x7fffffff, q26);
    ASSERT_EQUAL_128(0, 0x7fffffffffffffff, q27);
  }
}

TEST(neon_2regmisc_suqadd) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x017f8081ff00017f, 0x010080ff7f0180ff);

  __ Movi(v2.V2D(), 0x80008001ffff0000, 0xffff000000017ffd);
  __ Movi(v3.V2D(), 0xffff000080008001, 0x00017fffffff0001);

  __ Movi(v4.V2D(), 0x80000000fffffffe, 0xfffffff17ffffffe);
  __ Movi(v5.V2D(), 0xffffffff80000000, 0x7fffffff00000002);

  __ Movi(v6.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v7.V2D(), 0x8000000000000000, 0x8000000000000002);

  __ Mov(v16.V2D(), v0.V2D());
  __ Mov(v17.V2D(), v0.V2D());
  __ Mov(v18.V2D(), v2.V2D());
  __ Mov(v19.V2D(), v2.V2D());
  __ Mov(v20.V2D(), v4.V2D());
  __ Mov(v21.V2D(), v4.V2D());
  __ Mov(v22.V2D(), v6.V2D());

  __ Mov(v23.V2D(), v0.V2D());
  __ Mov(v24.V2D(), v2.V2D());
  __ Mov(v25.V2D(), v4.V2D());
  __ Mov(v26.V2D(), v6.V2D());

  __ Suqadd(v16.V8B(), v1.V8B());
  __ Suqadd(v17.V16B(), v1.V16B());
  __ Suqadd(v18.V4H(), v3.V4H());
  __ Suqadd(v19.V8H(), v3.V8H());
  __ Suqadd(v20.V2S(), v5.V2S());
  __ Suqadd(v21.V4S(), v5.V4S());
  __ Suqadd(v22.V2D(), v7.V2D());

  __ Suqadd(b23, b1);
  __ Suqadd(h24, h3);
  __ Suqadd(s25, s5);
  __ Suqadd(d26, d7);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x81817f7f7f7f007f, q16);
    ASSERT_EQUAL_128(0x7f7f7f7f7f807f7f, 0x81817f7f7f7f007f, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00007fff7fff7ffe, q18);
    ASSERT_EQUAL_128(0x7fff80017fff7fff, 0x00007fff7fff7ffe, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x7ffffff07fffffff, q20);
    ASSERT_EQUAL_128(0x7fffffff7ffffffe, 0x7ffffff07fffffff, q21);
    ASSERT_EQUAL_128(0x0000000000000001, 0x7fffffffffffffff, q22);

    ASSERT_EQUAL_128(0, 0x7f, q23);
    ASSERT_EQUAL_128(0, 0x7ffe, q24);
    ASSERT_EQUAL_128(0, 0x7fffffff, q25);
    ASSERT_EQUAL_128(0, 0x7fffffffffffffff, q26);
  }
}

TEST(neon_2regmisc_usqadd) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f7ffe);
  __ Movi(v1.V2D(), 0x017f8081ff00017f, 0x010080ff7f018002);

  __ Movi(v2.V2D(), 0x80008001fffe0000, 0xffff000000017ffd);
  __ Movi(v3.V2D(), 0xffff000000028001, 0x00017fffffff0001);

  __ Movi(v4.V2D(), 0x80000000fffffffe, 0x00000001fffffffe);
  __ Movi(v5.V2D(), 0xffffffff80000000, 0xfffffffe00000002);

  __ Movi(v6.V2D(), 0x8000000000000002, 0x7fffffffffffffff);
  __ Movi(v7.V2D(), 0x7fffffffffffffff, 0x8000000000000000);

  __ Mov(v16.V2D(), v0.V2D());
  __ Mov(v17.V2D(), v0.V2D());
  __ Mov(v18.V2D(), v2.V2D());
  __ Mov(v19.V2D(), v2.V2D());
  __ Mov(v20.V2D(), v4.V2D());
  __ Mov(v21.V2D(), v4.V2D());
  __ Mov(v22.V2D(), v6.V2D());

  __ Mov(v23.V2D(), v0.V2D());
  __ Mov(v24.V2D(), v2.V2D());
  __ Mov(v25.V2D(), v4.V2D());
  __ Mov(v26.V2D(), v6.V2D());

  __ Usqadd(v16.V8B(), v1.V8B());
  __ Usqadd(v17.V16B(), v1.V16B());
  __ Usqadd(v18.V4H(), v3.V4H());
  __ Usqadd(v19.V8H(), v3.V8H());
  __ Usqadd(v20.V2S(), v5.V2S());
  __ Usqadd(v21.V4S(), v5.V4S());
  __ Usqadd(v22.V2D(), v7.V2D());

  __ Usqadd(b23, b1);
  __ Usqadd(h24, h3);
  __ Usqadd(s25, s5);
  __ Usqadd(d26, d7);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x81817f00808000ff, q16);
    ASSERT_EQUAL_128(0x8080008080808080, 0x81817f00808000ff, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffff7fff00007ffe, q18);
    ASSERT_EQUAL_128(0x7fff8001ffff0000, 0xffff7fff00007ffe, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00000000ffffffff, q20);
    ASSERT_EQUAL_128(0x7fffffff7ffffffe, 0x00000000ffffffff, q21);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0x0000000000000000, q22);

    ASSERT_EQUAL_128(0, 0xff, q23);
    ASSERT_EQUAL_128(0, 0x7ffe, q24);
    ASSERT_EQUAL_128(0, 0xffffffff, q25);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q26);
  }
}

TEST(neon_2regmisc_xtn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Xtn(v16.V8B(), v0.V8H());
  __ Xtn2(v16.V16B(), v1.V8H());
  __ Xtn(v17.V4H(), v1.V4S());
  __ Xtn2(v17.V8H(), v2.V4S());
  __ Xtn(v18.V2S(), v3.V2D());
  __ Xtn2(v18.V4S(), v4.V2D());

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0001ff00ff0001ff, 0x01ff800181007f81, q16);
    ASSERT_EQUAL_128(0x0000ffff0000ffff, 0x8001000000007fff, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00000001ffffffff, q18);
  }
}


TEST(neon_2regmisc_sqxtn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f01007a81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Sqxtn(v16.V8B(), v0.V8H());
  __ Sqxtn2(v16.V16B(), v1.V8H());
  __ Sqxtn(v17.V4H(), v1.V4S());
  __ Sqxtn2(v17.V8H(), v2.V4S());
  __ Sqxtn(v18.V2S(), v3.V2D());
  __ Sqxtn2(v18.V4S(), v4.V2D());
  __ Sqxtn(b19, h0);
  __ Sqxtn(h20, s0);
  __ Sqxtn(s21, d0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x8080ff00ff00017f, 0x7f7a807f80807f80, q16);
    ASSERT_EQUAL_128(0x8000ffff00007fff, 0x8000800080007fff, q17);
    ASSERT_EQUAL_128(0x8000000000000000, 0x800000007fffffff, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000080, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000007fff, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000080000000, q21);
  }
}


TEST(neon_2regmisc_uqxtn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f01007a81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Uqxtn(v16.V8B(), v0.V8H());
  __ Uqxtn2(v16.V16B(), v1.V8H());
  __ Uqxtn(v17.V4H(), v1.V4S());
  __ Uqxtn2(v17.V8H(), v2.V4S());
  __ Uqxtn(v18.V2S(), v3.V2D());
  __ Uqxtn2(v18.V4S(), v4.V2D());
  __ Uqxtn(b19, h0);
  __ Uqxtn(h20, s0);
  __ Uqxtn(s21, d0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0xffffff00ff0001ff, 0xff7affffffffffff, q16);
    ASSERT_EQUAL_128(0xffffffff0000ffff, 0xffffffffffffffff, q17);
    ASSERT_EQUAL_128(0xffffffff00000000, 0xffffffffffffffff, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00000000000000ff, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000000000ffff, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00000000ffffffff, q21);
  }
}


TEST(neon_2regmisc_sqxtun) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f01007a81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Sqxtun(v16.V8B(), v0.V8H());
  __ Sqxtun2(v16.V16B(), v1.V8H());
  __ Sqxtun(v17.V4H(), v1.V4S());
  __ Sqxtun2(v17.V8H(), v2.V4S());
  __ Sqxtun(v18.V2S(), v3.V2D());
  __ Sqxtun2(v18.V4S(), v4.V2D());
  __ Sqxtun(b19, h0);
  __ Sqxtun(h20, s0);
  __ Sqxtun(s21, d0);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x00000000000001ff, 0xff7a00ff0000ff00, q16);
    ASSERT_EQUAL_128(0x000000000000ffff, 0x000000000000ffff, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00000000ffffffff, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000000000ffff, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q21);
  }
}

TEST(neon_3same_and) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x00aa55aaff55ff00, 0xaa55ff00555500ff);

  __ And(v16.V16B(), v0.V16B(), v0.V16B());  // self test
  __ And(v17.V16B(), v0.V16B(), v1.V16B());  // all combinations
  __ And(v24.V8B(), v0.V8B(), v0.V8B());     // self test
  __ And(v25.V8B(), v0.V8B(), v1.V8B());     // all combinations
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0xff00aa5500ff55aa, 0xff00aa5500ff55aa, q16);
    ASSERT_EQUAL_128(0x0000000000555500, 0xaa00aa00005500aa, q17);
    ASSERT_EQUAL_128(0, 0xff00aa5500ff55aa, q24);
    ASSERT_EQUAL_128(0, 0xaa00aa00005500aa, q25);
  }
}

TEST(neon_3same_bic) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x00ffaa00aa55aaff, 0xffff005500ff00ff);

  __ Bic(v16.V16B(), v0.V16B(), v0.V16B());  // self test
  __ Bic(v17.V16B(), v0.V16B(), v1.V16B());  // all combinations
  __ Bic(v24.V8B(), v0.V8B(), v0.V8B());     // self test
  __ Bic(v25.V8B(), v0.V8B(), v1.V8B());     // all combinations
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q16);
    ASSERT_EQUAL_128(0xff00005500aa5500, 0x0000aa0000005500, q17);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q24);
    ASSERT_EQUAL_128(0, 0x0000aa0000005500, q25);
  }
}

TEST(neon_3same_orr) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x00aa55aaff55ff00, 0xaa55ff00555500ff);

  __ Orr(v16.V16B(), v0.V16B(), v0.V16B());  // self test
  __ Orr(v17.V16B(), v0.V16B(), v1.V16B());  // all combinations
  __ Orr(v24.V8B(), v0.V8B(), v0.V8B());     // self test
  __ Orr(v25.V8B(), v0.V8B(), v1.V8B());     // all combinations
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0xff00aa5500ff55aa, 0xff00aa5500ff55aa, q16);
    ASSERT_EQUAL_128(0xffaaffffffffffaa, 0xff55ff5555ff55ff, q17);
    ASSERT_EQUAL_128(0, 0xff00aa5500ff55aa, q24);
    ASSERT_EQUAL_128(0, 0xff55ff5555ff55ff, q25);
  }
}

TEST(neon_3same_mov) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);

  __ Mov(v16.V16B(), v0.V16B());
  __ Mov(v17.V8H(), v0.V8H());
  __ Mov(v18.V4S(), v0.V4S());
  __ Mov(v19.V2D(), v0.V2D());

  __ Mov(v24.V8B(), v0.V8B());
  __ Mov(v25.V4H(), v0.V4H());
  __ Mov(v26.V2S(), v0.V2S());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xff00aa5500ff55aa, 0xff00aa5500ff55aa, q16);
    ASSERT_EQUAL_128(0xff00aa5500ff55aa, 0xff00aa5500ff55aa, q17);
    ASSERT_EQUAL_128(0xff00aa5500ff55aa, 0xff00aa5500ff55aa, q18);
    ASSERT_EQUAL_128(0xff00aa5500ff55aa, 0xff00aa5500ff55aa, q19);

    ASSERT_EQUAL_128(0x0, 0xff00aa5500ff55aa, q24);
    ASSERT_EQUAL_128(0x0, 0xff00aa5500ff55aa, q25);
    ASSERT_EQUAL_128(0x0, 0xff00aa5500ff55aa, q26);
  }
}

TEST(neon_3same_orn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x00aa55aaff55ff00, 0xaa55ff00555500ff);

  __ Orn(v16.V16B(), v0.V16B(), v0.V16B());  // self test
  __ Orn(v17.V16B(), v0.V16B(), v1.V16B());  // all combinations
  __ Orn(v24.V8B(), v0.V8B(), v0.V8B());     // self test
  __ Orn(v25.V8B(), v0.V8B(), v1.V8B());     // all combinations
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q16);
    ASSERT_EQUAL_128(0xff55aa5500ff55ff, 0xffaaaaffaaffffaa, q17);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q24);
    ASSERT_EQUAL_128(0, 0xffaaaaffaaffffaa, q25);
  }
}

TEST(neon_3same_eor) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xff00aa5500ff55aa, 0xff00aa5500ff55aa);
  __ Movi(v1.V2D(), 0x00ffaa00aa55aaff, 0xffff005500ff00ff);

  __ Eor(v16.V16B(), v0.V16B(), v0.V16B());  // self test
  __ Eor(v17.V16B(), v0.V16B(), v1.V16B());  // all combinations
  __ Eor(v24.V8B(), v0.V8B(), v0.V8B());     // self test
  __ Eor(v25.V8B(), v0.V8B(), v1.V8B());     // all combinations
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q16);
    ASSERT_EQUAL_128(0xffff0055aaaaff55, 0x00ffaa0000005555, q17);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q24);
    ASSERT_EQUAL_128(0, 0x00ffaa0000005555, q25);
  }
}

TEST(neon_3same_bif) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v16.V2D(), 0xffff0000ff00ffff, 0xffff00000000aaaa);
  __ Movi(v0.V2D(), 0xff00ff00ff005555, 0xaaaa5555aaaaaaaa);
  __ Movi(v1.V2D(), 0x00ff00ffff0055aa, 0x55aa55aa55aa55aa);

  __ Movi(v17.V2D(), 0x5555aa55cccccccc, 0x33333333f0f0f0f0);
  __ Movi(v2.V2D(), 0x555555aaff00ff00, 0xff00ff00ff00ff00);
  __ Movi(v3.V2D(), 0xaa55aa5500ffff00, 0x00ffff0000ffff00);

  __ Movi(v18.V2D(), 0, 0xf0f0f0f00f0f0f0f);
  __ Movi(v4.V2D(), 0, 0xf0f0f0f0f0f0f0f0);
  __ Movi(v5.V2D(), 0, 0x00ffff0000ffff00);

  __ Bif(v16.V16B(), v0.V16B(), v1.V16B());
  __ Bif(v17.V16B(), v2.V16B(), v3.V16B());
  __ Bif(v18.V8B(), v4.V8B(), v5.V8B());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xffffff00ff0055ff, 0xffaa0055aa00aaaa, q16);
    ASSERT_EQUAL_128(0x5555ffffffcccc00, 0xff333300fff0f000, q17);
    ASSERT_EQUAL_128(0, 0xf0f0f0f0f00f0ff0, q18);
  }
}

TEST(neon_3same_bit) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v16.V2D(), 0xffff0000ff00ffff, 0xffff00000000aaaa);
  __ Movi(v0.V2D(), 0xff00ff00ff005555, 0xaaaa5555aaaaaaaa);
  __ Movi(v1.V2D(), 0x00ff00ffff0055aa, 0x55aa55aa55aa55aa);

  __ Movi(v17.V2D(), 0x5555aa55cccccccc, 0x33333333f0f0f0f0);
  __ Movi(v2.V2D(), 0x555555aaff00ff00, 0xff00ff00ff00ff00);
  __ Movi(v3.V2D(), 0xaa55aa5500ffff00, 0x00ffff0000ffff00);

  __ Movi(v18.V2D(), 0, 0xf0f0f0f00f0f0f0f);
  __ Movi(v4.V2D(), 0, 0xf0f0f0f0f0f0f0f0);
  __ Movi(v5.V2D(), 0, 0x00ffff0000ffff00);

  __ Bit(v16.V16B(), v0.V16B(), v1.V16B());
  __ Bit(v17.V16B(), v2.V16B(), v3.V16B());
  __ Bit(v18.V8B(), v4.V8B(), v5.V8B());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xff000000ff00ff55, 0xaaff550000aaaaaa, q16);
    ASSERT_EQUAL_128(0x55550000cc00ffcc, 0x3300ff33f000fff0, q17);
    ASSERT_EQUAL_128(0, 0xf0f0f0f00ff0f00f, q18);
  }
}

TEST(neon_3same_bsl) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v16.V2D(), 0xffff0000ff00ffff, 0xffff00000000aaaa);
  __ Movi(v0.V2D(), 0xff00ff00ff005555, 0xaaaa5555aaaaaaaa);
  __ Movi(v1.V2D(), 0x00ff00ffff0055aa, 0x55aa55aa55aa55aa);

  __ Movi(v17.V2D(), 0x5555aa55cccccccc, 0x33333333f0f0f0f0);
  __ Movi(v2.V2D(), 0x555555aaff00ff00, 0xff00ff00ff00ff00);
  __ Movi(v3.V2D(), 0xaa55aa5500ffff00, 0x00ffff0000ffff00);

  __ Movi(v18.V2D(), 0, 0xf0f0f0f00f0f0f0f);
  __ Movi(v4.V2D(), 0, 0xf0f0f0f0f0f0f0f0);
  __ Movi(v5.V2D(), 0, 0x00ffff0000ffff00);

  __ Bsl(v16.V16B(), v0.V16B(), v1.V16B());
  __ Bsl(v17.V16B(), v2.V16B(), v3.V16B());
  __ Bsl(v18.V8B(), v4.V8B(), v5.V8B());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xff0000ffff005555, 0xaaaa55aa55aaffaa, q16);
    ASSERT_EQUAL_128(0xff550000cc33ff00, 0x33ccff00f00fff00, q17);
    ASSERT_EQUAL_128(0, 0xf0fffff000f0f000, q18);
  }
}


TEST(neon_3same_smax) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaa55555555, 0xffffffff0000aa55);
  __ Movi(v1.V2D(), 0x55aa5555aaaaaaaa, 0x00000000ffaa55ff);

  __ Smax(v16.V8B(), v0.V8B(), v1.V8B());
  __ Smax(v18.V4H(), v0.V4H(), v1.V4H());
  __ Smax(v20.V2S(), v0.V2S(), v1.V2S());

  __ Smax(v17.V16B(), v0.V16B(), v1.V16B());
  __ Smax(v19.V8H(), v0.V8H(), v1.V8H());
  __ Smax(v21.V4S(), v0.V4S(), v1.V4S());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0x0000000000005555, q16);
    ASSERT_EQUAL_128(0x0, 0x00000000000055ff, q18);
    ASSERT_EQUAL_128(0x0, 0x000000000000aa55, q20);
    ASSERT_EQUAL_128(0x55aa555555555555, 0x0000000000005555, q17);
    ASSERT_EQUAL_128(0x55aa555555555555, 0x00000000000055ff, q19);
    ASSERT_EQUAL_128(0x55aa555555555555, 0x000000000000aa55, q21);
  }
}


TEST(neon_3same_smaxp) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaa55555555, 0xffffffff0000aa55);
  __ Movi(v1.V2D(), 0x55aa5555aaaaaaaa, 0x00000000ffaa55ff);

  __ Smaxp(v16.V8B(), v0.V8B(), v1.V8B());
  __ Smaxp(v18.V4H(), v0.V4H(), v1.V4H());
  __ Smaxp(v20.V2S(), v0.V2S(), v1.V2S());

  __ Smaxp(v17.V16B(), v0.V16B(), v1.V16B());
  __ Smaxp(v19.V8H(), v0.V8H(), v1.V8H());
  __ Smaxp(v21.V4S(), v0.V4S(), v1.V4S());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0x0000ff55ffff0055, q16);
    ASSERT_EQUAL_128(0x0, 0x000055ffffff0000, q18);
    ASSERT_EQUAL_128(0x0, 0x000000000000aa55, q20);
    ASSERT_EQUAL_128(0x5555aaaa0000ff55, 0xaaaa5555ffff0055, q17);
    ASSERT_EQUAL_128(0x55aaaaaa000055ff, 0xaaaa5555ffff0000, q19);
    ASSERT_EQUAL_128(0x55aa555500000000, 0x555555550000aa55, q21);
  }
}


TEST(neon_addp_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344aafe80, 0x00112233aabbfc00);
  __ Movi(v1.V2D(), 0x55aa5555aaaaaaaa, 0x00000000ffaa55ff);
  __ Movi(v2.V2D(), 0xaaaaaaaa66555555, 0xffffffff0000aa00);

  __ Addp(d16, v0.V2D());
  __ Addp(d17, v1.V2D());
  __ Addp(d18, v2.V2D());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0x00224466ef66fa80, q16);
    ASSERT_EQUAL_128(0x0, 0x55aa5556aa5500a9, q17);
    ASSERT_EQUAL_128(0x0, 0xaaaaaaa96655ff55, q18);
  }
}

TEST(neon_acrosslanes_addv) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344aafe80, 0x00112233aabbfc00);
  __ Movi(v1.V2D(), 0x55aa5555aaaaaaaa, 0x00000000ffaa55ff);
  __ Movi(v2.V2D(), 0xaaaaaaaa66555555, 0xffffffff0000aa00);

  __ Addv(b16, v0.V8B());
  __ Addv(b17, v0.V16B());
  __ Addv(h18, v1.V4H());
  __ Addv(h19, v1.V8H());
  __ Addv(s20, v2.V4S());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0xc7, q16);
    ASSERT_EQUAL_128(0x0, 0x99, q17);
    ASSERT_EQUAL_128(0x0, 0x55a9, q18);
    ASSERT_EQUAL_128(0x0, 0x55fc, q19);
    ASSERT_EQUAL_128(0x0, 0x1100a9fe, q20);
  }
}


TEST(neon_acrosslanes_saddlv) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344aafe80, 0x00112233aabbfc00);
  __ Movi(v1.V2D(), 0x55aa5555aaaaaaaa, 0x00000000ffaa55ff);
  __ Movi(v2.V2D(), 0xaaaaaaaa66555555, 0xffffffff0000aa00);

  __ Saddlv(h16, v0.V8B());
  __ Saddlv(h17, v0.V16B());
  __ Saddlv(s18, v1.V4H());
  __ Saddlv(s19, v1.V8H());
  __ Saddlv(d20, v2.V4S());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0xffc7, q16);
    ASSERT_EQUAL_128(0x0, 0xff99, q17);
    ASSERT_EQUAL_128(0x0, 0x000055a9, q18);
    ASSERT_EQUAL_128(0x0, 0x000055fc, q19);
    ASSERT_EQUAL_128(0x0, 0x0000001100a9fe, q20);
  }
}


TEST(neon_acrosslanes_uaddlv) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344aafe80, 0x00112233aabbfc00);
  __ Movi(v1.V2D(), 0x55aa5555aaaaaaaa, 0x00000000ffaa55ff);
  __ Movi(v2.V2D(), 0xaaaaaaaa66555555, 0xffffffff0000aa00);

  __ Uaddlv(h16, v0.V8B());
  __ Uaddlv(h17, v0.V16B());
  __ Uaddlv(s18, v1.V4H());
  __ Uaddlv(s19, v1.V8H());
  __ Uaddlv(d20, v2.V4S());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0x02c7, q16);
    ASSERT_EQUAL_128(0x0, 0x0599, q17);
    ASSERT_EQUAL_128(0x0, 0x000155a9, q18);
    ASSERT_EQUAL_128(0x0, 0x000355fc, q19);
    ASSERT_EQUAL_128(0x0, 0x000000021100a9fe, q20);
  }
}


TEST(neon_acrosslanes_smaxv) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344aafe80, 0x00112233aabbfc00);
  __ Movi(v1.V2D(), 0x55aa5555aaaaaaaa, 0x00000000ffaa55ff);
  __ Movi(v2.V2D(), 0xaaaaaaaa66555555, 0xffffffff0000aa00);

  __ Smaxv(b16, v0.V8B());
  __ Smaxv(b17, v0.V16B());
  __ Smaxv(h18, v1.V4H());
  __ Smaxv(h19, v1.V8H());
  __ Smaxv(s20, v2.V4S());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0x33, q16);
    ASSERT_EQUAL_128(0x0, 0x44, q17);
    ASSERT_EQUAL_128(0x0, 0x55ff, q18);
    ASSERT_EQUAL_128(0x0, 0x55ff, q19);
    ASSERT_EQUAL_128(0x0, 0x66555555, q20);
  }
}


TEST(neon_acrosslanes_sminv) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344aafe80, 0x00112233aabbfc00);
  __ Movi(v1.V2D(), 0xfffa5555aaaaaaaa, 0x00000000ffaa55ff);
  __ Movi(v2.V2D(), 0xaaaaaaaa66555555, 0xffffffff0000aa00);

  __ Sminv(b16, v0.V8B());
  __ Sminv(b17, v0.V16B());
  __ Sminv(h18, v1.V4H());
  __ Sminv(h19, v1.V8H());
  __ Sminv(s20, v2.V4S());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0xaa, q16);
    ASSERT_EQUAL_128(0x0, 0x80, q17);
    ASSERT_EQUAL_128(0x0, 0xffaa, q18);
    ASSERT_EQUAL_128(0x0, 0xaaaa, q19);
    ASSERT_EQUAL_128(0x0, 0xaaaaaaaa, q20);
  }
}

TEST(neon_acrosslanes_umaxv) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344aafe80, 0x00112233aabbfc00);
  __ Movi(v1.V2D(), 0x55aa5555aaaaffab, 0x00000000ffaa55ff);
  __ Movi(v2.V2D(), 0xaaaaaaaa66555555, 0xffffffff0000aa00);

  __ Umaxv(b16, v0.V8B());
  __ Umaxv(b17, v0.V16B());
  __ Umaxv(h18, v1.V4H());
  __ Umaxv(h19, v1.V8H());
  __ Umaxv(s20, v2.V4S());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0xfc, q16);
    ASSERT_EQUAL_128(0x0, 0xfe, q17);
    ASSERT_EQUAL_128(0x0, 0xffaa, q18);
    ASSERT_EQUAL_128(0x0, 0xffab, q19);
    ASSERT_EQUAL_128(0x0, 0xffffffff, q20);
  }
}


TEST(neon_acrosslanes_uminv) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344aafe80, 0x02112233aabbfc01);
  __ Movi(v1.V2D(), 0xfffa5555aaaa0000, 0x00010003ffaa55ff);
  __ Movi(v2.V2D(), 0xaaaaaaaa66555555, 0xffffffff0000aa00);

  __ Uminv(b16, v0.V8B());
  __ Uminv(b17, v0.V16B());
  __ Uminv(h18, v1.V4H());
  __ Uminv(h19, v1.V8H());
  __ Uminv(s20, v2.V4S());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0x01, q16);
    ASSERT_EQUAL_128(0x0, 0x00, q17);
    ASSERT_EQUAL_128(0x0, 0x0001, q18);
    ASSERT_EQUAL_128(0x0, 0x0000, q19);
    ASSERT_EQUAL_128(0x0, 0x0000aa00, q20);
  }
}


TEST(neon_3same_smin) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaa55555555, 0xffffffff0000aa55);
  __ Movi(v1.V2D(), 0x55aa5555aaaaaaaa, 0x00000000ffaa55ff);

  __ Smin(v16.V8B(), v0.V8B(), v1.V8B());
  __ Smin(v18.V4H(), v0.V4H(), v1.V4H());
  __ Smin(v20.V2S(), v0.V2S(), v1.V2S());

  __ Smin(v17.V16B(), v0.V16B(), v1.V16B());
  __ Smin(v19.V8H(), v0.V8H(), v1.V8H());
  __ Smin(v21.V4S(), v0.V4S(), v1.V4S());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0xffffffffffaaaaff, q16);
    ASSERT_EQUAL_128(0x0, 0xffffffffffaaaa55, q18);
    ASSERT_EQUAL_128(0x0, 0xffffffffffaa55ff, q20);
    ASSERT_EQUAL_128(0xaaaaaaaaaaaaaaaa, 0xffffffffffaaaaff, q17);
    ASSERT_EQUAL_128(0xaaaaaaaaaaaaaaaa, 0xffffffffffaaaa55, q19);
    ASSERT_EQUAL_128(0xaaaaaaaaaaaaaaaa, 0xffffffffffaa55ff, q21);
  }
}


TEST(neon_3same_umax) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaa55555555, 0xffffffff0000aa55);
  __ Movi(v1.V2D(), 0x55aa5555aaaaaaaa, 0x00000000ffaa55ff);

  __ Umax(v16.V8B(), v0.V8B(), v1.V8B());
  __ Umax(v18.V4H(), v0.V4H(), v1.V4H());
  __ Umax(v20.V2S(), v0.V2S(), v1.V2S());

  __ Umax(v17.V16B(), v0.V16B(), v1.V16B());
  __ Umax(v19.V8H(), v0.V8H(), v1.V8H());
  __ Umax(v21.V4S(), v0.V4S(), v1.V4S());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0xffffffffffaaaaff, q16);
    ASSERT_EQUAL_128(0x0, 0xffffffffffaaaa55, q18);
    ASSERT_EQUAL_128(0x0, 0xffffffffffaa55ff, q20);
    ASSERT_EQUAL_128(0xaaaaaaaaaaaaaaaa, 0xffffffffffaaaaff, q17);
    ASSERT_EQUAL_128(0xaaaaaaaaaaaaaaaa, 0xffffffffffaaaa55, q19);
    ASSERT_EQUAL_128(0xaaaaaaaaaaaaaaaa, 0xffffffffffaa55ff, q21);
  }
}


TEST(neon_3same_umin) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaa55555555, 0xffffffff0000aa55);
  __ Movi(v1.V2D(), 0x55aa5555aaaaaaaa, 0x00000000ffaa55ff);

  __ Umin(v16.V8B(), v0.V8B(), v1.V8B());
  __ Umin(v18.V4H(), v0.V4H(), v1.V4H());
  __ Umin(v20.V2S(), v0.V2S(), v1.V2S());

  __ Umin(v17.V16B(), v0.V16B(), v1.V16B());
  __ Umin(v19.V8H(), v0.V8H(), v1.V8H());
  __ Umin(v21.V4S(), v0.V4S(), v1.V4S());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0x0000000000005555, q16);
    ASSERT_EQUAL_128(0x0, 0x00000000000055ff, q18);
    ASSERT_EQUAL_128(0x0, 0x000000000000aa55, q20);
    ASSERT_EQUAL_128(0x55aa555555555555, 0x0000000000005555, q17);
    ASSERT_EQUAL_128(0x55aa555555555555, 0x00000000000055ff, q19);
    ASSERT_EQUAL_128(0x55aa555555555555, 0x000000000000aa55, q21);
  }
}


TEST(neon_3same_extra_fcadd) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP, CPUFeatures::kFcma);

  START();

  // (0i, 5) (d)
  __ Movi(v0.V2D(), 0x0, 0x4014000000000000);
  // (5i, 0) (d)
  __ Movi(v1.V2D(), 0x4014000000000000, 0x0);
  // (10i, 10) (d)
  __ Movi(v2.V2D(), 0x4024000000000000, 0x4024000000000000);
  // (5i, 5), (5i, 5) (f)
  __ Movi(v3.V2D(), 0x40A0000040A00000, 0x40A0000040A00000);
  // (5i, 5), (0i, 0) (f)
  __ Movi(v4.V2D(), 0x40A0000040A00000, 0x0);
  // 324567i, 16000 (f)
  __ Movi(v5.V2D(), 0x0, 0x489E7AE0467A0000);

  // Subtraction (10, 10) - (5, 5) == (5, 5)
  __ Fcadd(v31.V2D(), v2.V2D(), v1.V2D(), 90);
  __ Fcadd(v31.V2D(), v31.V2D(), v0.V2D(), 270);

  // Addition (10, 10) + (5, 5) == (15, 15)
  __ Fcadd(v30.V2D(), v2.V2D(), v1.V2D(), 270);
  __ Fcadd(v30.V2D(), v30.V2D(), v0.V2D(), 90);

  // 2S
  __ Fcadd(v29.V2S(), v4.V2S(), v5.V2S(), 90);
  __ Fcadd(v28.V2S(), v4.V2S(), v5.V2S(), 270);

  // 4S
  __ Fcadd(v27.V4S(), v3.V4S(), v4.V4S(), 90);
  __ Fcadd(v26.V4S(), v3.V4S(), v4.V4S(), 270);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x4014000000000000, 0x4014000000000000, q31);
    ASSERT_EQUAL_128(0x402E000000000000, 0x402E000000000000, q30);
    ASSERT_EQUAL_128(0x0, 0x467a0000c89e7ae0, q29);  // (16000i, -324567)
    ASSERT_EQUAL_128(0x0, 0xc67a0000489e7ae0, q28);  // (-16000i, 324567)
    ASSERT_EQUAL_128(0x4120000000000000, 0x40A0000040A00000, q27);
    ASSERT_EQUAL_128(0x0000000041200000, 0x40A0000040A00000, q26);
  }
}


TEST(neon_3same_extra_fcmla) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP, CPUFeatures::kFcma);

  START();

  __ Movi(v1.V2D(), 0x0, 0x40A0000040400000);  // (5i, 3) (f)
  __ Movi(v2.V2D(), 0x0, 0x4040000040A00000);  // (3i, 5) (f)

  __ Movi(v3.V2D(), 0x0, 0x4000000040400000);  // (2i, 3) (f)
  __ Movi(v4.V2D(), 0x0, 0x40E000003F800000);  // (7i, 1) (f)

  __ Movi(v5.V2D(), 0x0, 0x4000000040400000);  // (2i, 3) (f)
  __ Movi(v6.V2D(), 0x0, 0x408000003F800000);  // (4i, 1) (f)

  // (1.5i, 2.5), (31.5i, 1024) (f)
  __ Movi(v7.V2D(), 0x3FC0000040200000, 0x41FC000044800000);
  // (2048i, 412.75), (3645i, 0) (f)
  __ Movi(v8.V2D(), 0x4500000043CE6000, 0x4563D00000000000);
  // (2000i, 450,000) (d)
  __ Movi(v9.V2D(), 0x409F400000000000, 0x411B774000000000);
  // (30,000i, 1250) (d)
  __ Movi(v10.V2D(), 0x40DD4C0000000000, 0x4093880000000000);

  // DST
  __ Movi(v24.V2D(), 0x0, 0x0);
  __ Movi(v25.V2D(), 0x0, 0x0);
  __ Movi(v26.V2D(), 0x0, 0x0);
  __ Movi(v27.V2D(), 0x0, 0x0);
  __ Movi(v28.V2D(), 0x0, 0x0);
  __ Movi(v29.V2D(), 0x0, 0x0);
  __ Movi(v30.V2D(), 0x0, 0x0);
  __ Movi(v31.V2D(), 0x0, 0x0);

  // Full calculations
  __ Fcmla(v31.V2S(), v1.V2S(), v2.V2S(), 90);
  __ Fcmla(v31.V2S(), v1.V2S(), v2.V2S(), 0);

  __ Fcmla(v30.V2S(), v3.V2S(), v4.V2S(), 0);
  __ Fcmla(v30.V2S(), v3.V2S(), v4.V2S(), 90);

  __ Fcmla(v29.V2S(), v5.V2S(), v6.V2S(), 90);
  __ Fcmla(v29.V2S(), v5.V2S(), v6.V2S(), 0);

  __ Fcmla(v28.V2D(), v9.V2D(), v10.V2D(), 0);
  __ Fcmla(v28.V2D(), v9.V2D(), v10.V2D(), 90);

  // Partial checks
  __ Fcmla(v27.V2S(), v1.V2S(), v2.V2S(), 0);
  __ Fcmla(v26.V2S(), v2.V2S(), v1.V2S(), 0);

  __ Fcmla(v25.V4S(), v7.V4S(), v8.V4S(), 270);
  __ Fcmla(v24.V4S(), v7.V4S(), v8.V4S(), 180);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0x4208000000000000, q31);  // (34i, 0)
    ASSERT_EQUAL_128(0x0, 0x41B80000C1300000, q30);  // (23i, -11)
    ASSERT_EQUAL_128(0x0, 0x41600000C0A00000, q29);  // (14i, -5)

    // (13502500000i, 502500000)
    ASSERT_EQUAL_128(0x4209267E65000000, 0x41BDF38AA0000000, q28);
    ASSERT_EQUAL_128(0x0, 0x4110000041700000, q27);  //  (9i, 15)
    ASSERT_EQUAL_128(0x0, 0x41C8000041700000, q26);  // (25i, 15)
    // (512i, 1.031875E3), (373248i, 0)
    ASSERT_EQUAL_128(0xc41ac80045400000, 0x0000000047e040c0, q25);
    // (619.125i, -3072), (0i, -114817.5)
    ASSERT_EQUAL_128(0xc5a00000c480fc00, 0xca63d00000000000, q24);
  }
}


TEST(neon_byelement_fcmla) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP, CPUFeatures::kFcma);

  START();

  // (5i, 3), (5i, 3) (f)
  __ Movi(v1.V2D(), 0x40A0000040400000, 0x40A0000040400000);
  // (3i, 5), (3i, 5) (f)
  __ Movi(v2.V2D(), 0x4040000040A00000, 0x4040000040A00000);
  // (7i, 1), (5i, 3) (f)
  __ Movi(v3.V2D(), 0x40E000003F800000, 0x40A0000040400000);
  // (4i, 1), (3i, 5) (f)
  __ Movi(v4.V2D(), 0x408000003F800000, 0x4040000040A00000);
  // (4i, 1), (7i, 1) (f)
  __ Movi(v5.V2D(), 0x408000003F800000, 0x40E000003F800000);
  // (2i, 3), (0, 0) (f)
  __ Movi(v6.V2D(), 0x4000000040400000, 0x0);

  // DST
  __ Movi(v22.V2D(), 0x0, 0x0);
  __ Movi(v23.V2D(), 0x0, 0x0);
  __ Movi(v24.V2D(), 0x0, 0x0);
  __ Movi(v25.V2D(), 0x0, 0x0);
  __ Movi(v26.V2D(), 0x0, 0x0);
  __ Movi(v27.V2D(), 0x0, 0x0);
  __ Movi(v28.V2D(), 0x0, 0x0);
  __ Movi(v29.V2D(), 0x0, 0x0);
  __ Movi(v30.V2D(), 0x0, 0x0);
  __ Movi(v31.V2D(), 0x0, 0x0);

  // Full calculation (pairs)
  __ Fcmla(v31.V4S(), v1.V4S(), v2.S(), 0, 90);
  __ Fcmla(v31.V4S(), v1.V4S(), v2.S(), 0, 0);
  __ Fcmla(v30.V4S(), v5.V4S(), v6.S(), 1, 90);
  __ Fcmla(v30.V4S(), v5.V4S(), v6.S(), 1, 0);

  // Rotations
  __ Fcmla(v29.V4S(), v3.V4S(), v4.S(), 1, 0);
  __ Fcmla(v28.V4S(), v3.V4S(), v4.S(), 1, 90);
  __ Fcmla(v27.V4S(), v3.V4S(), v4.S(), 1, 180);
  __ Fcmla(v26.V4S(), v3.V4S(), v4.S(), 1, 270);
  __ Fcmla(v25.V4S(), v3.V4S(), v4.S(), 0, 270);
  __ Fcmla(v24.V4S(), v3.V4S(), v4.S(), 0, 180);
  __ Fcmla(v23.V4S(), v3.V4S(), v4.S(), 0, 90);
  __ Fcmla(v22.V4S(), v3.V4S(), v4.S(), 0, 0);

  END();

  if (CAN_RUN()) {
    RUN();
    // (34i, 0), (34i, 0)
    ASSERT_EQUAL_128(0x4208000000000000, 0x4208000000000000, q31);
    // (14i, -5), (23i, -11)
    ASSERT_EQUAL_128(0x41600000C0A00000, 0x41B80000C1300000, q30);
    // (4i, 1), (12i, 3)
    ASSERT_EQUAL_128(0x408000003f800000, 0x4140000040400000, q29);
    // (7i, -28), (5i, -20)
    ASSERT_EQUAL_128(0x40e00000c1e00000, 0x40a00000c1a00000, q28);
    // (-4i, -1), (-12i, -3)
    ASSERT_EQUAL_128(0xc0800000bf800000, 0xc1400000c0400000, q27);
    // (-7i, 28), (-5i, 20)
    ASSERT_EQUAL_128(0xc0e0000041e00000, 0xc0a0000041a00000, q26);
    // (-35i, 21), (-25i, 15)
    ASSERT_EQUAL_128(0xc20c000041a80000, 0xc1c8000041700000, q25);
    // (-3i, -5), (-9i, -15)
    ASSERT_EQUAL_128(0xc0400000c0a00000, 0xc1100000c1700000, q24);
    // (35i, -21), (25i, -15)
    ASSERT_EQUAL_128(0x420c0000c1a80000, 0x41c80000c1700000, q23);
    // (3i, 5), (9i, 15)
    ASSERT_EQUAL_128(0x4040000040a00000, 0x4110000041700000, q22);
  }
}


TEST(neon_2regmisc_mvn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x00ff00ffff0055aa, 0x55aa55aa55aa55aa);

  __ Mvn(v16.V16B(), v0.V16B());
  __ Mvn(v17.V8H(), v0.V8H());
  __ Mvn(v18.V4S(), v0.V4S());
  __ Mvn(v19.V2D(), v0.V2D());

  __ Mvn(v24.V8B(), v0.V8B());
  __ Mvn(v25.V4H(), v0.V4H());
  __ Mvn(v26.V2S(), v0.V2S());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xff00ff0000ffaa55, 0xaa55aa55aa55aa55, q16);
    ASSERT_EQUAL_128(0xff00ff0000ffaa55, 0xaa55aa55aa55aa55, q17);
    ASSERT_EQUAL_128(0xff00ff0000ffaa55, 0xaa55aa55aa55aa55, q18);
    ASSERT_EQUAL_128(0xff00ff0000ffaa55, 0xaa55aa55aa55aa55, q19);

    ASSERT_EQUAL_128(0x0, 0xaa55aa55aa55aa55, q24);
    ASSERT_EQUAL_128(0x0, 0xaa55aa55aa55aa55, q25);
    ASSERT_EQUAL_128(0x0, 0xaa55aa55aa55aa55, q26);
  }
}


TEST(neon_2regmisc_not) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x00ff00ffff0055aa, 0x55aa55aa55aa55aa);
  __ Movi(v1.V2D(), 0, 0x00ffff0000ffff00);

  __ Not(v16.V16B(), v0.V16B());
  __ Not(v17.V8B(), v1.V8B());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xff00ff0000ffaa55, 0xaa55aa55aa55aa55, q16);
    ASSERT_EQUAL_128(0x0, 0xff0000ffff0000ff, q17);
  }
}


TEST(neon_2regmisc_cls_clz_cnt) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0001020304050607, 0x08090a0b0c0d0e0f);
  __ Movi(v1.V2D(), 0xfedcba9876543210, 0x0123456789abcdef);

  __ Cls(v16.V8B(), v1.V8B());
  __ Cls(v17.V16B(), v1.V16B());
  __ Cls(v18.V4H(), v1.V4H());
  __ Cls(v19.V8H(), v1.V8H());
  __ Cls(v20.V2S(), v1.V2S());
  __ Cls(v21.V4S(), v1.V4S());

  __ Clz(v22.V8B(), v0.V8B());
  __ Clz(v23.V16B(), v0.V16B());
  __ Clz(v24.V4H(), v0.V4H());
  __ Clz(v25.V8H(), v0.V8H());
  __ Clz(v26.V2S(), v0.V2S());
  __ Clz(v27.V4S(), v0.V4S());

  __ Cnt(v28.V8B(), v0.V8B());
  __ Cnt(v29.V16B(), v1.V16B());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x0601000000000102, q16);
    ASSERT_EQUAL_128(0x0601000000000102, 0x0601000000000102, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0006000000000001, q18);
    ASSERT_EQUAL_128(0x0006000000000001, 0x0006000000000001, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000600000000, q20);
    ASSERT_EQUAL_128(0x0000000600000000, 0x0000000600000000, q21);

    ASSERT_EQUAL_128(0x0000000000000000, 0x0404040404040404, q22);
    ASSERT_EQUAL_128(0x0807060605050505, 0x0404040404040404, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0004000400040004, q24);
    ASSERT_EQUAL_128(0x000f000600050005, 0x0004000400040004, q25);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000400000004, q26);
    ASSERT_EQUAL_128(0x0000000f00000005, 0x0000000400000004, q27);

    ASSERT_EQUAL_128(0x0000000000000000, 0x0102020302030304, q28);
    ASSERT_EQUAL_128(0x0705050305030301, 0x0103030503050507, q29);
  }
}

TEST(neon_2regmisc_rev) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0001020304050607, 0x08090a0b0c0d0e0f);
  __ Movi(v1.V2D(), 0xfedcba9876543210, 0x0123456789abcdef);

  __ Rev16(v16.V8B(), v0.V8B());
  __ Rev16(v17.V16B(), v0.V16B());

  __ Rev32(v18.V8B(), v0.V8B());
  __ Rev32(v19.V16B(), v0.V16B());
  __ Rev32(v20.V4H(), v0.V4H());
  __ Rev32(v21.V8H(), v0.V8H());

  __ Rev64(v22.V8B(), v0.V8B());
  __ Rev64(v23.V16B(), v0.V16B());
  __ Rev64(v24.V4H(), v0.V4H());
  __ Rev64(v25.V8H(), v0.V8H());
  __ Rev64(v26.V2S(), v0.V2S());
  __ Rev64(v27.V4S(), v0.V4S());

  __ Rbit(v28.V8B(), v1.V8B());
  __ Rbit(v29.V16B(), v1.V16B());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x09080b0a0d0c0f0e, q16);
    ASSERT_EQUAL_128(0x0100030205040706, 0x09080b0a0d0c0f0e, q17);

    ASSERT_EQUAL_128(0x0000000000000000, 0x0b0a09080f0e0d0c, q18);
    ASSERT_EQUAL_128(0x0302010007060504, 0x0b0a09080f0e0d0c, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0a0b08090e0f0c0d, q20);
    ASSERT_EQUAL_128(0x0203000106070405, 0x0a0b08090e0f0c0d, q21);

    ASSERT_EQUAL_128(0x0000000000000000, 0x0f0e0d0c0b0a0908, q22);
    ASSERT_EQUAL_128(0x0706050403020100, 0x0f0e0d0c0b0a0908, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0e0f0c0d0a0b0809, q24);
    ASSERT_EQUAL_128(0x0607040502030001, 0x0e0f0c0d0a0b0809, q25);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0c0d0e0f08090a0b, q26);
    ASSERT_EQUAL_128(0x0405060700010203, 0x0c0d0e0f08090a0b, q27);

    ASSERT_EQUAL_128(0x0000000000000000, 0x80c4a2e691d5b3f7, q28);
    ASSERT_EQUAL_128(0x7f3b5d196e2a4c08, 0x80c4a2e691d5b3f7, q29);
  }
}


TEST(neon_sli) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0001020304050607, 0x08090a0b0c0d0e0f);
  __ Movi(v1.V2D(), 0xfedcba9876543210, 0x0123456789abcdef);

  __ Mov(v16.V2D(), v0.V2D());
  __ Mov(v17.V2D(), v0.V2D());
  __ Mov(v18.V2D(), v0.V2D());
  __ Mov(v19.V2D(), v0.V2D());
  __ Mov(v20.V2D(), v0.V2D());
  __ Mov(v21.V2D(), v0.V2D());
  __ Mov(v22.V2D(), v0.V2D());
  __ Mov(v23.V2D(), v0.V2D());

  __ Sli(v16.V8B(), v1.V8B(), 4);
  __ Sli(v17.V16B(), v1.V16B(), 7);
  __ Sli(v18.V4H(), v1.V4H(), 8);
  __ Sli(v19.V8H(), v1.V8H(), 15);
  __ Sli(v20.V2S(), v1.V2S(), 0);
  __ Sli(v21.V4S(), v1.V4S(), 31);
  __ Sli(v22.V2D(), v1.V2D(), 48);

  __ Sli(d23, d1, 48);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x18395a7b9cbddeff, q16);
    ASSERT_EQUAL_128(0x0001020304050607, 0x88898a8b8c8d8e8f, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x2309670bab0def0f, q18);
    ASSERT_EQUAL_128(0x0001020304050607, 0x88098a0b8c0d8e0f, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0123456789abcdef, q20);
    ASSERT_EQUAL_128(0x0001020304050607, 0x88090a0b8c0d0e0f, q21);
    ASSERT_EQUAL_128(0x3210020304050607, 0xcdef0a0b0c0d0e0f, q22);

    ASSERT_EQUAL_128(0x0000000000000000, 0xcdef0a0b0c0d0e0f, q23);
  }
}


TEST(neon_sri) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0001020304050607, 0x08090a0b0c0d0e0f);
  __ Movi(v1.V2D(), 0xfedcba9876543210, 0x0123456789abcdef);

  __ Mov(v16.V2D(), v0.V2D());
  __ Mov(v17.V2D(), v0.V2D());
  __ Mov(v18.V2D(), v0.V2D());
  __ Mov(v19.V2D(), v0.V2D());
  __ Mov(v20.V2D(), v0.V2D());
  __ Mov(v21.V2D(), v0.V2D());
  __ Mov(v22.V2D(), v0.V2D());
  __ Mov(v23.V2D(), v0.V2D());

  __ Sri(v16.V8B(), v1.V8B(), 4);
  __ Sri(v17.V16B(), v1.V16B(), 7);
  __ Sri(v18.V4H(), v1.V4H(), 8);
  __ Sri(v19.V8H(), v1.V8H(), 15);
  __ Sri(v20.V2S(), v1.V2S(), 1);
  __ Sri(v21.V4S(), v1.V4S(), 31);
  __ Sri(v22.V2D(), v1.V2D(), 48);

  __ Sri(d23, d1, 48);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x00020406080a0c0e, q16);
    ASSERT_EQUAL_128(0x0101030304040606, 0x08080a0a0d0d0f0f, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x08010a450c890ecd, q18);
    ASSERT_EQUAL_128(0x0001020304040606, 0x08080a0a0c0d0e0f, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0091a2b344d5e6f7, q20);
    ASSERT_EQUAL_128(0x0001020304050606, 0x08090a0a0c0d0e0f, q21);
    ASSERT_EQUAL_128(0x000102030405fedc, 0x08090a0b0c0d0123, q22);

    ASSERT_EQUAL_128(0x0000000000000000, 0x08090a0b0c0d0123, q23);
  }
}


TEST(neon_shrn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Shrn(v16.V8B(), v0.V8H(), 8);
  __ Shrn2(v16.V16B(), v1.V8H(), 1);
  __ Shrn(v17.V4H(), v1.V4S(), 16);
  __ Shrn2(v17.V8H(), v2.V4S(), 1);
  __ Shrn(v18.V2S(), v3.V2D(), 32);
  __ Shrn2(v18.V4S(), v3.V2D(), 1);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0000ff00ff0000ff, 0x7f00817f80ff0180, q16);
    ASSERT_EQUAL_128(0x0000ffff0000ffff, 0x8000ffffffff0001, q17);
    ASSERT_EQUAL_128(0x00000000ffffffff, 0x800000007fffffff, q18);
  }
}


TEST(neon_rshrn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Rshrn(v16.V8B(), v0.V8H(), 8);
  __ Rshrn2(v16.V16B(), v1.V8H(), 1);
  __ Rshrn(v17.V4H(), v1.V4S(), 16);
  __ Rshrn2(v17.V8H(), v2.V4S(), 1);
  __ Rshrn(v18.V2S(), v3.V2D(), 32);
  __ Rshrn2(v18.V4S(), v3.V2D(), 1);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x0001000000000100, 0x7f01827f81ff0181, q16);
    ASSERT_EQUAL_128(0x0000000000000000, 0x8001ffffffff0001, q17);
    ASSERT_EQUAL_128(0x0000000100000000, 0x8000000080000000, q18);
  }
}


TEST(neon_uqshrn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Uqshrn(v16.V8B(), v0.V8H(), 8);
  __ Uqshrn2(v16.V16B(), v1.V8H(), 1);
  __ Uqshrn(v17.V4H(), v1.V4S(), 16);
  __ Uqshrn2(v17.V8H(), v2.V4S(), 1);
  __ Uqshrn(v18.V2S(), v3.V2D(), 32);
  __ Uqshrn2(v18.V4S(), v3.V2D(), 1);

  __ Uqshrn(b19, h0, 8);
  __ Uqshrn(h20, s1, 16);
  __ Uqshrn(s21, d3, 32);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0xffffff00ff0000ff, 0x7f00817f80ff0180, q16);
    ASSERT_EQUAL_128(0xffffffff0000ffff, 0x8000ffffffff0001, q17);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0x800000007fffffff, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000080, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000001, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000007fffffff, q21);
  }
}


TEST(neon_uqrshrn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Uqrshrn(v16.V8B(), v0.V8H(), 8);
  __ Uqrshrn2(v16.V16B(), v1.V8H(), 1);
  __ Uqrshrn(v17.V4H(), v1.V4S(), 16);
  __ Uqrshrn2(v17.V8H(), v2.V4S(), 1);
  __ Uqrshrn(v18.V2S(), v3.V2D(), 32);
  __ Uqrshrn2(v18.V4S(), v3.V2D(), 1);

  __ Uqrshrn(b19, h0, 8);
  __ Uqrshrn(h20, s1, 16);
  __ Uqrshrn(s21, d3, 32);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0xffffff00ff0001ff, 0x7f01827f81ff0181, q16);
    ASSERT_EQUAL_128(0xffffffff0000ffff, 0x8001ffffffff0001, q17);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0x8000000080000000, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000081, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000001, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000080000000, q21);
  }
}


TEST(neon_sqshrn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Sqshrn(v16.V8B(), v0.V8H(), 8);
  __ Sqshrn2(v16.V16B(), v1.V8H(), 1);
  __ Sqshrn(v17.V4H(), v1.V4S(), 16);
  __ Sqshrn2(v17.V8H(), v2.V4S(), 1);
  __ Sqshrn(v18.V2S(), v3.V2D(), 32);
  __ Sqshrn2(v18.V4S(), v3.V2D(), 1);

  __ Sqshrn(b19, h0, 8);
  __ Sqshrn(h20, s1, 16);
  __ Sqshrn(s21, d3, 32);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x8080ff00ff00007f, 0x7f00817f80ff0180, q16);
    ASSERT_EQUAL_128(0x8000ffff00007fff, 0x8000ffffffff0001, q17);
    ASSERT_EQUAL_128(0x800000007fffffff, 0x800000007fffffff, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000080, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000001, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000007fffffff, q21);
  }
}


TEST(neon_sqrshrn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Sqrshrn(v16.V8B(), v0.V8H(), 8);
  __ Sqrshrn2(v16.V16B(), v1.V8H(), 1);
  __ Sqrshrn(v17.V4H(), v1.V4S(), 16);
  __ Sqrshrn2(v17.V8H(), v2.V4S(), 1);
  __ Sqrshrn(v18.V2S(), v3.V2D(), 32);
  __ Sqrshrn2(v18.V4S(), v3.V2D(), 1);

  __ Sqrshrn(b19, h0, 8);
  __ Sqrshrn(h20, s1, 16);
  __ Sqrshrn(s21, d3, 32);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x808000000000017f, 0x7f01827f81ff0181, q16);
    ASSERT_EQUAL_128(0x8000000000007fff, 0x8001ffffffff0001, q17);
    ASSERT_EQUAL_128(0x800000007fffffff, 0x800000007fffffff, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000081, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000001, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000007fffffff, q21);
  }
}


TEST(neon_sqshrun) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Sqshrun(v16.V8B(), v0.V8H(), 8);
  __ Sqshrun2(v16.V16B(), v1.V8H(), 1);
  __ Sqshrun(v17.V4H(), v1.V4S(), 16);
  __ Sqshrun2(v17.V8H(), v2.V4S(), 1);
  __ Sqshrun(v18.V2S(), v3.V2D(), 32);
  __ Sqshrun2(v18.V4S(), v3.V2D(), 1);

  __ Sqshrun(b19, h0, 8);
  __ Sqshrun(h20, s1, 16);
  __ Sqshrun(s21, d3, 32);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x00000000000000ff, 0x7f00007f00000100, q16);
    ASSERT_EQUAL_128(0x000000000000ffff, 0x0000000000000001, q17);
    ASSERT_EQUAL_128(0x00000000ffffffff, 0x000000007fffffff, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000001, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000007fffffff, q21);
  }
}


TEST(neon_sqrshrun) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Sqrshrun(v16.V8B(), v0.V8H(), 8);
  __ Sqrshrun2(v16.V16B(), v1.V8H(), 1);
  __ Sqrshrun(v17.V4H(), v1.V4S(), 16);
  __ Sqrshrun2(v17.V8H(), v2.V4S(), 1);
  __ Sqrshrun(v18.V2S(), v3.V2D(), 32);
  __ Sqrshrun2(v18.V4S(), v3.V2D(), 1);

  __ Sqrshrun(b19, h0, 8);
  __ Sqrshrun(h20, s1, 16);
  __ Sqrshrun(s21, d3, 32);

  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x00000000000001ff, 0x7f01007f00000100, q16);
    ASSERT_EQUAL_128(0x000000000000ffff, 0x0000000000000001, q17);
    ASSERT_EQUAL_128(0x00000000ffffffff, 0x0000000080000000, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000001, q20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000080000000, q21);
  }
}

TEST(neon_modimm_bic) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v16.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v17.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v18.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v19.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v20.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v21.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v22.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v23.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v24.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v25.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v26.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v27.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);

  __ Bic(v16.V4H(), 0x00, 0);
  __ Bic(v17.V4H(), 0xff, 8);
  __ Bic(v18.V8H(), 0x00, 0);
  __ Bic(v19.V8H(), 0xff, 8);

  __ Bic(v20.V2S(), 0x00, 0);
  __ Bic(v21.V2S(), 0xff, 8);
  __ Bic(v22.V2S(), 0x00, 16);
  __ Bic(v23.V2S(), 0xff, 24);

  __ Bic(v24.V4S(), 0xff, 0);
  __ Bic(v25.V4S(), 0x00, 8);
  __ Bic(v26.V4S(), 0xff, 16);
  __ Bic(v27.V4S(), 0x00, 24);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0x5555ffff0000aaaa, q16);
    ASSERT_EQUAL_128(0x0, 0x005500ff000000aa, q17);
    ASSERT_EQUAL_128(0x00aaff55ff0055aa, 0x5555ffff0000aaaa, q18);
    ASSERT_EQUAL_128(0x00aa0055000000aa, 0x005500ff000000aa, q19);

    ASSERT_EQUAL_128(0x0, 0x5555ffff0000aaaa, q20);
    ASSERT_EQUAL_128(0x0, 0x555500ff000000aa, q21);
    ASSERT_EQUAL_128(0x0, 0x5555ffff0000aaaa, q22);
    ASSERT_EQUAL_128(0x0, 0x0055ffff0000aaaa, q23);

    ASSERT_EQUAL_128(0x00aaff00ff005500, 0x5555ff000000aa00, q24);
    ASSERT_EQUAL_128(0x00aaff55ff0055aa, 0x5555ffff0000aaaa, q25);
    ASSERT_EQUAL_128(0x0000ff55ff0055aa, 0x5500ffff0000aaaa, q26);
    ASSERT_EQUAL_128(0x00aaff55ff0055aa, 0x5555ffff0000aaaa, q27);
  }
}


TEST(neon_modimm_movi_16bit_any) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V4H(), 0xabab);
  __ Movi(v1.V4H(), 0xab00);
  __ Movi(v2.V4H(), 0xabff);
  __ Movi(v3.V8H(), 0x00ab);
  __ Movi(v4.V8H(), 0xffab);
  __ Movi(v5.V8H(), 0xabcd);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0xabababababababab, q0);
    ASSERT_EQUAL_128(0x0, 0xab00ab00ab00ab00, q1);
    ASSERT_EQUAL_128(0x0, 0xabffabffabffabff, q2);
    ASSERT_EQUAL_128(0x00ab00ab00ab00ab, 0x00ab00ab00ab00ab, q3);
    ASSERT_EQUAL_128(0xffabffabffabffab, 0xffabffabffabffab, q4);
    ASSERT_EQUAL_128(0xabcdabcdabcdabcd, 0xabcdabcdabcdabcd, q5);
  }
}


TEST(neon_modimm_movi_32bit_any) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2S(), 0x000000ab);
  __ Movi(v1.V2S(), 0x0000ab00);
  __ Movi(v2.V4S(), 0x00ab0000);
  __ Movi(v3.V4S(), 0xab000000);

  __ Movi(v4.V2S(), 0xffffffab);
  __ Movi(v5.V2S(), 0xffffabff);
  __ Movi(v6.V4S(), 0xffabffff);
  __ Movi(v7.V4S(), 0xabffffff);

  __ Movi(v16.V2S(), 0x0000abff);
  __ Movi(v17.V2S(), 0x00abffff);
  __ Movi(v18.V4S(), 0xffab0000);
  __ Movi(v19.V4S(), 0xffffab00);

  __ Movi(v20.V4S(), 0xabababab);
  __ Movi(v21.V4S(), 0xabcdabcd);
  __ Movi(v22.V4S(), 0xabcdef01);
  __ Movi(v23.V4S(), 0x00ffff00);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0x000000ab000000ab, q0);
    ASSERT_EQUAL_128(0x0, 0x0000ab000000ab00, q1);
    ASSERT_EQUAL_128(0x00ab000000ab0000, 0x00ab000000ab0000, q2);
    ASSERT_EQUAL_128(0xab000000ab000000, 0xab000000ab000000, q3);

    ASSERT_EQUAL_128(0x0, 0xffffffabffffffab, q4);
    ASSERT_EQUAL_128(0x0, 0xffffabffffffabff, q5);
    ASSERT_EQUAL_128(0xffabffffffabffff, 0xffabffffffabffff, q6);
    ASSERT_EQUAL_128(0xabffffffabffffff, 0xabffffffabffffff, q7);

    ASSERT_EQUAL_128(0x0, 0x0000abff0000abff, q16);
    ASSERT_EQUAL_128(0x0, 0x00abffff00abffff, q17);
    ASSERT_EQUAL_128(0xffab0000ffab0000, 0xffab0000ffab0000, q18);
    ASSERT_EQUAL_128(0xffffab00ffffab00, 0xffffab00ffffab00, q19);

    ASSERT_EQUAL_128(0xabababababababab, 0xabababababababab, q20);
    ASSERT_EQUAL_128(0xabcdabcdabcdabcd, 0xabcdabcdabcdabcd, q21);
    ASSERT_EQUAL_128(0xabcdef01abcdef01, 0xabcdef01abcdef01, q22);
    ASSERT_EQUAL_128(0x00ffff0000ffff00, 0x00ffff0000ffff00, q23);
  }
}


TEST(neon_modimm_movi_64bit_any) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V1D(), 0x00ffff0000ffffff);
  __ Movi(v1.V2D(), 0xabababababababab);
  __ Movi(v2.V2D(), 0xabcdabcdabcdabcd);
  __ Movi(v3.V2D(), 0xabcdef01abcdef01);
  __ Movi(v4.V1D(), 0xabcdef0123456789);
  __ Movi(v5.V2D(), 0xabcdef0123456789);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_64(0x00ffff0000ffffff, d0);
    ASSERT_EQUAL_128(0xabababababababab, 0xabababababababab, q1);
    ASSERT_EQUAL_128(0xabcdabcdabcdabcd, 0xabcdabcdabcdabcd, q2);
    ASSERT_EQUAL_128(0xabcdef01abcdef01, 0xabcdef01abcdef01, q3);
    ASSERT_EQUAL_64(0xabcdef0123456789, d4);
    ASSERT_EQUAL_128(0xabcdef0123456789, 0xabcdef0123456789, q5);
  }
}


TEST(neon_modimm_movi) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V8B(), 0xaa);
  __ Movi(v1.V16B(), 0x55);

  __ Movi(d2, 0x00ffff0000ffffff);
  __ Movi(v3.V2D(), 0x00ffff0000ffffff);

  __ Movi(v16.V4H(), 0x00, LSL, 0);
  __ Movi(v17.V4H(), 0xff, LSL, 8);
  __ Movi(v18.V8H(), 0x00, LSL, 0);
  __ Movi(v19.V8H(), 0xff, LSL, 8);

  __ Movi(v20.V2S(), 0x00, LSL, 0);
  __ Movi(v21.V2S(), 0xff, LSL, 8);
  __ Movi(v22.V2S(), 0x00, LSL, 16);
  __ Movi(v23.V2S(), 0xff, LSL, 24);

  __ Movi(v24.V4S(), 0xff, LSL, 0);
  __ Movi(v25.V4S(), 0x00, LSL, 8);
  __ Movi(v26.V4S(), 0xff, LSL, 16);
  __ Movi(v27.V4S(), 0x00, LSL, 24);

  __ Movi(v28.V2S(), 0xaa, MSL, 8);
  __ Movi(v29.V2S(), 0x55, MSL, 16);
  __ Movi(v30.V4S(), 0xff, MSL, 8);
  __ Movi(v31.V4S(), 0x00, MSL, 16);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0xaaaaaaaaaaaaaaaa, q0);
    ASSERT_EQUAL_128(0x5555555555555555, 0x5555555555555555, q1);

    ASSERT_EQUAL_128(0x0, 0x00ffff0000ffffff, q2);
    ASSERT_EQUAL_128(0x00ffff0000ffffff, 0x00ffff0000ffffff, q3);

    ASSERT_EQUAL_128(0x0, 0x0000000000000000, q16);
    ASSERT_EQUAL_128(0x0, 0xff00ff00ff00ff00, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q18);
    ASSERT_EQUAL_128(0xff00ff00ff00ff00, 0xff00ff00ff00ff00, q19);

    ASSERT_EQUAL_128(0x0, 0x0000000000000000, q20);
    ASSERT_EQUAL_128(0x0, 0x0000ff000000ff00, q21);
    ASSERT_EQUAL_128(0x0, 0x0000000000000000, q22);
    ASSERT_EQUAL_128(0x0, 0xff000000ff000000, q23);

    ASSERT_EQUAL_128(0x000000ff000000ff, 0x000000ff000000ff, q24);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q25);
    ASSERT_EQUAL_128(0x00ff000000ff0000, 0x00ff000000ff0000, q26);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, q27);

    ASSERT_EQUAL_128(0x0, 0x0000aaff0000aaff, q28);
    ASSERT_EQUAL_128(0x0, 0x0055ffff0055ffff, q29);
    ASSERT_EQUAL_128(0x0000ffff0000ffff, 0x0000ffff0000ffff, q30);
    ASSERT_EQUAL_128(0x0000ffff0000ffff, 0x0000ffff0000ffff, q31);
  }
}


TEST(neon_modimm_mvni) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Mvni(v16.V4H(), 0x00, LSL, 0);
  __ Mvni(v17.V4H(), 0xff, LSL, 8);
  __ Mvni(v18.V8H(), 0x00, LSL, 0);
  __ Mvni(v19.V8H(), 0xff, LSL, 8);

  __ Mvni(v20.V2S(), 0x00, LSL, 0);
  __ Mvni(v21.V2S(), 0xff, LSL, 8);
  __ Mvni(v22.V2S(), 0x00, LSL, 16);
  __ Mvni(v23.V2S(), 0xff, LSL, 24);

  __ Mvni(v24.V4S(), 0xff, LSL, 0);
  __ Mvni(v25.V4S(), 0x00, LSL, 8);
  __ Mvni(v26.V4S(), 0xff, LSL, 16);
  __ Mvni(v27.V4S(), 0x00, LSL, 24);

  __ Mvni(v28.V2S(), 0xaa, MSL, 8);
  __ Mvni(v29.V2S(), 0x55, MSL, 16);
  __ Mvni(v30.V4S(), 0xff, MSL, 8);
  __ Mvni(v31.V4S(), 0x00, MSL, 16);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0xffffffffffffffff, q16);
    ASSERT_EQUAL_128(0x0, 0x00ff00ff00ff00ff, q17);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q18);
    ASSERT_EQUAL_128(0x00ff00ff00ff00ff, 0x00ff00ff00ff00ff, q19);

    ASSERT_EQUAL_128(0x0, 0xffffffffffffffff, q20);
    ASSERT_EQUAL_128(0x0, 0xffff00ffffff00ff, q21);
    ASSERT_EQUAL_128(0x0, 0xffffffffffffffff, q22);
    ASSERT_EQUAL_128(0x0, 0x00ffffff00ffffff, q23);

    ASSERT_EQUAL_128(0xffffff00ffffff00, 0xffffff00ffffff00, q24);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q25);
    ASSERT_EQUAL_128(0xff00ffffff00ffff, 0xff00ffffff00ffff, q26);
    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q27);

    ASSERT_EQUAL_128(0x0, 0xffff5500ffff5500, q28);
    ASSERT_EQUAL_128(0x0, 0xffaa0000ffaa0000, q29);
    ASSERT_EQUAL_128(0xffff0000ffff0000, 0xffff0000ffff0000, q30);
    ASSERT_EQUAL_128(0xffff0000ffff0000, 0xffff0000ffff0000, q31);
  }
}


TEST(neon_modimm_orr) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v16.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v17.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v18.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v19.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v20.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v21.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v22.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v23.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v24.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v25.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v26.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);
  __ Movi(v27.V2D(), 0x00aaff55ff0055aa, 0x5555ffff0000aaaa);

  __ Orr(v16.V4H(), 0x00, 0);
  __ Orr(v17.V4H(), 0xff, 8);
  __ Orr(v18.V8H(), 0x00, 0);
  __ Orr(v19.V8H(), 0xff, 8);

  __ Orr(v20.V2S(), 0x00, 0);
  __ Orr(v21.V2S(), 0xff, 8);
  __ Orr(v22.V2S(), 0x00, 16);
  __ Orr(v23.V2S(), 0xff, 24);

  __ Orr(v24.V4S(), 0xff, 0);
  __ Orr(v25.V4S(), 0x00, 8);
  __ Orr(v26.V4S(), 0xff, 16);
  __ Orr(v27.V4S(), 0x00, 24);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0, 0x5555ffff0000aaaa, q16);
    ASSERT_EQUAL_128(0x0, 0xff55ffffff00ffaa, q17);
    ASSERT_EQUAL_128(0x00aaff55ff0055aa, 0x5555ffff0000aaaa, q18);
    ASSERT_EQUAL_128(0xffaaff55ff00ffaa, 0xff55ffffff00ffaa, q19);

    ASSERT_EQUAL_128(0x0, 0x5555ffff0000aaaa, q20);
    ASSERT_EQUAL_128(0x0, 0x5555ffff0000ffaa, q21);
    ASSERT_EQUAL_128(0x0, 0x5555ffff0000aaaa, q22);
    ASSERT_EQUAL_128(0x0, 0xff55ffffff00aaaa, q23);

    ASSERT_EQUAL_128(0x00aaffffff0055ff, 0x5555ffff0000aaff, q24);
    ASSERT_EQUAL_128(0x00aaff55ff0055aa, 0x5555ffff0000aaaa, q25);
    ASSERT_EQUAL_128(0x00ffff55ffff55aa, 0x55ffffff00ffaaaa, q26);
    ASSERT_EQUAL_128(0x00aaff55ff0055aa, 0x5555ffff0000aaaa, q27);
  }
}

TEST(ldr_literal_values_q) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  static const uint64_t kHalfValues[] = {0x8000000000000000,
                                         0x7fffffffffffffff,
                                         0x0000000000000000,
                                         0xffffffffffffffff,
                                         0x00ff00ff00ff00ff,
                                         0x1234567890abcdef};
  const int card = sizeof(kHalfValues) / sizeof(kHalfValues[0]);
  const Register& ref_low64 = x1;
  const Register& ref_high64 = x2;
  const Register& loaded_low64 = x3;
  const Register& loaded_high64 = x4;
  const VRegister& tgt = q0;

  START();
  __ Mov(x0, 0);

  for (int i = 0; i < card; i++) {
    __ Mov(ref_low64, kHalfValues[i]);
    for (int j = 0; j < card; j++) {
      __ Mov(ref_high64, kHalfValues[j]);
      __ Ldr(tgt, kHalfValues[j], kHalfValues[i]);
      __ Mov(loaded_low64, tgt.V2D(), 0);
      __ Mov(loaded_high64, tgt.V2D(), 1);
      __ Cmp(loaded_low64, ref_low64);
      __ Ccmp(loaded_high64, ref_high64, NoFlag, eq);
      __ Cset(x0, ne);
    }
  }
  END();

  if (CAN_RUN()) {
    RUN();

    // If one of the values differs, the trace can be used to identify which
    // one.
    ASSERT_EQUAL_64(0, x0);
  }
}

TEST(fmov_vec_imm) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();

  __ Fmov(v0.V2S(), 20.0);
  __ Fmov(v1.V4S(), 1024.0);

  __ Fmov(v2.V4H(), RawbitsToFloat16(0xC500U));
  __ Fmov(v3.V8H(), RawbitsToFloat16(0x4A80U));

  END();
  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_64(0x41A0000041A00000, d0);
    ASSERT_EQUAL_128(0x4480000044800000, 0x4480000044800000, q1);
    ASSERT_EQUAL_64(0xC500C500C500C500, d2);
    ASSERT_EQUAL_128(0x4A804A804A804A80, 0x4A804A804A804A80, q3);
  }
}

// TODO: add arbitrary values once load literal to Q registers is supported.
TEST(neon_modimm_fmov) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  // Immediates which can be encoded in the instructions.
  const float kOne = 1.0f;
  const float kPointFive = 0.5f;
  const double kMinusThirteen = -13.0;
  // Immediates which cannot be encoded in the instructions.
  const float kNonImmFP32 = 255.0f;
  const double kNonImmFP64 = 12.3456;

  START();
  __ Fmov(v11.V2S(), kOne);
  __ Fmov(v12.V4S(), kPointFive);
  __ Fmov(v22.V2D(), kMinusThirteen);
  __ Fmov(v13.V2S(), kNonImmFP32);
  __ Fmov(v14.V4S(), kNonImmFP32);
  __ Fmov(v23.V2D(), kNonImmFP64);
  __ Fmov(v1.V2S(), 0.0);
  __ Fmov(v2.V4S(), 0.0);
  __ Fmov(v3.V2D(), 0.0);
  __ Fmov(v4.V2S(), kFP32PositiveInfinity);
  __ Fmov(v5.V4S(), kFP32PositiveInfinity);
  __ Fmov(v6.V2D(), kFP64PositiveInfinity);
  END();

  if (CAN_RUN()) {
    RUN();

    const uint64_t kOne1S = FloatToRawbits(1.0);
    const uint64_t kOne2S = (kOne1S << 32) | kOne1S;
    const uint64_t kPointFive1S = FloatToRawbits(0.5);
    const uint64_t kPointFive2S = (kPointFive1S << 32) | kPointFive1S;
    const uint64_t kMinusThirteen1D = DoubleToRawbits(-13.0);
    const uint64_t kNonImmFP321S = FloatToRawbits(kNonImmFP32);
    const uint64_t kNonImmFP322S = (kNonImmFP321S << 32) | kNonImmFP321S;
    const uint64_t kNonImmFP641D = DoubleToRawbits(kNonImmFP64);
    const uint64_t kFP32Inf1S = FloatToRawbits(kFP32PositiveInfinity);
    const uint64_t kFP32Inf2S = (kFP32Inf1S << 32) | kFP32Inf1S;
    const uint64_t kFP64Inf1D = DoubleToRawbits(kFP64PositiveInfinity);

    ASSERT_EQUAL_128(0x0, kOne2S, q11);
    ASSERT_EQUAL_128(kPointFive2S, kPointFive2S, q12);
    ASSERT_EQUAL_128(kMinusThirteen1D, kMinusThirteen1D, q22);
    ASSERT_EQUAL_128(0x0, kNonImmFP322S, q13);
    ASSERT_EQUAL_128(kNonImmFP322S, kNonImmFP322S, q14);
    ASSERT_EQUAL_128(kNonImmFP641D, kNonImmFP641D, q23);
    ASSERT_EQUAL_128(0x0, 0x0, q1);
    ASSERT_EQUAL_128(0x0, 0x0, q2);
    ASSERT_EQUAL_128(0x0, 0x0, q3);
    ASSERT_EQUAL_128(0x0, kFP32Inf2S, q4);
    ASSERT_EQUAL_128(kFP32Inf2S, kFP32Inf2S, q5);
    ASSERT_EQUAL_128(kFP64Inf1D, kFP64Inf1D, q6);
  }
}


TEST(neon_perm) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0001020304050607, 0x08090a0b0c0d0e0f);
  __ Movi(v1.V2D(), 0x1011121314151617, 0x18191a1b1c1d1e1f);

  __ Trn1(v16.V16B(), v0.V16B(), v1.V16B());
  __ Trn2(v17.V16B(), v0.V16B(), v1.V16B());
  __ Zip1(v18.V16B(), v0.V16B(), v1.V16B());
  __ Zip2(v19.V16B(), v0.V16B(), v1.V16B());
  __ Uzp1(v20.V16B(), v0.V16B(), v1.V16B());
  __ Uzp2(v21.V16B(), v0.V16B(), v1.V16B());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x1101130315051707, 0x19091b0b1d0d1f0f, q16);
    ASSERT_EQUAL_128(0x1000120214041606, 0x18081a0a1c0c1e0e, q17);
    ASSERT_EQUAL_128(0x180819091a0a1b0b, 0x1c0c1d0d1e0e1f0f, q18);
    ASSERT_EQUAL_128(0x1000110112021303, 0x1404150516061707, q19);
    ASSERT_EQUAL_128(0x11131517191b1d1f, 0x01030507090b0d0f, q20);
    ASSERT_EQUAL_128(0x10121416181a1c1e, 0x00020406080a0c0e, q21);
  }
}


TEST(neon_copy_dup_element) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344556677, 0x8899aabbccddeeff);
  __ Movi(v1.V2D(), 0xffeddccbbaae9988, 0x7766554433221100);
  __ Movi(v2.V2D(), 0xffeddccbbaae9988, 0x0011223344556677);
  __ Movi(v3.V2D(), 0x7766554433221100, 0x8899aabbccddeeff);
  __ Movi(v4.V2D(), 0x7766554433221100, 0x0123456789abcdef);
  __ Movi(v5.V2D(), 0x0011223344556677, 0x0123456789abcdef);

  __ Dup(v16.V16B(), v0.B(), 0);
  __ Dup(v17.V8H(), v1.H(), 7);
  __ Dup(v18.V4S(), v1.S(), 3);
  __ Dup(v19.V2D(), v0.D(), 0);

  __ Dup(v20.V8B(), v0.B(), 0);
  __ Dup(v21.V4H(), v1.H(), 7);
  __ Dup(v22.V2S(), v1.S(), 3);

  __ Dup(v23.B(), v0.B(), 0);
  __ Dup(v24.H(), v1.H(), 7);
  __ Dup(v25.S(), v1.S(), 3);
  __ Dup(v26.D(), v0.D(), 0);

  __ Dup(v2.V16B(), v2.B(), 0);
  __ Dup(v3.V8H(), v3.H(), 7);
  __ Dup(v4.V4S(), v4.S(), 0);
  __ Dup(v5.V2D(), v5.D(), 1);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xffffffffffffffff, 0xffffffffffffffff, q16);
    ASSERT_EQUAL_128(0xffedffedffedffed, 0xffedffedffedffed, q17);
    ASSERT_EQUAL_128(0xffeddccbffeddccb, 0xffeddccbffeddccb, q18);
    ASSERT_EQUAL_128(0x8899aabbccddeeff, 0x8899aabbccddeeff, q19);

    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q20);
    ASSERT_EQUAL_128(0, 0xffedffedffedffed, q21);
    ASSERT_EQUAL_128(0, 0xffeddccbffeddccb, q22);

    ASSERT_EQUAL_128(0, 0x00000000000000ff, q23);
    ASSERT_EQUAL_128(0, 0x000000000000ffed, q24);
    ASSERT_EQUAL_128(0, 0x00000000ffeddccb, q25);
    ASSERT_EQUAL_128(0, 0x8899aabbccddeeff, q26);

    ASSERT_EQUAL_128(0x7777777777777777, 0x7777777777777777, q2);
    ASSERT_EQUAL_128(0x7766776677667766, 0x7766776677667766, q3);
    ASSERT_EQUAL_128(0x89abcdef89abcdef, 0x89abcdef89abcdef, q4);
    ASSERT_EQUAL_128(0x0011223344556677, 0x0011223344556677, q5);
  }
}


TEST(neon_copy_dup_general) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Mov(x0, 0x0011223344556677);

  __ Dup(v16.V16B(), w0);
  __ Dup(v17.V8H(), w0);
  __ Dup(v18.V4S(), w0);
  __ Dup(v19.V2D(), x0);

  __ Dup(v20.V8B(), w0);
  __ Dup(v21.V4H(), w0);
  __ Dup(v22.V2S(), w0);

  __ Dup(v2.V16B(), wzr);
  __ Dup(v3.V8H(), wzr);
  __ Dup(v4.V4S(), wzr);
  __ Dup(v5.V2D(), xzr);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x7777777777777777, 0x7777777777777777, q16);
    ASSERT_EQUAL_128(0x6677667766776677, 0x6677667766776677, q17);
    ASSERT_EQUAL_128(0x4455667744556677, 0x4455667744556677, q18);
    ASSERT_EQUAL_128(0x0011223344556677, 0x0011223344556677, q19);

    ASSERT_EQUAL_128(0, 0x7777777777777777, q20);
    ASSERT_EQUAL_128(0, 0x6677667766776677, q21);
    ASSERT_EQUAL_128(0, 0x4455667744556677, q22);

    ASSERT_EQUAL_128(0, 0, q2);
    ASSERT_EQUAL_128(0, 0, q3);
    ASSERT_EQUAL_128(0, 0, q4);
    ASSERT_EQUAL_128(0, 0, q5);
  }
}


TEST(neon_copy_ins_element) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344556677, 0x8899aabbccddeeff);
  __ Movi(v1.V2D(), 0xffeddccbbaae9988, 0x7766554433221100);
  __ Movi(v16.V2D(), 0x0123456789abcdef, 0xfedcba9876543210);
  __ Movi(v17.V2D(), 0xfedcba9876543210, 0x0123456789abcdef);
  __ Movi(v18.V2D(), 0x0011223344556677, 0x8899aabbccddeeff);
  __ Movi(v19.V2D(), 0x0011223344556677, 0x8899aabbccddeeff);

  __ Movi(v2.V2D(), 0, 0x0011223344556677);
  __ Movi(v3.V2D(), 0, 0x8899aabbccddeeff);
  __ Movi(v4.V2D(), 0, 0x0123456789abcdef);
  __ Movi(v5.V2D(), 0, 0x0123456789abcdef);

  __ Ins(v16.V16B(), 15, v0.V16B(), 0);
  __ Ins(v17.V8H(), 0, v1.V8H(), 7);
  __ Ins(v18.V4S(), 3, v1.V4S(), 0);
  __ Ins(v19.V2D(), 1, v0.V2D(), 0);

  __ Ins(v2.V16B(), 2, v2.V16B(), 0);
  __ Ins(v3.V8H(), 0, v3.V8H(), 7);
  __ Ins(v4.V4S(), 3, v4.V4S(), 0);
  __ Ins(v5.V2D(), 0, v5.V2D(), 1);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xff23456789abcdef, 0xfedcba9876543210, q16);
    ASSERT_EQUAL_128(0xfedcba9876543210, 0x0123456789abffed, q17);
    ASSERT_EQUAL_128(0x3322110044556677, 0x8899aabbccddeeff, q18);
    ASSERT_EQUAL_128(0x8899aabbccddeeff, 0x8899aabbccddeeff, q19);

    ASSERT_EQUAL_128(0, 0x0011223344776677, q2);
    ASSERT_EQUAL_128(0, 0x8899aabbccdd0000, q3);
    ASSERT_EQUAL_128(0x89abcdef00000000, 0x0123456789abcdef, q4);
    ASSERT_EQUAL_128(0, 0, q5);
  }
}


TEST(neon_copy_mov_element) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344556677, 0x8899aabbccddeeff);
  __ Movi(v1.V2D(), 0xffeddccbbaae9988, 0x7766554433221100);
  __ Movi(v16.V2D(), 0x0123456789abcdef, 0xfedcba9876543210);
  __ Movi(v17.V2D(), 0xfedcba9876543210, 0x0123456789abcdef);
  __ Movi(v18.V2D(), 0x0011223344556677, 0x8899aabbccddeeff);
  __ Movi(v19.V2D(), 0x0011223344556677, 0x8899aabbccddeeff);

  __ Movi(v2.V2D(), 0, 0x0011223344556677);
  __ Movi(v3.V2D(), 0, 0x8899aabbccddeeff);
  __ Movi(v4.V2D(), 0, 0x0123456789abcdef);
  __ Movi(v5.V2D(), 0, 0x0123456789abcdef);

  __ Mov(v16.V16B(), 15, v0.V16B(), 0);
  __ Mov(v17.V8H(), 0, v1.V8H(), 7);
  __ Mov(v18.V4S(), 3, v1.V4S(), 0);
  __ Mov(v19.V2D(), 1, v0.V2D(), 0);

  __ Mov(v2.V16B(), 2, v2.V16B(), 0);
  __ Mov(v3.V8H(), 0, v3.V8H(), 7);
  __ Mov(v4.V4S(), 3, v4.V4S(), 0);
  __ Mov(v5.V2D(), 0, v5.V2D(), 1);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xff23456789abcdef, 0xfedcba9876543210, q16);
    ASSERT_EQUAL_128(0xfedcba9876543210, 0x0123456789abffed, q17);
    ASSERT_EQUAL_128(0x3322110044556677, 0x8899aabbccddeeff, q18);
    ASSERT_EQUAL_128(0x8899aabbccddeeff, 0x8899aabbccddeeff, q19);

    ASSERT_EQUAL_128(0, 0x0011223344776677, q2);
    ASSERT_EQUAL_128(0, 0x8899aabbccdd0000, q3);
    ASSERT_EQUAL_128(0x89abcdef00000000, 0x0123456789abcdef, q4);
    ASSERT_EQUAL_128(0, 0, q5);
  }
}


TEST(neon_copy_smov) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0123456789abcdef, 0xfedcba9876543210);

  __ Smov(w0, v0.B(), 7);
  __ Smov(w1, v0.B(), 15);

  __ Smov(w2, v0.H(), 0);
  __ Smov(w3, v0.H(), 3);

  __ Smov(x4, v0.B(), 7);
  __ Smov(x5, v0.B(), 15);

  __ Smov(x6, v0.H(), 0);
  __ Smov(x7, v0.H(), 3);

  __ Smov(x16, v0.S(), 0);
  __ Smov(x17, v0.S(), 1);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_32(0xfffffffe, w0);
    ASSERT_EQUAL_32(0x00000001, w1);
    ASSERT_EQUAL_32(0x00003210, w2);
    ASSERT_EQUAL_32(0xfffffedc, w3);
    ASSERT_EQUAL_64(0xfffffffffffffffe, x4);
    ASSERT_EQUAL_64(0x0000000000000001, x5);
    ASSERT_EQUAL_64(0x0000000000003210, x6);
    ASSERT_EQUAL_64(0xfffffffffffffedc, x7);
    ASSERT_EQUAL_64(0x0000000076543210, x16);
    ASSERT_EQUAL_64(0xfffffffffedcba98, x17);
  }
}


TEST(neon_copy_umov_mov) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0123456789abcdef, 0xfedcba9876543210);

  __ Umov(w0, v0.B(), 15);
  __ Umov(w1, v0.H(), 0);
  __ Umov(w2, v0.S(), 3);
  __ Umov(x3, v0.D(), 1);

  __ Mov(w4, v0.S(), 3);
  __ Mov(x5, v0.D(), 1);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_32(0x00000001, w0);
    ASSERT_EQUAL_32(0x00003210, w1);
    ASSERT_EQUAL_32(0x01234567, w2);
    ASSERT_EQUAL_64(0x0123456789abcdef, x3);
    ASSERT_EQUAL_32(0x01234567, w4);
    ASSERT_EQUAL_64(0x0123456789abcdef, x5);
  }
}


TEST(neon_copy_ins_general) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Mov(x0, 0x0011223344556677);
  __ Movi(v16.V2D(), 0x0123456789abcdef, 0xfedcba9876543210);
  __ Movi(v17.V2D(), 0xfedcba9876543210, 0x0123456789abcdef);
  __ Movi(v18.V2D(), 0x0011223344556677, 0x8899aabbccddeeff);
  __ Movi(v19.V2D(), 0x0011223344556677, 0x8899aabbccddeeff);

  __ Movi(v2.V2D(), 0, 0x0011223344556677);
  __ Movi(v3.V2D(), 0, 0x8899aabbccddeeff);
  __ Movi(v4.V2D(), 0, 0x0123456789abcdef);
  __ Movi(v5.V2D(), 0, 0x0123456789abcdef);

  __ Ins(v16.V16B(), 15, w0);
  __ Ins(v17.V8H(), 0, w0);
  __ Ins(v18.V4S(), 3, w0);
  __ Ins(v19.V2D(), 0, x0);

  __ Ins(v2.V16B(), 2, w0);
  __ Ins(v3.V8H(), 0, w0);
  __ Ins(v4.V4S(), 3, w0);
  __ Ins(v5.V2D(), 1, x0);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x7723456789abcdef, 0xfedcba9876543210, q16);
    ASSERT_EQUAL_128(0xfedcba9876543210, 0x0123456789ab6677, q17);
    ASSERT_EQUAL_128(0x4455667744556677, 0x8899aabbccddeeff, q18);
    ASSERT_EQUAL_128(0x0011223344556677, 0x0011223344556677, q19);

    ASSERT_EQUAL_128(0, 0x0011223344776677, q2);
    ASSERT_EQUAL_128(0, 0x8899aabbccdd6677, q3);
    ASSERT_EQUAL_128(0x4455667700000000, 0x0123456789abcdef, q4);
    ASSERT_EQUAL_128(0x0011223344556677, 0x0123456789abcdef, q5);
  }
}


TEST(neon_extract_ext) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0011223344556677, 0x8899aabbccddeeff);
  __ Movi(v1.V2D(), 0xffeddccbbaae9988, 0x7766554433221100);

  __ Movi(v2.V2D(), 0, 0x0011223344556677);
  __ Movi(v3.V2D(), 0, 0x8899aabbccddeeff);

  __ Ext(v16.V16B(), v0.V16B(), v1.V16B(), 0);
  __ Ext(v17.V16B(), v0.V16B(), v1.V16B(), 15);
  __ Ext(v1.V16B(), v0.V16B(), v1.V16B(), 8);  // Dest is same as one Src
  __ Ext(v0.V16B(), v0.V16B(), v0.V16B(), 8);  // All reg are the same

  __ Ext(v18.V8B(), v2.V8B(), v3.V8B(), 0);
  __ Ext(v19.V8B(), v2.V8B(), v3.V8B(), 7);
  __ Ext(v2.V8B(), v2.V8B(), v3.V8B(), 4);  // Dest is same as one Src
  __ Ext(v3.V8B(), v3.V8B(), v3.V8B(), 4);  // All reg are the same

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0011223344556677, 0x8899aabbccddeeff, q16);
    ASSERT_EQUAL_128(0xeddccbbaae998877, 0x6655443322110000, q17);
    ASSERT_EQUAL_128(0x7766554433221100, 0x0011223344556677, q1);
    ASSERT_EQUAL_128(0x8899aabbccddeeff, 0x0011223344556677, q0);

    ASSERT_EQUAL_128(0, 0x0011223344556677, q18);
    ASSERT_EQUAL_128(0, 0x99aabbccddeeff00, q19);
    ASSERT_EQUAL_128(0, 0xccddeeff00112233, q2);
    ASSERT_EQUAL_128(0, 0xccddeeff8899aabb, q3);
  }
}


TEST(neon_3different_uaddl) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0000000000000000, 0x0000000000000000);
  __ Movi(v1.V2D(), 0, 0x00010280810e0fff);
  __ Movi(v2.V2D(), 0, 0x0101010101010101);

  __ Movi(v3.V2D(), 0x0000000000000000, 0x0000000000000000);
  __ Movi(v4.V2D(), 0x0000000000000000, 0x0000000000000000);
  __ Movi(v5.V2D(), 0, 0x0000000180008001);
  __ Movi(v6.V2D(), 0, 0x000e000ff000ffff);
  __ Movi(v7.V2D(), 0, 0x0001000100010001);

  __ Movi(v16.V2D(), 0x0000000000000000, 0x0000000000000000);
  __ Movi(v17.V2D(), 0x0000000000000000, 0x0000000000000000);
  __ Movi(v18.V2D(), 0, 0x0000000000000001);
  __ Movi(v19.V2D(), 0, 0x80000001ffffffff);
  __ Movi(v20.V2D(), 0, 0x0000000100000001);

  __ Uaddl(v0.V8H(), v1.V8B(), v2.V8B());

  __ Uaddl(v3.V4S(), v5.V4H(), v7.V4H());
  __ Uaddl(v4.V4S(), v6.V4H(), v7.V4H());

  __ Uaddl(v16.V2D(), v18.V2S(), v20.V2S());
  __ Uaddl(v17.V2D(), v19.V2S(), v20.V2S());


  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0001000200030081, 0x0082000f00100100, q0);
    ASSERT_EQUAL_128(0x0000000100000002, 0x0000800100008002, q3);
    ASSERT_EQUAL_128(0x0000000f00000010, 0x0000f00100010000, q4);
    ASSERT_EQUAL_128(0x0000000000000001, 0x0000000000000002, q16);
    ASSERT_EQUAL_128(0x0000000080000002, 0x0000000100000000, q17);
  }
}


TEST(neon_3different_addhn_subhn) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Addhn(v16.V8B(), v0.V8H(), v1.V8H());
  __ Addhn2(v16.V16B(), v2.V8H(), v3.V8H());
  __ Raddhn(v17.V8B(), v0.V8H(), v1.V8H());
  __ Raddhn2(v17.V16B(), v2.V8H(), v3.V8H());
  __ Subhn(v18.V8B(), v0.V8H(), v1.V8H());
  __ Subhn2(v18.V16B(), v2.V8H(), v3.V8H());
  __ Rsubhn(v19.V8B(), v0.V8H(), v1.V8H());
  __ Rsubhn2(v19.V16B(), v2.V8H(), v3.V8H());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000ff007fff7fff, 0xff81817f80ff0100, q16);
    ASSERT_EQUAL_128(0x0000000080008000, 0xff81817f81ff0201, q17);
    ASSERT_EQUAL_128(0x0000ffff80008000, 0xff80817f80ff0100, q18);
    ASSERT_EQUAL_128(0x0000000080008000, 0xff81827f81ff0101, q19);
  }
}

TEST(neon_d_only_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaaaaaaaaaa, 0xf0000000f000f0f0);
  __ Movi(v1.V2D(), 0x5555555555555555, 0x7fffffff7fff7f7f);
  __ Movi(v2.V2D(), 0xaaaaaaaaaaaaaaaa, 0x1000000010001010);
  __ Movi(v3.V2D(), 0xffffffffffffffff, 2);
  __ Movi(v4.V2D(), 0xffffffffffffffff, -2);

  __ Add(d16, d0, d0);
  __ Add(d17, d1, d1);
  __ Add(d18, d2, d2);
  __ Sub(d19, d0, d0);
  __ Sub(d20, d0, d1);
  __ Sub(d21, d1, d0);
  __ Ushl(d22, d0, d3);
  __ Ushl(d23, d0, d4);
  __ Sshl(d24, d0, d3);
  __ Sshl(d25, d0, d4);
  __ Ushr(d26, d0, 1);
  __ Sshr(d27, d0, 3);
  __ Shl(d28, d0, 0);
  __ Shl(d29, d0, 16);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0xe0000001e001e1e0, q16);
    ASSERT_EQUAL_128(0, 0xfffffffefffefefe, q17);
    ASSERT_EQUAL_128(0, 0x2000000020002020, q18);
    ASSERT_EQUAL_128(0, 0, q19);
    ASSERT_EQUAL_128(0, 0x7000000170017171, q20);
    ASSERT_EQUAL_128(0, 0x8ffffffe8ffe8e8f, q21);
    ASSERT_EQUAL_128(0, 0xc0000003c003c3c0, q22);
    ASSERT_EQUAL_128(0, 0x3c0000003c003c3c, q23);
    ASSERT_EQUAL_128(0, 0xc0000003c003c3c0, q24);
    ASSERT_EQUAL_128(0, 0xfc0000003c003c3c, q25);
    ASSERT_EQUAL_128(0, 0x7800000078007878, q26);
    ASSERT_EQUAL_128(0, 0xfe0000001e001e1e, q27);
    ASSERT_EQUAL_128(0, 0xf0000000f000f0f0, q28);
    ASSERT_EQUAL_128(0, 0x0000f000f0f00000, q29);
  }
}


TEST(neon_sqshl_imm_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0, 0x7f);
  __ Movi(v1.V2D(), 0x0, 0x80);
  __ Movi(v2.V2D(), 0x0, 0x01);
  __ Sqshl(b16, b0, 1);
  __ Sqshl(b17, b1, 1);
  __ Sqshl(b18, b2, 1);

  __ Movi(v0.V2D(), 0x0, 0x7fff);
  __ Movi(v1.V2D(), 0x0, 0x8000);
  __ Movi(v2.V2D(), 0x0, 0x0001);
  __ Sqshl(h19, h0, 1);
  __ Sqshl(h20, h1, 1);
  __ Sqshl(h21, h2, 1);

  __ Movi(v0.V2D(), 0x0, 0x7fffffff);
  __ Movi(v1.V2D(), 0x0, 0x80000000);
  __ Movi(v2.V2D(), 0x0, 0x00000001);
  __ Sqshl(s22, s0, 1);
  __ Sqshl(s23, s1, 1);
  __ Sqshl(s24, s2, 1);

  __ Movi(v0.V2D(), 0x0, 0x7fffffffffffffff);
  __ Movi(v1.V2D(), 0x0, 0x8000000000000000);
  __ Movi(v2.V2D(), 0x0, 0x0000000000000001);
  __ Sqshl(d25, d0, 1);
  __ Sqshl(d26, d1, 1);
  __ Sqshl(d27, d2, 1);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x7f, q16);
    ASSERT_EQUAL_128(0, 0x80, q17);
    ASSERT_EQUAL_128(0, 0x02, q18);

    ASSERT_EQUAL_128(0, 0x7fff, q19);
    ASSERT_EQUAL_128(0, 0x8000, q20);
    ASSERT_EQUAL_128(0, 0x0002, q21);

    ASSERT_EQUAL_128(0, 0x7fffffff, q22);
    ASSERT_EQUAL_128(0, 0x80000000, q23);
    ASSERT_EQUAL_128(0, 0x00000002, q24);

    ASSERT_EQUAL_128(0, 0x7fffffffffffffff, q25);
    ASSERT_EQUAL_128(0, 0x8000000000000000, q26);
    ASSERT_EQUAL_128(0, 0x0000000000000002, q27);
  }
}


TEST(neon_uqshl_imm_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0, 0x7f);
  __ Movi(v1.V2D(), 0x0, 0x80);
  __ Movi(v2.V2D(), 0x0, 0x01);
  __ Uqshl(b16, b0, 1);
  __ Uqshl(b17, b1, 1);
  __ Uqshl(b18, b2, 1);

  __ Movi(v0.V2D(), 0x0, 0x7fff);
  __ Movi(v1.V2D(), 0x0, 0x8000);
  __ Movi(v2.V2D(), 0x0, 0x0001);
  __ Uqshl(h19, h0, 1);
  __ Uqshl(h20, h1, 1);
  __ Uqshl(h21, h2, 1);

  __ Movi(v0.V2D(), 0x0, 0x7fffffff);
  __ Movi(v1.V2D(), 0x0, 0x80000000);
  __ Movi(v2.V2D(), 0x0, 0x00000001);
  __ Uqshl(s22, s0, 1);
  __ Uqshl(s23, s1, 1);
  __ Uqshl(s24, s2, 1);

  __ Movi(v0.V2D(), 0x0, 0x7fffffffffffffff);
  __ Movi(v1.V2D(), 0x0, 0x8000000000000000);
  __ Movi(v2.V2D(), 0x0, 0x0000000000000001);
  __ Uqshl(d25, d0, 1);
  __ Uqshl(d26, d1, 1);
  __ Uqshl(d27, d2, 1);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0xfe, q16);
    ASSERT_EQUAL_128(0, 0xff, q17);
    ASSERT_EQUAL_128(0, 0x02, q18);

    ASSERT_EQUAL_128(0, 0xfffe, q19);
    ASSERT_EQUAL_128(0, 0xffff, q20);
    ASSERT_EQUAL_128(0, 0x0002, q21);

    ASSERT_EQUAL_128(0, 0xfffffffe, q22);
    ASSERT_EQUAL_128(0, 0xffffffff, q23);
    ASSERT_EQUAL_128(0, 0x00000002, q24);

    ASSERT_EQUAL_128(0, 0xfffffffffffffffe, q25);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q26);
    ASSERT_EQUAL_128(0, 0x0000000000000002, q27);
  }
}


TEST(neon_sqshlu_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x0, 0x7f);
  __ Movi(v1.V2D(), 0x0, 0x80);
  __ Movi(v2.V2D(), 0x0, 0x01);
  __ Sqshlu(b16, b0, 2);
  __ Sqshlu(b17, b1, 2);
  __ Sqshlu(b18, b2, 2);

  __ Movi(v0.V2D(), 0x0, 0x7fff);
  __ Movi(v1.V2D(), 0x0, 0x8000);
  __ Movi(v2.V2D(), 0x0, 0x0001);
  __ Sqshlu(h19, h0, 2);
  __ Sqshlu(h20, h1, 2);
  __ Sqshlu(h21, h2, 2);

  __ Movi(v0.V2D(), 0x0, 0x7fffffff);
  __ Movi(v1.V2D(), 0x0, 0x80000000);
  __ Movi(v2.V2D(), 0x0, 0x00000001);
  __ Sqshlu(s22, s0, 2);
  __ Sqshlu(s23, s1, 2);
  __ Sqshlu(s24, s2, 2);

  __ Movi(v0.V2D(), 0x0, 0x7fffffffffffffff);
  __ Movi(v1.V2D(), 0x0, 0x8000000000000000);
  __ Movi(v2.V2D(), 0x0, 0x0000000000000001);
  __ Sqshlu(d25, d0, 2);
  __ Sqshlu(d26, d1, 2);
  __ Sqshlu(d27, d2, 2);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0xff, q16);
    ASSERT_EQUAL_128(0, 0x00, q17);
    ASSERT_EQUAL_128(0, 0x04, q18);

    ASSERT_EQUAL_128(0, 0xffff, q19);
    ASSERT_EQUAL_128(0, 0x0000, q20);
    ASSERT_EQUAL_128(0, 0x0004, q21);

    ASSERT_EQUAL_128(0, 0xffffffff, q22);
    ASSERT_EQUAL_128(0, 0x00000000, q23);
    ASSERT_EQUAL_128(0, 0x00000004, q24);

    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q25);
    ASSERT_EQUAL_128(0, 0x0000000000000000, q26);
    ASSERT_EQUAL_128(0, 0x0000000000000004, q27);
  }
}


TEST(neon_sshll) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);

  __ Sshll(v16.V8H(), v0.V8B(), 4);
  __ Sshll2(v17.V8H(), v0.V16B(), 4);

  __ Sshll(v18.V4S(), v1.V4H(), 8);
  __ Sshll2(v19.V4S(), v1.V8H(), 8);

  __ Sshll(v20.V2D(), v2.V2S(), 16);
  __ Sshll2(v21.V2D(), v2.V4S(), 16);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xf800f810fff00000, 0x001007f0f800f810, q16);
    ASSERT_EQUAL_128(0x07f000100000fff0, 0xf810f80007f00010, q17);
    ASSERT_EQUAL_128(0xffffff0000000000, 0x00000100007fff00, q18);
    ASSERT_EQUAL_128(0xff800000ff800100, 0xffffff0000000000, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00007fffffff0000, q20);
    ASSERT_EQUAL_128(0xffff800000000000, 0xffffffffffff0000, q21);
  }
}

TEST(neon_shll) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);

  __ Shll(v16.V8H(), v0.V8B(), 8);
  __ Shll2(v17.V8H(), v0.V16B(), 8);

  __ Shll(v18.V4S(), v1.V4H(), 16);
  __ Shll2(v19.V4S(), v1.V8H(), 16);

  __ Shll(v20.V2D(), v2.V2S(), 32);
  __ Shll2(v21.V2D(), v2.V4S(), 32);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x80008100ff000000, 0x01007f0080008100, q16);
    ASSERT_EQUAL_128(0x7f0001000000ff00, 0x810080007f000100, q17);
    ASSERT_EQUAL_128(0xffff000000000000, 0x000100007fff0000, q18);
    ASSERT_EQUAL_128(0x8000000080010000, 0xffff000000000000, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x7fffffff00000000, q20);
    ASSERT_EQUAL_128(0x8000000000000000, 0xffffffff00000000, q21);
  }
}

TEST(neon_ushll) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);

  __ Ushll(v16.V8H(), v0.V8B(), 4);
  __ Ushll2(v17.V8H(), v0.V16B(), 4);

  __ Ushll(v18.V4S(), v1.V4H(), 8);
  __ Ushll2(v19.V4S(), v1.V8H(), 8);

  __ Ushll(v20.V2D(), v2.V2S(), 16);
  __ Ushll2(v21.V2D(), v2.V4S(), 16);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x080008100ff00000, 0x001007f008000810, q16);
    ASSERT_EQUAL_128(0x07f0001000000ff0, 0x0810080007f00010, q17);
    ASSERT_EQUAL_128(0x00ffff0000000000, 0x00000100007fff00, q18);
    ASSERT_EQUAL_128(0x0080000000800100, 0x00ffff0000000000, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00007fffffff0000, q20);
    ASSERT_EQUAL_128(0x0000800000000000, 0x0000ffffffff0000, q21);
  }
}


TEST(neon_sxtl) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);

  __ Sxtl(v16.V8H(), v0.V8B());
  __ Sxtl2(v17.V8H(), v0.V16B());

  __ Sxtl(v18.V4S(), v1.V4H());
  __ Sxtl2(v19.V4S(), v1.V8H());

  __ Sxtl(v20.V2D(), v2.V2S());
  __ Sxtl2(v21.V2D(), v2.V4S());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xff80ff81ffff0000, 0x0001007fff80ff81, q16);
    ASSERT_EQUAL_128(0x007f00010000ffff, 0xff81ff80007f0001, q17);
    ASSERT_EQUAL_128(0xffffffff00000000, 0x0000000100007fff, q18);
    ASSERT_EQUAL_128(0xffff8000ffff8001, 0xffffffff00000000, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000007fffffff, q20);
    ASSERT_EQUAL_128(0xffffffff80000000, 0xffffffffffffffff, q21);
  }
}


TEST(neon_uxtl) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);

  __ Uxtl(v16.V8H(), v0.V8B());
  __ Uxtl2(v17.V8H(), v0.V16B());

  __ Uxtl(v18.V4S(), v1.V4H());
  __ Uxtl2(v19.V4S(), v1.V8H());

  __ Uxtl(v20.V2D(), v2.V2S());
  __ Uxtl2(v21.V2D(), v2.V4S());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0080008100ff0000, 0x0001007f00800081, q16);
    ASSERT_EQUAL_128(0x007f0001000000ff, 0x00810080007f0001, q17);
    ASSERT_EQUAL_128(0x0000ffff00000000, 0x0000000100007fff, q18);
    ASSERT_EQUAL_128(0x0000800000008001, 0x0000ffff00000000, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x000000007fffffff, q20);
    ASSERT_EQUAL_128(0x0000000080000000, 0x00000000ffffffff, q21);
  }
}


TEST(neon_ssra) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Mov(v16.V2D(), v0.V2D());
  __ Mov(v17.V2D(), v0.V2D());
  __ Mov(v18.V2D(), v1.V2D());
  __ Mov(v19.V2D(), v1.V2D());
  __ Mov(v20.V2D(), v2.V2D());
  __ Mov(v21.V2D(), v2.V2D());
  __ Mov(v22.V2D(), v3.V2D());
  __ Mov(v23.V2D(), v4.V2D());
  __ Mov(v24.V2D(), v3.V2D());
  __ Mov(v25.V2D(), v4.V2D());

  __ Ssra(v16.V8B(), v0.V8B(), 4);
  __ Ssra(v17.V16B(), v0.V16B(), 4);

  __ Ssra(v18.V4H(), v1.V4H(), 8);
  __ Ssra(v19.V8H(), v1.V8H(), 8);

  __ Ssra(v20.V2S(), v2.V2S(), 16);
  __ Ssra(v21.V4S(), v2.V4S(), 16);

  __ Ssra(v22.V2D(), v3.V2D(), 32);
  __ Ssra(v23.V2D(), v4.V2D(), 32);

  __ Ssra(d24, d3, 48);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x7879fe0001867879, q16);
    ASSERT_EQUAL_128(0x860100fe79788601, 0x7879fe0001867879, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0xfffe00000001807e, q18);
    ASSERT_EQUAL_128(0x7f807f81fffe0000, 0xfffe00000001807e, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000080007ffe, q20);
    ASSERT_EQUAL_128(0x7fff8000fffffffe, 0x0000000080007ffe, q21);
    ASSERT_EQUAL_128(0x7fffffff80000001, 0x800000007ffffffe, q22);
    ASSERT_EQUAL_128(0x7fffffff80000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0x8000000000007ffe, q24);
  }
}

TEST(neon_srsra) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Mov(v16.V2D(), v0.V2D());
  __ Mov(v17.V2D(), v0.V2D());
  __ Mov(v18.V2D(), v1.V2D());
  __ Mov(v19.V2D(), v1.V2D());
  __ Mov(v20.V2D(), v2.V2D());
  __ Mov(v21.V2D(), v2.V2D());
  __ Mov(v22.V2D(), v3.V2D());
  __ Mov(v23.V2D(), v4.V2D());
  __ Mov(v24.V2D(), v3.V2D());
  __ Mov(v25.V2D(), v4.V2D());

  __ Srsra(v16.V8B(), v0.V8B(), 4);
  __ Srsra(v17.V16B(), v0.V16B(), 4);

  __ Srsra(v18.V4H(), v1.V4H(), 8);
  __ Srsra(v19.V8H(), v1.V8H(), 8);

  __ Srsra(v20.V2S(), v2.V2S(), 16);
  __ Srsra(v21.V4S(), v2.V4S(), 16);

  __ Srsra(v22.V2D(), v3.V2D(), 32);
  __ Srsra(v23.V2D(), v4.V2D(), 32);

  __ Srsra(d24, d3, 48);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x7879ff0001877879, q16);
    ASSERT_EQUAL_128(0x870100ff79788701, 0x7879ff0001877879, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0xffff00000001807f, q18);
    ASSERT_EQUAL_128(0x7f807f81ffff0000, 0xffff00000001807f, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000080007fff, q20);
    ASSERT_EQUAL_128(0x7fff8000ffffffff, 0x0000000080007fff, q21);
    ASSERT_EQUAL_128(0x7fffffff80000001, 0x800000007fffffff, q22);
    ASSERT_EQUAL_128(0x7fffffff80000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0x8000000000007fff, q24);
  }
}

TEST(neon_usra) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Mov(v16.V2D(), v0.V2D());
  __ Mov(v17.V2D(), v0.V2D());
  __ Mov(v18.V2D(), v1.V2D());
  __ Mov(v19.V2D(), v1.V2D());
  __ Mov(v20.V2D(), v2.V2D());
  __ Mov(v21.V2D(), v2.V2D());
  __ Mov(v22.V2D(), v3.V2D());
  __ Mov(v23.V2D(), v4.V2D());
  __ Mov(v24.V2D(), v3.V2D());
  __ Mov(v25.V2D(), v4.V2D());

  __ Usra(v16.V8B(), v0.V8B(), 4);
  __ Usra(v17.V16B(), v0.V16B(), 4);

  __ Usra(v18.V4H(), v1.V4H(), 8);
  __ Usra(v19.V8H(), v1.V8H(), 8);

  __ Usra(v20.V2S(), v2.V2S(), 16);
  __ Usra(v21.V4S(), v2.V4S(), 16);

  __ Usra(v22.V2D(), v3.V2D(), 32);
  __ Usra(v23.V2D(), v4.V2D(), 32);

  __ Usra(d24, d3, 48);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x88890e0001868889, q16);
    ASSERT_EQUAL_128(0x8601000e89888601, 0x88890e0001868889, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00fe00000001807e, q18);
    ASSERT_EQUAL_128(0x8080808100fe0000, 0x00fe00000001807e, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000080007ffe, q20);
    ASSERT_EQUAL_128(0x800080000000fffe, 0x0000000080007ffe, q21);
    ASSERT_EQUAL_128(0x8000000080000001, 0x800000007ffffffe, q22);
    ASSERT_EQUAL_128(0x8000000080000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0x8000000000007ffe, q24);
  }
}

TEST(neon_ursra) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0x7f0100ff81807f01, 0x8081ff00017f8081);
  __ Movi(v1.V2D(), 0x80008001ffff0000, 0xffff000000017fff);
  __ Movi(v2.V2D(), 0x80000000ffffffff, 0x000000007fffffff);
  __ Movi(v3.V2D(), 0x8000000000000001, 0x7fffffffffffffff);
  __ Movi(v4.V2D(), 0x8000000000000000, 0x0000000000000000);

  __ Mov(v16.V2D(), v0.V2D());
  __ Mov(v17.V2D(), v0.V2D());
  __ Mov(v18.V2D(), v1.V2D());
  __ Mov(v19.V2D(), v1.V2D());
  __ Mov(v20.V2D(), v2.V2D());
  __ Mov(v21.V2D(), v2.V2D());
  __ Mov(v22.V2D(), v3.V2D());
  __ Mov(v23.V2D(), v4.V2D());
  __ Mov(v24.V2D(), v3.V2D());
  __ Mov(v25.V2D(), v4.V2D());

  __ Ursra(v16.V8B(), v0.V8B(), 4);
  __ Ursra(v17.V16B(), v0.V16B(), 4);

  __ Ursra(v18.V4H(), v1.V4H(), 8);
  __ Ursra(v19.V8H(), v1.V8H(), 8);

  __ Ursra(v20.V2S(), v2.V2S(), 16);
  __ Ursra(v21.V4S(), v2.V4S(), 16);

  __ Ursra(v22.V2D(), v3.V2D(), 32);
  __ Ursra(v23.V2D(), v4.V2D(), 32);

  __ Ursra(d24, d3, 48);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x88890f0001878889, q16);
    ASSERT_EQUAL_128(0x8701000f89888701, 0x88890f0001878889, q17);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00ff00000001807f, q18);
    ASSERT_EQUAL_128(0x8080808100ff0000, 0x00ff00000001807f, q19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000080007fff, q20);
    ASSERT_EQUAL_128(0x800080000000ffff, 0x0000000080007fff, q21);
    ASSERT_EQUAL_128(0x8000000080000001, 0x800000007fffffff, q22);
    ASSERT_EQUAL_128(0x8000000080000000, 0x0000000000000000, q23);
    ASSERT_EQUAL_128(0x0000000000000000, 0x8000000000007fff, q24);
  }
}


TEST(neon_uqshl_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaaaaaaaaaa, 0xf0000000f000f0f0);
  __ Movi(v1.V2D(), 0x5555555555555555, 0x7fffffff7fff7f7f);
  __ Movi(v2.V2D(), 0xaaaaaaaaaaaaaaaa, 0x0000000000000001);
  __ Movi(v3.V2D(), 0xaaaaaaaaaaaaaaaa, 0xffffffffffffffff);

  __ Uqshl(b16, b0, b2);
  __ Uqshl(b17, b0, b3);
  __ Uqshl(b18, b1, b2);
  __ Uqshl(b19, b1, b3);
  __ Uqshl(h20, h0, h2);
  __ Uqshl(h21, h0, h3);
  __ Uqshl(h22, h1, h2);
  __ Uqshl(h23, h1, h3);
  __ Uqshl(s24, s0, s2);
  __ Uqshl(s25, s0, s3);
  __ Uqshl(s26, s1, s2);
  __ Uqshl(s27, s1, s3);
  __ Uqshl(d28, d0, d2);
  __ Uqshl(d29, d0, d3);
  __ Uqshl(d30, d1, d2);
  __ Uqshl(d31, d1, d3);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0xff, q16);
    ASSERT_EQUAL_128(0, 0x78, q17);
    ASSERT_EQUAL_128(0, 0xfe, q18);
    ASSERT_EQUAL_128(0, 0x3f, q19);
    ASSERT_EQUAL_128(0, 0xffff, q20);
    ASSERT_EQUAL_128(0, 0x7878, q21);
    ASSERT_EQUAL_128(0, 0xfefe, q22);
    ASSERT_EQUAL_128(0, 0x3fbf, q23);
    ASSERT_EQUAL_128(0, 0xffffffff, q24);
    ASSERT_EQUAL_128(0, 0x78007878, q25);
    ASSERT_EQUAL_128(0, 0xfffefefe, q26);
    ASSERT_EQUAL_128(0, 0x3fffbfbf, q27);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q28);
    ASSERT_EQUAL_128(0, 0x7800000078007878, q29);
    ASSERT_EQUAL_128(0, 0xfffffffefffefefe, q30);
    ASSERT_EQUAL_128(0, 0x3fffffffbfffbfbf, q31);
  }
}


TEST(neon_sqshl_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaaaaaaaaaa, 0xbfffffffbfffbfbf);
  __ Movi(v1.V2D(), 0x5555555555555555, 0x4000000040004040);
  __ Movi(v2.V2D(), 0xaaaaaaaaaaaaaaaa, 0x0000000000000001);
  __ Movi(v3.V2D(), 0xaaaaaaaaaaaaaaaa, 0xffffffffffffffff);

  __ Sqshl(b16, b0, b2);
  __ Sqshl(b17, b0, b3);
  __ Sqshl(b18, b1, b2);
  __ Sqshl(b19, b1, b3);
  __ Sqshl(h20, h0, h2);
  __ Sqshl(h21, h0, h3);
  __ Sqshl(h22, h1, h2);
  __ Sqshl(h23, h1, h3);
  __ Sqshl(s24, s0, s2);
  __ Sqshl(s25, s0, s3);
  __ Sqshl(s26, s1, s2);
  __ Sqshl(s27, s1, s3);
  __ Sqshl(d28, d0, d2);
  __ Sqshl(d29, d0, d3);
  __ Sqshl(d30, d1, d2);
  __ Sqshl(d31, d1, d3);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x80, q16);
    ASSERT_EQUAL_128(0, 0xdf, q17);
    ASSERT_EQUAL_128(0, 0x7f, q18);
    ASSERT_EQUAL_128(0, 0x20, q19);
    ASSERT_EQUAL_128(0, 0x8000, q20);
    ASSERT_EQUAL_128(0, 0xdfdf, q21);
    ASSERT_EQUAL_128(0, 0x7fff, q22);
    ASSERT_EQUAL_128(0, 0x2020, q23);
    ASSERT_EQUAL_128(0, 0x80000000, q24);
    ASSERT_EQUAL_128(0, 0xdfffdfdf, q25);
    ASSERT_EQUAL_128(0, 0x7fffffff, q26);
    ASSERT_EQUAL_128(0, 0x20002020, q27);
    ASSERT_EQUAL_128(0, 0x8000000000000000, q28);
    ASSERT_EQUAL_128(0, 0xdfffffffdfffdfdf, q29);
    ASSERT_EQUAL_128(0, 0x7fffffffffffffff, q30);
    ASSERT_EQUAL_128(0, 0x2000000020002020, q31);
  }
}


TEST(neon_urshl_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaaaaaaaaaa, 0xf0000000f000f0f0);
  __ Movi(v1.V2D(), 0x5555555555555555, 0x7fffffff7fff7f7f);
  __ Movi(v2.V2D(), 0xaaaaaaaaaaaaaaaa, 0x0000000000000001);
  __ Movi(v3.V2D(), 0xaaaaaaaaaaaaaaaa, 0xffffffffffffffff);

  __ Urshl(d28, d0, d2);
  __ Urshl(d29, d0, d3);
  __ Urshl(d30, d1, d2);
  __ Urshl(d31, d1, d3);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0xe0000001e001e1e0, q28);
    ASSERT_EQUAL_128(0, 0x7800000078007878, q29);
    ASSERT_EQUAL_128(0, 0xfffffffefffefefe, q30);
    ASSERT_EQUAL_128(0, 0x3fffffffbfffbfc0, q31);
  }
}


TEST(neon_srshl_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaaaaaaaaaa, 0xbfffffffbfffbfbf);
  __ Movi(v1.V2D(), 0x5555555555555555, 0x4000000040004040);
  __ Movi(v2.V2D(), 0xaaaaaaaaaaaaaaaa, 0x0000000000000001);
  __ Movi(v3.V2D(), 0xaaaaaaaaaaaaaaaa, 0xffffffffffffffff);

  __ Srshl(d28, d0, d2);
  __ Srshl(d29, d0, d3);
  __ Srshl(d30, d1, d2);
  __ Srshl(d31, d1, d3);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x7fffffff7fff7f7e, q28);
    ASSERT_EQUAL_128(0, 0xdfffffffdfffdfe0, q29);
    ASSERT_EQUAL_128(0, 0x8000000080008080, q30);
    ASSERT_EQUAL_128(0, 0x2000000020002020, q31);
  }
}


TEST(neon_uqrshl_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaaaaaaaaaa, 0xf0000000f000f0f0);
  __ Movi(v1.V2D(), 0x5555555555555555, 0x7fffffff7fff7f7f);
  __ Movi(v2.V2D(), 0xaaaaaaaaaaaaaaaa, 0x0000000000000001);
  __ Movi(v3.V2D(), 0xaaaaaaaaaaaaaaaa, 0xffffffffffffffff);

  __ Uqrshl(b16, b0, b2);
  __ Uqrshl(b17, b0, b3);
  __ Uqrshl(b18, b1, b2);
  __ Uqrshl(b19, b1, b3);
  __ Uqrshl(h20, h0, h2);
  __ Uqrshl(h21, h0, h3);
  __ Uqrshl(h22, h1, h2);
  __ Uqrshl(h23, h1, h3);
  __ Uqrshl(s24, s0, s2);
  __ Uqrshl(s25, s0, s3);
  __ Uqrshl(s26, s1, s2);
  __ Uqrshl(s27, s1, s3);
  __ Uqrshl(d28, d0, d2);
  __ Uqrshl(d29, d0, d3);
  __ Uqrshl(d30, d1, d2);
  __ Uqrshl(d31, d1, d3);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0xff, q16);
    ASSERT_EQUAL_128(0, 0x78, q17);
    ASSERT_EQUAL_128(0, 0xfe, q18);
    ASSERT_EQUAL_128(0, 0x40, q19);
    ASSERT_EQUAL_128(0, 0xffff, q20);
    ASSERT_EQUAL_128(0, 0x7878, q21);
    ASSERT_EQUAL_128(0, 0xfefe, q22);
    ASSERT_EQUAL_128(0, 0x3fc0, q23);
    ASSERT_EQUAL_128(0, 0xffffffff, q24);
    ASSERT_EQUAL_128(0, 0x78007878, q25);
    ASSERT_EQUAL_128(0, 0xfffefefe, q26);
    ASSERT_EQUAL_128(0, 0x3fffbfc0, q27);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q28);
    ASSERT_EQUAL_128(0, 0x7800000078007878, q29);
    ASSERT_EQUAL_128(0, 0xfffffffefffefefe, q30);
    ASSERT_EQUAL_128(0, 0x3fffffffbfffbfc0, q31);
  }
}


TEST(neon_sqrshl_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaaaaaaaaaa, 0xbfffffffbfffbfbf);
  __ Movi(v1.V2D(), 0x5555555555555555, 0x4000000040004040);
  __ Movi(v2.V2D(), 0xaaaaaaaaaaaaaaaa, 0x0000000000000001);
  __ Movi(v3.V2D(), 0xaaaaaaaaaaaaaaaa, 0xffffffffffffffff);

  __ Sqrshl(b16, b0, b2);
  __ Sqrshl(b17, b0, b3);
  __ Sqrshl(b18, b1, b2);
  __ Sqrshl(b19, b1, b3);
  __ Sqrshl(h20, h0, h2);
  __ Sqrshl(h21, h0, h3);
  __ Sqrshl(h22, h1, h2);
  __ Sqrshl(h23, h1, h3);
  __ Sqrshl(s24, s0, s2);
  __ Sqrshl(s25, s0, s3);
  __ Sqrshl(s26, s1, s2);
  __ Sqrshl(s27, s1, s3);
  __ Sqrshl(d28, d0, d2);
  __ Sqrshl(d29, d0, d3);
  __ Sqrshl(d30, d1, d2);
  __ Sqrshl(d31, d1, d3);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x80, q16);
    ASSERT_EQUAL_128(0, 0xe0, q17);
    ASSERT_EQUAL_128(0, 0x7f, q18);
    ASSERT_EQUAL_128(0, 0x20, q19);
    ASSERT_EQUAL_128(0, 0x8000, q20);
    ASSERT_EQUAL_128(0, 0xdfe0, q21);
    ASSERT_EQUAL_128(0, 0x7fff, q22);
    ASSERT_EQUAL_128(0, 0x2020, q23);
    ASSERT_EQUAL_128(0, 0x80000000, q24);
    ASSERT_EQUAL_128(0, 0xdfffdfe0, q25);
    ASSERT_EQUAL_128(0, 0x7fffffff, q26);
    ASSERT_EQUAL_128(0, 0x20002020, q27);
    ASSERT_EQUAL_128(0, 0x8000000000000000, q28);
    ASSERT_EQUAL_128(0, 0xdfffffffdfffdfe0, q29);
    ASSERT_EQUAL_128(0, 0x7fffffffffffffff, q30);
    ASSERT_EQUAL_128(0, 0x2000000020002020, q31);
  }
}


TEST(neon_uqadd_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaaaaaaaaaa, 0xf0000000f000f0f0);
  __ Movi(v1.V2D(), 0x5555555555555555, 0x7fffffff7fff7f7f);
  __ Movi(v2.V2D(), 0xaaaaaaaaaaaaaaaa, 0x1000000010001010);

  __ Uqadd(b16, b0, b0);
  __ Uqadd(b17, b1, b1);
  __ Uqadd(b18, b2, b2);
  __ Uqadd(h19, h0, h0);
  __ Uqadd(h20, h1, h1);
  __ Uqadd(h21, h2, h2);
  __ Uqadd(s22, s0, s0);
  __ Uqadd(s23, s1, s1);
  __ Uqadd(s24, s2, s2);
  __ Uqadd(d25, d0, d0);
  __ Uqadd(d26, d1, d1);
  __ Uqadd(d27, d2, d2);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0xff, q16);
    ASSERT_EQUAL_128(0, 0xfe, q17);
    ASSERT_EQUAL_128(0, 0x20, q18);
    ASSERT_EQUAL_128(0, 0xffff, q19);
    ASSERT_EQUAL_128(0, 0xfefe, q20);
    ASSERT_EQUAL_128(0, 0x2020, q21);
    ASSERT_EQUAL_128(0, 0xffffffff, q22);
    ASSERT_EQUAL_128(0, 0xfffefefe, q23);
    ASSERT_EQUAL_128(0, 0x20002020, q24);
    ASSERT_EQUAL_128(0, 0xffffffffffffffff, q25);
    ASSERT_EQUAL_128(0, 0xfffffffefffefefe, q26);
    ASSERT_EQUAL_128(0, 0x2000000020002020, q27);
  }
}


TEST(neon_sqadd_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaaaaaaaaaa, 0x8000000180018181);
  __ Movi(v1.V2D(), 0x5555555555555555, 0x7fffffff7fff7f7f);
  __ Movi(v2.V2D(), 0xaaaaaaaaaaaaaaaa, 0x1000000010001010);

  __ Sqadd(b16, b0, b0);
  __ Sqadd(b17, b1, b1);
  __ Sqadd(b18, b2, b2);
  __ Sqadd(h19, h0, h0);
  __ Sqadd(h20, h1, h1);
  __ Sqadd(h21, h2, h2);
  __ Sqadd(s22, s0, s0);
  __ Sqadd(s23, s1, s1);
  __ Sqadd(s24, s2, s2);
  __ Sqadd(d25, d0, d0);
  __ Sqadd(d26, d1, d1);
  __ Sqadd(d27, d2, d2);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0x80, q16);
    ASSERT_EQUAL_128(0, 0x7f, q17);
    ASSERT_EQUAL_128(0, 0x20, q18);
    ASSERT_EQUAL_128(0, 0x8000, q19);
    ASSERT_EQUAL_128(0, 0x7fff, q20);
    ASSERT_EQUAL_128(0, 0x2020, q21);
    ASSERT_EQUAL_128(0, 0x80000000, q22);
    ASSERT_EQUAL_128(0, 0x7fffffff, q23);
    ASSERT_EQUAL_128(0, 0x20002020, q24);
    ASSERT_EQUAL_128(0, 0x8000000000000000, q25);
    ASSERT_EQUAL_128(0, 0x7fffffffffffffff, q26);
    ASSERT_EQUAL_128(0, 0x2000000020002020, q27);
  }
}


TEST(neon_uqsub_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaaaaaaaaaa, 0xf0000000f000f0f0);
  __ Movi(v1.V2D(), 0x5555555555555555, 0x7fffffff7fff7f7f);

  __ Uqsub(b16, b0, b0);
  __ Uqsub(b17, b0, b1);
  __ Uqsub(b18, b1, b0);
  __ Uqsub(h19, h0, h0);
  __ Uqsub(h20, h0, h1);
  __ Uqsub(h21, h1, h0);
  __ Uqsub(s22, s0, s0);
  __ Uqsub(s23, s0, s1);
  __ Uqsub(s24, s1, s0);
  __ Uqsub(d25, d0, d0);
  __ Uqsub(d26, d0, d1);
  __ Uqsub(d27, d1, d0);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0, q16);
    ASSERT_EQUAL_128(0, 0x71, q17);
    ASSERT_EQUAL_128(0, 0, q18);

    ASSERT_EQUAL_128(0, 0, q19);
    ASSERT_EQUAL_128(0, 0x7171, q20);
    ASSERT_EQUAL_128(0, 0, q21);

    ASSERT_EQUAL_128(0, 0, q22);
    ASSERT_EQUAL_128(0, 0x70017171, q23);
    ASSERT_EQUAL_128(0, 0, q24);

    ASSERT_EQUAL_128(0, 0, q25);
    ASSERT_EQUAL_128(0, 0x7000000170017171, q26);
    ASSERT_EQUAL_128(0, 0, q27);
  }
}


TEST(neon_sqsub_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();

  __ Movi(v0.V2D(), 0xaaaaaaaaaaaaaaaa, 0xf0000000f000f0f0);
  __ Movi(v1.V2D(), 0x5555555555555555, 0x7eeeeeee7eee7e7e);

  __ Sqsub(b16, b0, b0);
  __ Sqsub(b17, b0, b1);
  __ Sqsub(b18, b1, b0);
  __ Sqsub(h19, h0, h0);
  __ Sqsub(h20, h0, h1);
  __ Sqsub(h21, h1, h0);
  __ Sqsub(s22, s0, s0);
  __ Sqsub(s23, s0, s1);
  __ Sqsub(s24, s1, s0);
  __ Sqsub(d25, d0, d0);
  __ Sqsub(d26, d0, d1);
  __ Sqsub(d27, d1, d0);

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0, 0, q16);
    ASSERT_EQUAL_128(0, 0x80, q17);
    ASSERT_EQUAL_128(0, 0x7f, q18);

    ASSERT_EQUAL_128(0, 0, q19);
    ASSERT_EQUAL_128(0, 0x8000, q20);
    ASSERT_EQUAL_128(0, 0x7fff, q21);

    ASSERT_EQUAL_128(0, 0, q22);
    ASSERT_EQUAL_128(0, 0x80000000, q23);
    ASSERT_EQUAL_128(0, 0x7fffffff, q24);

    ASSERT_EQUAL_128(0, 0, q25);
    ASSERT_EQUAL_128(0, 0x8000000000000000, q26);
    ASSERT_EQUAL_128(0, 0x7fffffffffffffff, q27);
  }
}


TEST(neon_fmla_fmls) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();
  __ Movi(v0.V2D(), 0x3f80000040000000, 0x4100000000000000);
  __ Movi(v1.V2D(), 0x400000003f800000, 0x000000003f800000);
  __ Movi(v2.V2D(), 0x3f800000ffffffff, 0x7f800000ff800000);
  __ Mov(v16.V16B(), v0.V16B());
  __ Mov(v17.V16B(), v0.V16B());
  __ Mov(v18.V16B(), v0.V16B());
  __ Mov(v19.V16B(), v0.V16B());
  __ Mov(v20.V16B(), v0.V16B());
  __ Mov(v21.V16B(), v0.V16B());

  __ Fmla(v16.V2S(), v1.V2S(), v2.V2S());
  __ Fmla(v17.V4S(), v1.V4S(), v2.V4S());
  __ Fmla(v18.V2D(), v1.V2D(), v2.V2D());
  __ Fmls(v19.V2S(), v1.V2S(), v2.V2S());
  __ Fmls(v20.V4S(), v1.V4S(), v2.V4S());
  __ Fmls(v21.V2D(), v1.V2D(), v2.V2D());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x7fc00000ff800000, q16);
    ASSERT_EQUAL_128(0x40400000ffffffff, 0x7fc00000ff800000, q17);
    ASSERT_EQUAL_128(0x3f9800015f8003f7, 0x41000000000000fe, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x7fc000007f800000, q19);
    ASSERT_EQUAL_128(0xbf800000ffffffff, 0x7fc000007f800000, q20);
    ASSERT_EQUAL_128(0xbf8000023f0007ee, 0x40fffffffffffe04, q21);
  }
}


TEST(neon_fmla_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v1.V2D(), 0x51a051a051a051a0, 0x51a051a051a051a0);
  __ Movi(v2.V2D(), 0x7c007c007c007c00, 0x7c007c007c007c00);
  __ Movi(v3.V2D(), 0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00);
  __ Movi(v4.V2D(), 0x7e007e007e007e00, 0x7e007e007e007e00);
  __ Movi(v5.V2D(), 0x7c017c017c017c01, 0x7c017c017c017c01);
  __ Movi(v6.V2D(), 0x0000000000000000, 0x0000000000000000);
  __ Mov(v16.V2D(), v0.V2D());
  __ Mov(v17.V2D(), v0.V2D());
  __ Mov(v18.V2D(), v4.V2D());
  __ Mov(v19.V2D(), v5.V2D());
  __ Mov(v20.V2D(), v0.V2D());
  __ Mov(v21.V2D(), v0.V2D());
  __ Mov(v22.V2D(), v4.V2D());
  __ Mov(v23.V2D(), v5.V2D());

  __ Fmla(v16.V8H(), v0.V8H(), v1.V8H());
  __ Fmla(v17.V8H(), v2.V8H(), v3.V8H());
  __ Fmla(v18.V8H(), v2.V8H(), v6.V8H());
  __ Fmla(v19.V8H(), v3.V8H(), v6.V8H());
  __ Fmla(v20.V4H(), v0.V4H(), v1.V4H());
  __ Fmla(v21.V4H(), v2.V4H(), v3.V4H());
  __ Fmla(v22.V4H(), v2.V4H(), v6.V4H());
  __ Fmla(v23.V4H(), v3.V4H(), v6.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x55c055c055c055c0, 0x55c055c055c055c0, v16);
    ASSERT_EQUAL_128(0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00, v17);
    ASSERT_EQUAL_128(0x7e007e007e007e00, 0x7e007e007e007e00, v18);
    ASSERT_EQUAL_128(0x7e017e017e017e01, 0x7e017e017e017e01, v19);
    ASSERT_EQUAL_128(0, 0x55c055c055c055c0, v20);
    ASSERT_EQUAL_128(0, 0xfc00fc00fc00fc00, v21);
    ASSERT_EQUAL_128(0, 0x7e007e007e007e00, v22);
    ASSERT_EQUAL_128(0, 0x7e017e017e017e01, v23);
  }
}


TEST(neon_fmls_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v1.V2D(), 0x51a051a051a051a0, 0x51a051a051a051a0);
  __ Movi(v2.V2D(), 0x7c007c007c007c00, 0x7c007c007c007c00);
  __ Movi(v3.V2D(), 0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00);
  __ Movi(v4.V2D(), 0x7e007e007e007e00, 0x7e007e007e007e00);
  __ Movi(v5.V2D(), 0x7c017c017c017c01, 0x7c017c017c017c01);
  __ Movi(v6.V2D(), 0x0000000000000000, 0x0000000000000000);
  __ Mov(v16.V2D(), v0.V2D());
  __ Mov(v17.V2D(), v0.V2D());
  __ Mov(v18.V2D(), v4.V2D());
  __ Mov(v19.V2D(), v5.V2D());
  __ Mov(v20.V2D(), v0.V2D());
  __ Mov(v21.V2D(), v0.V2D());
  __ Mov(v22.V2D(), v4.V2D());
  __ Mov(v23.V2D(), v5.V2D());

  __ Fmls(v16.V8H(), v0.V8H(), v1.V8H());
  __ Fmls(v17.V8H(), v2.V8H(), v3.V8H());
  __ Fmls(v18.V8H(), v2.V8H(), v6.V8H());
  __ Fmls(v19.V8H(), v3.V8H(), v6.V8H());
  __ Fmls(v20.V4H(), v0.V4H(), v1.V4H());
  __ Fmls(v21.V4H(), v2.V4H(), v3.V4H());
  __ Fmls(v22.V4H(), v2.V4H(), v6.V4H());
  __ Fmls(v23.V4H(), v3.V4H(), v6.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xd580d580d580d580, 0xd580d580d580d580, v16);
    ASSERT_EQUAL_128(0x7c007c007c007c00, 0x7c007c007c007c00, v17);
    ASSERT_EQUAL_128(0x7e007e007e007e00, 0x7e007e007e007e00, v18);
    ASSERT_EQUAL_128(0x7e017e017e017e01, 0x7e017e017e017e01, v19);
    ASSERT_EQUAL_128(0, 0xd580d580d580d580, v20);
    ASSERT_EQUAL_128(0, 0x7c007c007c007c00, v21);
    ASSERT_EQUAL_128(0, 0x7e007e007e007e00, v22);
    ASSERT_EQUAL_128(0, 0x7e017e017e017e01, v23);
  }
}


TEST(neon_fhm) {
  // Test basic operation of fmlal{2} and fmlsl{2}. The simulator tests have
  // more comprehensive input sets.
  SETUP_WITH_FEATURES(CPUFeatures::kFP,
                      CPUFeatures::kNEON,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFHM);

  START();
  // Test multiplications:
  //        v30                               v31
  //  [0]   65504 (max normal)          *     65504 (max normal)
  //  [1]   -1                          *     0
  //  [2]   2^-24 (min subnormal)       *     2^-24 (min subnormal)
  //  [3]   -2^-24 (min subnormal)      *     65504 (max normal)
  //  [4]   6.10e-5 (min normal)        *     0.99...
  //  [5]   0                           *     -0
  //  [6]   -0                          *     0
  //  [7]   -Inf                        *     -Inf
  __ Movi(v30.V8H(), 0xfc00800000000400, 0x80010001bc007bff);
  __ Movi(v31.V8H(), 0xfc00000080003bff, 0x7bff000100007bff);

  // Accumulators for use with Fmlal{2}:
  // v0.S[0] = 384
  // v0.S[1] = -0
  __ Movi(v0.V4S(), 0xdeadbeefdeadbeef, 0x8000000043c00000);
  // v1.S[0] = -(2^-48 + 2^-71)
  // v1.S[1] = 0
  __ Movi(v1.V4S(), 0xdeadbeefdeadbeef, 0x00000000a7800001);
  // v2.S[0] = 128
  // v2.S[1] = 0
  // v2.S[2] = 1
  // v2.S[3] = 1
  __ Movi(v2.V4S(), 0x3f8000003f800000, 0x0000000043000000);
  // v3.S[0] = 0
  // v3.S[1] = -0
  // v3.S[2] = -0
  // v3.S[3] = 0
  __ Movi(v3.V4S(), 0x0000000080000000, 0x8000000000000000);
  // For Fmlsl{2}, we simply negate the accumulators above so that the Fmlsl{2}
  // results are just the negation of the Fmlal{2} results.
  __ Fneg(v4.V4S(), v0.V4S());
  __ Fneg(v5.V4S(), v1.V4S());
  __ Fneg(v6.V4S(), v2.V4S());
  __ Fneg(v7.V4S(), v3.V4S());

  __ Fmlal(v0.V2S(), v30.V2H(), v31.V2H());
  __ Fmlal2(v1.V2S(), v30.V2H(), v31.V2H());
  __ Fmlal(v2.V4S(), v30.V4H(), v31.V4H());
  __ Fmlal2(v3.V4S(), v30.V4H(), v31.V4H());

  __ Fmlsl(v4.V2S(), v30.V2H(), v31.V2H());
  __ Fmlsl2(v5.V2S(), v30.V2H(), v31.V2H());
  __ Fmlsl(v6.V4S(), v30.V4H(), v31.V4H());
  __ Fmlsl2(v7.V4S(), v30.V4H(), v31.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    // Fmlal(2S)
    // v0.S[0] = 384 + (65504 * 65504) = 4290774528 (rounded from 4290774400)
    // v0.S[1] = -0 + (-1 * 0) = -0
    ASSERT_EQUAL_128(0x0000000000000000, 0x800000004f7fc006, v0);
    // Fmlal2(2S)
    // v1.S[0] = -(2^-48 + 2^-71) + (2^-24 * 2^-24) = -2^-71
    // v1.S[1] = 0 + (-2^-24 * 65504) = -0.003904...
    ASSERT_EQUAL_128(0x0000000000000000, 0xbb7fe0009c000000, v1);
    // Fmlal(4S)
    // v2.S[0] = 128 + (65504 * 65504) = 4290774016 (rounded from 4290774144)
    // v2.S[1] = 0 + (-1 * 0) = 0
    // v2.S[2] = 1 + (2^-24 * 2^-24) = 1 (rounded)
    // v2.S[3] = 1 + (-2^-24 * 65504) = 0.996...
    ASSERT_EQUAL_128(0x3f7f00203f800000, 0x000000004f7fc004, v2);
    // Fmlal2(4S)
    // v3.S[0] = 0 + (6.103516e-5 * 0.99...) = 6.100535e-5
    // v3.S[1] = -0 + (0 * -0) = -0
    // v3.S[2] = -0 + (-0 * 0) = -0
    // v3.S[3] = 0 + (-Inf * -Inf) = Inf
    ASSERT_EQUAL_128(0x7f80000080000000, 0x80000000387fe000, v3);

    // Fmlsl results are mostly the same, but negated.
    ASSERT_EQUAL_128(0x0000000000000000, 0x00000000cf7fc006, v4);
    ASSERT_EQUAL_128(0x0000000000000000, 0x3b7fe0001c000000, v5);
    // In this case: v6.S[1] = 0 - (0 * -0) = 0
    ASSERT_EQUAL_128(0xbf7f0020bf800000, 0x00000000cf7fc004, v6);
    ASSERT_EQUAL_128(0xff80000000000000, 0x00000000b87fe000, v7);
  }
}


TEST(neon_byelement_fhm) {
  // Test basic operation of fmlal{2} and fmlsl{2} (by element). The simulator
  // tests have more comprehensive input sets.
  SETUP_WITH_FEATURES(CPUFeatures::kFP,
                      CPUFeatures::kNEON,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFHM);

  START();
  // Set up multiplication inputs.
  //
  // v30.H[0] = 65504 (max normal)
  // v30.H[1] = -1
  // v30.H[2] = 2^-24 (min subnormal)
  // v30.H[3] = -2^-24 (min subnormal)
  // v30.H[4] = 6.10e-5 (min normal)
  // v30.H[5] = 0
  // v30.H[6] = -0
  // v30.H[7] = -Inf
  __ Movi(v30.V8H(), 0xfc00800000000400, 0x80010001bc007bff);

  // Each test instruction should only use one lane of vm, so set up unique
  // registers with poison values in other lanes. The poison NaN avoids the
  // default NaN (so it shouldn't be encountered accidentally), but is otherwise
  // arbitrary.
  VRegister poison = v29;
  __ Movi(v29.V8H(), 0x7f417f417f417f41, 0x7f417f417f417f41);
  // v31.H[0,2,4,...]: 0.9995117 (the value just below 1)
  // v31.H[1,3,5,...]: 1.000977 (the value just above 1)
  __ Movi(v31.V8H(), 0x3bff3c013bff3c01, 0x3bff3c013bff3c01);
  // Set up [v8,v15] as vm inputs.
  for (int i = 0; i <= 7; i++) {
    VRegister vm(i + 8);
    __ Mov(vm, poison);
    __ Ins(vm.V8H(), i, v31.V8H(), i);
  }

  // Accumulators for use with Fmlal{2}:
  // v0.S[0] = 2^-8
  // v0.S[1] = 1
  __ Movi(v0.V4S(), 0xdeadbeefdeadbeef, 0x3f8000003b800000);
  // v1.S[0] = -1.5 * 2^-49
  // v1.S[1] = 0
  __ Movi(v1.V4S(), 0xdeadbeefdeadbeef, 0x00000000a7400000);
  // v2.S[0] = 0
  // v2.S[1] = 2^14
  // v2.S[2] = 1.5 * 2^-48
  // v2.S[3] = Inf
  __ Movi(v2.V4S(), 0x7f80000027c00000, 0xc680000000000000);
  // v3.S[0] = 0
  // v3.S[1] = -0
  // v3.S[2] = -0
  // v3.S[3] = 0
  __ Movi(v3.V4S(), 0x0000000080000000, 0x8000000000000000);
  // For Fmlsl{2}, we simply negate the accumulators above so that the Fmlsl{2}
  // results are just the negation of the Fmlal{2} results.
  __ Fneg(v4.V4S(), v0.V4S());
  __ Fneg(v5.V4S(), v1.V4S());
  __ Fneg(v6.V4S(), v2.V4S());
  __ Fneg(v7.V4S(), v3.V4S());

  __ Fmlal(v0.V2S(), v30.V2H(), v8.H(), 0);
  __ Fmlal2(v1.V2S(), v30.V2H(), v9.H(), 1);
  __ Fmlal(v2.V4S(), v30.V4H(), v10.H(), 2);
  __ Fmlal2(v3.V4S(), v30.V4H(), v11.H(), 3);

  __ Fmlsl(v4.V2S(), v30.V2H(), v12.H(), 4);
  __ Fmlsl2(v5.V2S(), v30.V2H(), v13.H(), 5);
  __ Fmlsl(v6.V4S(), v30.V4H(), v14.H(), 6);
  __ Fmlsl2(v7.V4S(), v30.V4H(), v15.H(), 7);
  END();

  if (CAN_RUN()) {
    RUN();

    // Fmlal(2S)
    // v0.S[0] = 2^-8 + (65504 * 1.000977) = 65567.96875 (rounded)
    // v0.S[1] = 1 + (-1 * 1.000977) = -0.000976...
    ASSERT_EQUAL_128(0x0000000000000000, 0xba80000047800ffc, v0);
    // Fmlal2(2S)
    // v1.S[0] = (-1.5 * 2^-49) + (2^-24 * 0.9995117) = 5.958e-8 (rounded)
    // v1.S[1] = 0 + (-2^-24 * 0.9995117) = -5.958e-8
    ASSERT_EQUAL_128(0x0000000000000000, 0xb37fe000337fdfff, v1);
    // Fmlal(4S)
    // v2.S[0] = 0 + (65504 * 1.000977) = 65566.96875
    // v2.S[1] = 2^14 + (-1 * 1.000977) = -16385 (rounded from -16385.5)
    // v2.S[2] = (1.5 * 2^-48) + (2^-24 * 1.000977) = 5.966e-8 (rounded up)
    // v2.S[3] = Inf + (-2^-24 * 1.000977) = Inf
    ASSERT_EQUAL_128(0x7f80000033802001, 0xc680020047800ffc, v2);
    // Fmlal2(4S)
    // v3.S[0] = 0 + (6.103516e-5 * 0.9995117) = 6.100535e-5
    // v3.S[1] = -0 + (0 * 0.9995117) = 0
    // v3.S[2] = -0 + (-0 * 0.9995117) = -0
    // v3.S[3] = 0 + (-Inf * 0.9995117) = -Inf
    ASSERT_EQUAL_128(0xff80000080000000, 0x00000000387fe000, v3);

    // Fmlsl results are mostly the same, but negated.
    ASSERT_EQUAL_128(0x0000000000000000, 0x3a800000c7800ffc, v4);
    ASSERT_EQUAL_128(0x0000000000000000, 0x337fe000b37fdfff, v5);
    ASSERT_EQUAL_128(0xff800000b3802001, 0x46800200c7800ffc, v6);
    // In this case: v7.S[2] = 0 - (-0 * 0.9995117) = 0
    ASSERT_EQUAL_128(0x7f80000000000000, 0x00000000b87fe000, v7);
  }
}


TEST(neon_fmulx_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();
  __ Fmov(s0, 2.0);
  __ Fmov(s1, 0.5);
  __ Fmov(s2, 0.0);
  __ Fmov(s3, -0.0);
  __ Fmov(s4, kFP32PositiveInfinity);
  __ Fmov(s5, kFP32NegativeInfinity);
  __ Fmulx(s16, s0, s1);
  __ Fmulx(s17, s2, s4);
  __ Fmulx(s18, s2, s5);
  __ Fmulx(s19, s3, s4);
  __ Fmulx(s20, s3, s5);

  __ Fmov(d21, 2.0);
  __ Fmov(d22, 0.5);
  __ Fmov(d23, 0.0);
  __ Fmov(d24, -0.0);
  __ Fmov(d25, kFP64PositiveInfinity);
  __ Fmov(d26, kFP64NegativeInfinity);
  __ Fmulx(d27, d21, d22);
  __ Fmulx(d28, d23, d25);
  __ Fmulx(d29, d23, d26);
  __ Fmulx(d30, d24, d25);
  __ Fmulx(d31, d24, d26);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s16);
    ASSERT_EQUAL_FP32(2.0, s17);
    ASSERT_EQUAL_FP32(-2.0, s18);
    ASSERT_EQUAL_FP32(-2.0, s19);
    ASSERT_EQUAL_FP32(2.0, s20);
    ASSERT_EQUAL_FP64(1.0, d27);
    ASSERT_EQUAL_FP64(2.0, d28);
    ASSERT_EQUAL_FP64(-2.0, d29);
    ASSERT_EQUAL_FP64(-2.0, d30);
    ASSERT_EQUAL_FP64(2.0, d31);
  }
}


TEST(neon_fmulx_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v1.V2D(), 0x3800380038003800, 0x3800380038003800);
  __ Movi(v2.V2D(), 0x0000000000000000, 0x0000000000000000);
  __ Movi(v3.V2D(), 0x8000800080008000, 0x8000800080008000);
  __ Movi(v4.V2D(), 0x7c007c007c007c00, 0x7c007c007c007c00);
  __ Movi(v5.V2D(), 0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00);
  __ Fmulx(v6.V8H(), v0.V8H(), v1.V8H());
  __ Fmulx(v7.V8H(), v2.V8H(), v4.V8H());
  __ Fmulx(v8.V8H(), v2.V8H(), v5.V8H());
  __ Fmulx(v9.V8H(), v3.V8H(), v4.V8H());
  __ Fmulx(v10.V8H(), v3.V8H(), v5.V8H());
  __ Fmulx(v11.V4H(), v0.V4H(), v1.V4H());
  __ Fmulx(v12.V4H(), v2.V4H(), v4.V4H());
  __ Fmulx(v13.V4H(), v2.V4H(), v5.V4H());
  __ Fmulx(v14.V4H(), v3.V4H(), v4.V4H());
  __ Fmulx(v15.V4H(), v3.V4H(), v5.V4H());
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_128(0x3c003c003c003c00, 0x3c003c003c003c00, v6);
    ASSERT_EQUAL_128(0x4000400040004000, 0x4000400040004000, v7);
    ASSERT_EQUAL_128(0xc000c000c000c000, 0xc000c000c000c000, v8);
    ASSERT_EQUAL_128(0xc000c000c000c000, 0xc000c000c000c000, v9);
    ASSERT_EQUAL_128(0x4000400040004000, 0x4000400040004000, v10);
    ASSERT_EQUAL_128(0, 0x3c003c003c003c00, v11);
    ASSERT_EQUAL_128(0, 0x4000400040004000, v12);
    ASSERT_EQUAL_128(0, 0xc000c000c000c000, v13);
    ASSERT_EQUAL_128(0, 0xc000c000c000c000, v14);
    ASSERT_EQUAL_128(0, 0x4000400040004000, v15);
  }
}


TEST(neon_fmulx_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFPHalf);

  START();
  __ Fmov(h0, Float16(2.0));
  __ Fmov(h1, Float16(0.5));
  __ Fmov(h2, Float16(0.0));
  __ Fmov(h3, Float16(-0.0));
  __ Fmov(h4, kFP16PositiveInfinity);
  __ Fmov(h5, kFP16NegativeInfinity);
  __ Fmulx(h6, h0, h1);
  __ Fmulx(h7, h2, h4);
  __ Fmulx(h8, h2, h5);
  __ Fmulx(h9, h3, h4);
  __ Fmulx(h10, h3, h5);
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_FP16(Float16(1.0), h6);
    ASSERT_EQUAL_FP16(Float16(2.0), h7);
    ASSERT_EQUAL_FP16(Float16(-2.0), h8);
    ASSERT_EQUAL_FP16(Float16(-2.0), h9);
    ASSERT_EQUAL_FP16(Float16(2.0), h10);
  }
}

TEST(neon_fabd_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v1.V2D(), 0x3800380038003800, 0x3800380038003800);
  __ Movi(v2.V2D(), 0x0000000000000000, 0x0000000000000000);
  __ Movi(v3.V2D(), 0x8000800080008000, 0x8000800080008000);
  __ Movi(v4.V2D(), 0x7c007c007c007c00, 0x7c007c007c007c00);
  __ Movi(v5.V2D(), 0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00);

  __ Fabd(v6.V8H(), v1.V8H(), v0.V8H());
  __ Fabd(v7.V8H(), v2.V8H(), v3.V8H());
  __ Fabd(v8.V8H(), v2.V8H(), v5.V8H());
  __ Fabd(v9.V8H(), v3.V8H(), v4.V8H());
  __ Fabd(v10.V8H(), v3.V8H(), v5.V8H());
  __ Fabd(v11.V4H(), v1.V4H(), v0.V4H());
  __ Fabd(v12.V4H(), v2.V4H(), v3.V4H());
  __ Fabd(v13.V4H(), v2.V4H(), v5.V4H());
  __ Fabd(v14.V4H(), v3.V4H(), v4.V4H());
  __ Fabd(v15.V4H(), v3.V4H(), v5.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x3e003e003e003e00, 0x3e003e003e003e00, v6);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0000000000000000, v7);
    ASSERT_EQUAL_128(0x7c007c007c007c00, 0x7c007c007c007c00, v8);
    ASSERT_EQUAL_128(0x7c007c007c007c00, 0x7c007c007c007c00, v9);
    ASSERT_EQUAL_128(0x7c007c007c007c00, 0x7c007c007c007c00, v10);
    ASSERT_EQUAL_128(0, 0x3e003e003e003e00, v11);
    ASSERT_EQUAL_128(0, 0x0000000000000000, v12);
    ASSERT_EQUAL_128(0, 0x7c007c007c007c00, v13);
    ASSERT_EQUAL_128(0, 0x7c007c007c007c00, v14);
    ASSERT_EQUAL_128(0, 0x7c007c007c007c00, v15);
  }
}


TEST(neon_fabd_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFPHalf);

  START();
  __ Fmov(h0, Float16(2.0));
  __ Fmov(h1, Float16(0.5));
  __ Fmov(h2, Float16(0.0));
  __ Fmov(h3, Float16(-0.0));
  __ Fmov(h4, kFP16PositiveInfinity);
  __ Fmov(h5, kFP16NegativeInfinity);
  __ Fabd(h16, h1, h0);
  __ Fabd(h17, h2, h3);
  __ Fabd(h18, h2, h5);
  __ Fabd(h19, h3, h4);
  __ Fabd(h20, h3, h5);
  END();

  if (CAN_RUN()) {
    RUN();
    ASSERT_EQUAL_FP16(Float16(1.5), h16);
    ASSERT_EQUAL_FP16(Float16(0.0), h17);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h18);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h19);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h20);
  }
}


TEST(neon_fabd_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();
  __ Fmov(s0, 2.0);
  __ Fmov(s1, 0.5);
  __ Fmov(s2, 0.0);
  __ Fmov(s3, -0.0);
  __ Fmov(s4, kFP32PositiveInfinity);
  __ Fmov(s5, kFP32NegativeInfinity);
  __ Fabd(s16, s1, s0);
  __ Fabd(s17, s2, s3);
  __ Fabd(s18, s2, s5);
  __ Fabd(s19, s3, s4);
  __ Fabd(s20, s3, s5);

  __ Fmov(d21, 2.0);
  __ Fmov(d22, 0.5);
  __ Fmov(d23, 0.0);
  __ Fmov(d24, -0.0);
  __ Fmov(d25, kFP64PositiveInfinity);
  __ Fmov(d26, kFP64NegativeInfinity);
  __ Fabd(d27, d21, d22);
  __ Fabd(d28, d23, d24);
  __ Fabd(d29, d23, d26);
  __ Fabd(d30, d24, d25);
  __ Fabd(d31, d24, d26);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.5, s16);
    ASSERT_EQUAL_FP32(0.0, s17);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s18);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s19);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s20);
    ASSERT_EQUAL_FP64(1.5, d27);
    ASSERT_EQUAL_FP64(0.0, d28);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d29);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d30);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d31);
  }
}


TEST(neon_frecps_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v1.V2D(), 0xbc00bc00bc00bc00, 0xbc00bc00bc00bc00);
  __ Movi(v2.V2D(), 0x51a051a051a051a0, 0x51a051a051a051a0);
  __ Movi(v3.V2D(), 0x7c007c007c007c00, 0x7c007c007c007c00);
  __ Movi(v4.V2D(), 0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00);

  __ Frecps(v5.V8H(), v0.V8H(), v2.V8H());
  __ Frecps(v6.V8H(), v1.V8H(), v2.V8H());
  __ Frecps(v7.V8H(), v0.V8H(), v3.V8H());
  __ Frecps(v8.V8H(), v0.V8H(), v4.V8H());
  __ Frecps(v9.V4H(), v0.V4H(), v2.V4H());
  __ Frecps(v10.V4H(), v1.V4H(), v2.V4H());
  __ Frecps(v11.V4H(), v0.V4H(), v3.V4H());
  __ Frecps(v12.V4H(), v0.V4H(), v4.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xd580d580d580d580, 0xd580d580d580d580, v5);
    ASSERT_EQUAL_128(0x51e051e051e051e0, 0x51e051e051e051e0, v6);
    ASSERT_EQUAL_128(0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00, v7);
    ASSERT_EQUAL_128(0x7c007c007c007c00, 0x7c007c007c007c00, v8);
    ASSERT_EQUAL_128(0, 0xd580d580d580d580, v9);
    ASSERT_EQUAL_128(0, 0x51e051e051e051e0, v10);
    ASSERT_EQUAL_128(0, 0xfc00fc00fc00fc00, v11);
    ASSERT_EQUAL_128(0, 0x7c007c007c007c00, v12);
  }
}


TEST(neon_frecps_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFPHalf);

  START();
  __ Fmov(h0, Float16(2.0));
  __ Fmov(h1, Float16(-1.0));
  __ Fmov(h2, Float16(45.0));
  __ Fmov(h3, kFP16PositiveInfinity);
  __ Fmov(h4, kFP16NegativeInfinity);

  __ Frecps(h5, h0, h2);
  __ Frecps(h6, h1, h2);
  __ Frecps(h7, h0, h3);
  __ Frecps(h8, h0, h4);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(-88.0), h5);
    ASSERT_EQUAL_FP16(Float16(47.0), h6);
    ASSERT_EQUAL_FP16(kFP16NegativeInfinity, h7);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h8);
  }
}


TEST(neon_frsqrts_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v1.V2D(), 0xbc00bc00bc00bc00, 0xbc00bc00bc00bc00);
  __ Movi(v2.V2D(), 0x51a051a051a051a0, 0x51a051a051a051a0);
  __ Movi(v3.V2D(), 0x7c007c007c007c00, 0x7c007c007c007c00);
  __ Movi(v4.V2D(), 0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00);

  __ Frsqrts(v5.V8H(), v0.V8H(), v2.V8H());
  __ Frsqrts(v6.V8H(), v1.V8H(), v2.V8H());
  __ Frsqrts(v7.V8H(), v0.V8H(), v3.V8H());
  __ Frsqrts(v8.V8H(), v0.V8H(), v4.V8H());
  __ Frsqrts(v9.V4H(), v0.V4H(), v2.V4H());
  __ Frsqrts(v10.V4H(), v1.V4H(), v2.V4H());
  __ Frsqrts(v11.V4H(), v0.V4H(), v3.V4H());
  __ Frsqrts(v12.V4H(), v0.V4H(), v4.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xd170d170d170d170, 0xd170d170d170d170, v5);
    ASSERT_EQUAL_128(0x4e004e004e004e00, 0x4e004e004e004e00, v6);
    ASSERT_EQUAL_128(0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00, v7);
    ASSERT_EQUAL_128(0x7c007c007c007c00, 0x7c007c007c007c00, v8);
    ASSERT_EQUAL_128(0, 0xd170d170d170d170, v9);
    ASSERT_EQUAL_128(0, 0x4e004e004e004e00, v10);
    ASSERT_EQUAL_128(0, 0xfc00fc00fc00fc00, v11);
    ASSERT_EQUAL_128(0, 0x7c007c007c007c00, v12);
  }
}


TEST(neon_frsqrts_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFPHalf);

  START();
  __ Fmov(h0, Float16(2.0));
  __ Fmov(h1, Float16(-1.0));
  __ Fmov(h2, Float16(45.0));
  __ Fmov(h3, kFP16PositiveInfinity);
  __ Fmov(h4, kFP16NegativeInfinity);

  __ Frsqrts(h5, h0, h2);
  __ Frsqrts(h6, h1, h2);
  __ Frsqrts(h7, h0, h3);
  __ Frsqrts(h8, h0, h4);
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(-43.5), h5);
    ASSERT_EQUAL_FP16(Float16(24.0), h6);
    ASSERT_EQUAL_FP16(kFP16NegativeInfinity, h7);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h8);
  }
}


TEST(neon_faddp_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x3c0040003c004000, 0x3c0040003c004000);
  __ Movi(v1.V2D(), 0xfc007c00fc007c00, 0xfc007c00fc007c00);
  __ Movi(v2.V2D(), 0x0000800000008000, 0x0000800000008000);
  __ Movi(v3.V2D(), 0x7e007c017e007c01, 0x7e007c017e007c01);

  __ Faddp(v4.V8H(), v1.V8H(), v0.V8H());
  __ Faddp(v5.V8H(), v3.V8H(), v2.V8H());
  __ Faddp(v6.V4H(), v1.V4H(), v0.V4H());
  __ Faddp(v7.V4H(), v3.V4H(), v2.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x4200420042004200, 0x7e007e007e007e00, v4);
    ASSERT_EQUAL_128(0x0000000000000000, 0x7e017e017e017e01, v5);
    ASSERT_EQUAL_128(0, 0x420042007e007e00, v6);
    ASSERT_EQUAL_128(0, 0x000000007e017e01, v7);
  }
}


TEST(neon_faddp_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();
  __ Movi(d0, 0x3f80000040000000);
  __ Movi(d1, 0xff8000007f800000);
  __ Movi(d2, 0x0000000080000000);
  __ Faddp(s0, v0.V2S());
  __ Faddp(s1, v1.V2S());
  __ Faddp(s2, v2.V2S());

  __ Movi(v3.V2D(), 0xc000000000000000, 0x4000000000000000);
  __ Movi(v4.V2D(), 0xfff8000000000000, 0x7ff8000000000000);
  __ Movi(v5.V2D(), 0x0000000000000000, 0x8000000000000000);
  __ Faddp(d3, v3.V2D());
  __ Faddp(d4, v4.V2D());
  __ Faddp(d5, v5.V2D());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(3.0, s0);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s1);
    ASSERT_EQUAL_FP32(0.0, s2);
    ASSERT_EQUAL_FP64(0.0, d3);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d4);
    ASSERT_EQUAL_FP64(0.0, d5);
  }
}


TEST(neon_faddp_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(s0, 0x3c004000);
  __ Movi(s1, 0xfc007c00);
  __ Movi(s2, 0x00008000);
  __ Faddp(h0, v0.V2H());
  __ Faddp(h1, v1.V2H());
  __ Faddp(h2, v2.V2H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(3.0), h0);
    ASSERT_EQUAL_FP16(kFP16DefaultNaN, h1);
    ASSERT_EQUAL_FP16(Float16(0.0), h2);
  }
}


TEST(neon_fmaxp_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();
  __ Movi(d0, 0x3f80000040000000);
  __ Movi(d1, 0xff8000007f800000);
  __ Movi(d2, 0x7fc00000ff800000);
  __ Fmaxp(s0, v0.V2S());
  __ Fmaxp(s1, v1.V2S());
  __ Fmaxp(s2, v2.V2S());

  __ Movi(v3.V2D(), 0x3ff0000000000000, 0x4000000000000000);
  __ Movi(v4.V2D(), 0xfff0000000000000, 0x7ff0000000000000);
  __ Movi(v5.V2D(), 0x7ff0000000000000, 0x7ff8000000000000);
  __ Fmaxp(d3, v3.V2D());
  __ Fmaxp(d4, v4.V2D());
  __ Fmaxp(d5, v5.V2D());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(2.0, s0);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s1);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s2);
    ASSERT_EQUAL_FP64(2.0, d3);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d4);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d5);
  }
}


TEST(neon_fmaxp_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(s0, 0x3c004000);
  __ Movi(s1, 0xfc007c00);
  __ Movi(s2, 0x7e00fc00);
  __ Fmaxp(h0, v0.V2H());
  __ Fmaxp(h1, v1.V2H());
  __ Fmaxp(h2, v2.V2H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(2.0), h0);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h1);
    ASSERT_EQUAL_FP16(kFP16DefaultNaN, h2);
  }
}


TEST(neon_fmax_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x3c003c003c003c00, 0x3c003c003c003c00);
  __ Movi(v1.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v2.V2D(), 0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00);
  __ Movi(v3.V2D(), 0x7c007c007c007c00, 0x7c007c007c007c00);
  __ Movi(v4.V2D(), 0x7e007e007e007e00, 0x7e007e007e007e00);
  __ Movi(v5.V2D(), 0x7c017c017c017c01, 0x7c017c017c017c01);

  __ Fmax(v6.V8H(), v0.V8H(), v1.V8H());
  __ Fmax(v7.V8H(), v2.V8H(), v3.V8H());
  __ Fmax(v8.V8H(), v4.V8H(), v0.V8H());
  __ Fmax(v9.V8H(), v5.V8H(), v1.V8H());
  __ Fmax(v10.V4H(), v0.V4H(), v1.V4H());
  __ Fmax(v11.V4H(), v2.V4H(), v3.V4H());
  __ Fmax(v12.V4H(), v4.V4H(), v0.V4H());
  __ Fmax(v13.V4H(), v5.V4H(), v1.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x4000400040004000, 0x4000400040004000, v6);
    ASSERT_EQUAL_128(0x7c007c007c007c00, 0x7c007c007c007c00, v7);
    ASSERT_EQUAL_128(0x7e007e007e007e00, 0x7e007e007e007e00, v8);
    ASSERT_EQUAL_128(0x7e017e017e017e01, 0x7e017e017e017e01, v9);
    ASSERT_EQUAL_128(0, 0x4000400040004000, v10);
    ASSERT_EQUAL_128(0, 0x7c007c007c007c00, v11);
    ASSERT_EQUAL_128(0, 0x7e007e007e007e00, v12);
    ASSERT_EQUAL_128(0, 0x7e017e017e017e01, v13);
  }
}


TEST(neon_fmaxp_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x3c0040003c004000, 0x3c0040003c004000);
  __ Movi(v1.V2D(), 0xfc007c00fc007c00, 0xfc007c00fc007c00);
  __ Movi(v2.V2D(), 0x7e003c007e003c00, 0x7e003c007e003c00);
  __ Movi(v3.V2D(), 0x7c0140007c014000, 0x7c0140007c014000);

  __ Fmaxp(v6.V8H(), v0.V8H(), v1.V8H());
  __ Fmaxp(v7.V8H(), v2.V8H(), v3.V8H());
  __ Fmaxp(v8.V4H(), v0.V4H(), v1.V4H());
  __ Fmaxp(v9.V4H(), v2.V4H(), v3.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x7c007c007c007c00, 0x4000400040004000, v6);
    ASSERT_EQUAL_128(0x7e017e017e017e01, 0x7e007e007e007e00, v7);
    ASSERT_EQUAL_128(0, 0x7c007c0040004000, v8);
    ASSERT_EQUAL_128(0, 0x7e017e017e007e00, v9);
  }
}


TEST(neon_fmaxnm_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x3c003c003c003c00, 0x3c003c003c003c00);
  __ Movi(v1.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v2.V2D(), 0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00);
  __ Movi(v3.V2D(), 0x7c007c007c007c00, 0x7c007c007c007c00);
  __ Movi(v4.V2D(), 0x7e007e007e007e00, 0x7e007e007e007e00);
  __ Movi(v5.V2D(), 0x7c017c017c017c01, 0x7c017c017c017c01);

  __ Fmaxnm(v6.V8H(), v0.V8H(), v1.V8H());
  __ Fmaxnm(v7.V8H(), v2.V8H(), v3.V8H());
  __ Fmaxnm(v8.V8H(), v4.V8H(), v0.V8H());
  __ Fmaxnm(v9.V8H(), v5.V8H(), v1.V8H());
  __ Fmaxnm(v10.V4H(), v0.V4H(), v1.V4H());
  __ Fmaxnm(v11.V4H(), v2.V4H(), v3.V4H());
  __ Fmaxnm(v12.V4H(), v4.V4H(), v0.V4H());
  __ Fmaxnm(v13.V4H(), v5.V4H(), v1.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x4000400040004000, 0x4000400040004000, v6);
    ASSERT_EQUAL_128(0x7c007c007c007c00, 0x7c007c007c007c00, v7);
    ASSERT_EQUAL_128(0x3c003c003c003c00, 0x3c003c003c003c00, v8);
    ASSERT_EQUAL_128(0x7e017e017e017e01, 0x7e017e017e017e01, v9);
    ASSERT_EQUAL_128(0, 0x4000400040004000, v10);
    ASSERT_EQUAL_128(0, 0x7c007c007c007c00, v11);
    ASSERT_EQUAL_128(0, 0x3c003c003c003c00, v12);
    ASSERT_EQUAL_128(0, 0x7e017e017e017e01, v13);
  }
}


TEST(neon_fmaxnmp_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x3c0040003c004000, 0x3c0040003c004000);
  __ Movi(v1.V2D(), 0xfc007c00fc007c00, 0xfc007c00fc007c00);
  __ Movi(v2.V2D(), 0x7e003c007e003c00, 0x7e003c007e003c00);
  __ Movi(v3.V2D(), 0x7c0140007c014000, 0x7c0140007c014000);

  __ Fmaxnmp(v6.V8H(), v0.V8H(), v1.V8H());
  __ Fmaxnmp(v7.V8H(), v2.V8H(), v3.V8H());
  __ Fmaxnmp(v8.V4H(), v0.V4H(), v1.V4H());
  __ Fmaxnmp(v9.V4H(), v2.V4H(), v3.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x7c007c007c007c00, 0x4000400040004000, v6);
    ASSERT_EQUAL_128(0x7e017e017e017e01, 0x3c003c003c003c00, v7);
    ASSERT_EQUAL_128(0, 0x7c007c0040004000, v8);
    ASSERT_EQUAL_128(0, 0x7e017e013c003c00, v9);
  }
}


TEST(neon_fmaxnmp_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();
  __ Movi(d0, 0x3f80000040000000);
  __ Movi(d1, 0xff8000007f800000);
  __ Movi(d2, 0x7fc00000ff800000);
  __ Fmaxnmp(s0, v0.V2S());
  __ Fmaxnmp(s1, v1.V2S());
  __ Fmaxnmp(s2, v2.V2S());

  __ Movi(v3.V2D(), 0x3ff0000000000000, 0x4000000000000000);
  __ Movi(v4.V2D(), 0xfff0000000000000, 0x7ff0000000000000);
  __ Movi(v5.V2D(), 0x7ff8000000000000, 0xfff0000000000000);
  __ Fmaxnmp(d3, v3.V2D());
  __ Fmaxnmp(d4, v4.V2D());
  __ Fmaxnmp(d5, v5.V2D());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(2.0, s0);
    ASSERT_EQUAL_FP32(kFP32PositiveInfinity, s1);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s2);
    ASSERT_EQUAL_FP64(2.0, d3);
    ASSERT_EQUAL_FP64(kFP64PositiveInfinity, d4);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d5);
  }
}


TEST(neon_fmaxnmp_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(s0, 0x3c004000);
  __ Movi(s1, 0xfc007c00);
  __ Movi(s2, 0x7e00fc00);
  __ Fmaxnmp(h0, v0.V2H());
  __ Fmaxnmp(h1, v1.V2H());
  __ Fmaxnmp(h2, v2.V2H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(2.0), h0);
    ASSERT_EQUAL_FP16(kFP16PositiveInfinity, h1);
    ASSERT_EQUAL_FP16(kFP16NegativeInfinity, h2);
  }
}


TEST(neon_fminp_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();
  __ Movi(d0, 0x3f80000040000000);
  __ Movi(d1, 0xff8000007f800000);
  __ Movi(d2, 0x7fc00000ff800000);
  __ Fminp(s0, v0.V2S());
  __ Fminp(s1, v1.V2S());
  __ Fminp(s2, v2.V2S());

  __ Movi(v3.V2D(), 0x3ff0000000000000, 0x4000000000000000);
  __ Movi(v4.V2D(), 0xfff0000000000000, 0x7ff0000000000000);
  __ Movi(v5.V2D(), 0x7ff0000000000000, 0x7ff8000000000000);
  __ Fminp(d3, v3.V2D());
  __ Fminp(d4, v4.V2D());
  __ Fminp(d5, v5.V2D());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s1);
    ASSERT_EQUAL_FP32(kFP32DefaultNaN, s2);
    ASSERT_EQUAL_FP64(1.0, d3);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d4);
    ASSERT_EQUAL_FP64(kFP64DefaultNaN, d5);
  }
}


TEST(neon_fminp_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(s0, 0x3c004000);
  __ Movi(s1, 0xfc007c00);
  __ Movi(s2, 0x7e00fc00);
  __ Fminp(h0, v0.V2H());
  __ Fminp(h1, v1.V2H());
  __ Fminp(h2, v2.V2H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(1.0), h0);
    ASSERT_EQUAL_FP16(kFP16NegativeInfinity, h1);
    ASSERT_EQUAL_FP16(kFP16DefaultNaN, h2);
  }
}


TEST(neon_fmin_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x3c003c003c003c00, 0x3c003c003c003c00);
  __ Movi(v1.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v2.V2D(), 0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00);
  __ Movi(v3.V2D(), 0x7c007c007c007c00, 0x7c007c007c007c00);
  __ Movi(v4.V2D(), 0x7e007e007e007e00, 0x7e007e007e007e00);
  __ Movi(v5.V2D(), 0x7c017c017c017c01, 0x7c017c017c017c01);

  __ Fmin(v6.V8H(), v0.V8H(), v1.V8H());
  __ Fmin(v7.V8H(), v2.V8H(), v3.V8H());
  __ Fmin(v8.V8H(), v4.V8H(), v0.V8H());
  __ Fmin(v9.V8H(), v5.V8H(), v1.V8H());
  __ Fmin(v10.V4H(), v0.V4H(), v1.V4H());
  __ Fmin(v11.V4H(), v2.V4H(), v3.V4H());
  __ Fmin(v12.V4H(), v4.V4H(), v0.V4H());
  __ Fmin(v13.V4H(), v5.V4H(), v1.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x3c003c003c003c00, 0x3c003c003c003c00, v6);
    ASSERT_EQUAL_128(0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00, v7);
    ASSERT_EQUAL_128(0x7e007e007e007e00, 0x7e007e007e007e00, v8);
    ASSERT_EQUAL_128(0x7e017e017e017e01, 0x7e017e017e017e01, v9);
    ASSERT_EQUAL_128(0, 0x3c003c003c003c00, v10);
    ASSERT_EQUAL_128(0, 0xfc00fc00fc00fc00, v11);
    ASSERT_EQUAL_128(0, 0x7e007e007e007e00, v12);
    ASSERT_EQUAL_128(0, 0x7e017e017e017e01, v13);
  }
}


TEST(neon_fminp_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x3c0040003c004000, 0x3c0040003c004000);
  __ Movi(v1.V2D(), 0xfc007c00fc007c00, 0xfc007c00fc007c00);
  __ Movi(v2.V2D(), 0x7e003c007e003c00, 0x7e003c007e003c00);
  __ Movi(v3.V2D(), 0x7c0140007c014000, 0x7c0140007c014000);

  __ Fminp(v6.V8H(), v0.V8H(), v1.V8H());
  __ Fminp(v7.V8H(), v2.V8H(), v3.V8H());
  __ Fminp(v8.V4H(), v0.V4H(), v1.V4H());
  __ Fminp(v9.V4H(), v2.V4H(), v3.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xfc00fc00fc00fc00, 0x3c003c003c003c00, v6);
    ASSERT_EQUAL_128(0x7e017e017e017e01, 0x7e007e007e007e00, v7);
    ASSERT_EQUAL_128(0, 0xfc00fc003c003c00, v8);
    ASSERT_EQUAL_128(0, 0x7e017e017e007e00, v9);
  }
}


TEST(neon_fminnm_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x3c003c003c003c00, 0x3c003c003c003c00);
  __ Movi(v1.V2D(), 0x4000400040004000, 0x4000400040004000);
  __ Movi(v2.V2D(), 0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00);
  __ Movi(v3.V2D(), 0x7c007c007c007c00, 0x7c007c007c007c00);
  __ Movi(v4.V2D(), 0x7e007e007e007e00, 0x7e007e007e007e00);
  __ Movi(v5.V2D(), 0x7c017c017c017c01, 0x7c017c017c017c01);

  __ Fminnm(v6.V8H(), v0.V8H(), v1.V8H());
  __ Fminnm(v7.V8H(), v2.V8H(), v3.V8H());
  __ Fminnm(v8.V8H(), v4.V8H(), v0.V8H());
  __ Fminnm(v9.V8H(), v5.V8H(), v1.V8H());
  __ Fminnm(v10.V4H(), v0.V4H(), v1.V4H());
  __ Fminnm(v11.V4H(), v2.V4H(), v3.V4H());
  __ Fminnm(v12.V4H(), v4.V4H(), v0.V4H());
  __ Fminnm(v13.V4H(), v5.V4H(), v1.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x3c003c003c003c00, 0x3c003c003c003c00, v6);
    ASSERT_EQUAL_128(0xfc00fc00fc00fc00, 0xfc00fc00fc00fc00, v7);
    ASSERT_EQUAL_128(0x3c003c003c003c00, 0x3c003c003c003c00, v8);
    ASSERT_EQUAL_128(0x7e017e017e017e01, 0x7e017e017e017e01, v9);
    ASSERT_EQUAL_128(0, 0x3c003c003c003c00, v10);
    ASSERT_EQUAL_128(0, 0xfc00fc00fc00fc00, v11);
    ASSERT_EQUAL_128(0, 0x3c003c003c003c00, v12);
    ASSERT_EQUAL_128(0, 0x7e017e017e017e01, v13);
  }
}


TEST(neon_fminnmp_h) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(v0.V2D(), 0x3c0040003c004000, 0x3c0040003c004000);
  __ Movi(v1.V2D(), 0xfc007c00fc007c00, 0xfc007c00fc007c00);
  __ Movi(v2.V2D(), 0x7e003c007e003c00, 0x7e003c007e003c00);
  __ Movi(v3.V2D(), 0x7c0140007c014000, 0x7c0140007c014000);

  __ Fminnmp(v6.V8H(), v0.V8H(), v1.V8H());
  __ Fminnmp(v7.V8H(), v2.V8H(), v3.V8H());
  __ Fminnmp(v8.V4H(), v0.V4H(), v1.V4H());
  __ Fminnmp(v9.V4H(), v2.V4H(), v3.V4H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0xfc00fc00fc00fc00, 0x3c003c003c003c00, v6);
    ASSERT_EQUAL_128(0x7e017e017e017e01, 0x3c003c003c003c00, v7);
    ASSERT_EQUAL_128(0, 0xfc00fc003c003c00, v8);
    ASSERT_EQUAL_128(0, 0x7e017e013c003c00, v9);
  }
}


TEST(neon_fminnmp_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON, CPUFeatures::kFP);

  START();
  __ Movi(d0, 0x3f80000040000000);
  __ Movi(d1, 0xff8000007f800000);
  __ Movi(d2, 0x7fc00000ff800000);
  __ Fminnmp(s0, v0.V2S());
  __ Fminnmp(s1, v1.V2S());
  __ Fminnmp(s2, v2.V2S());

  __ Movi(v3.V2D(), 0x3ff0000000000000, 0x4000000000000000);
  __ Movi(v4.V2D(), 0xfff0000000000000, 0x7ff0000000000000);
  __ Movi(v5.V2D(), 0x7ff8000000000000, 0xfff0000000000000);
  __ Fminnmp(d3, v3.V2D());
  __ Fminnmp(d4, v4.V2D());
  __ Fminnmp(d5, v5.V2D());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP32(1.0, s0);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s1);
    ASSERT_EQUAL_FP32(kFP32NegativeInfinity, s2);
    ASSERT_EQUAL_FP64(1.0, d3);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d4);
    ASSERT_EQUAL_FP64(kFP64NegativeInfinity, d5);
  }
}


TEST(neon_fminnmp_h_scalar) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf);

  START();
  __ Movi(s0, 0x3c004000);
  __ Movi(s1, 0xfc007c00);
  __ Movi(s2, 0x7e00fc00);
  __ Fminnmp(h0, v0.V2H());
  __ Fminnmp(h1, v1.V2H());
  __ Fminnmp(h2, v2.V2H());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(Float16(1.0), h0);
    ASSERT_EQUAL_FP16(kFP16NegativeInfinity, h1);
    ASSERT_EQUAL_FP16(kFP16NegativeInfinity, h2);
  }
}

static uint64_t Float16ToV4H(Float16 f) {
  uint64_t bits = static_cast<uint64_t>(Float16ToRawbits(f));
  return (bits << 48) | (bits << 32) | (bits << 16) | bits;
}


static void FminFmaxFloat16Helper(Float16 n,
                                  Float16 m,
                                  Float16 min,
                                  Float16 max,
                                  Float16 minnm,
                                  Float16 maxnm) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kNEONHalf,
                      CPUFeatures::kFPHalf);

  START();
  __ Fmov(h0, n);
  __ Fmov(h1, m);
  __ Fmov(v0.V8H(), n);
  __ Fmov(v1.V8H(), m);
  __ Fmin(h28, h0, h1);
  __ Fmin(v2.V4H(), v0.V4H(), v1.V4H());
  __ Fmin(v3.V8H(), v0.V8H(), v1.V8H());
  __ Fmax(h29, h0, h1);
  __ Fmax(v4.V4H(), v0.V4H(), v1.V4H());
  __ Fmax(v5.V8H(), v0.V8H(), v1.V8H());
  __ Fminnm(h30, h0, h1);
  __ Fminnm(v6.V4H(), v0.V4H(), v1.V4H());
  __ Fminnm(v7.V8H(), v0.V8H(), v1.V8H());
  __ Fmaxnm(h31, h0, h1);
  __ Fmaxnm(v8.V4H(), v0.V4H(), v1.V4H());
  __ Fmaxnm(v9.V8H(), v0.V8H(), v1.V8H());
  END();

  uint64_t min_vec = Float16ToV4H(min);
  uint64_t max_vec = Float16ToV4H(max);
  uint64_t minnm_vec = Float16ToV4H(minnm);
  uint64_t maxnm_vec = Float16ToV4H(maxnm);

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_FP16(min, h28);
    ASSERT_EQUAL_FP16(max, h29);
    ASSERT_EQUAL_FP16(minnm, h30);
    ASSERT_EQUAL_FP16(maxnm, h31);


    ASSERT_EQUAL_128(0, min_vec, v2);
    ASSERT_EQUAL_128(min_vec, min_vec, v3);
    ASSERT_EQUAL_128(0, max_vec, v4);
    ASSERT_EQUAL_128(max_vec, max_vec, v5);
    ASSERT_EQUAL_128(0, minnm_vec, v6);
    ASSERT_EQUAL_128(minnm_vec, minnm_vec, v7);
    ASSERT_EQUAL_128(0, maxnm_vec, v8);
    ASSERT_EQUAL_128(maxnm_vec, maxnm_vec, v9);
  }
}

static Float16 MinMaxHelper(Float16 n,
                            Float16 m,
                            bool min,
                            Float16 quiet_nan_substitute = Float16(0.0)) {
  const uint64_t kFP16QuietNaNMask = 0x0200;
  uint16_t raw_n = Float16ToRawbits(n);
  uint16_t raw_m = Float16ToRawbits(m);

  if (IsSignallingNaN(n)) {
    // n is signalling NaN.
    return RawbitsToFloat16(raw_n | kFP16QuietNaNMask);
  } else if (IsSignallingNaN(m)) {
    // m is signalling NaN.
    return RawbitsToFloat16(raw_m | kFP16QuietNaNMask);
  } else if (IsZero(quiet_nan_substitute)) {
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

  uint16_t sign_mask = 0x8000;
  if (IsZero(n) && IsZero(m) && ((raw_n & sign_mask) != (raw_m & sign_mask))) {
    return min ? Float16(-0.0) : Float16(0.0);
  }

  if (FPToDouble(n, kIgnoreDefaultNaN) < FPToDouble(m, kIgnoreDefaultNaN)) {
    return min ? n : m;
  }
  return min ? m : n;
}

TEST(fmax_fmin_h) {
  // Use non-standard NaNs to check that the payload bits are preserved.
  Float16 snan = RawbitsToFloat16(0x7c12);
  Float16 qnan = RawbitsToFloat16(0x7e34);

  Float16 snan_processed = RawbitsToFloat16(0x7e12);
  Float16 qnan_processed = qnan;

  VIXL_ASSERT(IsSignallingNaN(snan));
  VIXL_ASSERT(IsQuietNaN(qnan));
  VIXL_ASSERT(IsQuietNaN(snan_processed));
  VIXL_ASSERT(IsQuietNaN(qnan_processed));

  // Bootstrap tests.
  FminFmaxFloat16Helper(Float16(0),
                        Float16(0),
                        Float16(0),
                        Float16(0),
                        Float16(0),
                        Float16(0));
  FminFmaxFloat16Helper(Float16(0),
                        Float16(1),
                        Float16(0),
                        Float16(1),
                        Float16(0),
                        Float16(1));
  FminFmaxFloat16Helper(kFP16PositiveInfinity,
                        kFP16NegativeInfinity,
                        kFP16NegativeInfinity,
                        kFP16PositiveInfinity,
                        kFP16NegativeInfinity,
                        kFP16PositiveInfinity);
  FminFmaxFloat16Helper(snan,
                        Float16(0),
                        snan_processed,
                        snan_processed,
                        snan_processed,
                        snan_processed);
  FminFmaxFloat16Helper(Float16(0),
                        snan,
                        snan_processed,
                        snan_processed,
                        snan_processed,
                        snan_processed);
  FminFmaxFloat16Helper(qnan,
                        Float16(0),
                        qnan_processed,
                        qnan_processed,
                        Float16(0),
                        Float16(0));
  FminFmaxFloat16Helper(Float16(0),
                        qnan,
                        qnan_processed,
                        qnan_processed,
                        Float16(0),
                        Float16(0));
  FminFmaxFloat16Helper(qnan,
                        snan,
                        snan_processed,
                        snan_processed,
                        snan_processed,
                        snan_processed);
  FminFmaxFloat16Helper(snan,
                        qnan,
                        snan_processed,
                        snan_processed,
                        snan_processed,
                        snan_processed);

  // Iterate over all combinations of inputs.
  Float16 inputs[] = {RawbitsToFloat16(0x7bff),
                      RawbitsToFloat16(0x0400),
                      Float16(1.0),
                      Float16(0.0),
                      RawbitsToFloat16(0xfbff),
                      RawbitsToFloat16(0x8400),
                      Float16(-1.0),
                      Float16(-0.0),
                      kFP16PositiveInfinity,
                      kFP16NegativeInfinity,
                      kFP16QuietNaN,
                      kFP16SignallingNaN};

  const int count = sizeof(inputs) / sizeof(inputs[0]);

  for (int in = 0; in < count; in++) {
    Float16 n = inputs[in];
    for (int im = 0; im < count; im++) {
      Float16 m = inputs[im];
      FminFmaxFloat16Helper(n,
                            m,
                            MinMaxHelper(n, m, true),
                            MinMaxHelper(n, m, false),
                            MinMaxHelper(n, m, true, kFP16PositiveInfinity),
                            MinMaxHelper(n, m, false, kFP16NegativeInfinity));
    }
  }
}

TEST(neon_frint_saturating) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON,
                      CPUFeatures::kFP,
                      CPUFeatures::kFrintToFixedSizedInt);

  START();

  __ Movi(v0.V2D(), 0x3f8000003f8ccccd, 0x3fc000003ff33333);
  __ Movi(v1.V2D(), 0x3e200000be200000, 0x7f800000ff800000);
  __ Movi(v2.V2D(), 0xfff0000000000000, 0x7ff0000000000000);
  __ Frint32x(v16.V2S(), v0.V2S());
  __ Frint32x(v17.V4S(), v1.V4S());
  __ Frint32x(v18.V2D(), v2.V2D());
  __ Frint64x(v19.V2S(), v0.V2S());
  __ Frint64x(v20.V4S(), v1.V4S());
  __ Frint64x(v21.V2D(), v2.V2D());
  __ Frint32z(v22.V2S(), v0.V2S());
  __ Frint32z(v23.V4S(), v1.V4S());
  __ Frint32z(v24.V2D(), v2.V2D());
  __ Frint64z(v25.V2S(), v0.V2S());
  __ Frint64z(v26.V4S(), v1.V4S());
  __ Frint64z(v27.V2D(), v2.V2D());

  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x0000000000000000, 0x4000000040000000, q16);
    ASSERT_EQUAL_128(0x0000000080000000, 0xcf000000cf000000, q17);
    ASSERT_EQUAL_128(0xc1e0000000000000, 0xc1e0000000000000, q18);
    ASSERT_EQUAL_128(0x0000000000000000, 0x4000000040000000, q19);
    ASSERT_EQUAL_128(0x0000000080000000, 0xdf000000df000000, q20);
    ASSERT_EQUAL_128(0xc3e0000000000000, 0xc3e0000000000000, q21);
    ASSERT_EQUAL_128(0x0000000000000000, 0x3f8000003f800000, q22);
    ASSERT_EQUAL_128(0x0000000080000000, 0xcf000000cf000000, q23);
    ASSERT_EQUAL_128(0xc1e0000000000000, 0xc1e0000000000000, q24);
    ASSERT_EQUAL_128(0x0000000000000000, 0x3f8000003f800000, q25);
    ASSERT_EQUAL_128(0x0000000080000000, 0xdf000000df000000, q26);
    ASSERT_EQUAL_128(0xc3e0000000000000, 0xc3e0000000000000, q27);
  }
}


TEST(neon_tbl) {
  SETUP_WITH_FEATURES(CPUFeatures::kNEON);

  START();
  __ Movi(v30.V2D(), 0xbf561e188b1280e9, 0xbd542b8cbd24e8e8);
  __ Movi(v31.V2D(), 0xb5e9883d2c88a46d, 0x12276d5b614c915e);
  __ Movi(v0.V2D(), 0xc45b7782bc5ecd72, 0x5dd4fe5a4bc6bf5e);
  __ Movi(v1.V2D(), 0x1e3254094bd1746a, 0xf099ecf50e861c80);

  __ Movi(v4.V2D(), 0xf80c030100031f16, 0x00070504031201ff);
  __ Movi(v5.V2D(), 0x1f01001afc14202a, 0x2a081e1b0c02020c);
  __ Movi(v6.V2D(), 0x353f1a13022a2360, 0x2c464a00203a0a33);
  __ Movi(v7.V2D(), 0x64801a1c054cf30d, 0x793a2c052e213739);

  __ Movi(v8.V2D(), 0xb7f60ad7d7d88f13, 0x13eefc240496e842);
  __ Movi(v9.V2D(), 0x1be199c7c69b47ec, 0x8e4b9919f6eed443);
  __ Movi(v10.V2D(), 0x9bd2e1654c69e48f, 0x2143d089e426c6d2);
  __ Movi(v11.V2D(), 0xc31dbdc4a0393065, 0x1ecc2077caaf64d8);
  __ Movi(v12.V2D(), 0x29b24463967bc6eb, 0xdaf59970df01c93b);
  __ Movi(v13.V2D(), 0x3e20a4a4cb6813f4, 0x20a5832713dae669);
  __ Movi(v14.V2D(), 0xc5ff9a94041b1fdf, 0x2f46cde38cba2682);
  __ Movi(v15.V2D(), 0xd8cc5b0e61f387e6, 0xe69d6d314971e8fd);

  __ Tbl(v8.V16B(), v1.V16B(), v4.V16B());
  __ Tbl(v9.V16B(), v0.V16B(), v1.V16B(), v5.V16B());
  __ Tbl(v10.V16B(), v31.V16B(), v0.V16B(), v1.V16B(), v6.V16B());
  __ Tbl(v11.V16B(), v30.V16B(), v31.V16B(), v0.V16B(), v1.V16B(), v7.V16B());
  __ Tbl(v12.V8B(), v1.V16B(), v4.V8B());
  __ Tbl(v13.V8B(), v0.V16B(), v1.V16B(), v5.V8B());
  __ Tbl(v14.V8B(), v31.V16B(), v0.V16B(), v1.V16B(), v6.V8B());
  __ Tbl(v15.V8B(), v30.V16B(), v31.V16B(), v0.V16B(), v1.V16B(), v7.V8B());

  __ Movi(v16.V2D(), 0xb7f60ad7d7d88f13, 0x13eefc240496e842);
  __ Movi(v17.V2D(), 0x1be199c7c69b47ec, 0x8e4b9919f6eed443);
  __ Movi(v18.V2D(), 0x9bd2e1654c69e48f, 0x2143d089e426c6d2);
  __ Movi(v19.V2D(), 0xc31dbdc4a0393065, 0x1ecc2077caaf64d8);
  __ Movi(v20.V2D(), 0x29b24463967bc6eb, 0xdaf59970df01c93b);
  __ Movi(v21.V2D(), 0x3e20a4a4cb6813f4, 0x20a5832713dae669);
  __ Movi(v22.V2D(), 0xc5ff9a94041b1fdf, 0x2f46cde38cba2682);
  __ Movi(v23.V2D(), 0xd8cc5b0e61f387e6, 0xe69d6d314971e8fd);

  __ Tbx(v16.V16B(), v1.V16B(), v4.V16B());
  __ Tbx(v17.V16B(), v0.V16B(), v1.V16B(), v5.V16B());
  __ Tbx(v18.V16B(), v31.V16B(), v0.V16B(), v1.V16B(), v6.V16B());
  __ Tbx(v19.V16B(), v30.V16B(), v31.V16B(), v0.V16B(), v1.V16B(), v7.V16B());
  __ Tbx(v20.V8B(), v1.V16B(), v4.V8B());
  __ Tbx(v21.V8B(), v0.V16B(), v1.V16B(), v5.V8B());
  __ Tbx(v22.V8B(), v31.V16B(), v0.V16B(), v1.V16B(), v6.V8B());
  __ Tbx(v23.V8B(), v30.V16B(), v31.V16B(), v0.V16B(), v1.V16B(), v7.V8B());
  END();

  if (CAN_RUN()) {
    RUN();

    ASSERT_EQUAL_128(0x00090e1c800e0000, 0x80f0ecf50e001c00, v8);
    ASSERT_EQUAL_128(0x1ebf5ed100f50000, 0x0072324b82c6c682, v9);
    ASSERT_EQUAL_128(0x00005e4b4cd10e00, 0x0900005e80008800, v10);
    ASSERT_EQUAL_128(0x0000883d2b00001e, 0x00d1822b5bbff074, v11);
    ASSERT_EQUAL_128(0x0000000000000000, 0x80f0ecf50e001c00, v12);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0072324b82c6c682, v13);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0900005e80008800, v14);
    ASSERT_EQUAL_128(0x0000000000000000, 0x00d1822b5bbff074, v15);

    ASSERT_EQUAL_128(0xb7090e1c800e8f13, 0x80f0ecf50e961c42, v16);
    ASSERT_EQUAL_128(0x1ebf5ed1c6f547ec, 0x8e72324b82c6c682, v17);
    ASSERT_EQUAL_128(0x9bd25e4b4cd10e8f, 0x0943d05e802688d2, v18);
    ASSERT_EQUAL_128(0xc31d883d2b39301e, 0x1ed1822b5bbff074, v19);
    ASSERT_EQUAL_128(0x0000000000000000, 0x80f0ecf50e011c3b, v20);
    ASSERT_EQUAL_128(0x0000000000000000, 0x2072324b82c6c682, v21);
    ASSERT_EQUAL_128(0x0000000000000000, 0x0946cd5e80ba8882, v22);
    ASSERT_EQUAL_128(0x0000000000000000, 0xe6d1822b5bbff074, v23);
  }
}


}  // namespace aarch64
}  // namespace vixl
