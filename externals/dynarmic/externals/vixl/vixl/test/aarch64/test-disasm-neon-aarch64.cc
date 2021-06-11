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

TEST(load_store_v_offset) {
  SETUP();

  COMPARE(ldr(s0, MemOperand(x1)), "ldr s0, [x1]");
  COMPARE(ldr(s2, MemOperand(x3, 4)), "ldr s2, [x3, #4]");
  COMPARE(ldr(s4, MemOperand(x5, 16380)), "ldr s4, [x5, #16380]");
  COMPARE(ldr(d6, MemOperand(x7)), "ldr d6, [x7]");
  COMPARE(ldr(d8, MemOperand(x9, 8)), "ldr d8, [x9, #8]");
  COMPARE(ldr(d10, MemOperand(x11, 32760)), "ldr d10, [x11, #32760]");
  COMPARE(str(s12, MemOperand(x13)), "str s12, [x13]");
  COMPARE(str(s14, MemOperand(x15, 4)), "str s14, [x15, #4]");
  COMPARE(str(s16, MemOperand(x17, 16380)), "str s16, [x17, #16380]");
  COMPARE(str(d18, MemOperand(x19)), "str d18, [x19]");
  COMPARE(str(d20, MemOperand(x21, 8)), "str d20, [x21, #8]");
  COMPARE(str(d22, MemOperand(x23, 32760)), "str d22, [x23, #32760]");

  COMPARE(ldr(b0, MemOperand(x1)), "ldr b0, [x1]");
  COMPARE(ldr(b2, MemOperand(x3, 1)), "ldr b2, [x3, #1]");
  COMPARE(ldr(b4, MemOperand(x5, 4095)), "ldr b4, [x5, #4095]");
  COMPARE(ldr(h6, MemOperand(x7)), "ldr h6, [x7]");
  COMPARE(ldr(h8, MemOperand(x9, 2)), "ldr h8, [x9, #2]");
  COMPARE(ldr(h10, MemOperand(x11, 8190)), "ldr h10, [x11, #8190]");
  COMPARE(ldr(q12, MemOperand(x13)), "ldr q12, [x13]");
  COMPARE(ldr(q14, MemOperand(x15, 16)), "ldr q14, [x15, #16]");
  COMPARE(ldr(q16, MemOperand(x17, 65520)), "ldr q16, [x17, #65520]");
  COMPARE(str(b18, MemOperand(x19)), "str b18, [x19]");
  COMPARE(str(b20, MemOperand(x21, 1)), "str b20, [x21, #1]");
  COMPARE(str(b22, MemOperand(x23, 4095)), "str b22, [x23, #4095]");
  COMPARE(str(h24, MemOperand(x25)), "str h24, [x25]");
  COMPARE(str(h26, MemOperand(x27, 2)), "str h26, [x27, #2]");
  COMPARE(str(h28, MemOperand(x29, 8190)), "str h28, [x29, #8190]");
  COMPARE(str(q30, MemOperand(x30)), "str q30, [x30]");
  COMPARE(str(q31, MemOperand(x1, 16)), "str q31, [x1, #16]");
  COMPARE(str(q0, MemOperand(x3, 65520)), "str q0, [x3, #65520]");

  COMPARE(ldr(s24, MemOperand(sp)), "ldr s24, [sp]");
  COMPARE(ldr(d25, MemOperand(sp, 8)), "ldr d25, [sp, #8]");
  COMPARE(ldr(b26, MemOperand(sp, 1)), "ldr b26, [sp, #1]");
  COMPARE(ldr(h27, MemOperand(sp, 2)), "ldr h27, [sp, #2]");
  COMPARE(ldr(q28, MemOperand(sp, 16)), "ldr q28, [sp, #16]");

  CLEANUP();
}


TEST(load_store_v_pre) {
  SETUP();

  COMPARE(ldr(s0, MemOperand(x1, 4, PreIndex)), "ldr s0, [x1, #4]!");
  COMPARE(ldr(s2, MemOperand(x3, 255, PreIndex)), "ldr s2, [x3, #255]!");
  COMPARE(ldr(s4, MemOperand(x5, -256, PreIndex)), "ldr s4, [x5, #-256]!");
  COMPARE(ldr(d6, MemOperand(x7, 8, PreIndex)), "ldr d6, [x7, #8]!");
  COMPARE(ldr(d8, MemOperand(x9, 255, PreIndex)), "ldr d8, [x9, #255]!");
  COMPARE(ldr(d10, MemOperand(x11, -256, PreIndex)), "ldr d10, [x11, #-256]!");

  COMPARE(str(s12, MemOperand(x13, 4, PreIndex)), "str s12, [x13, #4]!");
  COMPARE(str(s14, MemOperand(x15, 255, PreIndex)), "str s14, [x15, #255]!");
  COMPARE(str(s16, MemOperand(x17, -256, PreIndex)), "str s16, [x17, #-256]!");
  COMPARE(str(d18, MemOperand(x19, 8, PreIndex)), "str d18, [x19, #8]!");
  COMPARE(str(d20, MemOperand(x21, 255, PreIndex)), "str d20, [x21, #255]!");
  COMPARE(str(d22, MemOperand(x23, -256, PreIndex)), "str d22, [x23, #-256]!");

  COMPARE(ldr(b0, MemOperand(x1, 1, PreIndex)), "ldr b0, [x1, #1]!");
  COMPARE(ldr(b2, MemOperand(x3, 255, PreIndex)), "ldr b2, [x3, #255]!");
  COMPARE(ldr(b4, MemOperand(x5, -256, PreIndex)), "ldr b4, [x5, #-256]!");
  COMPARE(ldr(h6, MemOperand(x7, 2, PreIndex)), "ldr h6, [x7, #2]!");
  COMPARE(ldr(h8, MemOperand(x9, 255, PreIndex)), "ldr h8, [x9, #255]!");
  COMPARE(ldr(h10, MemOperand(x11, -256, PreIndex)), "ldr h10, [x11, #-256]!");
  COMPARE(ldr(q12, MemOperand(x13, 16, PreIndex)), "ldr q12, [x13, #16]!");
  COMPARE(ldr(q14, MemOperand(x15, 255, PreIndex)), "ldr q14, [x15, #255]!");
  COMPARE(ldr(q16, MemOperand(x17, -256, PreIndex)), "ldr q16, [x17, #-256]!");

  COMPARE(str(b18, MemOperand(x19, 1, PreIndex)), "str b18, [x19, #1]!");
  COMPARE(str(b20, MemOperand(x21, 255, PreIndex)), "str b20, [x21, #255]!");
  COMPARE(str(b22, MemOperand(x23, -256, PreIndex)), "str b22, [x23, #-256]!");
  COMPARE(str(h24, MemOperand(x25, 2, PreIndex)), "str h24, [x25, #2]!");
  COMPARE(str(h26, MemOperand(x27, 255, PreIndex)), "str h26, [x27, #255]!");
  COMPARE(str(h28, MemOperand(x29, -256, PreIndex)), "str h28, [x29, #-256]!");
  COMPARE(str(q30, MemOperand(x1, 16, PreIndex)), "str q30, [x1, #16]!");
  COMPARE(str(q31, MemOperand(x3, 255, PreIndex)), "str q31, [x3, #255]!");
  COMPARE(str(q0, MemOperand(x5, -256, PreIndex)), "str q0, [x5, #-256]!");

  COMPARE(str(b24, MemOperand(sp, 1, PreIndex)), "str b24, [sp, #1]!");
  COMPARE(str(h25, MemOperand(sp, -2, PreIndex)), "str h25, [sp, #-2]!");
  COMPARE(str(s26, MemOperand(sp, 4, PreIndex)), "str s26, [sp, #4]!");
  COMPARE(str(d27, MemOperand(sp, -8, PreIndex)), "str d27, [sp, #-8]!");
  COMPARE(str(q28, MemOperand(sp, 16, PreIndex)), "str q28, [sp, #16]!");

  COMPARE(ldr(b0, MemOperand(x1, 0, PreIndex)), "ldr b0, [x1, #0]!");
  COMPARE(ldr(h2, MemOperand(x3, 0, PreIndex)), "ldr h2, [x3, #0]!");
  COMPARE(ldr(s4, MemOperand(x5, 0, PreIndex)), "ldr s4, [x5, #0]!");
  COMPARE(ldr(d6, MemOperand(x7, 0, PreIndex)), "ldr d6, [x7, #0]!");
  COMPARE(ldr(q8, MemOperand(x9, 0, PreIndex)), "ldr q8, [x9, #0]!");
  COMPARE(str(b0, MemOperand(x1, 0, PreIndex)), "str b0, [x1, #0]!");
  COMPARE(str(h2, MemOperand(x3, 0, PreIndex)), "str h2, [x3, #0]!");
  COMPARE(str(s4, MemOperand(x5, 0, PreIndex)), "str s4, [x5, #0]!");
  COMPARE(str(d6, MemOperand(x7, 0, PreIndex)), "str d6, [x7, #0]!");
  COMPARE(str(q8, MemOperand(x9, 0, PreIndex)), "str q8, [x9, #0]!");

  CLEANUP();
}


TEST(load_store_v_post) {
  SETUP();

  COMPARE(ldr(s0, MemOperand(x1, 4, PostIndex)), "ldr s0, [x1], #4");
  COMPARE(ldr(s2, MemOperand(x3, 255, PostIndex)), "ldr s2, [x3], #255");
  COMPARE(ldr(s4, MemOperand(x5, -256, PostIndex)), "ldr s4, [x5], #-256");
  COMPARE(ldr(d6, MemOperand(x7, 8, PostIndex)), "ldr d6, [x7], #8");
  COMPARE(ldr(d8, MemOperand(x9, 255, PostIndex)), "ldr d8, [x9], #255");
  COMPARE(ldr(d10, MemOperand(x11, -256, PostIndex)), "ldr d10, [x11], #-256");

  COMPARE(str(s12, MemOperand(x13, 4, PostIndex)), "str s12, [x13], #4");
  COMPARE(str(s14, MemOperand(x15, 255, PostIndex)), "str s14, [x15], #255");
  COMPARE(str(s16, MemOperand(x17, -256, PostIndex)), "str s16, [x17], #-256");
  COMPARE(str(d18, MemOperand(x19, 8, PostIndex)), "str d18, [x19], #8");
  COMPARE(str(d20, MemOperand(x21, 255, PostIndex)), "str d20, [x21], #255");
  COMPARE(str(d22, MemOperand(x23, -256, PostIndex)), "str d22, [x23], #-256");

  COMPARE(ldr(b0, MemOperand(x1, 4, PostIndex)), "ldr b0, [x1], #4");
  COMPARE(ldr(b2, MemOperand(x3, 255, PostIndex)), "ldr b2, [x3], #255");
  COMPARE(ldr(b4, MemOperand(x5, -256, PostIndex)), "ldr b4, [x5], #-256");
  COMPARE(ldr(h6, MemOperand(x7, 8, PostIndex)), "ldr h6, [x7], #8");
  COMPARE(ldr(h8, MemOperand(x9, 255, PostIndex)), "ldr h8, [x9], #255");
  COMPARE(ldr(h10, MemOperand(x11, -256, PostIndex)), "ldr h10, [x11], #-256");
  COMPARE(ldr(q12, MemOperand(x13, 8, PostIndex)), "ldr q12, [x13], #8");
  COMPARE(ldr(q14, MemOperand(x15, 255, PostIndex)), "ldr q14, [x15], #255");
  COMPARE(ldr(q16, MemOperand(x17, -256, PostIndex)), "ldr q16, [x17], #-256");

  COMPARE(str(b18, MemOperand(x19, 4, PostIndex)), "str b18, [x19], #4");
  COMPARE(str(b20, MemOperand(x21, 255, PostIndex)), "str b20, [x21], #255");
  COMPARE(str(b22, MemOperand(x23, -256, PostIndex)), "str b22, [x23], #-256");
  COMPARE(str(h24, MemOperand(x25, 8, PostIndex)), "str h24, [x25], #8");
  COMPARE(str(h26, MemOperand(x27, 255, PostIndex)), "str h26, [x27], #255");
  COMPARE(str(h28, MemOperand(x29, -256, PostIndex)), "str h28, [x29], #-256");
  COMPARE(str(q30, MemOperand(x1, 8, PostIndex)), "str q30, [x1], #8");
  COMPARE(str(q31, MemOperand(x3, 255, PostIndex)), "str q31, [x3], #255");
  COMPARE(str(q0, MemOperand(x5, -256, PostIndex)), "str q0, [x5], #-256");

  COMPARE(ldr(b24, MemOperand(sp, -1, PreIndex)), "ldr b24, [sp, #-1]!");
  COMPARE(ldr(h25, MemOperand(sp, 2, PreIndex)), "ldr h25, [sp, #2]!");
  COMPARE(ldr(s26, MemOperand(sp, -4, PreIndex)), "ldr s26, [sp, #-4]!");
  COMPARE(ldr(d27, MemOperand(sp, 8, PreIndex)), "ldr d27, [sp, #8]!");
  COMPARE(ldr(q28, MemOperand(sp, -16, PreIndex)), "ldr q28, [sp, #-16]!");

  COMPARE(ldr(b0, MemOperand(x1, 0, PostIndex)), "ldr b0, [x1], #0");
  COMPARE(ldr(h2, MemOperand(x3, 0, PostIndex)), "ldr h2, [x3], #0");
  COMPARE(ldr(s4, MemOperand(x5, 0, PostIndex)), "ldr s4, [x5], #0");
  COMPARE(ldr(d6, MemOperand(x7, 0, PostIndex)), "ldr d6, [x7], #0");
  COMPARE(ldr(q8, MemOperand(x9, 0, PostIndex)), "ldr q8, [x9], #0");
  COMPARE(str(b0, MemOperand(x1, 0, PostIndex)), "str b0, [x1], #0");
  COMPARE(str(h2, MemOperand(x3, 0, PostIndex)), "str h2, [x3], #0");
  COMPARE(str(s4, MemOperand(x5, 0, PostIndex)), "str s4, [x5], #0");
  COMPARE(str(d6, MemOperand(x7, 0, PostIndex)), "str d6, [x7], #0");
  COMPARE(str(q8, MemOperand(x9, 0, PostIndex)), "str q8, [x9], #0");

  CLEANUP();
}


TEST(load_store_v_regoffset) {
  SETUP();

  COMPARE(ldr(b0, MemOperand(x1, x2)), "ldr b0, [x1, x2]");
  COMPARE(ldr(b1, MemOperand(x2, w3, UXTW)), "ldr b1, [x2, w3, uxtw]");
  COMPARE(ldr(b2, MemOperand(x3, w4, SXTW)), "ldr b2, [x3, w4, sxtw]");
  // We can't assemble this instruction, but we check it disassembles correctly.
  COMPARE(dci(0x3c657883), "ldr b3, [x4, x5, lsl #0]");
  COMPARE(ldr(b30, MemOperand(sp, xzr)), "ldr b30, [sp, xzr]");
  COMPARE(ldr(b31, MemOperand(sp, wzr, UXTW)), "ldr b31, [sp, wzr, uxtw]");

  COMPARE(ldr(h0, MemOperand(x1, x2)), "ldr h0, [x1, x2]");
  COMPARE(ldr(h1, MemOperand(x2, w3, UXTW)), "ldr h1, [x2, w3, uxtw]");
  COMPARE(ldr(h2, MemOperand(x3, w4, SXTW)), "ldr h2, [x3, w4, sxtw]");
  COMPARE(ldr(h3, MemOperand(x4, w5, UXTW, 1)), "ldr h3, [x4, w5, uxtw #1]");
  COMPARE(ldr(h4, MemOperand(x5, w5, SXTW, 1)), "ldr h4, [x5, w5, sxtw #1]");
  COMPARE(ldr(h30, MemOperand(sp, xzr)), "ldr h30, [sp, xzr]");
  COMPARE(ldr(h31, MemOperand(sp, wzr, SXTW, 1)),
          "ldr h31, [sp, wzr, sxtw #1]");

  COMPARE(ldr(s0, MemOperand(x1, x2)), "ldr s0, [x1, x2]");
  COMPARE(ldr(s1, MemOperand(x2, w3, UXTW)), "ldr s1, [x2, w3, uxtw]");
  COMPARE(ldr(s2, MemOperand(x3, w4, SXTW)), "ldr s2, [x3, w4, sxtw]");
  COMPARE(ldr(s3, MemOperand(x4, w5, UXTW, 2)), "ldr s3, [x4, w5, uxtw #2]");
  COMPARE(ldr(s4, MemOperand(x5, w5, SXTW, 2)), "ldr s4, [x5, w5, sxtw #2]");
  COMPARE(ldr(s30, MemOperand(sp, xzr)), "ldr s30, [sp, xzr]");
  COMPARE(ldr(s31, MemOperand(sp, wzr, SXTW, 2)),
          "ldr s31, [sp, wzr, sxtw #2]");

  COMPARE(ldr(d0, MemOperand(x1, x2)), "ldr d0, [x1, x2]");
  COMPARE(ldr(d1, MemOperand(x2, w3, UXTW)), "ldr d1, [x2, w3, uxtw]");
  COMPARE(ldr(d2, MemOperand(x3, w4, SXTW)), "ldr d2, [x3, w4, sxtw]");
  COMPARE(ldr(d3, MemOperand(x4, w5, UXTW, 3)), "ldr d3, [x4, w5, uxtw #3]");
  COMPARE(ldr(d4, MemOperand(x5, w5, SXTW, 3)), "ldr d4, [x5, w5, sxtw #3]");
  COMPARE(ldr(d30, MemOperand(sp, xzr)), "ldr d30, [sp, xzr]");
  COMPARE(ldr(d31, MemOperand(sp, wzr, SXTW, 3)),
          "ldr d31, [sp, wzr, sxtw #3]");

  COMPARE(ldr(q0, MemOperand(x1, x2)), "ldr q0, [x1, x2]");
  COMPARE(ldr(q1, MemOperand(x2, w3, UXTW)), "ldr q1, [x2, w3, uxtw]");
  COMPARE(ldr(q2, MemOperand(x3, w4, SXTW)), "ldr q2, [x3, w4, sxtw]");
  COMPARE(ldr(q3, MemOperand(x4, w5, UXTW, 4)), "ldr q3, [x4, w5, uxtw #4]");
  COMPARE(ldr(q4, MemOperand(x5, w5, SXTW, 4)), "ldr q4, [x5, w5, sxtw #4]");
  COMPARE(ldr(q30, MemOperand(sp, xzr)), "ldr q30, [sp, xzr]");
  COMPARE(ldr(q31, MemOperand(sp, wzr, SXTW, 4)),
          "ldr q31, [sp, wzr, sxtw #4]");

  COMPARE(str(b0, MemOperand(x1, x2)), "str b0, [x1, x2]");
  COMPARE(str(b1, MemOperand(x2, w3, UXTW)), "str b1, [x2, w3, uxtw]");
  COMPARE(str(b2, MemOperand(x3, w4, SXTW)), "str b2, [x3, w4, sxtw]");
  // We can't assemble this instruction, but we check it disassembles correctly.
  COMPARE(dci(0x3c257883), "str b3, [x4, x5, lsl #0]");
  COMPARE(str(b30, MemOperand(sp, xzr)), "str b30, [sp, xzr]");
  COMPARE(str(b31, MemOperand(sp, wzr, UXTW)), "str b31, [sp, wzr, uxtw]");

  COMPARE(str(h0, MemOperand(x1, x2)), "str h0, [x1, x2]");
  COMPARE(str(h1, MemOperand(x2, w3, UXTW)), "str h1, [x2, w3, uxtw]");
  COMPARE(str(h2, MemOperand(x3, w4, SXTW)), "str h2, [x3, w4, sxtw]");
  COMPARE(str(h3, MemOperand(x4, w5, UXTW, 1)), "str h3, [x4, w5, uxtw #1]");
  COMPARE(str(h4, MemOperand(x5, w5, SXTW, 1)), "str h4, [x5, w5, sxtw #1]");
  COMPARE(str(h30, MemOperand(sp, xzr)), "str h30, [sp, xzr]");
  COMPARE(str(h31, MemOperand(sp, wzr, SXTW, 1)),
          "str h31, [sp, wzr, sxtw #1]");

  COMPARE(str(s0, MemOperand(x1, x2)), "str s0, [x1, x2]");
  COMPARE(str(s1, MemOperand(x2, w3, UXTW)), "str s1, [x2, w3, uxtw]");
  COMPARE(str(s2, MemOperand(x3, w4, SXTW)), "str s2, [x3, w4, sxtw]");
  COMPARE(str(s3, MemOperand(x4, w5, UXTW, 2)), "str s3, [x4, w5, uxtw #2]");
  COMPARE(str(s4, MemOperand(x5, w5, SXTW, 2)), "str s4, [x5, w5, sxtw #2]");
  COMPARE(str(s30, MemOperand(sp, xzr)), "str s30, [sp, xzr]");
  COMPARE(str(s31, MemOperand(sp, wzr, SXTW, 2)),
          "str s31, [sp, wzr, sxtw #2]");

  COMPARE(str(d0, MemOperand(x1, x2)), "str d0, [x1, x2]");
  COMPARE(str(d1, MemOperand(x2, w3, UXTW)), "str d1, [x2, w3, uxtw]");
  COMPARE(str(d2, MemOperand(x3, w4, SXTW)), "str d2, [x3, w4, sxtw]");
  COMPARE(str(d3, MemOperand(x4, w5, UXTW, 3)), "str d3, [x4, w5, uxtw #3]");
  COMPARE(str(d4, MemOperand(x5, w5, SXTW, 3)), "str d4, [x5, w5, sxtw #3]");
  COMPARE(str(d30, MemOperand(sp, xzr)), "str d30, [sp, xzr]");
  COMPARE(str(d31, MemOperand(sp, wzr, SXTW, 3)),
          "str d31, [sp, wzr, sxtw #3]");

  COMPARE(str(q0, MemOperand(x1, x2)), "str q0, [x1, x2]");
  COMPARE(str(q1, MemOperand(x2, w3, UXTW)), "str q1, [x2, w3, uxtw]");
  COMPARE(str(q2, MemOperand(x3, w4, SXTW)), "str q2, [x3, w4, sxtw]");
  COMPARE(str(q3, MemOperand(x4, w5, UXTW, 4)), "str q3, [x4, w5, uxtw #4]");
  COMPARE(str(q4, MemOperand(x5, w5, SXTW, 4)), "str q4, [x5, w5, sxtw #4]");
  COMPARE(str(q30, MemOperand(sp, xzr)), "str q30, [sp, xzr]");
  COMPARE(str(q31, MemOperand(sp, wzr, SXTW, 4)),
          "str q31, [sp, wzr, sxtw #4]");

  CLEANUP();
}

#define VLIST2(v) \
  v, VRegister((v.GetCode() + 1) % 32, v.GetSizeInBits(), v.GetLanes())
#define VLIST3(v) \
  VLIST2(v), VRegister((v.GetCode() + 2) % 32, v.GetSizeInBits(), v.GetLanes())
#define VLIST4(v) \
  VLIST3(v), VRegister((v.GetCode() + 3) % 32, v.GetSizeInBits(), v.GetLanes())


#define NEON_FORMAT_LIST(V) \
  V(V8B(), "8b")            \
  V(V16B(), "16b")          \
  V(V4H(), "4h")            \
  V(V8H(), "8h")            \
  V(V2S(), "2s")            \
  V(V4S(), "4s")            \
  V(V2D(), "2d")

#define NEON_FORMAT_LIST_LP(V)  \
  V(V4H(), "4h", V8B(), "8b")   \
  V(V2S(), "2s", V4H(), "4h")   \
  V(V1D(), "1d", V2S(), "2s")   \
  V(V8H(), "8h", V16B(), "16b") \
  V(V4S(), "4s", V8H(), "8h")   \
  V(V2D(), "2d", V4S(), "4s")

#define NEON_FORMAT_LIST_LW(V) \
  V(V8H(), "8h", V8B(), "8b")  \
  V(V4S(), "4s", V4H(), "4h")  \
  V(V2D(), "2d", V2S(), "2s")

#define NEON_FORMAT_LIST_LW2(V) \
  V(V8H(), "8h", V16B(), "16b") \
  V(V4S(), "4s", V8H(), "8h")   \
  V(V2D(), "2d", V4S(), "4s")

#define NEON_FORMAT_LIST_BHS(V) \
  V(V8B(), "8b")                \
  V(V16B(), "16b")              \
  V(V4H(), "4h")                \
  V(V8H(), "8h")                \
  V(V2S(), "2s")                \
  V(V4S(), "4s")

#define NEON_FORMAT_LIST_HS(V) \
  V(V4H(), "4h")               \
  V(V8H(), "8h")               \
  V(V2S(), "2s")               \
  V(V4S(), "4s")

TEST(neon_load_store_vector) {
  SETUP();

#define DISASM_INST(M, S)                                               \
  COMPARE_MACRO(Ld1(v0.M, MemOperand(x15)), "ld1 {v0." S "}, [x15]");   \
  COMPARE_MACRO(Ld1(v1.M, v2.M, MemOperand(x16)),                       \
                "ld1 {v1." S ", v2." S "}, [x16]");                     \
  COMPARE_MACRO(Ld1(v3.M, v4.M, v5.M, MemOperand(x17)),                 \
                "ld1 {v3." S ", v4." S ", v5." S "}, [x17]");           \
  COMPARE_MACRO(Ld1(v6.M, v7.M, v8.M, v9.M, MemOperand(x18)),           \
                "ld1 {v6." S ", v7." S ", v8." S ", v9." S "}, [x18]")  \
  COMPARE_MACRO(Ld1(v30.M, v31.M, v0.M, v1.M, MemOperand(sp)),          \
                "ld1 {v30." S ", v31." S ", v0." S ", v1." S "}, [sp]") \
  COMPARE_MACRO(Ld2(v1.M, v2.M, MemOperand(x16)),                       \
                "ld2 {v1." S ", v2." S "}, [x16]");                     \
  COMPARE_MACRO(Ld3(v3.M, v4.M, v5.M, MemOperand(x17)),                 \
                "ld3 {v3." S ", v4." S ", v5." S "}, [x17]");           \
  COMPARE_MACRO(Ld4(v6.M, v7.M, v8.M, v9.M, MemOperand(x18)),           \
                "ld4 {v6." S ", v7." S ", v8." S ", v9." S "}, [x18]")  \
  COMPARE_MACRO(Ld4(v30.M, v31.M, v0.M, v1.M, MemOperand(sp)),          \
                "ld4 {v30." S ", v31." S ", v0." S ", v1." S "}, [sp]") \
  NEON_FORMAT_LIST(DISASM_INST);
#undef DISASM_INST

#define DISASM_INST(M, S)                                                      \
  COMPARE_MACRO(Ld1(v0.M, MemOperand(x15, x20, PostIndex)),                    \
                "ld1 {v0." S "}, [x15], x20");                                 \
  COMPARE_MACRO(Ld1(v1.M, v2.M, MemOperand(x16, x21, PostIndex)),              \
                "ld1 {v1." S ", v2." S "}, [x16], x21");                       \
  COMPARE_MACRO(Ld1(v3.M, v4.M, v5.M, MemOperand(x17, x22, PostIndex)),        \
                "ld1 {v3." S ", v4." S ", v5." S "}, [x17], x22");             \
  COMPARE_MACRO(Ld1(v6.M, v7.M, v8.M, v9.M, MemOperand(x18, x23, PostIndex)),  \
                "ld1 {v6." S ", v7." S ", v8." S ", v9." S "}, [x18], x23")    \
  COMPARE_MACRO(Ld1(v30.M, v31.M, v0.M, v1.M, MemOperand(sp, x24, PostIndex)), \
                "ld1 {v30." S ", v31." S ", v0." S ", v1." S "}, [sp], x24")   \
  COMPARE_MACRO(Ld2(v1.M, v2.M, MemOperand(x16, x21, PostIndex)),              \
                "ld2 {v1." S ", v2." S "}, [x16], x21");                       \
  COMPARE_MACRO(Ld3(v3.M, v4.M, v5.M, MemOperand(x17, x22, PostIndex)),        \
                "ld3 {v3." S ", v4." S ", v5." S "}, [x17], x22");             \
  COMPARE_MACRO(Ld4(v6.M, v7.M, v8.M, v9.M, MemOperand(x18, x23, PostIndex)),  \
                "ld4 {v6." S ", v7." S ", v8." S ", v9." S "}, [x18], x23")    \
  COMPARE_MACRO(Ld4(v30.M, v31.M, v0.M, v1.M, MemOperand(sp, x24, PostIndex)), \
                "ld4 {v30." S ", v31." S ", v0." S ", v1." S "}, [sp], x24")   \
  NEON_FORMAT_LIST(DISASM_INST);
#undef DISASM_INST

  COMPARE_MACRO(Ld1(v0.V8B(), MemOperand(x15, 8, PostIndex)),
                "ld1 {v0.8b}, [x15], #8");
  COMPARE_MACRO(Ld1(v1.V16B(), MemOperand(x16, 16, PostIndex)),
                "ld1 {v1.16b}, [x16], #16");
  COMPARE_MACRO(Ld1(v2.V4H(), v3.V4H(), MemOperand(x17, 16, PostIndex)),
                "ld1 {v2.4h, v3.4h}, [x17], #16");
  COMPARE_MACRO(Ld1(v4.V8H(), v5.V8H(), MemOperand(x18, 32, PostIndex)),
                "ld1 {v4.8h, v5.8h}, [x18], #32");
  COMPARE_MACRO(Ld1(v16.V2S(),
                    v17.V2S(),
                    v18.V2S(),
                    MemOperand(x19, 24, PostIndex)),
                "ld1 {v16.2s, v17.2s, v18.2s}, [x19], #24");
  COMPARE_MACRO(Ld1(v16.V4S(),
                    v17.V4S(),
                    v18.V4S(),
                    MemOperand(x19, 48, PostIndex)),
                "ld1 {v16.4s, v17.4s, v18.4s}, [x19], #48");
  COMPARE_MACRO(Ld1(v19.V2S(),
                    v20.V2S(),
                    v21.V2S(),
                    v22.V2S(),
                    MemOperand(x20, 32, PostIndex)),
                "ld1 {v19.2s, v20.2s, v21.2s, v22.2s}, [x20], #32");
  COMPARE_MACRO(Ld1(v23.V2D(),
                    v24.V2D(),
                    v25.V2D(),
                    v26.V2D(),
                    MemOperand(x21, 64, PostIndex)),
                "ld1 {v23.2d, v24.2d, v25.2d, v26.2d}, [x21], #64");

  COMPARE_MACRO(Ld2(v2.V4H(), v3.V4H(), MemOperand(x17, 16, PostIndex)),
                "ld2 {v2.4h, v3.4h}, [x17], #16");
  COMPARE_MACRO(Ld2(v4.V8H(), v5.V8H(), MemOperand(x18, 32, PostIndex)),
                "ld2 {v4.8h, v5.8h}, [x18], #32");
  COMPARE_MACRO(Ld3(v16.V2S(),
                    v17.V2S(),
                    v18.V2S(),
                    MemOperand(x19, 24, PostIndex)),
                "ld3 {v16.2s, v17.2s, v18.2s}, [x19], #24");
  COMPARE_MACRO(Ld3(v16.V4S(),
                    v17.V4S(),
                    v18.V4S(),
                    MemOperand(x19, 48, PostIndex)),
                "ld3 {v16.4s, v17.4s, v18.4s}, [x19], #48");
  COMPARE_MACRO(Ld4(v19.V2S(),
                    v20.V2S(),
                    v21.V2S(),
                    v22.V2S(),
                    MemOperand(x20, 32, PostIndex)),
                "ld4 {v19.2s, v20.2s, v21.2s, v22.2s}, [x20], #32");
  COMPARE_MACRO(Ld4(v23.V2D(),
                    v24.V2D(),
                    v25.V2D(),
                    v26.V2D(),
                    MemOperand(x21, 64, PostIndex)),
                "ld4 {v23.2d, v24.2d, v25.2d, v26.2d}, [x21], #64");

  COMPARE_MACRO(Ld1(v0.V1D(), MemOperand(x16)), "ld1 {v0.1d}, [x16]");
  COMPARE_MACRO(Ld1(v1.V1D(), v2.V1D(), MemOperand(x17, 16, PostIndex)),
                "ld1 {v1.1d, v2.1d}, [x17], #16");
  COMPARE_MACRO(Ld1(v3.V1D(),
                    v4.V1D(),
                    v5.V1D(),
                    MemOperand(x18, x19, PostIndex)),
                "ld1 {v3.1d, v4.1d, v5.1d}, [x18], x19");
  COMPARE_MACRO(Ld1(v30.V1D(),
                    v31.V1D(),
                    v0.V1D(),
                    v1.V1D(),
                    MemOperand(x20, 32, PostIndex)),
                "ld1 {v30.1d, v31.1d, v0.1d, v1.1d}, [x20], #32");
  COMPARE_MACRO(Ld1(d30, d31, d0, d1, MemOperand(x21, x22, PostIndex)),
                "ld1 {v30.1d, v31.1d, v0.1d, v1.1d}, [x21], x22");

#define DISASM_INST(M, S)                                                   \
  COMPARE_MACRO(St1(v20.M, MemOperand(x15)), "st1 {v20." S "}, [x15]");     \
  COMPARE_MACRO(St1(v21.M, v22.M, MemOperand(x16)),                         \
                "st1 {v21." S ", v22." S "}, [x16]");                       \
  COMPARE_MACRO(St1(v23.M, v24.M, v25.M, MemOperand(x17)),                  \
                "st1 {v23." S ", v24." S ", v25." S "}, [x17]");            \
  COMPARE_MACRO(St1(v26.M, v27.M, v28.M, v29.M, MemOperand(x18)),           \
                "st1 {v26." S ", v27." S ", v28." S ", v29." S "}, [x18]"); \
  COMPARE_MACRO(St1(v30.M, v31.M, v0.M, v1.M, MemOperand(sp)),              \
                "st1 {v30." S ", v31." S ", v0." S ", v1." S "}, [sp]");    \
  COMPARE_MACRO(St2(VLIST2(v21.M), MemOperand(x16)),                        \
                "st2 {v21." S ", v22." S "}, [x16]");                       \
  COMPARE_MACRO(St3(v23.M, v24.M, v25.M, MemOperand(x17)),                  \
                "st3 {v23." S ", v24." S ", v25." S "}, [x17]");            \
  COMPARE_MACRO(St4(v30.M, v31.M, v0.M, v1.M, MemOperand(sp)),              \
                "st4 {v30." S ", v31." S ", v0." S ", v1." S "}, [sp]");
  NEON_FORMAT_LIST(DISASM_INST);
#undef DISASM_INST

#define DISASM_INST(M, S)                                                      \
  COMPARE_MACRO(St1(v0.M, MemOperand(x15, x20, PostIndex)),                    \
                "st1 {v0." S "}, [x15], x20");                                 \
  COMPARE_MACRO(St1(v1.M, v2.M, MemOperand(x16, x21, PostIndex)),              \
                "st1 {v1." S ", v2." S "}, [x16], x21");                       \
  COMPARE_MACRO(St1(v3.M, v4.M, v5.M, MemOperand(x17, x22, PostIndex)),        \
                "st1 {v3." S ", v4." S ", v5." S "}, [x17], x22");             \
  COMPARE_MACRO(St1(v6.M, v7.M, v8.M, v9.M, MemOperand(x18, x23, PostIndex)),  \
                "st1 {v6." S ", v7." S ", v8." S ", v9." S "}, [x18], x23");   \
  COMPARE_MACRO(St1(v30.M, v31.M, v0.M, v1.M, MemOperand(sp, x24, PostIndex)), \
                "st1 {v30." S ", v31." S ", v0." S ", v1." S "}, [sp], x24");  \
  COMPARE_MACRO(St2(v1.M, v2.M, MemOperand(x16, x21, PostIndex)),              \
                "st2 {v1." S ", v2." S "}, [x16], x21");                       \
  COMPARE_MACRO(St3(v3.M, v4.M, v5.M, MemOperand(x17, x22, PostIndex)),        \
                "st3 {v3." S ", v4." S ", v5." S "}, [x17], x22");             \
  COMPARE_MACRO(St4(v6.M, v7.M, v8.M, v9.M, MemOperand(x18, x23, PostIndex)),  \
                "st4 {v6." S ", v7." S ", v8." S ", v9." S "}, [x18], x23");   \
  COMPARE_MACRO(St4(v30.M, v31.M, v0.M, v1.M, MemOperand(sp, x24, PostIndex)), \
                "st4 {v30." S ", v31." S ", v0." S ", v1." S "}, [sp], x24");
  NEON_FORMAT_LIST(DISASM_INST);
#undef DISASM_INST

  COMPARE_MACRO(St1(v0.V8B(), MemOperand(x15, 8, PostIndex)),
                "st1 {v0.8b}, [x15], #8");
  COMPARE_MACRO(St1(v1.V16B(), MemOperand(x16, 16, PostIndex)),
                "st1 {v1.16b}, [x16], #16");
  COMPARE_MACRO(St1(v2.V4H(), v3.V4H(), MemOperand(x17, 16, PostIndex)),
                "st1 {v2.4h, v3.4h}, [x17], #16");
  COMPARE_MACRO(St1(v4.V8H(), v5.V8H(), MemOperand(x18, 32, PostIndex)),
                "st1 {v4.8h, v5.8h}, [x18], #32");
  COMPARE_MACRO(St1(v16.V2S(),
                    v17.V2S(),
                    v18.V2S(),
                    MemOperand(x19, 24, PostIndex)),
                "st1 {v16.2s, v17.2s, v18.2s}, [x19], #24");
  COMPARE_MACRO(St1(v16.V4S(),
                    v17.V4S(),
                    v18.V4S(),
                    MemOperand(x19, 48, PostIndex)),
                "st1 {v16.4s, v17.4s, v18.4s}, [x19], #48");
  COMPARE_MACRO(St1(v19.V2S(),
                    v20.V2S(),
                    v21.V2S(),
                    v22.V2S(),
                    MemOperand(x20, 32, PostIndex)),
                "st1 {v19.2s, v20.2s, v21.2s, v22.2s}, [x20], #32");
  COMPARE_MACRO(St1(v23.V2D(),
                    v24.V2D(),
                    v25.V2D(),
                    v26.V2D(),
                    MemOperand(x21, 64, PostIndex)),
                "st1 {v23.2d, v24.2d, v25.2d, v26.2d}, [x21], #64");
  COMPARE_MACRO(St2(v1.V16B(), v2.V16B(), MemOperand(x16, 32, PostIndex)),
                "st2 {v1.16b, v2.16b}, [x16], #32");
  COMPARE_MACRO(St2(v2.V4H(), v3.V4H(), MemOperand(x17, 16, PostIndex)),
                "st2 {v2.4h, v3.4h}, [x17], #16");
  COMPARE_MACRO(St2(v4.V8H(), v5.V8H(), MemOperand(x18, 32, PostIndex)),
                "st2 {v4.8h, v5.8h}, [x18], #32");
  COMPARE_MACRO(St3(v16.V2S(),
                    v17.V2S(),
                    v18.V2S(),
                    MemOperand(x19, 24, PostIndex)),
                "st3 {v16.2s, v17.2s, v18.2s}, [x19], #24");
  COMPARE_MACRO(St3(v16.V4S(),
                    v17.V4S(),
                    v18.V4S(),
                    MemOperand(x19, 48, PostIndex)),
                "st3 {v16.4s, v17.4s, v18.4s}, [x19], #48");
  COMPARE_MACRO(St4(v19.V2S(),
                    v20.V2S(),
                    v21.V2S(),
                    v22.V2S(),
                    MemOperand(x20, 32, PostIndex)),
                "st4 {v19.2s, v20.2s, v21.2s, v22.2s}, [x20], #32");
  COMPARE_MACRO(St4(v23.V2D(),
                    v24.V2D(),
                    v25.V2D(),
                    v26.V2D(),
                    MemOperand(x21, 64, PostIndex)),
                "st4 {v23.2d, v24.2d, v25.2d, v26.2d}, [x21], #64");

  COMPARE_MACRO(St1(v0.V1D(), MemOperand(x16)), "st1 {v0.1d}, [x16]");
  COMPARE_MACRO(St1(v1.V1D(), v2.V1D(), MemOperand(x17, 16, PostIndex)),
                "st1 {v1.1d, v2.1d}, [x17], #16");
  COMPARE_MACRO(St1(v3.V1D(),
                    v4.V1D(),
                    v5.V1D(),
                    MemOperand(x18, x19, PostIndex)),
                "st1 {v3.1d, v4.1d, v5.1d}, [x18], x19");
  COMPARE_MACRO(St1(v30.V1D(),
                    v31.V1D(),
                    v0.V1D(),
                    v1.V1D(),
                    MemOperand(x20, 32, PostIndex)),
                "st1 {v30.1d, v31.1d, v0.1d, v1.1d}, [x20], #32");
  COMPARE_MACRO(St1(d30, d31, d0, d1, MemOperand(x21, x22, PostIndex)),
                "st1 {v30.1d, v31.1d, v0.1d, v1.1d}, [x21], x22");

  CLEANUP();
}


TEST(neon_load_store_vector_unallocated) {
  SETUP();

  // Encodings marked as unallocated during decoding to an instruction class.
  const char* expected = "unallocated (Unallocated)";

  // Encodings marked as unallocated within instruction classes
  const char* expected_2 = "unallocated (NEONLoadStoreMultiStruct)";
  const char* expected_3 = "unallocated (NEONLoadStoreMultiStructPostIndex)";

  // LD[1-4] (multiple structures) (no offset)
  COMPARE(dci(0x0c401000), expected);    // opcode = 0b0001
  COMPARE(dci(0x0c403000), expected);    // opcode = 0b0011
  COMPARE(dci(0x0c405000), expected);    // opcode = 0b0101
  COMPARE(dci(0x0c409000), expected);    // opcode = 0b1001
  COMPARE(dci(0x0c40b000), expected);    // opcode = 0b1011
  COMPARE(dci(0x0c40c000), expected);    // opcode = 0b1100
  COMPARE(dci(0x0c40d000), expected);    // opcode = 0b1101
  COMPARE(dci(0x0c40e000), expected);    // opcode = 0b1110
  COMPARE(dci(0x0c40f000), expected);    // opcode = 0b1111
  COMPARE(dci(0x0c400c00), expected_2);  // opcode = 0b0000, size:Q = 0b110
  COMPARE(dci(0x0c404c00), expected_2);  // opcode = 0b0100, size:Q = 0b110
  COMPARE(dci(0x0c408c00), expected_2);  // opcode = 0b1000, size:Q = 0b110

  // ST[1-4] (multiple structures) (no offset)
  COMPARE(dci(0x0c001000), expected);    // opcode = 0b0001
  COMPARE(dci(0x0c003000), expected);    // opcode = 0b0011
  COMPARE(dci(0x0c005000), expected);    // opcode = 0b0101
  COMPARE(dci(0x0c009000), expected);    // opcode = 0b1001
  COMPARE(dci(0x0c00b000), expected);    // opcode = 0b1011
  COMPARE(dci(0x0c00c000), expected);    // opcode = 0b1100
  COMPARE(dci(0x0c00d000), expected);    // opcode = 0b1101
  COMPARE(dci(0x0c00e000), expected);    // opcode = 0b1110
  COMPARE(dci(0x0c00f000), expected);    // opcode = 0b1111
  COMPARE(dci(0x0c000c00), expected_2);  // opcode = 0b0000, size:Q = 0b110
  COMPARE(dci(0x0c004c00), expected_2);  // opcode = 0b0100, size:Q = 0b110
  COMPARE(dci(0x0c008c00), expected_2);  // opcode = 0b1000, size:Q = 0b110

  // LD[1-4] (multiple structures) (post index)
  COMPARE(dci(0x0cc01000), expected);    // opcode = 0b0001
  COMPARE(dci(0x0cc03000), expected);    // opcode = 0b0011
  COMPARE(dci(0x0cc05000), expected);    // opcode = 0b0101
  COMPARE(dci(0x0cc09000), expected);    // opcode = 0b1001
  COMPARE(dci(0x0cc0b000), expected);    // opcode = 0b1011
  COMPARE(dci(0x0cc0c000), expected);    // opcode = 0b1100
  COMPARE(dci(0x0cc0d000), expected);    // opcode = 0b1101
  COMPARE(dci(0x0cc0e000), expected);    // opcode = 0b1110
  COMPARE(dci(0x0cc0f000), expected);    // opcode = 0b1111
  COMPARE(dci(0x0cc00c00), expected_3);  // opcode = 0b0000, size:Q = 0b110
  COMPARE(dci(0x0cc04c00), expected_3);  // opcode = 0b0100, size:Q = 0b110
  COMPARE(dci(0x0cc08c00), expected_3);  // opcode = 0b1000, size:Q = 0b110

  // ST[1-4] (multiple structures) (post index)
  COMPARE(dci(0x0c801000), expected);    // opcode = 0b0001
  COMPARE(dci(0x0c803000), expected);    // opcode = 0b0011
  COMPARE(dci(0x0c805000), expected);    // opcode = 0b0101
  COMPARE(dci(0x0c809000), expected);    // opcode = 0b1001
  COMPARE(dci(0x0c80b000), expected);    // opcode = 0b1011
  COMPARE(dci(0x0c80c000), expected);    // opcode = 0b1100
  COMPARE(dci(0x0c80d000), expected);    // opcode = 0b1101
  COMPARE(dci(0x0c80e000), expected);    // opcode = 0b1110
  COMPARE(dci(0x0c80f000), expected);    // opcode = 0b1111
  COMPARE(dci(0x0c800c00), expected_3);  // opcode = 0b0000, size:Q = 0b110
  COMPARE(dci(0x0c804c00), expected_3);  // opcode = 0b0100, size:Q = 0b110
  COMPARE(dci(0x0c808c00), expected_3);  // opcode = 0b1000, size:Q = 0b110

  CLEANUP();
}


TEST(neon_load_store_lane) {
  SETUP();

  COMPARE_MACRO(Ld1(v0.V8B(), 0, MemOperand(x15)), "ld1 {v0.b}[0], [x15]");
  COMPARE_MACRO(Ld1(v1.V16B(), 1, MemOperand(x16)), "ld1 {v1.b}[1], [x16]");
  COMPARE_MACRO(Ld1(v2.V4H(), 2, MemOperand(x17)), "ld1 {v2.h}[2], [x17]");
  COMPARE_MACRO(Ld1(v3.V8H(), 3, MemOperand(x18)), "ld1 {v3.h}[3], [x18]");
  COMPARE_MACRO(Ld1(v4.V2S(), 0, MemOperand(x19)), "ld1 {v4.s}[0], [x19]");
  COMPARE_MACRO(Ld1(v5.V4S(), 1, MemOperand(x20)), "ld1 {v5.s}[1], [x20]");
  COMPARE_MACRO(Ld1(v6.V2D(), 0, MemOperand(x21)), "ld1 {v6.d}[0], [x21]");
  COMPARE_MACRO(Ld1(v7.B(), 7, MemOperand(x22)), "ld1 {v7.b}[7], [x22]");
  COMPARE_MACRO(Ld1(v8.B(), 15, MemOperand(x23)), "ld1 {v8.b}[15], [x23]");
  COMPARE_MACRO(Ld1(v9.H(), 3, MemOperand(x24)), "ld1 {v9.h}[3], [x24]");
  COMPARE_MACRO(Ld1(v10.H(), 7, MemOperand(x25)), "ld1 {v10.h}[7], [x25]");
  COMPARE_MACRO(Ld1(v11.S(), 1, MemOperand(x26)), "ld1 {v11.s}[1], [x26]");
  COMPARE_MACRO(Ld1(v12.S(), 3, MemOperand(x27)), "ld1 {v12.s}[3], [x27]");
  COMPARE_MACRO(Ld1(v13.D(), 1, MemOperand(sp)), "ld1 {v13.d}[1], [sp]");

  COMPARE_MACRO(Ld1(v0.V8B(), 0, MemOperand(x15, x0, PostIndex)),
                "ld1 {v0.b}[0], [x15], x0");
  COMPARE_MACRO(Ld1(v1.V16B(), 1, MemOperand(x16, 1, PostIndex)),
                "ld1 {v1.b}[1], [x16], #1");
  COMPARE_MACRO(Ld1(v2.V4H(), 2, MemOperand(x17, 2, PostIndex)),
                "ld1 {v2.h}[2], [x17], #2");
  COMPARE_MACRO(Ld1(v3.V8H(), 3, MemOperand(x18, x1, PostIndex)),
                "ld1 {v3.h}[3], [x18], x1");
  COMPARE_MACRO(Ld1(v4.V2S(), 0, MemOperand(x19, x2, PostIndex)),
                "ld1 {v4.s}[0], [x19], x2");
  COMPARE_MACRO(Ld1(v5.V4S(), 1, MemOperand(x20, 4, PostIndex)),
                "ld1 {v5.s}[1], [x20], #4");
  COMPARE_MACRO(Ld1(v6.V2D(), 0, MemOperand(x21, 8, PostIndex)),
                "ld1 {v6.d}[0], [x21], #8");
  COMPARE_MACRO(Ld1(v7.B(), 7, MemOperand(x22, 1, PostIndex)),
                "ld1 {v7.b}[7], [x22], #1");
  COMPARE_MACRO(Ld1(v8.B(), 15, MemOperand(x23, x3, PostIndex)),
                "ld1 {v8.b}[15], [x23], x3");
  COMPARE_MACRO(Ld1(v9.H(), 3, MemOperand(x24, x4, PostIndex)),
                "ld1 {v9.h}[3], [x24], x4");
  COMPARE_MACRO(Ld1(v10.H(), 7, MemOperand(x25, 2, PostIndex)),
                "ld1 {v10.h}[7], [x25], #2");
  COMPARE_MACRO(Ld1(v11.S(), 1, MemOperand(x26, 4, PostIndex)),
                "ld1 {v11.s}[1], [x26], #4");
  COMPARE_MACRO(Ld1(v12.S(), 3, MemOperand(x27, x5, PostIndex)),
                "ld1 {v12.s}[3], [x27], x5");
  COMPARE_MACRO(Ld1(v12.S(), 3, MemOperand(x27, 4, PostIndex)),
                "ld1 {v12.s}[3], [x27], #4");
  COMPARE_MACRO(Ld1(v13.D(), 1, MemOperand(sp, x6, PostIndex)),
                "ld1 {v13.d}[1], [sp], x6");
  COMPARE_MACRO(Ld1(v13.D(), 1, MemOperand(sp, 8, PostIndex)),
                "ld1 {v13.d}[1], [sp], #8");

  COMPARE_MACRO(Ld2(v0.V8B(), v1.V8B(), 0, MemOperand(x15)),
                "ld2 {v0.b, v1.b}[0], [x15]");
  COMPARE_MACRO(Ld2(v1.V16B(), v2.V16B(), 1, MemOperand(x16)),
                "ld2 {v1.b, v2.b}[1], [x16]");
  COMPARE_MACRO(Ld2(v2.V4H(), v3.V4H(), 2, MemOperand(x17)),
                "ld2 {v2.h, v3.h}[2], [x17]");
  COMPARE_MACRO(Ld2(v3.V8H(), v4.V8H(), 3, MemOperand(x18)),
                "ld2 {v3.h, v4.h}[3], [x18]");
  COMPARE_MACRO(Ld2(v4.V2S(), v5.V2S(), 0, MemOperand(x19)),
                "ld2 {v4.s, v5.s}[0], [x19]");
  COMPARE_MACRO(Ld2(v5.V4S(), v6.V4S(), 1, MemOperand(x20)),
                "ld2 {v5.s, v6.s}[1], [x20]");
  COMPARE_MACRO(Ld2(v6.V2D(), v7.V2D(), 0, MemOperand(x21)),
                "ld2 {v6.d, v7.d}[0], [x21]");
  COMPARE_MACRO(Ld2(v7.B(), v8.B(), 7, MemOperand(x22)),
                "ld2 {v7.b, v8.b}[7], [x22]");
  COMPARE_MACRO(Ld2(v8.B(), v9.B(), 15, MemOperand(x23)),
                "ld2 {v8.b, v9.b}[15], [x23]");
  COMPARE_MACRO(Ld2(v9.H(), v10.H(), 3, MemOperand(x24)),
                "ld2 {v9.h, v10.h}[3], [x24]");
  COMPARE_MACRO(Ld2(v10.H(), v11.H(), 7, MemOperand(x25)),
                "ld2 {v10.h, v11.h}[7], [x25]");
  COMPARE_MACRO(Ld2(v11.S(), v12.S(), 1, MemOperand(x26)),
                "ld2 {v11.s, v12.s}[1], [x26]");
  COMPARE_MACRO(Ld2(v12.S(), v13.S(), 3, MemOperand(x27)),
                "ld2 {v12.s, v13.s}[3], [x27]");
  COMPARE_MACRO(Ld2(v13.D(), v14.D(), 1, MemOperand(sp)),
                "ld2 {v13.d, v14.d}[1], [sp]");

  COMPARE_MACRO(Ld2(v0.V8B(), v1.V8B(), 0, MemOperand(x15, x0, PostIndex)),
                "ld2 {v0.b, v1.b}[0], [x15], x0");
  COMPARE_MACRO(Ld2(v1.V16B(), v2.V16B(), 1, MemOperand(x16, 2, PostIndex)),
                "ld2 {v1.b, v2.b}[1], [x16], #2");
  COMPARE_MACRO(Ld2(v2.V4H(), v3.V4H(), 2, MemOperand(x17, 4, PostIndex)),
                "ld2 {v2.h, v3.h}[2], [x17], #4");
  COMPARE_MACRO(Ld2(v3.V8H(), v4.V8H(), 3, MemOperand(x18, x1, PostIndex)),
                "ld2 {v3.h, v4.h}[3], [x18], x1");
  COMPARE_MACRO(Ld2(v4.V2S(), v5.V2S(), 0, MemOperand(x19, x2, PostIndex)),
                "ld2 {v4.s, v5.s}[0], [x19], x2");
  COMPARE_MACRO(Ld2(v5.V4S(), v6.V4S(), 1, MemOperand(x20, 8, PostIndex)),
                "ld2 {v5.s, v6.s}[1], [x20], #8");
  COMPARE_MACRO(Ld2(v6.V2D(), v7.V2D(), 0, MemOperand(x21, 16, PostIndex)),
                "ld2 {v6.d, v7.d}[0], [x21], #16");
  COMPARE_MACRO(Ld2(v7.B(), v8.B(), 7, MemOperand(x22, 2, PostIndex)),
                "ld2 {v7.b, v8.b}[7], [x22], #2");
  COMPARE_MACRO(Ld2(v8.B(), v9.B(), 15, MemOperand(x23, x3, PostIndex)),
                "ld2 {v8.b, v9.b}[15], [x23], x3");
  COMPARE_MACRO(Ld2(v9.H(), v10.H(), 3, MemOperand(x24, x4, PostIndex)),
                "ld2 {v9.h, v10.h}[3], [x24], x4");
  COMPARE_MACRO(Ld2(v10.H(), v11.H(), 7, MemOperand(x25, 4, PostIndex)),
                "ld2 {v10.h, v11.h}[7], [x25], #4");
  COMPARE_MACRO(Ld2(v11.S(), v12.S(), 1, MemOperand(x26, 8, PostIndex)),
                "ld2 {v11.s, v12.s}[1], [x26], #8");
  COMPARE_MACRO(Ld2(v12.S(), v13.S(), 3, MemOperand(x27, x5, PostIndex)),
                "ld2 {v12.s, v13.s}[3], [x27], x5");
  COMPARE_MACRO(Ld2(v11.S(), v12.S(), 3, MemOperand(x26, 8, PostIndex)),
                "ld2 {v11.s, v12.s}[3], [x26], #8");
  COMPARE_MACRO(Ld2(v13.D(), v14.D(), 1, MemOperand(sp, x6, PostIndex)),
                "ld2 {v13.d, v14.d}[1], [sp], x6");
  COMPARE_MACRO(Ld2(v13.D(), v14.D(), 1, MemOperand(sp, 16, PostIndex)),
                "ld2 {v13.d, v14.d}[1], [sp], #16");

  COMPARE_MACRO(Ld3(v0.V8B(), v1.V8B(), v2.V8B(), 0, MemOperand(x15)),
                "ld3 {v0.b, v1.b, v2.b}[0], [x15]");
  COMPARE_MACRO(Ld3(v1.V16B(), v2.V16B(), v3.V16B(), 1, MemOperand(x16)),
                "ld3 {v1.b, v2.b, v3.b}[1], [x16]");
  COMPARE_MACRO(Ld3(v2.V4H(), v3.V4H(), v4.V4H(), 2, MemOperand(x17)),
                "ld3 {v2.h, v3.h, v4.h}[2], [x17]");
  COMPARE_MACRO(Ld3(v3.V8H(), v4.V8H(), v5.V8H(), 3, MemOperand(x18)),
                "ld3 {v3.h, v4.h, v5.h}[3], [x18]");
  COMPARE_MACRO(Ld3(v4.V2S(), v5.V2S(), v6.V2S(), 0, MemOperand(x19)),
                "ld3 {v4.s, v5.s, v6.s}[0], [x19]");
  COMPARE_MACRO(Ld3(v5.V4S(), v6.V4S(), v7.V4S(), 1, MemOperand(x20)),
                "ld3 {v5.s, v6.s, v7.s}[1], [x20]");
  COMPARE_MACRO(Ld3(v6.V2D(), v7.V2D(), v8.V2D(), 0, MemOperand(x21)),
                "ld3 {v6.d, v7.d, v8.d}[0], [x21]");
  COMPARE_MACRO(Ld3(v7.B(), v8.B(), v9.B(), 7, MemOperand(x22)),
                "ld3 {v7.b, v8.b, v9.b}[7], [x22]");
  COMPARE_MACRO(Ld3(v8.B(), v9.B(), v10.B(), 15, MemOperand(x23)),
                "ld3 {v8.b, v9.b, v10.b}[15], [x23]");
  COMPARE_MACRO(Ld3(v9.H(), v10.H(), v11.H(), 3, MemOperand(x24)),
                "ld3 {v9.h, v10.h, v11.h}[3], [x24]");
  COMPARE_MACRO(Ld3(v10.H(), v11.H(), v12.H(), 7, MemOperand(x25)),
                "ld3 {v10.h, v11.h, v12.h}[7], [x25]");
  COMPARE_MACRO(Ld3(v11.S(), v12.S(), v13.S(), 1, MemOperand(x26)),
                "ld3 {v11.s, v12.s, v13.s}[1], [x26]");
  COMPARE_MACRO(Ld3(v12.S(), v13.S(), v14.S(), 3, MemOperand(x27)),
                "ld3 {v12.s, v13.s, v14.s}[3], [x27]");
  COMPARE_MACRO(Ld3(v13.D(), v14.D(), v15.D(), 1, MemOperand(sp)),
                "ld3 {v13.d, v14.d, v15.d}[1], [sp]");

  COMPARE_MACRO(Ld3(v0.V8B(),
                    v1.V8B(),
                    v2.V8B(),
                    0,
                    MemOperand(x15, x0, PostIndex)),
                "ld3 {v0.b, v1.b, v2.b}[0], [x15], x0");
  COMPARE_MACRO(Ld3(v1.V16B(),
                    v2.V16B(),
                    v3.V16B(),
                    1,
                    MemOperand(x16, 3, PostIndex)),
                "ld3 {v1.b, v2.b, v3.b}[1], [x16], #3");
  COMPARE_MACRO(Ld3(v2.V4H(),
                    v3.V4H(),
                    v4.V4H(),
                    2,
                    MemOperand(x17, 6, PostIndex)),
                "ld3 {v2.h, v3.h, v4.h}[2], [x17], #6");
  COMPARE_MACRO(Ld3(v3.V8H(),
                    v4.V8H(),
                    v5.V8H(),
                    3,
                    MemOperand(x18, x1, PostIndex)),
                "ld3 {v3.h, v4.h, v5.h}[3], [x18], x1");
  COMPARE_MACRO(Ld3(v4.V2S(),
                    v5.V2S(),
                    v6.V2S(),
                    0,
                    MemOperand(x19, x2, PostIndex)),
                "ld3 {v4.s, v5.s, v6.s}[0], [x19], x2");
  COMPARE_MACRO(Ld3(v5.V4S(),
                    v6.V4S(),
                    v7.V4S(),
                    1,
                    MemOperand(x20, 12, PostIndex)),
                "ld3 {v5.s, v6.s, v7.s}[1], [x20], #12");
  COMPARE_MACRO(Ld3(v6.V2D(),
                    v7.V2D(),
                    v8.V2D(),
                    0,
                    MemOperand(x21, 24, PostIndex)),
                "ld3 {v6.d, v7.d, v8.d}[0], [x21], #24");
  COMPARE_MACRO(Ld3(v7.B(), v8.B(), v9.B(), 7, MemOperand(x22, 3, PostIndex)),
                "ld3 {v7.b, v8.b, v9.b}[7], [x22], #3");
  COMPARE_MACRO(Ld3(v8.B(),
                    v9.B(),
                    v10.B(),
                    15,
                    MemOperand(x23, x3, PostIndex)),
                "ld3 {v8.b, v9.b, v10.b}[15], [x23], x3");
  COMPARE_MACRO(Ld3(v9.H(),
                    v10.H(),
                    v11.H(),
                    3,
                    MemOperand(x24, x4, PostIndex)),
                "ld3 {v9.h, v10.h, v11.h}[3], [x24], x4");
  COMPARE_MACRO(Ld3(v10.H(),
                    v11.H(),
                    v12.H(),
                    7,
                    MemOperand(x25, 6, PostIndex)),
                "ld3 {v10.h, v11.h, v12.h}[7], [x25], #6");
  COMPARE_MACRO(Ld3(v11.S(),
                    v12.S(),
                    v13.S(),
                    1,
                    MemOperand(x26, 12, PostIndex)),
                "ld3 {v11.s, v12.s, v13.s}[1], [x26], #12");
  COMPARE_MACRO(Ld3(v12.S(),
                    v13.S(),
                    v14.S(),
                    3,
                    MemOperand(x27, x5, PostIndex)),
                "ld3 {v12.s, v13.s, v14.s}[3], [x27], x5");
  COMPARE_MACRO(Ld3(v12.S(),
                    v13.S(),
                    v14.S(),
                    3,
                    MemOperand(x27, 12, PostIndex)),
                "ld3 {v12.s, v13.s, v14.s}[3], [x27], #12");
  COMPARE_MACRO(Ld3(v13.D(),
                    v14.D(),
                    v15.D(),
                    1,
                    MemOperand(sp, x6, PostIndex)),
                "ld3 {v13.d, v14.d, v15.d}[1], [sp], x6");
  COMPARE_MACRO(Ld3(v13.D(),
                    v14.D(),
                    v15.D(),
                    1,
                    MemOperand(sp, 24, PostIndex)),
                "ld3 {v13.d, v14.d, v15.d}[1], [sp], #24");

  COMPARE_MACRO(Ld4(v0.V8B(), v1.V8B(), v2.V8B(), v3.V8B(), 0, MemOperand(x15)),
                "ld4 {v0.b, v1.b, v2.b, v3.b}[0], [x15]");
  COMPARE_MACRO(Ld4(v1.V16B(),
                    v2.V16B(),
                    v3.V16B(),
                    v4.V16B(),
                    1,
                    MemOperand(x16)),
                "ld4 {v1.b, v2.b, v3.b, v4.b}[1], [x16]");
  COMPARE_MACRO(Ld4(v2.V4H(), v3.V4H(), v4.V4H(), v5.V4H(), 2, MemOperand(x17)),
                "ld4 {v2.h, v3.h, v4.h, v5.h}[2], [x17]");
  COMPARE_MACRO(Ld4(v3.V8H(), v4.V8H(), v5.V8H(), v6.V8H(), 3, MemOperand(x18)),
                "ld4 {v3.h, v4.h, v5.h, v6.h}[3], [x18]");
  COMPARE_MACRO(Ld4(v4.V2S(), v5.V2S(), v6.V2S(), v7.V2S(), 0, MemOperand(x19)),
                "ld4 {v4.s, v5.s, v6.s, v7.s}[0], [x19]");
  COMPARE_MACRO(Ld4(v5.V4S(), v6.V4S(), v7.V4S(), v8.V4S(), 1, MemOperand(x20)),
                "ld4 {v5.s, v6.s, v7.s, v8.s}[1], [x20]");
  COMPARE_MACRO(Ld4(v6.V2D(), v7.V2D(), v8.V2D(), v9.V2D(), 0, MemOperand(x21)),
                "ld4 {v6.d, v7.d, v8.d, v9.d}[0], [x21]");
  COMPARE_MACRO(Ld4(v7.B(), v8.B(), v9.B(), v10.B(), 7, MemOperand(x22)),
                "ld4 {v7.b, v8.b, v9.b, v10.b}[7], [x22]");
  COMPARE_MACRO(Ld4(v8.B(), v9.B(), v10.B(), v11.B(), 15, MemOperand(x23)),
                "ld4 {v8.b, v9.b, v10.b, v11.b}[15], [x23]");
  COMPARE_MACRO(Ld4(v9.H(), v10.H(), v11.H(), v12.H(), 3, MemOperand(x24)),
                "ld4 {v9.h, v10.h, v11.h, v12.h}[3], [x24]");
  COMPARE_MACRO(Ld4(v10.H(), v11.H(), v12.H(), v13.H(), 7, MemOperand(x25)),
                "ld4 {v10.h, v11.h, v12.h, v13.h}[7], [x25]");
  COMPARE_MACRO(Ld4(v11.S(), v12.S(), v13.S(), v14.S(), 1, MemOperand(x26)),
                "ld4 {v11.s, v12.s, v13.s, v14.s}[1], [x26]");
  COMPARE_MACRO(Ld4(v12.S(), v13.S(), v14.S(), v15.S(), 3, MemOperand(x27)),
                "ld4 {v12.s, v13.s, v14.s, v15.s}[3], [x27]");
  COMPARE_MACRO(Ld4(v13.D(), v14.D(), v15.D(), v16.D(), 1, MemOperand(sp)),
                "ld4 {v13.d, v14.d, v15.d, v16.d}[1], [sp]");

  COMPARE_MACRO(Ld4(v0.V8B(),
                    v1.V8B(),
                    v2.V8B(),
                    v3.V8B(),
                    0,
                    MemOperand(x15, x0, PostIndex)),
                "ld4 {v0.b, v1.b, v2.b, v3.b}[0], [x15], x0");
  COMPARE_MACRO(Ld4(v1.V16B(),
                    v2.V16B(),
                    v3.V16B(),
                    v4.V16B(),
                    1,
                    MemOperand(x16, 4, PostIndex)),
                "ld4 {v1.b, v2.b, v3.b, v4.b}[1], [x16], #4");
  COMPARE_MACRO(Ld4(v2.V4H(),
                    v3.V4H(),
                    v4.V4H(),
                    v5.V4H(),
                    2,
                    MemOperand(x17, 8, PostIndex)),
                "ld4 {v2.h, v3.h, v4.h, v5.h}[2], [x17], #8");
  COMPARE_MACRO(Ld4(v3.V8H(),
                    v4.V8H(),
                    v5.V8H(),
                    v6.V8H(),
                    3,
                    MemOperand(x18, x1, PostIndex)),
                "ld4 {v3.h, v4.h, v5.h, v6.h}[3], [x18], x1");
  COMPARE_MACRO(Ld4(v4.V2S(),
                    v5.V2S(),
                    v6.V2S(),
                    v7.V2S(),
                    0,
                    MemOperand(x19, x2, PostIndex)),
                "ld4 {v4.s, v5.s, v6.s, v7.s}[0], [x19], x2");
  COMPARE_MACRO(Ld4(v5.V4S(),
                    v6.V4S(),
                    v7.V4S(),
                    v8.V4S(),
                    1,
                    MemOperand(x20, 16, PostIndex)),
                "ld4 {v5.s, v6.s, v7.s, v8.s}[1], [x20], #16");
  COMPARE_MACRO(Ld4(v6.V2D(),
                    v7.V2D(),
                    v8.V2D(),
                    v9.V2D(),
                    0,
                    MemOperand(x21, 32, PostIndex)),
                "ld4 {v6.d, v7.d, v8.d, v9.d}[0], [x21], #32");
  COMPARE_MACRO(Ld4(v7.B(),
                    v8.B(),
                    v9.B(),
                    v10.B(),
                    7,
                    MemOperand(x22, 4, PostIndex)),
                "ld4 {v7.b, v8.b, v9.b, v10.b}[7], [x22], #4");
  COMPARE_MACRO(Ld4(v8.B(),
                    v9.B(),
                    v10.B(),
                    v11.B(),
                    15,
                    MemOperand(x23, x3, PostIndex)),
                "ld4 {v8.b, v9.b, v10.b, v11.b}[15], [x23], x3");
  COMPARE_MACRO(Ld4(v9.H(),
                    v10.H(),
                    v11.H(),
                    v12.H(),
                    3,
                    MemOperand(x24, x4, PostIndex)),
                "ld4 {v9.h, v10.h, v11.h, v12.h}[3], [x24], x4");
  COMPARE_MACRO(Ld4(v10.H(),
                    v11.H(),
                    v12.H(),
                    v13.H(),
                    7,
                    MemOperand(x25, 8, PostIndex)),
                "ld4 {v10.h, v11.h, v12.h, v13.h}[7], [x25], #8");
  COMPARE_MACRO(Ld4(v11.S(),
                    v12.S(),
                    v13.S(),
                    v14.S(),
                    1,
                    MemOperand(x26, 16, PostIndex)),
                "ld4 {v11.s, v12.s, v13.s, v14.s}[1], [x26], #16");
  COMPARE_MACRO(Ld4(v12.S(),
                    v13.S(),
                    v14.S(),
                    v15.S(),
                    3,
                    MemOperand(x27, x5, PostIndex)),
                "ld4 {v12.s, v13.s, v14.s, v15.s}[3], [x27], x5");
  COMPARE_MACRO(Ld4(v11.S(),
                    v12.S(),
                    v13.S(),
                    v14.S(),
                    3,
                    MemOperand(x26, 16, PostIndex)),
                "ld4 {v11.s, v12.s, v13.s, v14.s}[3], [x26], #16");
  COMPARE_MACRO(Ld4(v13.D(),
                    v14.D(),
                    v15.D(),
                    v16.D(),
                    1,
                    MemOperand(sp, x6, PostIndex)),
                "ld4 {v13.d, v14.d, v15.d, v16.d}[1], [sp], x6");
  COMPARE_MACRO(Ld4(v13.D(),
                    v14.D(),
                    v15.D(),
                    v16.D(),
                    1,
                    MemOperand(sp, 32, PostIndex)),
                "ld4 {v13.d, v14.d, v15.d, v16.d}[1], [sp], #32");

  COMPARE_MACRO(St1(v0.V8B(), 0, MemOperand(x15)), "st1 {v0.b}[0], [x15]");
  COMPARE_MACRO(St1(v1.V16B(), 1, MemOperand(x16)), "st1 {v1.b}[1], [x16]");
  COMPARE_MACRO(St1(v2.V4H(), 2, MemOperand(x17)), "st1 {v2.h}[2], [x17]");
  COMPARE_MACRO(St1(v3.V8H(), 3, MemOperand(x18)), "st1 {v3.h}[3], [x18]");
  COMPARE_MACRO(St1(v4.V2S(), 0, MemOperand(x19)), "st1 {v4.s}[0], [x19]");
  COMPARE_MACRO(St1(v5.V4S(), 1, MemOperand(x20)), "st1 {v5.s}[1], [x20]");
  COMPARE_MACRO(St1(v6.V2D(), 0, MemOperand(x21)), "st1 {v6.d}[0], [x21]");
  COMPARE_MACRO(St1(v7.B(), 7, MemOperand(x22)), "st1 {v7.b}[7], [x22]");
  COMPARE_MACRO(St1(v8.B(), 15, MemOperand(x23)), "st1 {v8.b}[15], [x23]");
  COMPARE_MACRO(St1(v9.H(), 3, MemOperand(x24)), "st1 {v9.h}[3], [x24]");
  COMPARE_MACRO(St1(v10.H(), 7, MemOperand(x25)), "st1 {v10.h}[7], [x25]");
  COMPARE_MACRO(St1(v11.S(), 1, MemOperand(x26)), "st1 {v11.s}[1], [x26]");
  COMPARE_MACRO(St1(v12.S(), 3, MemOperand(x27)), "st1 {v12.s}[3], [x27]");
  COMPARE_MACRO(St1(v13.D(), 1, MemOperand(sp)), "st1 {v13.d}[1], [sp]");

  COMPARE_MACRO(St1(v0.V8B(), 0, MemOperand(x15, x0, PostIndex)),
                "st1 {v0.b}[0], [x15], x0");
  COMPARE_MACRO(St1(v1.V16B(), 1, MemOperand(x16, 1, PostIndex)),
                "st1 {v1.b}[1], [x16], #1");
  COMPARE_MACRO(St1(v2.V4H(), 2, MemOperand(x17, 2, PostIndex)),
                "st1 {v2.h}[2], [x17], #2");
  COMPARE_MACRO(St1(v3.V8H(), 3, MemOperand(x18, x1, PostIndex)),
                "st1 {v3.h}[3], [x18], x1");
  COMPARE_MACRO(St1(v4.V2S(), 0, MemOperand(x19, x2, PostIndex)),
                "st1 {v4.s}[0], [x19], x2");
  COMPARE_MACRO(St1(v5.V4S(), 1, MemOperand(x20, 4, PostIndex)),
                "st1 {v5.s}[1], [x20], #4");
  COMPARE_MACRO(St1(v6.V2D(), 0, MemOperand(x21, 8, PostIndex)),
                "st1 {v6.d}[0], [x21], #8");
  COMPARE_MACRO(St1(v7.B(), 7, MemOperand(x22, 1, PostIndex)),
                "st1 {v7.b}[7], [x22], #1");
  COMPARE_MACRO(St1(v8.B(), 15, MemOperand(x23, x3, PostIndex)),
                "st1 {v8.b}[15], [x23], x3");
  COMPARE_MACRO(St1(v9.H(), 3, MemOperand(x24, x4, PostIndex)),
                "st1 {v9.h}[3], [x24], x4");
  COMPARE_MACRO(St1(v10.H(), 7, MemOperand(x25, 2, PostIndex)),
                "st1 {v10.h}[7], [x25], #2");
  COMPARE_MACRO(St1(v11.S(), 1, MemOperand(x26, 4, PostIndex)),
                "st1 {v11.s}[1], [x26], #4");
  COMPARE_MACRO(St1(v12.S(), 3, MemOperand(x27, x5, PostIndex)),
                "st1 {v12.s}[3], [x27], x5");
  COMPARE_MACRO(St1(v13.D(), 1, MemOperand(sp, x6, PostIndex)),
                "st1 {v13.d}[1], [sp], x6");
  COMPARE_MACRO(St2(v0.V8B(), v1.V8B(), 0, MemOperand(x15, x0, PostIndex)),
                "st2 {v0.b, v1.b}[0], [x15], x0");
  COMPARE_MACRO(St2(v1.V16B(), v2.V16B(), 1, MemOperand(x16, 2, PostIndex)),
                "st2 {v1.b, v2.b}[1], [x16], #2");
  COMPARE_MACRO(St2(v2.V4H(), v3.V4H(), 2, MemOperand(x17, 4, PostIndex)),
                "st2 {v2.h, v3.h}[2], [x17], #4");
  COMPARE_MACRO(St2(v3.V8H(), v4.V8H(), 3, MemOperand(x18, x1, PostIndex)),
                "st2 {v3.h, v4.h}[3], [x18], x1");
  COMPARE_MACRO(St2(v4.V2S(), v5.V2S(), 0, MemOperand(x19, x2, PostIndex)),
                "st2 {v4.s, v5.s}[0], [x19], x2");
  COMPARE_MACRO(St2(v5.V4S(), v6.V4S(), 1, MemOperand(x20, 8, PostIndex)),
                "st2 {v5.s, v6.s}[1], [x20], #8");
  COMPARE_MACRO(St2(v6.V2D(), v7.V2D(), 0, MemOperand(x21, 16, PostIndex)),
                "st2 {v6.d, v7.d}[0], [x21], #16");
  COMPARE_MACRO(St2(v7.B(), v8.B(), 7, MemOperand(x22, 2, PostIndex)),
                "st2 {v7.b, v8.b}[7], [x22], #2");
  COMPARE_MACRO(St2(v8.B(), v9.B(), 15, MemOperand(x23, x3, PostIndex)),
                "st2 {v8.b, v9.b}[15], [x23], x3");
  COMPARE_MACRO(St2(v9.H(), v10.H(), 3, MemOperand(x24, x4, PostIndex)),
                "st2 {v9.h, v10.h}[3], [x24], x4");
  COMPARE_MACRO(St2(v10.H(), v11.H(), 7, MemOperand(x25, 4, PostIndex)),
                "st2 {v10.h, v11.h}[7], [x25], #4");
  COMPARE_MACRO(St2(v11.S(), v12.S(), 1, MemOperand(x26, 8, PostIndex)),
                "st2 {v11.s, v12.s}[1], [x26], #8");
  COMPARE_MACRO(St2(v12.S(), v13.S(), 3, MemOperand(x27, x5, PostIndex)),
                "st2 {v12.s, v13.s}[3], [x27], x5");
  COMPARE_MACRO(St2(v13.D(), v14.D(), 1, MemOperand(sp, x6, PostIndex)),
                "st2 {v13.d, v14.d}[1], [sp], x6");
  COMPARE_MACRO(St3(VLIST3(v0.V8B()), 0, MemOperand(x15, x0, PostIndex)),
                "st3 {v0.b, v1.b, v2.b}[0], [x15], x0");
  COMPARE_MACRO(St3(VLIST3(v1.V16B()), 1, MemOperand(x16, 3, PostIndex)),
                "st3 {v1.b, v2.b, v3.b}[1], [x16], #3");
  COMPARE_MACRO(St3(VLIST3(v2.V4H()), 2, MemOperand(x17, 6, PostIndex)),
                "st3 {v2.h, v3.h, v4.h}[2], [x17], #6");
  COMPARE_MACRO(St3(VLIST3(v3.V8H()), 3, MemOperand(x18, x1, PostIndex)),
                "st3 {v3.h, v4.h, v5.h}[3], [x18], x1");
  COMPARE_MACRO(St3(VLIST3(v4.V2S()), 0, MemOperand(x19, x2, PostIndex)),
                "st3 {v4.s, v5.s, v6.s}[0], [x19], x2");
  COMPARE_MACRO(St3(VLIST3(v5.V4S()), 1, MemOperand(x20, 12, PostIndex)),
                "st3 {v5.s, v6.s, v7.s}[1], [x20], #12");
  COMPARE_MACRO(St3(VLIST3(v6.V2D()), 0, MemOperand(x21, 24, PostIndex)),
                "st3 {v6.d, v7.d, v8.d}[0], [x21], #24");
  COMPARE_MACRO(St3(VLIST3(v7.B()), 7, MemOperand(x22, 3, PostIndex)),
                "st3 {v7.b, v8.b, v9.b}[7], [x22], #3");
  COMPARE_MACRO(St3(VLIST3(v8.B()), 15, MemOperand(x23, x3, PostIndex)),
                "st3 {v8.b, v9.b, v10.b}[15], [x23], x3");
  COMPARE_MACRO(St3(VLIST3(v9.H()), 3, MemOperand(x24, x4, PostIndex)),
                "st3 {v9.h, v10.h, v11.h}[3], [x24], x4");
  COMPARE_MACRO(St3(VLIST3(v10.H()), 7, MemOperand(x25, 6, PostIndex)),
                "st3 {v10.h, v11.h, v12.h}[7], [x25], #6");
  COMPARE_MACRO(St3(VLIST3(v11.S()), 1, MemOperand(x26, 12, PostIndex)),
                "st3 {v11.s, v12.s, v13.s}[1], [x26], #12");
  COMPARE_MACRO(St3(VLIST3(v12.S()), 3, MemOperand(x27, x5, PostIndex)),
                "st3 {v12.s, v13.s, v14.s}[3], [x27], x5");
  COMPARE_MACRO(St3(VLIST3(v13.D()), 1, MemOperand(sp, x6, PostIndex)),
                "st3 {v13.d, v14.d, v15.d}[1], [sp], x6");

  COMPARE_MACRO(St4(VLIST4(v0.V8B()), 0, MemOperand(x15, x0, PostIndex)),
                "st4 {v0.b, v1.b, v2.b, v3.b}[0], [x15], x0");
  COMPARE_MACRO(St4(VLIST4(v1.V16B()), 1, MemOperand(x16, 4, PostIndex)),
                "st4 {v1.b, v2.b, v3.b, v4.b}[1], [x16], #4");
  COMPARE_MACRO(St4(VLIST4(v2.V4H()), 2, MemOperand(x17, 8, PostIndex)),
                "st4 {v2.h, v3.h, v4.h, v5.h}[2], [x17], #8");
  COMPARE_MACRO(St4(VLIST4(v3.V8H()), 3, MemOperand(x18, x1, PostIndex)),
                "st4 {v3.h, v4.h, v5.h, v6.h}[3], [x18], x1");
  COMPARE_MACRO(St4(VLIST4(v4.V2S()), 0, MemOperand(x19, x2, PostIndex)),
                "st4 {v4.s, v5.s, v6.s, v7.s}[0], [x19], x2");
  COMPARE_MACRO(St4(VLIST4(v5.V4S()), 1, MemOperand(x20, 16, PostIndex)),
                "st4 {v5.s, v6.s, v7.s, v8.s}[1], [x20], #16");
  COMPARE_MACRO(St4(VLIST4(v6.V2D()), 0, MemOperand(x21, 32, PostIndex)),
                "st4 {v6.d, v7.d, v8.d, v9.d}[0], [x21], #32");
  COMPARE_MACRO(St4(VLIST4(v7.B()), 7, MemOperand(x22, 4, PostIndex)),
                "st4 {v7.b, v8.b, v9.b, v10.b}[7], [x22], #4");
  COMPARE_MACRO(St4(VLIST4(v8.B()), 15, MemOperand(x23, x3, PostIndex)),
                "st4 {v8.b, v9.b, v10.b, v11.b}[15], [x23], x3");
  COMPARE_MACRO(St4(VLIST4(v9.H()), 3, MemOperand(x24, x4, PostIndex)),
                "st4 {v9.h, v10.h, v11.h, v12.h}[3], [x24], x4");
  COMPARE_MACRO(St4(VLIST4(v10.H()), 7, MemOperand(x25, 8, PostIndex)),
                "st4 {v10.h, v11.h, v12.h, v13.h}[7], [x25], #8");
  COMPARE_MACRO(St4(VLIST4(v11.S()), 1, MemOperand(x26, 16, PostIndex)),
                "st4 {v11.s, v12.s, v13.s, v14.s}[1], [x26], #16");
  COMPARE_MACRO(St4(VLIST4(v12.S()), 3, MemOperand(x27, x5, PostIndex)),
                "st4 {v12.s, v13.s, v14.s, v15.s}[3], [x27], x5");
  COMPARE_MACRO(St4(VLIST4(v13.D()), 1, MemOperand(sp, x6, PostIndex)),
                "st4 {v13.d, v14.d, v15.d, v16.d}[1], [sp], x6");

  CLEANUP();
}


TEST(neon_load_store_lane_unallocated) {
  SETUP();

  const char* expected = "unallocated (Unallocated)";
  // LD1 (single structure) (no offset)
  COMPARE(dci(0x0d404400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0d408800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0d409400), expected);  // .d, size<0> = 1, S = 1
  // LD2 (single structure) (no offset)
  COMPARE(dci(0x0d604400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0d608800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0d609400), expected);  // .d, size<0> = 1, S = 1
  // LD3 (single structure) (no offset)
  COMPARE(dci(0x0d406400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0d40a800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0d40b400), expected);  // .d, size<0> = 1, S = 1
  // LD4 (single structure) (no offset)
  COMPARE(dci(0x0d606400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0d60a800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0d60b400), expected);  // .d, size<0> = 1, S = 1
  // ST1 (single structure) (no offset)
  COMPARE(dci(0x0d004400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0d008800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0d009400), expected);  // .d, size<0> = 1, S = 1
  // ST2 (single structure) (no offset)
  COMPARE(dci(0x0d204400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0d208800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0d209400), expected);  // .d, size<0> = 1, S = 1
  // ST3 (single structure) (no offset)
  COMPARE(dci(0x0d006400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0d00a800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0d00b400), expected);  // .d, size<0> = 1, S = 1
  // ST4 (single structure) (no offset)
  COMPARE(dci(0x0d206400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0d20a800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0d20b400), expected);  // .d, size<0> = 1, S = 1

  // LD1 (single structure) (post index)
  COMPARE(dci(0x0dc04400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0dc08800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0dc09400), expected);  // .d, size<0> = 1, S = 1
  // LD2 (single structure) (post index)
  COMPARE(dci(0x0de04400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0de08800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0de09400), expected);  // .d, size<0> = 1, S = 1
  // LD3 (single structure) (post index)
  COMPARE(dci(0x0dc06400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0dc0a800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0dc0b400), expected);  // .d, size<0> = 1, S = 1
  // LD4 (single structure) (post index)
  COMPARE(dci(0x0de06400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0de0a800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0de0b400), expected);  // .d, size<0> = 1, S = 1
  // ST1 (single structure) (post index)
  COMPARE(dci(0x0d804400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0d808800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0d809400), expected);  // .d, size<0> = 1, S = 1
  // ST2 (single structure) (post index)
  COMPARE(dci(0x0da04400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0da08800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0da09400), expected);  // .d, size<0> = 1, S = 1
  // ST3 (single structure) (post index)
  COMPARE(dci(0x0d806400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0d80a800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0d80b400), expected);  // .d, size<0> = 1, S = 1
  // ST4 (single structure) (post index)
  COMPARE(dci(0x0da06400), expected);  // .h, size<0> = 1
  COMPARE(dci(0x0da0a800), expected);  // .s, size<1> = 1
  COMPARE(dci(0x0da0b400), expected);  // .d, size<0> = 1, S = 1

  CLEANUP();
}


TEST(neon_load_all_lanes) {
  SETUP();

  COMPARE_MACRO(Ld1r(v14.V8B(), MemOperand(x0)), "ld1r {v14.8b}, [x0]");
  COMPARE_MACRO(Ld1r(v15.V16B(), MemOperand(x1)), "ld1r {v15.16b}, [x1]");
  COMPARE_MACRO(Ld1r(v16.V4H(), MemOperand(x2)), "ld1r {v16.4h}, [x2]");
  COMPARE_MACRO(Ld1r(v17.V8H(), MemOperand(x3)), "ld1r {v17.8h}, [x3]");
  COMPARE_MACRO(Ld1r(v18.V2S(), MemOperand(x4)), "ld1r {v18.2s}, [x4]");
  COMPARE_MACRO(Ld1r(v19.V4S(), MemOperand(x5)), "ld1r {v19.4s}, [x5]");
  COMPARE_MACRO(Ld1r(v20.V2D(), MemOperand(sp)), "ld1r {v20.2d}, [sp]");
  COMPARE_MACRO(Ld1r(v21.V1D(), MemOperand(x30)), "ld1r {v21.1d}, [x30]");

  COMPARE_MACRO(Ld1r(v22.V8B(), MemOperand(x6, 1, PostIndex)),
                "ld1r {v22.8b}, [x6], #1");
  COMPARE_MACRO(Ld1r(v23.V16B(), MemOperand(x7, x16, PostIndex)),
                "ld1r {v23.16b}, [x7], x16");
  COMPARE_MACRO(Ld1r(v24.V4H(), MemOperand(x8, x17, PostIndex)),
                "ld1r {v24.4h}, [x8], x17");
  COMPARE_MACRO(Ld1r(v25.V8H(), MemOperand(x9, 2, PostIndex)),
                "ld1r {v25.8h}, [x9], #2");
  COMPARE_MACRO(Ld1r(v26.V2S(), MemOperand(x10, 4, PostIndex)),
                "ld1r {v26.2s}, [x10], #4");
  COMPARE_MACRO(Ld1r(v27.V4S(), MemOperand(x11, x18, PostIndex)),
                "ld1r {v27.4s}, [x11], x18");
  COMPARE_MACRO(Ld1r(v28.V2D(), MemOperand(x12, 8, PostIndex)),
                "ld1r {v28.2d}, [x12], #8");
  COMPARE_MACRO(Ld1r(v29.V1D(), MemOperand(x13, 8, PostIndex)),
                "ld1r {v29.1d}, [x13], #8");

  COMPARE_MACRO(Ld2r(v14.V8B(), v15.V8B(), MemOperand(x0)),
                "ld2r {v14.8b, v15.8b}, [x0]");
  COMPARE_MACRO(Ld2r(v15.V16B(), v16.V16B(), MemOperand(x1)),
                "ld2r {v15.16b, v16.16b}, [x1]");
  COMPARE_MACRO(Ld2r(v16.V4H(), v17.V4H(), MemOperand(x2)),
                "ld2r {v16.4h, v17.4h}, [x2]");
  COMPARE_MACRO(Ld2r(v17.V8H(), v18.V8H(), MemOperand(x3)),
                "ld2r {v17.8h, v18.8h}, [x3]");
  COMPARE_MACRO(Ld2r(v18.V2S(), v19.V2S(), MemOperand(x4)),
                "ld2r {v18.2s, v19.2s}, [x4]");
  COMPARE_MACRO(Ld2r(v19.V4S(), v20.V4S(), MemOperand(x5)),
                "ld2r {v19.4s, v20.4s}, [x5]");
  COMPARE_MACRO(Ld2r(v20.V2D(), v21.V2D(), MemOperand(sp)),
                "ld2r {v20.2d, v21.2d}, [sp]");
  COMPARE_MACRO(Ld2r(v21.V8B(), v22.V8B(), MemOperand(x6, 2, PostIndex)),
                "ld2r {v21.8b, v22.8b}, [x6], #2");
  COMPARE_MACRO(Ld2r(v22.V16B(), v23.V16B(), MemOperand(x7, x16, PostIndex)),
                "ld2r {v22.16b, v23.16b}, [x7], x16");
  COMPARE_MACRO(Ld2r(v23.V4H(), v24.V4H(), MemOperand(x8, x17, PostIndex)),
                "ld2r {v23.4h, v24.4h}, [x8], x17");
  COMPARE_MACRO(Ld2r(v24.V8H(), v25.V8H(), MemOperand(x9, 4, PostIndex)),
                "ld2r {v24.8h, v25.8h}, [x9], #4");
  COMPARE_MACRO(Ld2r(v25.V2S(), v26.V2S(), MemOperand(x10, 8, PostIndex)),
                "ld2r {v25.2s, v26.2s}, [x10], #8");
  COMPARE_MACRO(Ld2r(v26.V4S(), v27.V4S(), MemOperand(x11, x18, PostIndex)),
                "ld2r {v26.4s, v27.4s}, [x11], x18");
  COMPARE_MACRO(Ld2r(v27.V2D(), v28.V2D(), MemOperand(x12, 16, PostIndex)),
                "ld2r {v27.2d, v28.2d}, [x12], #16");

  COMPARE_MACRO(Ld3r(v14.V8B(), v15.V8B(), v16.V8B(), MemOperand(x0)),
                "ld3r {v14.8b, v15.8b, v16.8b}, [x0]");
  COMPARE_MACRO(Ld3r(v15.V16B(), v16.V16B(), v17.V16B(), MemOperand(x1)),
                "ld3r {v15.16b, v16.16b, v17.16b}, [x1]");
  COMPARE_MACRO(Ld3r(v16.V4H(), v17.V4H(), v18.V4H(), MemOperand(x2)),
                "ld3r {v16.4h, v17.4h, v18.4h}, [x2]");
  COMPARE_MACRO(Ld3r(v17.V8H(), v18.V8H(), v19.V8H(), MemOperand(x3)),
                "ld3r {v17.8h, v18.8h, v19.8h}, [x3]");
  COMPARE_MACRO(Ld3r(v18.V2S(), v19.V2S(), v20.V2S(), MemOperand(x4)),
                "ld3r {v18.2s, v19.2s, v20.2s}, [x4]");
  COMPARE_MACRO(Ld3r(v19.V4S(), v20.V4S(), v21.V4S(), MemOperand(x5)),
                "ld3r {v19.4s, v20.4s, v21.4s}, [x5]");
  COMPARE_MACRO(Ld3r(v20.V2D(), v21.V2D(), v22.V2D(), MemOperand(sp)),
                "ld3r {v20.2d, v21.2d, v22.2d}, [sp]");
  COMPARE_MACRO(Ld3r(v21.V8B(),
                     v22.V8B(),
                     v23.V8B(),
                     MemOperand(x6, 3, PostIndex)),
                "ld3r {v21.8b, v22.8b, v23.8b}, [x6], #3");
  COMPARE_MACRO(Ld3r(v22.V16B(),
                     v23.V16B(),
                     v24.V16B(),
                     MemOperand(x7, x16, PostIndex)),
                "ld3r {v22.16b, v23.16b, v24.16b}, [x7], x16");
  COMPARE_MACRO(Ld3r(v23.V4H(),
                     v24.V4H(),
                     v25.V4H(),
                     MemOperand(x8, x17, PostIndex)),
                "ld3r {v23.4h, v24.4h, v25.4h}, [x8], x17");
  COMPARE_MACRO(Ld3r(v24.V8H(),
                     v25.V8H(),
                     v26.V8H(),
                     MemOperand(x9, 6, PostIndex)),
                "ld3r {v24.8h, v25.8h, v26.8h}, [x9], #6");
  COMPARE_MACRO(Ld3r(v25.V2S(),
                     v26.V2S(),
                     v27.V2S(),
                     MemOperand(x10, 12, PostIndex)),
                "ld3r {v25.2s, v26.2s, v27.2s}, [x10], #12");
  COMPARE_MACRO(Ld3r(v26.V4S(),
                     v27.V4S(),
                     v28.V4S(),
                     MemOperand(x11, x18, PostIndex)),
                "ld3r {v26.4s, v27.4s, v28.4s}, [x11], x18");
  COMPARE_MACRO(Ld3r(v27.V2D(),
                     v28.V2D(),
                     v29.V2D(),
                     MemOperand(x12, 24, PostIndex)),
                "ld3r {v27.2d, v28.2d, v29.2d}, [x12], #24");

  COMPARE_MACRO(Ld4r(v14.V8B(),
                     v15.V8B(),
                     v16.V8B(),
                     v17.V8B(),
                     MemOperand(x0)),
                "ld4r {v14.8b, v15.8b, v16.8b, v17.8b}, [x0]");
  COMPARE_MACRO(Ld4r(v15.V16B(),
                     v16.V16B(),
                     v17.V16B(),
                     v18.V16B(),
                     MemOperand(x1)),
                "ld4r {v15.16b, v16.16b, v17.16b, v18.16b}, [x1]");
  COMPARE_MACRO(Ld4r(v16.V4H(),
                     v17.V4H(),
                     v18.V4H(),
                     v19.V4H(),
                     MemOperand(x2)),
                "ld4r {v16.4h, v17.4h, v18.4h, v19.4h}, [x2]");
  COMPARE_MACRO(Ld4r(v17.V8H(),
                     v18.V8H(),
                     v19.V8H(),
                     v20.V8H(),
                     MemOperand(x3)),
                "ld4r {v17.8h, v18.8h, v19.8h, v20.8h}, [x3]");
  COMPARE_MACRO(Ld4r(v18.V2S(),
                     v19.V2S(),
                     v20.V2S(),
                     v21.V2S(),
                     MemOperand(x4)),
                "ld4r {v18.2s, v19.2s, v20.2s, v21.2s}, [x4]");
  COMPARE_MACRO(Ld4r(v19.V4S(),
                     v20.V4S(),
                     v21.V4S(),
                     v22.V4S(),
                     MemOperand(x5)),
                "ld4r {v19.4s, v20.4s, v21.4s, v22.4s}, [x5]");
  COMPARE_MACRO(Ld4r(v20.V2D(),
                     v21.V2D(),
                     v22.V2D(),
                     v23.V2D(),
                     MemOperand(sp)),
                "ld4r {v20.2d, v21.2d, v22.2d, v23.2d}, [sp]");
  COMPARE_MACRO(Ld4r(v21.V8B(),
                     v22.V8B(),
                     v23.V8B(),
                     v24.V8B(),
                     MemOperand(x6, 4, PostIndex)),
                "ld4r {v21.8b, v22.8b, v23.8b, v24.8b}, [x6], #4");
  COMPARE_MACRO(Ld4r(v22.V16B(),
                     v23.V16B(),
                     v24.V16B(),
                     v25.V16B(),
                     MemOperand(x7, x16, PostIndex)),
                "ld4r {v22.16b, v23.16b, v24.16b, v25.16b}, [x7], x16");
  COMPARE_MACRO(Ld4r(v23.V4H(),
                     v24.V4H(),
                     v25.V4H(),
                     v26.V4H(),
                     MemOperand(x8, x17, PostIndex)),
                "ld4r {v23.4h, v24.4h, v25.4h, v26.4h}, [x8], x17");
  COMPARE_MACRO(Ld4r(v24.V8H(),
                     v25.V8H(),
                     v26.V8H(),
                     v27.V8H(),
                     MemOperand(x9, 8, PostIndex)),
                "ld4r {v24.8h, v25.8h, v26.8h, v27.8h}, [x9], #8");
  COMPARE_MACRO(Ld4r(v25.V2S(),
                     v26.V2S(),
                     v27.V2S(),
                     v28.V2S(),
                     MemOperand(x10, 16, PostIndex)),
                "ld4r {v25.2s, v26.2s, v27.2s, v28.2s}, [x10], #16");
  COMPARE_MACRO(Ld4r(v26.V4S(),
                     v27.V4S(),
                     v28.V4S(),
                     v29.V4S(),
                     MemOperand(x11, x18, PostIndex)),
                "ld4r {v26.4s, v27.4s, v28.4s, v29.4s}, [x11], x18");
  COMPARE_MACRO(Ld4r(v27.V2D(),
                     v28.V2D(),
                     v29.V2D(),
                     v30.V2D(),
                     MemOperand(x12, 32, PostIndex)),
                "ld4r {v27.2d, v28.2d, v29.2d, v30.2d}, [x12], #32");

  CLEANUP();
}


TEST(neon_load_all_lanes_unallocated) {
  SETUP();

  const char* expected = "unallocated (Unallocated)";
  // LD1R (single structure) (no offset)
  COMPARE(dci(0x0d00c000), expected);  // L = 0
  COMPARE(dci(0x0d40d000), expected);  // S = 1
  // LD2R (single structure) (no offset)
  COMPARE(dci(0x0d20c000), expected);  // L = 0
  COMPARE(dci(0x0d60d000), expected);  // S = 1
  // LD3R (single structure) (no offset)
  COMPARE(dci(0x0d00e000), expected);  // L = 0
  COMPARE(dci(0x0d40f000), expected);  // S = 1
  // LD4R (single structure) (no offset)
  COMPARE(dci(0x0d20e000), expected);  // L = 0
  COMPARE(dci(0x0d60f000), expected);  // S = 1

  // LD1R (single structure) (post index)
  COMPARE(dci(0x0d80c000), expected);  // L = 0
  COMPARE(dci(0x0dc0d000), expected);  // S = 1
  // LD2R (single structure) (post index)
  COMPARE(dci(0x0da0c000), expected);  // L = 0
  COMPARE(dci(0x0de0d000), expected);  // S = 1
  // LD3R (single structure) (post index)
  COMPARE(dci(0x0d80e000), expected);  // L = 0
  COMPARE(dci(0x0dc0f000), expected);  // S = 1
  // LD4R (single structure) (post index)
  COMPARE(dci(0x0da0e000), expected);  // L = 0
  COMPARE(dci(0x0de0f000), expected);  // S = 1

  CLEANUP();
}


TEST(neon_3same) {
  SETUP();

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Cmeq(v0.M, v1.M, v2.M), "cmeq v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Cmge(v0.M, v1.M, v2.M), "cmge v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Cmgt(v0.M, v1.M, v2.M), "cmgt v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Cmhi(v0.M, v1.M, v2.M), "cmhi v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Cmhs(v0.M, v1.M, v2.M), "cmhs v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Cmtst(v0.M, v1.M, v2.M), "cmtst v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Add(v0.M, v1.M, v2.M), "add v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Sub(v3.M, v4.M, v5.M), "sub v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Sabd(v3.M, v4.M, v5.M), "sabd v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Uabd(v3.M, v4.M, v5.M), "uabd v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Saba(v3.M, v4.M, v5.M), "saba v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Uaba(v3.M, v4.M, v5.M), "uaba v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Smax(v3.M, v4.M, v5.M), "smax v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Smin(v3.M, v4.M, v5.M), "smin v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Umax(v3.M, v4.M, v5.M), "umax v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Umin(v3.M, v4.M, v5.M), "umin v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Smaxp(v3.M, v4.M, v5.M), "smaxp v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Sminp(v3.M, v4.M, v5.M), "sminp v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Umaxp(v3.M, v4.M, v5.M), "umaxp v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Uminp(v3.M, v4.M, v5.M), "uminp v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Uqadd(v6.M, v7.M, v8.M), "uqadd v6." S ", v7." S ", v8." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                  \
  COMPARE_MACRO(Sqadd(v9.M, v10.M, v11.M), \
                "sqadd v9." S ", v10." S   \
                ", "                       \
                "v11." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Uqsub(v6.M, v7.M, v8.M), "uqsub v6." S ", v7." S ", v8." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                  \
  COMPARE_MACRO(Sqsub(v9.M, v10.M, v11.M), \
                "sqsub v9." S ", v10." S   \
                ", "                       \
                "v11." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                  \
  COMPARE_MACRO(Sshl(v12.M, v13.M, v14.M), \
                "sshl v12." S ", v13." S   \
                ", "                       \
                "v14." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                  \
  COMPARE_MACRO(Ushl(v15.M, v16.M, v17.M), \
                "ushl v15." S ", v16." S   \
                ", "                       \
                "v17." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Sqshl(v18.M, v19.M, v20.M), \
                "sqshl v18." S ", v19." S ", v20." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Uqshl(v21.M, v22.M, v23.M), \
                "uqshl v21." S ", v22." S ", v23." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Srshl(v24.M, v25.M, v26.M), \
                "srshl v24." S ", v25." S ", v26." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Urshl(v27.M, v28.M, v29.M), \
                "urshl v27." S ", v28." S ", v29." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Sqrshl(v30.M, v31.M, v0.M), \
                "sqrshl v30." S ", v31." S ", v0." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Uqrshl(v1.M, v2.M, v3.M), "uqrshl v1." S ", v2." S ", v3." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Shadd(v4.M, v5.M, v6.M), "shadd v4." S ", v5." S ", v6." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Uhadd(v7.M, v8.M, v9.M), "uhadd v7." S ", v8." S ", v9." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                    \
  COMPARE_MACRO(Srhadd(v10.M, v11.M, v12.M), \
                "srhadd v10." S ", v11." S ", v12." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                    \
  COMPARE_MACRO(Urhadd(v13.M, v14.M, v15.M), \
                "urhadd v13." S ", v14." S ", v15." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Shsub(v16.M, v17.M, v18.M), \
                "shsub v16." S ", v17." S ", v18." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Uhsub(v19.M, v20.M, v21.M), \
                "uhsub v19." S ", v20." S ", v21." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                  \
  COMPARE_MACRO(Addp(v19.M, v20.M, v21.M), \
                "addp v19." S ", v20." S   \
                ", "                       \
                "v21." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Mla(v19.M, v20.M, v21.M), "mla v19." S ", v20." S ", v21." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Mls(v19.M, v20.M, v21.M), "mls v19." S ", v20." S ", v21." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Mul(v19.M, v20.M, v21.M), "mul v19." S ", v20." S ", v21." S);
  NEON_FORMAT_LIST_BHS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                  \
  COMPARE_MACRO(Sqdmulh(v1.M, v2.M, v3.M), \
                "sqdmulh v1." S ", v2." S  \
                ", "                       \
                "v3." S);
  NEON_FORMAT_LIST_HS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Sqrdmulh(v1.M, v2.M, v3.M), \
                "sqrdmulh v1." S ", v2." S ", v3." S);
  NEON_FORMAT_LIST_HS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Sqrdmlah(v1.M, v2.M, v3.M), \
                "sqrdmlah v1." S ", v2." S ", v3." S);
  NEON_FORMAT_LIST_HS(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Sqrdmlsh(v1.M, v2.M, v3.M), \
                "sqrdmlsh v1." S ", v2." S ", v3." S);
  NEON_FORMAT_LIST_HS(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Sdot(v1.V2S(), v2.V8B(), v3.V8B()), "sdot v1.2s, v2.8b, v3.8b");
  COMPARE_MACRO(Sdot(v1.V4S(), v2.V16B(), v3.V16B()),
                "sdot v1.4s, v2.16b, v3.16b");

  COMPARE_MACRO(Udot(v1.V2S(), v2.V8B(), v3.V8B()), "udot v1.2s, v2.8b, v3.8b");
  COMPARE_MACRO(Udot(v1.V4S(), v2.V16B(), v3.V16B()),
                "udot v1.4s, v2.16b, v3.16b");

  COMPARE_MACRO(And(v6.V8B(), v7.V8B(), v8.V8B()), "and v6.8b, v7.8b, v8.8b");
  COMPARE_MACRO(And(v6.V16B(), v7.V16B(), v8.V16B()),
                "and v6.16b, v7.16b, v8.16b");

  COMPARE_MACRO(Bic(v6.V8B(), v7.V8B(), v8.V8B()), "bic v6.8b, v7.8b, v8.8b");
  COMPARE_MACRO(Bic(v6.V16B(), v7.V16B(), v8.V16B()),
                "bic v6.16b, v7.16b, v8.16b");

  COMPARE_MACRO(Orr(v6.V8B(), v7.V8B(), v8.V8B()), "orr v6.8b, v7.8b, v8.8b");
  COMPARE_MACRO(Orr(v6.V16B(), v7.V16B(), v8.V16B()),
                "orr v6.16b, v7.16b, v8.16b");

  COMPARE_MACRO(Orr(v6.V8B(), v7.V8B(), v7.V8B()), "mov v6.8b, v7.8b");
  COMPARE_MACRO(Orr(v6.V16B(), v7.V16B(), v7.V16B()), "mov v6.16b, v7.16b");

  COMPARE_MACRO(Mov(v6.V8B(), v8.V8B()), "mov v6.8b, v8.8b");
  COMPARE_MACRO(Mov(v6.V16B(), v8.V16B()), "mov v6.16b, v8.16b");

  COMPARE_MACRO(Orn(v6.V8B(), v7.V8B(), v8.V8B()), "orn v6.8b, v7.8b, v8.8b");
  COMPARE_MACRO(Orn(v6.V16B(), v7.V16B(), v8.V16B()),
                "orn v6.16b, v7.16b, v8.16b");

  COMPARE_MACRO(Eor(v6.V8B(), v7.V8B(), v8.V8B()), "eor v6.8b, v7.8b, v8.8b");
  COMPARE_MACRO(Eor(v6.V16B(), v7.V16B(), v8.V16B()),
                "eor v6.16b, v7.16b, v8.16b");

  COMPARE_MACRO(Bif(v6.V8B(), v7.V8B(), v8.V8B()), "bif v6.8b, v7.8b, v8.8b");
  COMPARE_MACRO(Bif(v6.V16B(), v7.V16B(), v8.V16B()),
                "bif v6.16b, v7.16b, v8.16b");

  COMPARE_MACRO(Bit(v6.V8B(), v7.V8B(), v8.V8B()), "bit v6.8b, v7.8b, v8.8b");
  COMPARE_MACRO(Bit(v6.V16B(), v7.V16B(), v8.V16B()),
                "bit v6.16b, v7.16b, v8.16b");

  COMPARE_MACRO(Bsl(v6.V8B(), v7.V8B(), v8.V8B()), "bsl v6.8b, v7.8b, v8.8b");
  COMPARE_MACRO(Bsl(v6.V16B(), v7.V16B(), v8.V16B()),
                "bsl v6.16b, v7.16b, v8.16b");

  COMPARE_MACRO(Pmul(v6.V8B(), v7.V8B(), v8.V8B()), "pmul v6.8b, v7.8b, v8.8b");
  COMPARE_MACRO(Pmul(v6.V16B(), v7.V16B(), v8.V16B()),
                "pmul v6.16b, v7.16b, v8.16b");

  CLEANUP();
}

TEST(neon_3same_fp16) {
  SETUP();

  COMPARE_MACRO(Fmaxnm(v0.V8H(), v1.V8H(), v2.V8H()),
                "fmaxnm v0.8h, v1.8h, v2.8h");
  COMPARE_MACRO(Fmaxnm(v3.V4H(), v4.V4H(), v5.V4H()),
                "fmaxnm v3.4h, v4.4h, v5.4h");
  COMPARE_MACRO(Fmla(v6.V8H(), v7.V8H(), v8.V8H()), "fmla v6.8h, v7.8h, v8.8h");
  COMPARE_MACRO(Fmla(v9.V4H(), v10.V4H(), v11.V4H()),
                "fmla v9.4h, v10.4h, v11.4h");
  COMPARE_MACRO(Fadd(v12.V8H(), v13.V8H(), v14.V8H()),
                "fadd v12.8h, v13.8h, v14.8h");
  COMPARE_MACRO(Fadd(v15.V4H(), v16.V4H(), v17.V4H()),
                "fadd v15.4h, v16.4h, v17.4h");
  COMPARE_MACRO(Fmulx(v18.V8H(), v19.V8H(), v20.V8H()),
                "fmulx v18.8h, v19.8h, v20.8h");
  COMPARE_MACRO(Fmulx(v21.V4H(), v22.V4H(), v23.V4H()),
                "fmulx v21.4h, v22.4h, v23.4h");
  COMPARE_MACRO(Fcmeq(v24.V8H(), v25.V8H(), v26.V8H()),
                "fcmeq v24.8h, v25.8h, v26.8h");
  COMPARE_MACRO(Fcmeq(v27.V4H(), v28.V4H(), v29.V4H()),
                "fcmeq v27.4h, v28.4h, v29.4h");
  COMPARE_MACRO(Fmax(v30.V8H(), v0.V8H(), v1.V8H()),
                "fmax v30.8h, v0.8h, v1.8h");
  COMPARE_MACRO(Fmax(v2.V4H(), v3.V4H(), v4.V4H()), "fmax v2.4h, v3.4h, v4.4h");
  COMPARE_MACRO(Frecps(v5.V8H(), v6.V8H(), v7.V8H()),
                "frecps v5.8h, v6.8h, v7.8h");
  COMPARE_MACRO(Frecps(v8.V4H(), v9.V4H(), v10.V4H()),
                "frecps v8.4h, v9.4h, v10.4h");
  COMPARE_MACRO(Fminnm(v11.V8H(), v12.V8H(), v13.V8H()),
                "fminnm v11.8h, v12.8h, v13.8h");
  COMPARE_MACRO(Fminnm(v14.V4H(), v15.V4H(), v16.V4H()),
                "fminnm v14.4h, v15.4h, v16.4h");
  COMPARE_MACRO(Fmls(v17.V8H(), v18.V8H(), v19.V8H()),
                "fmls v17.8h, v18.8h, v19.8h");
  COMPARE_MACRO(Fmls(v20.V4H(), v21.V4H(), v22.V4H()),
                "fmls v20.4h, v21.4h, v22.4h");
  COMPARE_MACRO(Fsub(v23.V8H(), v24.V8H(), v25.V8H()),
                "fsub v23.8h, v24.8h, v25.8h");
  COMPARE_MACRO(Fsub(v26.V4H(), v27.V4H(), v28.V4H()),
                "fsub v26.4h, v27.4h, v28.4h");
  COMPARE_MACRO(Fmin(v29.V8H(), v30.V8H(), v0.V8H()),
                "fmin v29.8h, v30.8h, v0.8h");
  COMPARE_MACRO(Fmin(v1.V4H(), v2.V4H(), v3.V4H()), "fmin v1.4h, v2.4h, v3.4h");
  COMPARE_MACRO(Frsqrts(v4.V8H(), v5.V8H(), v6.V8H()),
                "frsqrts v4.8h, v5.8h, v6.8h");
  COMPARE_MACRO(Frsqrts(v7.V4H(), v8.V4H(), v9.V4H()),
                "frsqrts v7.4h, v8.4h, v9.4h");
  COMPARE_MACRO(Fmaxnmp(v10.V8H(), v11.V8H(), v12.V8H()),
                "fmaxnmp v10.8h, v11.8h, v12.8h");
  COMPARE_MACRO(Fmaxnmp(v13.V4H(), v14.V4H(), v15.V4H()),
                "fmaxnmp v13.4h, v14.4h, v15.4h");
  COMPARE_MACRO(Faddp(v16.V8H(), v17.V8H(), v18.V8H()),
                "faddp v16.8h, v17.8h, v18.8h");
  COMPARE_MACRO(Faddp(v19.V4H(), v20.V4H(), v21.V4H()),
                "faddp v19.4h, v20.4h, v21.4h");
  COMPARE_MACRO(Fmul(v22.V8H(), v23.V8H(), v24.V8H()),
                "fmul v22.8h, v23.8h, v24.8h");
  COMPARE_MACRO(Fmul(v25.V4H(), v26.V4H(), v27.V4H()),
                "fmul v25.4h, v26.4h, v27.4h");
  COMPARE_MACRO(Fcmge(v28.V8H(), v29.V8H(), v30.V8H()),
                "fcmge v28.8h, v29.8h, v30.8h");
  COMPARE_MACRO(Fcmge(v0.V4H(), v1.V4H(), v2.V4H()),
                "fcmge v0.4h, v1.4h, v2.4h");
  COMPARE_MACRO(Facge(v3.V8H(), v4.V8H(), v5.V8H()),
                "facge v3.8h, v4.8h, v5.8h");
  COMPARE_MACRO(Facge(v6.V4H(), v7.V4H(), v8.V4H()),
                "facge v6.4h, v7.4h, v8.4h");
  COMPARE_MACRO(Fmaxp(v9.V8H(), v10.V8H(), v11.V8H()),
                "fmaxp v9.8h, v10.8h, v11.8h");
  COMPARE_MACRO(Fmaxp(v12.V4H(), v13.V4H(), v14.V4H()),
                "fmaxp v12.4h, v13.4h, v14.4h");
  COMPARE_MACRO(Fdiv(v15.V8H(), v16.V8H(), v17.V8H()),
                "fdiv v15.8h, v16.8h, v17.8h");
  COMPARE_MACRO(Fdiv(v18.V4H(), v19.V4H(), v20.V4H()),
                "fdiv v18.4h, v19.4h, v20.4h");
  COMPARE_MACRO(Fminnmp(v21.V8H(), v22.V8H(), v23.V8H()),
                "fminnmp v21.8h, v22.8h, v23.8h");
  COMPARE_MACRO(Fminnmp(v24.V4H(), v25.V4H(), v26.V4H()),
                "fminnmp v24.4h, v25.4h, v26.4h");
  COMPARE_MACRO(Fabd(v27.V8H(), v28.V8H(), v29.V8H()),
                "fabd v27.8h, v28.8h, v29.8h");
  COMPARE_MACRO(Fabd(v30.V4H(), v0.V4H(), v1.V4H()),
                "fabd v30.4h, v0.4h, v1.4h");
  COMPARE_MACRO(Fcmgt(v2.V8H(), v3.V8H(), v4.V8H()),
                "fcmgt v2.8h, v3.8h, v4.8h");
  COMPARE_MACRO(Fcmgt(v5.V4H(), v6.V4H(), v7.V4H()),
                "fcmgt v5.4h, v6.4h, v7.4h");
  COMPARE_MACRO(Facgt(v8.V8H(), v9.V8H(), v10.V8H()),
                "facgt v8.8h, v9.8h, v10.8h");
  COMPARE_MACRO(Facgt(v11.V4H(), v12.V4H(), v13.V4H()),
                "facgt v11.4h, v12.4h, v13.4h");
  COMPARE_MACRO(Fminp(v14.V8H(), v15.V8H(), v16.V8H()),
                "fminp v14.8h, v15.8h, v16.8h");
  COMPARE_MACRO(Fminp(v17.V4H(), v18.V4H(), v19.V4H()),
                "fminp v17.4h, v18.4h, v19.4h");

  CLEANUP();
}

TEST(neon_3same_extra_fcadd) {
  SETUP();

  COMPARE_MACRO(Fcadd(v4.V4H(), v5.V4H(), v6.V4H(), 270),
                "fcadd v4.4h, v5.4h, v6.4h, #270");
  COMPARE_MACRO(Fcadd(v4.V8H(), v5.V8H(), v6.V8H(), 90),
                "fcadd v4.8h, v5.8h, v6.8h, #90");
  COMPARE_MACRO(Fcadd(v1.V2S(), v2.V2S(), v3.V2S(), 90),
                "fcadd v1.2s, v2.2s, v3.2s, #90");
  COMPARE_MACRO(Fcadd(v1.V4S(), v2.V4S(), v3.V4S(), 270),
                "fcadd v1.4s, v2.4s, v3.4s, #270");
  COMPARE_MACRO(Fcadd(v29.V2D(), v30.V2D(), v31.V2D(), 90),
                "fcadd v29.2d, v30.2d, v31.2d, #90");

  COMPARE_MACRO(Fcmla(v4.V8H(), v5.V8H(), v6.V8H(), 270),
                "fcmla v4.8h, v5.8h, v6.8h, #270");
  COMPARE_MACRO(Fcmla(v9.V2S(), v8.V2S(), v7.V2S(), 180),
                "fcmla v9.2s, v8.2s, v7.2s, #180");
  COMPARE_MACRO(Fcmla(v11.V4S(), v12.V4S(), v13.V4S(), 90),
                "fcmla v11.4s, v12.4s, v13.4s, #90");
  COMPARE_MACRO(Fcmla(v21.V2D(), v22.V2D(), v23.V2D(), 0),
                "fcmla v21.2d, v22.2d, v23.2d, #0");

  // The FC* instructions span multiple opcodes, so they have special cases in
  // the disassembler. Verify that similar encodings don't get disassembled as
  // FC* instructions.
  COMPARE(dci(0x2e00ec00), "unallocated (Unallocated)");  // opcode = 0x1101
  COMPARE(dci(0x2e00fc00), "unallocated (Unallocated)");  // opcode = 0x1111

  CLEANUP();
}

#define NEON_FORMAT_LIST_FP(V) \
  V(V2S(), "2s")               \
  V(V4S(), "4s")               \
  V(V2D(), "2d")

#define NEON_FORMAT_LIST_FP_FP16(V) \
  NEON_FORMAT_LIST_FP(V)            \
  V(V4H(), "4h")                    \
  V(V8H(), "8h")

TEST(neon_fp_3same) {
  SETUP();

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fadd(v0.M, v1.M, v2.M), "fadd v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST_FP_FP16(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fsub(v3.M, v4.M, v5.M), "fsub v3." S ", v4." S ", v5." S);
  NEON_FORMAT_LIST_FP_FP16(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fmul(v6.M, v7.M, v8.M), "fmul v6." S ", v7." S ", v8." S);
  NEON_FORMAT_LIST_FP_FP16(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fdiv(v9.M, v10.M, v11.M), "fdiv v9." S ", v10." S ", v11." S);
  NEON_FORMAT_LIST_FP_FP16(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                  \
  COMPARE_MACRO(Fmin(v12.M, v13.M, v14.M), \
                "fmin v12." S ", v13." S   \
                ", "                       \
                "v14." S);
  NEON_FORMAT_LIST_FP_FP16(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                    \
  COMPARE_MACRO(Fminnm(v15.M, v16.M, v17.M), \
                "fminnm v15." S ", v16." S ", v17." S);
  NEON_FORMAT_LIST_FP_FP16(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                  \
  COMPARE_MACRO(Fmax(v18.M, v19.M, v20.M), \
                "fmax v18." S ", v19." S   \
                ", "                       \
                "v20." S);
  NEON_FORMAT_LIST_FP_FP16(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                    \
  COMPARE_MACRO(Fmaxnm(v21.M, v22.M, v23.M), \
                "fmaxnm v21." S ", v22." S ", v23." S);
  NEON_FORMAT_LIST_FP_FP16(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                    \
  COMPARE_MACRO(Frecps(v24.M, v25.M, v26.M), \
                "frecps v24." S ", v25." S ", v26." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                     \
  COMPARE_MACRO(Frsqrts(v27.M, v28.M, v29.M), \
                "frsqrts v27." S ", v28." S ", v29." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                  \
  COMPARE_MACRO(Fmulx(v30.M, v31.M, v0.M), \
                "fmulx v30." S ", v31." S  \
                ", "                       \
                "v0." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fmla(v1.M, v2.M, v3.M), "fmla v1." S ", v2." S ", v3." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fmls(v4.M, v5.M, v6.M), "fmls v4." S ", v5." S ", v6." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fabd(v7.M, v8.M, v9.M), "fabd v7." S ", v8." S ", v9." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Faddp(v10.M, v11.M, v12.M), \
                "faddp v10." S ", v11." S ", v12." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Fmaxp(v13.M, v14.M, v15.M), \
                "fmaxp v13." S ", v14." S ", v15." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Fminp(v16.M, v17.M, v18.M), \
                "fminp v16." S ", v17." S ", v18." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                     \
  COMPARE_MACRO(Fmaxnmp(v19.M, v20.M, v21.M), \
                "fmaxnmp v19." S ", v20." S ", v21." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                     \
  COMPARE_MACRO(Fminnmp(v22.M, v23.M, v24.M), \
                "fminnmp v22." S ", v23." S ", v24." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Fcmeq(v25.M, v26.M, v27.M), \
                "fcmeq v25." S ", v26." S ", v27." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Fcmge(v25.M, v26.M, v27.M), \
                "fcmge v25." S ", v26." S ", v27." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Fcmgt(v25.M, v26.M, v27.M), \
                "fcmgt v25." S ", v26." S ", v27." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Facge(v25.M, v26.M, v27.M), \
                "facge v25." S ", v26." S ", v27." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S)                   \
  COMPARE_MACRO(Facgt(v25.M, v26.M, v27.M), \
                "facgt v25." S ", v26." S ", v27." S);
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

  // FMLAL and so on have mixed operands, but still use the NEON3Same encoding.
  COMPARE_MACRO(Fmlal(v5.V2S(), v12.V2H(), v6.V2H()),
                "fmlal v5.2s, v12.2h, v6.2h");
  COMPARE_MACRO(Fmlal2(v14.V2S(), v10.V2H(), v9.V2H()),
                "fmlal2 v14.2s, v10.2h, v9.2h");
  COMPARE_MACRO(Fmlsl(v31.V2S(), v9.V2H(), v28.V2H()),
                "fmlsl v31.2s, v9.2h, v28.2h");
  COMPARE_MACRO(Fmlsl2(v21.V2S(), v22.V2H(), v2.V2H()),
                "fmlsl2 v21.2s, v22.2h, v2.2h");
  COMPARE_MACRO(Fmlal(v26.V4S(), v26.V4H(), v30.V4H()),
                "fmlal v26.4s, v26.4h, v30.4h");
  COMPARE_MACRO(Fmlal2(v15.V4S(), v18.V4H(), v25.V4H()),
                "fmlal2 v15.4s, v18.4h, v25.4h");
  COMPARE_MACRO(Fmlsl(v9.V4S(), v4.V4H(), v23.V4H()),
                "fmlsl v9.4s, v4.4h, v23.4h");
  COMPARE_MACRO(Fmlsl2(v28.V4S(), v28.V4H(), v17.V4H()),
                "fmlsl2 v28.4s, v28.4h, v17.4h");

  // Verify that unallocated encodings similar to FMLAL (and so on) are properly
  // handled.
  COMPARE(dci(0x0e66ed85), "unallocated (Unallocated)");
  COMPARE(dci(0x2e69cd4e), "unallocated (Unallocated)");
  COMPARE(dci(0x0efced3f), "unallocated (Unallocated)");
  COMPARE(dci(0x2ee2ced5), "unallocated (Unallocated)");
  COMPARE(dci(0x4e7eef5a), "unallocated (Unallocated)");
  COMPARE(dci(0x6e79ce4f), "unallocated (Unallocated)");
  COMPARE(dci(0x4ef7ec89), "unallocated (Unallocated)");
  COMPARE(dci(0x6ef1cf9c), "unallocated (Unallocated)");

  CLEANUP();
}


#define NEON_SCALAR_FORMAT_LIST(V) \
  V(B(), "b")                      \
  V(H(), "h")                      \
  V(S(), "s")                      \
  V(D(), "d")

TEST(neon_scalar_3same) {
  SETUP();

  // Instructions that only support D-sized scalar operations.
  COMPARE_MACRO(Add(v0.D(), v1.D(), v2.D()), "add d0, d1, d2");
  COMPARE_MACRO(Sub(v3.D(), v4.D(), v5.D()), "sub d3, d4, d5");
  COMPARE_MACRO(Cmeq(v0.D(), v1.D(), v2.D()), "cmeq d0, d1, d2");
  COMPARE_MACRO(Cmge(v3.D(), v4.D(), v5.D()), "cmge d3, d4, d5");
  COMPARE_MACRO(Cmgt(v6.D(), v7.D(), v8.D()), "cmgt d6, d7, d8");
  COMPARE_MACRO(Cmhi(v0.D(), v1.D(), v2.D()), "cmhi d0, d1, d2");
  COMPARE_MACRO(Cmhs(v3.D(), v4.D(), v5.D()), "cmhs d3, d4, d5");
  COMPARE_MACRO(Cmtst(v6.D(), v7.D(), v8.D()), "cmtst d6, d7, d8");
  COMPARE_MACRO(Ushl(v6.D(), v7.D(), v8.D()), "ushl d6, d7, d8");
  COMPARE_MACRO(Sshl(v6.D(), v7.D(), v8.D()), "sshl d6, d7, d8");
  COMPARE_MACRO(Urshl(v9.D(), v10.D(), v11.D()), "urshl d9, d10, d11");
  COMPARE_MACRO(Srshl(v9.D(), v10.D(), v11.D()), "srshl d9, d10, d11");

  // Instructions that support S and D-sized scalar operations.
  COMPARE_MACRO(Frecps(v12.S(), v13.S(), v14.S()), "frecps s12, s13, s14");
  COMPARE_MACRO(Frecps(v15.D(), v16.D(), v17.D()), "frecps d15, d16, d17");
  COMPARE_MACRO(Frsqrts(v18.S(), v19.S(), v20.S()), "frsqrts s18, s19, s20");
  COMPARE_MACRO(Frsqrts(v21.D(), v22.D(), v23.D()), "frsqrts d21, d22, d23");
  COMPARE_MACRO(Fmulx(v12.S(), v13.S(), v14.S()), "fmulx s12, s13, s14");
  COMPARE_MACRO(Fmulx(v15.D(), v16.D(), v17.D()), "fmulx d15, d16, d17");
  COMPARE_MACRO(Fcmeq(v12.S(), v13.S(), v14.S()), "fcmeq s12, s13, s14");
  COMPARE_MACRO(Fcmeq(v15.D(), v16.D(), v17.D()), "fcmeq d15, d16, d17");
  COMPARE_MACRO(Fcmge(v12.S(), v13.S(), v14.S()), "fcmge s12, s13, s14");
  COMPARE_MACRO(Fcmge(v15.D(), v16.D(), v17.D()), "fcmge d15, d16, d17");
  COMPARE_MACRO(Fcmgt(v12.S(), v13.S(), v14.S()), "fcmgt s12, s13, s14");
  COMPARE_MACRO(Fcmgt(v15.D(), v16.D(), v17.D()), "fcmgt d15, d16, d17");
  COMPARE_MACRO(Fcmge(v12.S(), v13.S(), v14.S()), "fcmge s12, s13, s14");
  COMPARE_MACRO(Fcmge(v15.D(), v16.D(), v17.D()), "fcmge d15, d16, d17");
  COMPARE_MACRO(Facgt(v12.S(), v13.S(), v14.S()), "facgt s12, s13, s14");
  COMPARE_MACRO(Facgt(v15.D(), v16.D(), v17.D()), "facgt d15, d16, d17");

  // Instructions that support H and S-sized scalar operations.
  COMPARE_MACRO(Sqdmulh(v12.S(), v13.S(), v14.S()), "sqdmulh s12, s13, s14");
  COMPARE_MACRO(Sqdmulh(v15.H(), v16.H(), v17.H()), "sqdmulh h15, h16, h17");
  COMPARE_MACRO(Sqrdmulh(v12.S(), v13.S(), v14.S()), "sqrdmulh s12, s13, s14");
  COMPARE_MACRO(Sqrdmulh(v15.H(), v16.H(), v17.H()), "sqrdmulh h15, h16, h17");
  COMPARE_MACRO(Sqrdmlah(v12.S(), v13.S(), v14.S()), "sqrdmlah s12, s13, s14");
  COMPARE_MACRO(Sqrdmlah(v15.H(), v16.H(), v17.H()), "sqrdmlah h15, h16, h17");
  COMPARE_MACRO(Sqrdmlsh(v12.S(), v13.S(), v14.S()), "sqrdmlsh s12, s13, s14");
  COMPARE_MACRO(Sqrdmlsh(v15.H(), v16.H(), v17.H()), "sqrdmlsh h15, h16, h17");

#define DISASM_INST(M, R) \
  COMPARE_MACRO(Uqadd(v6.M, v7.M, v8.M), "uqadd " R "6, " R "7, " R "8");
  NEON_SCALAR_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, R) \
  COMPARE_MACRO(Uqsub(v9.M, v10.M, v11.M), "uqsub " R "9, " R "10, " R "11");
  NEON_SCALAR_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, R)                    \
  COMPARE_MACRO(Sqadd(v12.M, v13.M, v14.M),  \
                "sqadd " R "12, " R "13, " R \
                "1"                          \
                "4");
  NEON_SCALAR_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, R)                    \
  COMPARE_MACRO(Sqsub(v15.M, v16.M, v17.M),  \
                "sqsub " R "15, " R "16, " R \
                "1"                          \
                "7");
  NEON_SCALAR_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, R)                    \
  COMPARE_MACRO(Uqshl(v18.M, v19.M, v20.M),  \
                "uqshl " R "18, " R "19, " R \
                "2"                          \
                "0");
  NEON_SCALAR_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, R)                    \
  COMPARE_MACRO(Sqshl(v21.M, v22.M, v23.M),  \
                "sqshl " R "21, " R "22, " R \
                "2"                          \
                "3");
  NEON_SCALAR_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, R) \
  COMPARE_MACRO(Uqrshl(v30.M, v31.M, v0.M), "uqrshl " R "30, " R "31, " R "0");
  NEON_SCALAR_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, R) \
  COMPARE_MACRO(Sqrshl(v1.M, v2.M, v3.M), "sqrshl " R "1, " R "2, " R "3");
  NEON_SCALAR_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

  CLEANUP();
}


TEST(neon_scalar_3same_fp16) {
  SETUP();

  COMPARE_MACRO(Fmulx(v0.H(), v1.H(), v2.H()), "fmulx h0, h1, h2");
  COMPARE_MACRO(Fcmeq(v3.H(), v4.H(), v5.H()), "fcmeq h3, h4, h5");
  COMPARE_MACRO(Fcmge(v6.H(), v7.H(), v8.H()), "fcmge h6, h7, h8");
  COMPARE_MACRO(Fcmgt(v9.H(), v10.H(), v11.H()), "fcmgt h9, h10, h11");
  COMPARE_MACRO(Facge(v12.H(), v13.H(), v14.H()), "facge h12, h13, h14");
  COMPARE_MACRO(Facgt(v15.H(), v16.H(), v17.H()), "facgt h15, h16, h17");
  COMPARE_MACRO(Frecps(v18.H(), v19.H(), v20.H()), "frecps h18, h19, h20");
  COMPARE_MACRO(Frsqrts(v21.H(), v22.H(), v23.H()), "frsqrts h21, h22, h23");
  COMPARE_MACRO(Fabd(v24.H(), v25.H(), v26.H()), "fabd h24, h25, h26");

  CLEANUP();
}


TEST(neon_byelement) {
  SETUP();

  COMPARE_MACRO(Mul(v0.V4H(), v1.V4H(), v2.H(), 0),
                "mul v0.4h, v1.4h, v2.h[0]");
  COMPARE_MACRO(Mul(v2.V8H(), v3.V8H(), v15.H(), 7),
                "mul v2.8h, v3.8h, v15.h[7]");
  COMPARE_MACRO(Mul(v0.V2S(), v1.V2S(), v2.S(), 0),
                "mul v0.2s, v1.2s, v2.s[0]");
  COMPARE_MACRO(Mul(v2.V4S(), v3.V4S(), v15.S(), 3),
                "mul v2.4s, v3.4s, v15.s[3]");

  COMPARE_MACRO(Mla(v0.V4H(), v1.V4H(), v2.H(), 0),
                "mla v0.4h, v1.4h, v2.h[0]");
  COMPARE_MACRO(Mla(v2.V8H(), v3.V8H(), v15.H(), 7),
                "mla v2.8h, v3.8h, v15.h[7]");
  COMPARE_MACRO(Mla(v0.V2S(), v1.V2S(), v2.S(), 0),
                "mla v0.2s, v1.2s, v2.s[0]");
  COMPARE_MACRO(Mla(v2.V4S(), v3.V4S(), v15.S(), 3),
                "mla v2.4s, v3.4s, v15.s[3]");

  COMPARE_MACRO(Mls(v0.V4H(), v1.V4H(), v2.H(), 0),
                "mls v0.4h, v1.4h, v2.h[0]");
  COMPARE_MACRO(Mls(v2.V8H(), v3.V8H(), v15.H(), 7),
                "mls v2.8h, v3.8h, v15.h[7]");
  COMPARE_MACRO(Mls(v0.V2S(), v1.V2S(), v2.S(), 0),
                "mls v0.2s, v1.2s, v2.s[0]");
  COMPARE_MACRO(Mls(v2.V4S(), v3.V4S(), v15.S(), 3),
                "mls v2.4s, v3.4s, v15.s[3]");

  COMPARE_MACRO(Sqdmulh(v0.V4H(), v1.V4H(), v2.H(), 0),
                "sqdmulh v0.4h, v1.4h, v2.h[0]");
  COMPARE_MACRO(Sqdmulh(v2.V8H(), v3.V8H(), v15.H(), 7),
                "sqdmulh v2.8h, v3.8h, v15.h[7]");
  COMPARE_MACRO(Sqdmulh(v0.V2S(), v1.V2S(), v2.S(), 0),
                "sqdmulh v0.2s, v1.2s, v2.s[0]");
  COMPARE_MACRO(Sqdmulh(v2.V4S(), v3.V4S(), v15.S(), 3),
                "sqdmulh v2.4s, v3.4s, v15.s[3]");
  COMPARE_MACRO(Sqdmulh(h0, h1, v2.H(), 0), "sqdmulh h0, h1, v2.h[0]");
  COMPARE_MACRO(Sqdmulh(s0, s1, v2.S(), 0), "sqdmulh s0, s1, v2.s[0]");

  COMPARE_MACRO(Sqrdmulh(v0.V4H(), v1.V4H(), v2.H(), 0),
                "sqrdmulh v0.4h, v1.4h, v2.h[0]");
  COMPARE_MACRO(Sqrdmulh(v2.V8H(), v3.V8H(), v15.H(), 7),
                "sqrdmulh v2.8h, v3.8h, v15.h[7]");
  COMPARE_MACRO(Sqrdmulh(v0.V2S(), v1.V2S(), v2.S(), 0),
                "sqrdmulh v0.2s, v1.2s, v2.s[0]");
  COMPARE_MACRO(Sqrdmulh(v2.V4S(), v3.V4S(), v15.S(), 3),
                "sqrdmulh v2.4s, v3.4s, v15.s[3]");
  COMPARE_MACRO(Sqrdmulh(h0, h1, v2.H(), 0), "sqrdmulh h0, h1, v2.h[0]");
  COMPARE_MACRO(Sqrdmulh(s0, s1, v2.S(), 0), "sqrdmulh s0, s1, v2.s[0]");

  COMPARE_MACRO(Sdot(v0.V2S(), v1.V8B(), v2.S4B(), 0),
                "sdot v0.2s, v1.8b, v2.4b[0]");
  COMPARE_MACRO(Sdot(v2.V4S(), v3.V16B(), v15.S4B(), 3),
                "sdot v2.4s, v3.16b, v15.4b[3]");

  COMPARE_MACRO(Sqrdmlah(v0.V4H(), v1.V4H(), v2.H(), 0),
                "sqrdmlah v0.4h, v1.4h, v2.h[0]");
  COMPARE_MACRO(Sqrdmlah(v2.V8H(), v3.V8H(), v15.H(), 7),
                "sqrdmlah v2.8h, v3.8h, v15.h[7]");
  COMPARE_MACRO(Sqrdmlah(v0.V2S(), v1.V2S(), v2.S(), 0),
                "sqrdmlah v0.2s, v1.2s, v2.s[0]");
  COMPARE_MACRO(Sqrdmlah(v2.V4S(), v3.V4S(), v15.S(), 3),
                "sqrdmlah v2.4s, v3.4s, v15.s[3]");
  COMPARE_MACRO(Sqrdmlah(h0, h1, v2.H(), 0), "sqrdmlah h0, h1, v2.h[0]");
  COMPARE_MACRO(Sqrdmlah(s0, s1, v2.S(), 0), "sqrdmlah s0, s1, v2.s[0]");

  COMPARE_MACRO(Udot(v0.V2S(), v1.V8B(), v2.S4B(), 0),
                "udot v0.2s, v1.8b, v2.4b[0]");
  COMPARE_MACRO(Udot(v2.V4S(), v3.V16B(), v15.S4B(), 3),
                "udot v2.4s, v3.16b, v15.4b[3]");

  COMPARE_MACRO(Sqrdmlsh(v0.V4H(), v1.V4H(), v2.H(), 0),
                "sqrdmlsh v0.4h, v1.4h, v2.h[0]");
  COMPARE_MACRO(Sqrdmlsh(v2.V8H(), v3.V8H(), v15.H(), 7),
                "sqrdmlsh v2.8h, v3.8h, v15.h[7]");
  COMPARE_MACRO(Sqrdmlsh(v0.V2S(), v1.V2S(), v2.S(), 0),
                "sqrdmlsh v0.2s, v1.2s, v2.s[0]");
  COMPARE_MACRO(Sqrdmlsh(v2.V4S(), v3.V4S(), v15.S(), 3),
                "sqrdmlsh v2.4s, v3.4s, v15.s[3]");
  COMPARE_MACRO(Sqrdmlsh(h0, h1, v2.H(), 0), "sqrdmlsh h0, h1, v2.h[0]");
  COMPARE_MACRO(Sqrdmlsh(s0, s1, v2.S(), 0), "sqrdmlsh s0, s1, v2.s[0]");

  COMPARE_MACRO(Smull(v0.V4S(), v1.V4H(), v2.H(), 0),
                "smull v0.4s, v1.4h, v2.h[0]");
  COMPARE_MACRO(Smull2(v2.V4S(), v3.V8H(), v4.H(), 7),
                "smull2 v2.4s, v3.8h, v4.h[7]");
  COMPARE_MACRO(Smull(v0.V2D(), v1.V2S(), v2.S(), 0),
                "smull v0.2d, v1.2s, v2.s[0]");
  COMPARE_MACRO(Smull2(v2.V2D(), v3.V4S(), v4.S(), 3),
                "smull2 v2.2d, v3.4s, v4.s[3]");

  COMPARE_MACRO(Umull(v0.V4S(), v1.V4H(), v2.H(), 0),
                "umull v0.4s, v1.4h, v2.h[0]");
  COMPARE_MACRO(Umull2(v2.V4S(), v3.V8H(), v4.H(), 7),
                "umull2 v2.4s, v3.8h, v4.h[7]");
  COMPARE_MACRO(Umull(v0.V2D(), v1.V2S(), v2.S(), 0),
                "umull v0.2d, v1.2s, v2.s[0]");
  COMPARE_MACRO(Umull2(v2.V2D(), v3.V4S(), v4.S(), 3),
                "umull2 v2.2d, v3.4s, v4.s[3]");

  COMPARE_MACRO(Smlal(v0.V4S(), v1.V4H(), v2.H(), 0),
                "smlal v0.4s, v1.4h, v2.h[0]");
  COMPARE_MACRO(Smlal2(v2.V4S(), v3.V8H(), v4.H(), 7),
                "smlal2 v2.4s, v3.8h, v4.h[7]");
  COMPARE_MACRO(Smlal(v0.V2D(), v1.V2S(), v2.S(), 0),
                "smlal v0.2d, v1.2s, v2.s[0]");
  COMPARE_MACRO(Smlal2(v2.V2D(), v3.V4S(), v4.S(), 3),
                "smlal2 v2.2d, v3.4s, v4.s[3]");

  COMPARE_MACRO(Umlal(v0.V4S(), v1.V4H(), v2.H(), 0),
                "umlal v0.4s, v1.4h, v2.h[0]");
  COMPARE_MACRO(Umlal2(v2.V4S(), v3.V8H(), v4.H(), 7),
                "umlal2 v2.4s, v3.8h, v4.h[7]");
  COMPARE_MACRO(Umlal(v0.V2D(), v1.V2S(), v2.S(), 0),
                "umlal v0.2d, v1.2s, v2.s[0]");
  COMPARE_MACRO(Umlal2(v2.V2D(), v3.V4S(), v4.S(), 3),
                "umlal2 v2.2d, v3.4s, v4.s[3]");

  COMPARE_MACRO(Smlsl(v0.V4S(), v1.V4H(), v2.H(), 0),
                "smlsl v0.4s, v1.4h, v2.h[0]");
  COMPARE_MACRO(Smlsl2(v2.V4S(), v3.V8H(), v4.H(), 7),
                "smlsl2 v2.4s, v3.8h, v4.h[7]");
  COMPARE_MACRO(Smlsl(v0.V2D(), v1.V2S(), v2.S(), 0),
                "smlsl v0.2d, v1.2s, v2.s[0]");
  COMPARE_MACRO(Smlsl2(v2.V2D(), v3.V4S(), v4.S(), 3),
                "smlsl2 v2.2d, v3.4s, v4.s[3]");

  COMPARE_MACRO(Umlsl(v0.V4S(), v1.V4H(), v2.H(), 0),
                "umlsl v0.4s, v1.4h, v2.h[0]");
  COMPARE_MACRO(Umlsl2(v2.V4S(), v3.V8H(), v4.H(), 7),
                "umlsl2 v2.4s, v3.8h, v4.h[7]");
  COMPARE_MACRO(Umlsl(v0.V2D(), v1.V2S(), v2.S(), 0),
                "umlsl v0.2d, v1.2s, v2.s[0]");
  COMPARE_MACRO(Umlsl2(v2.V2D(), v3.V4S(), v4.S(), 3),
                "umlsl2 v2.2d, v3.4s, v4.s[3]");

  COMPARE_MACRO(Sqdmull(v0.V4S(), v1.V4H(), v2.H(), 0),
                "sqdmull v0.4s, v1.4h, v2.h[0]");
  COMPARE_MACRO(Sqdmull2(v2.V4S(), v3.V8H(), v4.H(), 7),
                "sqdmull2 v2.4s, v3.8h, v4.h[7]");
  COMPARE_MACRO(Sqdmull(v0.V2D(), v1.V2S(), v2.S(), 0),
                "sqdmull v0.2d, v1.2s, v2.s[0]");
  COMPARE_MACRO(Sqdmull2(v2.V2D(), v3.V4S(), v4.S(), 3),
                "sqdmull2 v2.2d, v3.4s, v4.s[3]");
  COMPARE_MACRO(Sqdmull(s0, h1, v2.H(), 0), "sqdmull s0, h1, v2.h[0]");
  COMPARE_MACRO(Sqdmull(d0, s1, v2.S(), 0), "sqdmull d0, s1, v2.s[0]");

  COMPARE_MACRO(Sqdmlal(v0.V4S(), v1.V4H(), v2.H(), 0),
                "sqdmlal v0.4s, v1.4h, v2.h[0]");
  COMPARE_MACRO(Sqdmlal2(v2.V4S(), v3.V8H(), v4.H(), 7),
                "sqdmlal2 v2.4s, v3.8h, v4.h[7]");
  COMPARE_MACRO(Sqdmlal(v0.V2D(), v1.V2S(), v2.S(), 0),
                "sqdmlal v0.2d, v1.2s, v2.s[0]");
  COMPARE_MACRO(Sqdmlal2(v2.V2D(), v3.V4S(), v4.S(), 3),
                "sqdmlal2 v2.2d, v3.4s, v4.s[3]");
  COMPARE_MACRO(Sqdmlal(s0, h1, v2.H(), 0), "sqdmlal s0, h1, v2.h[0]");
  COMPARE_MACRO(Sqdmlal(d0, s1, v2.S(), 0), "sqdmlal d0, s1, v2.s[0]");

  COMPARE_MACRO(Sqdmlsl(v0.V4S(), v1.V4H(), v2.H(), 0),
                "sqdmlsl v0.4s, v1.4h, v2.h[0]");
  COMPARE_MACRO(Sqdmlsl2(v2.V4S(), v3.V8H(), v4.H(), 7),
                "sqdmlsl2 v2.4s, v3.8h, v4.h[7]");
  COMPARE_MACRO(Sqdmlsl(v0.V2D(), v1.V2S(), v2.S(), 0),
                "sqdmlsl v0.2d, v1.2s, v2.s[0]");
  COMPARE_MACRO(Sqdmlsl2(v2.V2D(), v3.V4S(), v4.S(), 3),
                "sqdmlsl2 v2.2d, v3.4s, v4.s[3]");
  COMPARE_MACRO(Sqdmlsl(s0, h1, v2.H(), 0), "sqdmlsl s0, h1, v2.h[0]");
  COMPARE_MACRO(Sqdmlsl(d0, s1, v2.S(), 0), "sqdmlsl d0, s1, v2.s[0]");

  // FMLAL and so on are special cases in that the {2} variants operate
  // independently from the lane count.
  COMPARE_MACRO(Fmlal(v5.V2S(), v12.V2H(), v6.H(), 7),
                "fmlal v5.2s, v12.2h, v6.h[7]");
  COMPARE_MACRO(Fmlal2(v14.V2S(), v10.V2H(), v9.H(), 6),
                "fmlal2 v14.2s, v10.2h, v9.h[6]");
  COMPARE_MACRO(Fmlsl(v31.V2S(), v9.V2H(), v15.H(), 5),
                "fmlsl v31.2s, v9.2h, v15.h[5]");
  COMPARE_MACRO(Fmlsl2(v21.V2S(), v22.V2H(), v2.H(), 4),
                "fmlsl2 v21.2s, v22.2h, v2.h[4]");
  COMPARE_MACRO(Fmlal(v26.V4S(), v26.V4H(), v0.H(), 3),
                "fmlal v26.4s, v26.4h, v0.h[3]");
  COMPARE_MACRO(Fmlal2(v15.V4S(), v18.V4H(), v15.H(), 2),
                "fmlal2 v15.4s, v18.4h, v15.h[2]");
  COMPARE_MACRO(Fmlsl(v9.V4S(), v4.V4H(), v3.H(), 1),
                "fmlsl v9.4s, v4.4h, v3.h[1]");
  COMPARE_MACRO(Fmlsl2(v28.V4S(), v28.V4H(), v7.H(), 0),
                "fmlsl2 v28.4s, v28.4h, v7.h[0]");

  CLEANUP();
}


TEST(neon_fp_byelement) {
  SETUP();

  COMPARE_MACRO(Fmul(v0.V4H(), v1.V4H(), v2.H(), 0),
                "fmul v0.4h, v1.4h, v2.h[0]");
  COMPARE_MACRO(Fmul(v2.V8H(), v3.V8H(), v15.H(), 3),
                "fmul v2.8h, v3.8h, v15.h[3]");
  COMPARE_MACRO(Fmul(v0.V2S(), v1.V2S(), v2.S(), 0),
                "fmul v0.2s, v1.2s, v2.s[0]");
  COMPARE_MACRO(Fmul(v2.V4S(), v3.V4S(), v15.S(), 3),
                "fmul v2.4s, v3.4s, v15.s[3]");
  COMPARE_MACRO(Fmul(v0.V2D(), v1.V2D(), v2.D(), 0),
                "fmul v0.2d, v1.2d, v2.d[0]");
  COMPARE_MACRO(Fmul(d0, d1, v2.D(), 0), "fmul d0, d1, v2.d[0]");
  COMPARE_MACRO(Fmul(s0, s1, v2.S(), 0), "fmul s0, s1, v2.s[0]");
  COMPARE_MACRO(Fmul(h0, h1, v2.H(), 0), "fmul h0, h1, v2.h[0]");

  COMPARE_MACRO(Fmla(v0.V4H(), v1.V4H(), v2.H(), 0),
                "fmla v0.4h, v1.4h, v2.h[0]");
  COMPARE_MACRO(Fmla(v2.V8H(), v3.V8H(), v15.H(), 3),
                "fmla v2.8h, v3.8h, v15.h[3]");
  COMPARE_MACRO(Fmla(v0.V2S(), v1.V2S(), v2.S(), 0),
                "fmla v0.2s, v1.2s, v2.s[0]");
  COMPARE_MACRO(Fmla(v2.V4S(), v3.V4S(), v15.S(), 3),
                "fmla v2.4s, v3.4s, v15.s[3]");
  COMPARE_MACRO(Fmla(v0.V2D(), v1.V2D(), v2.D(), 0),
                "fmla v0.2d, v1.2d, v2.d[0]");
  COMPARE_MACRO(Fmla(d0, d1, v2.D(), 0), "fmla d0, d1, v2.d[0]");
  COMPARE_MACRO(Fmla(s0, s1, v2.S(), 0), "fmla s0, s1, v2.s[0]");
  COMPARE_MACRO(Fmla(h0, h1, v2.H(), 0), "fmla h0, h1, v2.h[0]");

  COMPARE_MACRO(Fmls(v0.V4H(), v1.V4H(), v2.H(), 0),
                "fmls v0.4h, v1.4h, v2.h[0]");
  COMPARE_MACRO(Fmls(v2.V8H(), v3.V8H(), v15.H(), 3),
                "fmls v2.8h, v3.8h, v15.h[3]");
  COMPARE_MACRO(Fmls(v0.V2S(), v1.V2S(), v2.S(), 0),
                "fmls v0.2s, v1.2s, v2.s[0]");
  COMPARE_MACRO(Fmls(v2.V4S(), v3.V4S(), v15.S(), 3),
                "fmls v2.4s, v3.4s, v15.s[3]");
  COMPARE_MACRO(Fmls(v0.V2D(), v1.V2D(), v2.D(), 0),
                "fmls v0.2d, v1.2d, v2.d[0]");
  COMPARE_MACRO(Fmls(d0, d1, v2.D(), 0), "fmls d0, d1, v2.d[0]");
  COMPARE_MACRO(Fmls(s0, s1, v2.S(), 0), "fmls s0, s1, v2.s[0]");
  COMPARE_MACRO(Fmls(h0, h1, v2.H(), 0), "fmls h0, h1, v2.h[0]");

  COMPARE_MACRO(Fmulx(v0.V4H(), v1.V4H(), v2.H(), 0),
                "fmulx v0.4h, v1.4h, v2.h[0]");
  COMPARE_MACRO(Fmulx(v2.V8H(), v3.V8H(), v15.H(), 3),
                "fmulx v2.8h, v3.8h, v15.h[3]");
  COMPARE_MACRO(Fmulx(v0.V2S(), v1.V2S(), v2.S(), 0),
                "fmulx v0.2s, v1.2s, v2.s[0]");
  COMPARE_MACRO(Fmulx(v2.V4S(), v3.V4S(), v8.S(), 3),
                "fmulx v2.4s, v3.4s, v8.s[3]");
  COMPARE_MACRO(Fmulx(v0.V2D(), v1.V2D(), v2.D(), 0),
                "fmulx v0.2d, v1.2d, v2.d[0]");
  COMPARE_MACRO(Fmulx(d0, d1, v2.D(), 0), "fmulx d0, d1, v2.d[0]");
  COMPARE_MACRO(Fmulx(s0, s1, v2.S(), 0), "fmulx s0, s1, v2.s[0]");
  COMPARE_MACRO(Fmulx(h0, h1, v2.H(), 0), "fmulx h0, h1, v2.h[0]");

  COMPARE_MACRO(Fcmla(v0.V4S(), v1.V4S(), v2.S(), 0, 270),
                "fcmla v0.4s, v1.4s, v2.s[0], #270");
  COMPARE_MACRO(Fcmla(v0.V4S(), v1.V4S(), v2.S(), 1, 180),
                "fcmla v0.4s, v1.4s, v2.s[1], #180");
  COMPARE_MACRO(Fcmla(v0.V4H(), v1.V4H(), v2.H(), 2, 90),
                "fcmla v0.4h, v1.4h, v2.h[2], #90");
  COMPARE_MACRO(Fcmla(v0.V8H(), v1.V8H(), v2.H(), 3, 0),
                "fcmla v0.8h, v1.8h, v2.h[3], #0");

  CLEANUP();
}


TEST(neon_3different) {
  SETUP();

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Uaddl(v0.TA, v1.TB, v2.TB), \
                "uaddl v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Uaddl2(v0.TA, v1.TB, v2.TB), \
                "uaddl2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Uaddw(v0.TA, v1.TA, v2.TB), \
                "uaddw v0." TAS ", v1." TAS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Uaddw2(v0.TA, v1.TA, v2.TB), \
                "uaddw2 v0." TAS ", v1." TAS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Saddl(v0.TA, v1.TB, v2.TB), \
                "saddl v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Saddl2(v0.TA, v1.TB, v2.TB), \
                "saddl2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Saddw(v0.TA, v1.TA, v2.TB), \
                "saddw v0." TAS ", v1." TAS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Saddw2(v0.TA, v1.TA, v2.TB), \
                "saddw2 v0." TAS ", v1." TAS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Usubl(v0.TA, v1.TB, v2.TB), \
                "usubl v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Usubl2(v0.TA, v1.TB, v2.TB), \
                "usubl2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Usubw(v0.TA, v1.TA, v2.TB), \
                "usubw v0." TAS ", v1." TAS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Usubw2(v0.TA, v1.TA, v2.TB), \
                "usubw2 v0." TAS ", v1." TAS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Ssubl(v0.TA, v1.TB, v2.TB), \
                "ssubl v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Ssubl2(v0.TA, v1.TB, v2.TB), \
                "ssubl2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Ssubw(v0.TA, v1.TA, v2.TB), \
                "ssubw v0." TAS ", v1." TAS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Ssubw2(v0.TA, v1.TA, v2.TB), \
                "ssubw2 v0." TAS ", v1." TAS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Sabal(v0.TA, v1.TB, v2.TB), \
                "sabal v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Sabal2(v0.TA, v1.TB, v2.TB), \
                "sabal2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Uabal(v0.TA, v1.TB, v2.TB), \
                "uabal v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Uabal2(v0.TA, v1.TB, v2.TB), \
                "uabal2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Sabdl(v0.TA, v1.TB, v2.TB), \
                "sabdl v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Sabdl2(v0.TA, v1.TB, v2.TB), \
                "sabdl2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Uabdl(v0.TA, v1.TB, v2.TB), \
                "uabdl v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Uabdl2(v0.TA, v1.TB, v2.TB), \
                "uabdl2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Smlal(v0.TA, v1.TB, v2.TB), \
                "smlal v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Smlal2(v0.TA, v1.TB, v2.TB), \
                "smlal2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Umlsl(v0.TA, v1.TB, v2.TB), \
                "umlsl v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Umlsl2(v0.TA, v1.TB, v2.TB), \
                "umlsl2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Smlsl(v0.TA, v1.TB, v2.TB), \
                "smlsl v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Smlsl2(v0.TA, v1.TB, v2.TB), \
                "smlsl2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Umlsl(v0.TA, v1.TB, v2.TB), \
                "umlsl v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Umlsl2(v0.TA, v1.TB, v2.TB), \
                "umlsl2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Smull(v0.TA, v1.TB, v2.TB), \
                "smull v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Smull2(v0.TA, v1.TB, v2.TB), \
                "smull2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)       \
  COMPARE_MACRO(Umull(v0.TA, v1.TB, v2.TB), \
                "umull v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS)        \
  COMPARE_MACRO(Umull2(v0.TA, v1.TB, v2.TB), \
                "umull2 v0." TAS ", v1." TBS ", v2." TBS);
  NEON_FORMAT_LIST_LW2(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Sqdmull(v0.V4S(), v1.V4H(), v2.V4H()),
                "sqdmull v0.4s, v1.4h, v2.4h");
  COMPARE_MACRO(Sqdmull(v1.V2D(), v2.V2S(), v3.V2S()),
                "sqdmull v1.2d, v2.2s, v3.2s");
  COMPARE_MACRO(Sqdmull2(v2.V4S(), v3.V8H(), v4.V8H()),
                "sqdmull2 v2.4s, v3.8h, v4.8h");
  COMPARE_MACRO(Sqdmull2(v3.V2D(), v4.V4S(), v5.V4S()),
                "sqdmull2 v3.2d, v4.4s, v5.4s");
  COMPARE_MACRO(Sqdmull(s0, h1, h2), "sqdmull s0, h1, h2");
  COMPARE_MACRO(Sqdmull(d1, s2, s3), "sqdmull d1, s2, s3");

  COMPARE_MACRO(Sqdmlal(v0.V4S(), v1.V4H(), v2.V4H()),
                "sqdmlal v0.4s, v1.4h, v2.4h");
  COMPARE_MACRO(Sqdmlal(v1.V2D(), v2.V2S(), v3.V2S()),
                "sqdmlal v1.2d, v2.2s, v3.2s");
  COMPARE_MACRO(Sqdmlal2(v2.V4S(), v3.V8H(), v4.V8H()),
                "sqdmlal2 v2.4s, v3.8h, v4.8h");
  COMPARE_MACRO(Sqdmlal2(v3.V2D(), v4.V4S(), v5.V4S()),
                "sqdmlal2 v3.2d, v4.4s, v5.4s");
  COMPARE_MACRO(Sqdmlal(s0, h1, h2), "sqdmlal s0, h1, h2");
  COMPARE_MACRO(Sqdmlal(d1, s2, s3), "sqdmlal d1, s2, s3");

  COMPARE_MACRO(Sqdmlsl(v0.V4S(), v1.V4H(), v2.V4H()),
                "sqdmlsl v0.4s, v1.4h, v2.4h");
  COMPARE_MACRO(Sqdmlsl(v1.V2D(), v2.V2S(), v3.V2S()),
                "sqdmlsl v1.2d, v2.2s, v3.2s");
  COMPARE_MACRO(Sqdmlsl2(v2.V4S(), v3.V8H(), v4.V8H()),
                "sqdmlsl2 v2.4s, v3.8h, v4.8h");
  COMPARE_MACRO(Sqdmlsl2(v3.V2D(), v4.V4S(), v5.V4S()),
                "sqdmlsl2 v3.2d, v4.4s, v5.4s");
  COMPARE_MACRO(Sqdmlsl(s0, h1, h2), "sqdmlsl s0, h1, h2");
  COMPARE_MACRO(Sqdmlsl(d1, s2, s3), "sqdmlsl d1, s2, s3");

  COMPARE_MACRO(Addhn(v0.V8B(), v1.V8H(), v2.V8H()),
                "addhn v0.8b, v1.8h, v2.8h");
  COMPARE_MACRO(Addhn(v1.V4H(), v2.V4S(), v3.V4S()),
                "addhn v1.4h, v2.4s, v3.4s");
  COMPARE_MACRO(Addhn(v2.V2S(), v3.V2D(), v4.V2D()),
                "addhn v2.2s, v3.2d, v4.2d");
  COMPARE_MACRO(Addhn2(v0.V16B(), v1.V8H(), v5.V8H()),
                "addhn2 v0.16b, v1.8h, v5.8h");
  COMPARE_MACRO(Addhn2(v1.V8H(), v2.V4S(), v6.V4S()),
                "addhn2 v1.8h, v2.4s, v6.4s");
  COMPARE_MACRO(Addhn2(v2.V4S(), v3.V2D(), v7.V2D()),
                "addhn2 v2.4s, v3.2d, v7.2d");

  COMPARE_MACRO(Raddhn(v0.V8B(), v1.V8H(), v2.V8H()),
                "raddhn v0.8b, v1.8h, v2.8h");
  COMPARE_MACRO(Raddhn(v1.V4H(), v2.V4S(), v3.V4S()),
                "raddhn v1.4h, v2.4s, v3.4s");
  COMPARE_MACRO(Raddhn(v2.V2S(), v3.V2D(), v4.V2D()),
                "raddhn v2.2s, v3.2d, v4.2d");
  COMPARE_MACRO(Raddhn2(v0.V16B(), v1.V8H(), v5.V8H()),
                "raddhn2 v0.16b, v1.8h, v5.8h");
  COMPARE_MACRO(Raddhn2(v1.V8H(), v2.V4S(), v6.V4S()),
                "raddhn2 v1.8h, v2.4s, v6.4s");
  COMPARE_MACRO(Raddhn2(v2.V4S(), v3.V2D(), v7.V2D()),
                "raddhn2 v2.4s, v3.2d, v7.2d");

  COMPARE_MACRO(Subhn(v1.V4H(), v2.V4S(), v3.V4S()),
                "subhn v1.4h, v2.4s, v3.4s");
  COMPARE_MACRO(Subhn(v2.V2S(), v3.V2D(), v4.V2D()),
                "subhn v2.2s, v3.2d, v4.2d");
  COMPARE_MACRO(Subhn2(v0.V16B(), v1.V8H(), v5.V8H()),
                "subhn2 v0.16b, v1.8h, v5.8h");
  COMPARE_MACRO(Subhn2(v1.V8H(), v2.V4S(), v6.V4S()),
                "subhn2 v1.8h, v2.4s, v6.4s");
  COMPARE_MACRO(Subhn2(v2.V4S(), v3.V2D(), v7.V2D()),
                "subhn2 v2.4s, v3.2d, v7.2d");

  COMPARE_MACRO(Rsubhn(v0.V8B(), v1.V8H(), v2.V8H()),
                "rsubhn v0.8b, v1.8h, v2.8h");
  COMPARE_MACRO(Rsubhn(v1.V4H(), v2.V4S(), v3.V4S()),
                "rsubhn v1.4h, v2.4s, v3.4s");
  COMPARE_MACRO(Rsubhn(v2.V2S(), v3.V2D(), v4.V2D()),
                "rsubhn v2.2s, v3.2d, v4.2d");
  COMPARE_MACRO(Rsubhn2(v0.V16B(), v1.V8H(), v5.V8H()),
                "rsubhn2 v0.16b, v1.8h, v5.8h");
  COMPARE_MACRO(Rsubhn2(v1.V8H(), v2.V4S(), v6.V4S()),
                "rsubhn2 v1.8h, v2.4s, v6.4s");
  COMPARE_MACRO(Rsubhn2(v2.V4S(), v3.V2D(), v7.V2D()),
                "rsubhn2 v2.4s, v3.2d, v7.2d");

  COMPARE_MACRO(Pmull(v0.V8H(), v1.V8B(), v2.V8B()),
                "pmull v0.8h, v1.8b, v2.8b");
  COMPARE_MACRO(Pmull2(v2.V8H(), v3.V16B(), v4.V16B()),
                "pmull2 v2.8h, v3.16b, v4.16b");

  CLEANUP();
}


TEST(neon_perm) {
  SETUP();

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Trn1(v0.M, v1.M, v2.M), "trn1 v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Trn2(v0.M, v1.M, v2.M), "trn2 v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Uzp1(v0.M, v1.M, v2.M), "uzp1 v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Uzp2(v0.M, v1.M, v2.M), "uzp2 v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Zip1(v0.M, v1.M, v2.M), "zip1 v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Zip2(v0.M, v1.M, v2.M), "zip2 v0." S ", v1." S ", v2." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

  CLEANUP();
}


TEST(neon_copy) {
  SETUP();

  COMPARE_MACRO(Ins(v1.V16B(), 4, v5.V16B(), 0), "mov v1.b[4], v5.b[0]");
  COMPARE_MACRO(Ins(v2.V8B(), 5, v6.V8B(), 1), "mov v2.b[5], v6.b[1]");
  COMPARE_MACRO(Ins(v3.B(), 6, v7.B(), 2), "mov v3.b[6], v7.b[2]");
  COMPARE_MACRO(Ins(v4.V8H(), 7, v8.V8H(), 3), "mov v4.h[7], v8.h[3]");
  COMPARE_MACRO(Ins(v5.V4H(), 3, v9.V4H(), 0), "mov v5.h[3], v9.h[0]");
  COMPARE_MACRO(Ins(v6.H(), 6, v1.H(), 1), "mov v6.h[6], v1.h[1]");
  COMPARE_MACRO(Ins(v7.V4S(), 2, v2.V4S(), 2), "mov v7.s[2], v2.s[2]");
  COMPARE_MACRO(Ins(v8.V2S(), 1, v3.V2S(), 0), "mov v8.s[1], v3.s[0]");
  COMPARE_MACRO(Ins(v9.S(), 0, v4.S(), 1), "mov v9.s[0], v4.s[1]");
  COMPARE_MACRO(Ins(v1.V2D(), 1, v5.V2D(), 0), "mov v1.d[1], v5.d[0]");
  COMPARE_MACRO(Ins(v2.D(), 0, v6.D(), 1), "mov v2.d[0], v6.d[1]");

  COMPARE_MACRO(Mov(v3.V16B(), 4, v7.V16B(), 0), "mov v3.b[4], v7.b[0]");
  COMPARE_MACRO(Mov(v4.V8B(), 5, v8.V8B(), 1), "mov v4.b[5], v8.b[1]");
  COMPARE_MACRO(Mov(v5.B(), 6, v9.B(), 2), "mov v5.b[6], v9.b[2]");
  COMPARE_MACRO(Mov(v6.V8H(), 7, v1.V8H(), 3), "mov v6.h[7], v1.h[3]");
  COMPARE_MACRO(Mov(v7.V4H(), 0, v2.V4H(), 0), "mov v7.h[0], v2.h[0]");
  COMPARE_MACRO(Mov(v8.H(), 1, v3.H(), 1), "mov v8.h[1], v3.h[1]");
  COMPARE_MACRO(Mov(v9.V4S(), 2, v4.V4S(), 2), "mov v9.s[2], v4.s[2]");
  COMPARE_MACRO(Mov(v1.V2S(), 3, v5.V2S(), 0), "mov v1.s[3], v5.s[0]");
  COMPARE_MACRO(Mov(v2.S(), 0, v6.S(), 1), "mov v2.s[0], v6.s[1]");
  COMPARE_MACRO(Mov(v3.V2D(), 1, v7.V2D(), 0), "mov v3.d[1], v7.d[0]");
  COMPARE_MACRO(Mov(v4.D(), 0, v8.D(), 1), "mov v4.d[0], v8.d[1]");

  COMPARE_MACRO(Ins(v1.V16B(), 4, w0), "mov v1.b[4], w0");
  COMPARE_MACRO(Ins(v2.V8B(), 5, w1), "mov v2.b[5], w1");
  COMPARE_MACRO(Ins(v3.B(), 6, w2), "mov v3.b[6], w2");
  COMPARE_MACRO(Ins(v4.V8H(), 7, w3), "mov v4.h[7], w3");
  COMPARE_MACRO(Ins(v5.V4H(), 3, w0), "mov v5.h[3], w0");
  COMPARE_MACRO(Ins(v6.H(), 6, w1), "mov v6.h[6], w1");
  COMPARE_MACRO(Ins(v7.V4S(), 2, w2), "mov v7.s[2], w2");
  COMPARE_MACRO(Ins(v8.V2S(), 1, w0), "mov v8.s[1], w0");
  COMPARE_MACRO(Ins(v9.S(), 0, w1), "mov v9.s[0], w1");
  COMPARE_MACRO(Ins(v1.V2D(), 1, x0), "mov v1.d[1], x0");
  COMPARE_MACRO(Ins(v2.D(), 0, x1), "mov v2.d[0], x1");

  COMPARE_MACRO(Mov(v1.V16B(), 4, w0), "mov v1.b[4], w0");
  COMPARE_MACRO(Mov(v2.V8B(), 5, w1), "mov v2.b[5], w1");
  COMPARE_MACRO(Mov(v3.B(), 6, w2), "mov v3.b[6], w2");
  COMPARE_MACRO(Mov(v4.V8H(), 7, w3), "mov v4.h[7], w3");
  COMPARE_MACRO(Mov(v5.V4H(), 3, w0), "mov v5.h[3], w0");
  COMPARE_MACRO(Mov(v6.H(), 6, w1), "mov v6.h[6], w1");
  COMPARE_MACRO(Mov(v7.V4S(), 2, w2), "mov v7.s[2], w2");
  COMPARE_MACRO(Mov(v8.V2S(), 1, w0), "mov v8.s[1], w0");
  COMPARE_MACRO(Mov(v9.S(), 0, w1), "mov v9.s[0], w1");
  COMPARE_MACRO(Mov(v1.V2D(), 1, x0), "mov v1.d[1], x0");
  COMPARE_MACRO(Mov(v2.D(), 0, x1), "mov v2.d[0], x1");

  COMPARE_MACRO(Dup(v5.V8B(), v9.V8B(), 6), "dup v5.8b, v9.b[6]");
  COMPARE_MACRO(Dup(v6.V16B(), v1.V16B(), 5), "dup v6.16b, v1.b[5]");
  COMPARE_MACRO(Dup(v7.V4H(), v2.V4H(), 4), "dup v7.4h, v2.h[4]");
  COMPARE_MACRO(Dup(v8.V8H(), v3.V8H(), 3), "dup v8.8h, v3.h[3]");
  COMPARE_MACRO(Dup(v9.V2S(), v4.V2S(), 2), "dup v9.2s, v4.s[2]");
  COMPARE_MACRO(Dup(v1.V4S(), v5.V4S(), 1), "dup v1.4s, v5.s[1]");
  COMPARE_MACRO(Dup(v2.V2D(), v6.V2D(), 0), "dup v2.2d, v6.d[0]");

  COMPARE_MACRO(Dup(v5.B(), v9.B(), 6), "mov b5, v9.b[6]");
  COMPARE_MACRO(Dup(v7.H(), v2.H(), 4), "mov h7, v2.h[4]");
  COMPARE_MACRO(Dup(v9.S(), v4.S(), 2), "mov s9, v4.s[2]");
  COMPARE_MACRO(Dup(v2.D(), v6.D(), 0), "mov d2, v6.d[0]");

  COMPARE_MACRO(Mov(v5.B(), v9.B(), 6), "mov b5, v9.b[6]");
  COMPARE_MACRO(Mov(v7.H(), v2.H(), 4), "mov h7, v2.h[4]");
  COMPARE_MACRO(Mov(v9.S(), v4.S(), 2), "mov s9, v4.s[2]");
  COMPARE_MACRO(Mov(v2.D(), v6.D(), 0), "mov d2, v6.d[0]");

  COMPARE_MACRO(Mov(v0.B(), v1.V8B(), 7), "mov b0, v1.b[7]");
  COMPARE_MACRO(Mov(b2, v3.V16B(), 15), "mov b2, v3.b[15]");
  COMPARE_MACRO(Mov(v4.H(), v5.V4H(), 3), "mov h4, v5.h[3]");
  COMPARE_MACRO(Mov(h6, v7.V8H(), 7), "mov h6, v7.h[7]");
  COMPARE_MACRO(Mov(v8.S(), v9.V2S(), 1), "mov s8, v9.s[1]");
  COMPARE_MACRO(Mov(s10, v11.V4S(), 3), "mov s10, v11.s[3]");
  COMPARE_MACRO(Mov(v12.D(), v13.V2D(), 1), "mov d12, v13.d[1]");

  COMPARE_MACRO(Dup(v5.V8B(), w0), "dup v5.8b, w0");
  COMPARE_MACRO(Dup(v6.V16B(), w1), "dup v6.16b, w1");
  COMPARE_MACRO(Dup(v7.V4H(), w2), "dup v7.4h, w2");
  COMPARE_MACRO(Dup(v8.V8H(), w3), "dup v8.8h, w3");
  COMPARE_MACRO(Dup(v9.V2S(), w4), "dup v9.2s, w4");
  COMPARE_MACRO(Dup(v1.V4S(), w5), "dup v1.4s, w5");
  COMPARE_MACRO(Dup(v2.V2D(), x6), "dup v2.2d, x6");

  COMPARE_MACRO(Smov(w0, v1.V16B(), 4), "smov w0, v1.b[4]");
  COMPARE_MACRO(Smov(w1, v2.V8B(), 5), "smov w1, v2.b[5]");
  COMPARE_MACRO(Smov(w2, v3.B(), 6), "smov w2, v3.b[6]");
  COMPARE_MACRO(Smov(w3, v4.V8H(), 7), "smov w3, v4.h[7]");
  COMPARE_MACRO(Smov(w0, v5.V4H(), 3), "smov w0, v5.h[3]");
  COMPARE_MACRO(Smov(w1, v6.H(), 6), "smov w1, v6.h[6]");

  COMPARE_MACRO(Smov(x0, v1.V16B(), 4), "smov x0, v1.b[4]");
  COMPARE_MACRO(Smov(x1, v2.V8B(), 5), "smov x1, v2.b[5]");
  COMPARE_MACRO(Smov(x2, v3.B(), 6), "smov x2, v3.b[6]");
  COMPARE_MACRO(Smov(x3, v4.V8H(), 7), "smov x3, v4.h[7]");
  COMPARE_MACRO(Smov(x0, v5.V4H(), 3), "smov x0, v5.h[3]");
  COMPARE_MACRO(Smov(x1, v6.H(), 6), "smov x1, v6.h[6]");
  COMPARE_MACRO(Smov(x2, v7.V4S(), 2), "smov x2, v7.s[2]");
  COMPARE_MACRO(Smov(x0, v8.V2S(), 1), "smov x0, v8.s[1]");
  COMPARE_MACRO(Smov(x1, v9.S(), 0), "smov x1, v9.s[0]");

  COMPARE_MACRO(Umov(w0, v1.V16B(), 4), "umov w0, v1.b[4]");
  COMPARE_MACRO(Umov(w1, v2.V8B(), 5), "umov w1, v2.b[5]");
  COMPARE_MACRO(Umov(w2, v3.B(), 6), "umov w2, v3.b[6]");
  COMPARE_MACRO(Umov(w3, v4.V8H(), 7), "umov w3, v4.h[7]");
  COMPARE_MACRO(Umov(w0, v5.V4H(), 3), "umov w0, v5.h[3]");
  COMPARE_MACRO(Umov(w1, v6.H(), 6), "umov w1, v6.h[6]");
  COMPARE_MACRO(Umov(w2, v7.V4S(), 2), "mov w2, v7.s[2]");
  COMPARE_MACRO(Umov(w0, v8.V2S(), 1), "mov w0, v8.s[1]");
  COMPARE_MACRO(Umov(w1, v9.S(), 0), "mov w1, v9.s[0]");
  COMPARE_MACRO(Umov(x0, v1.V2D(), 1), "mov x0, v1.d[1]");
  COMPARE_MACRO(Umov(x1, v2.D(), 0), "mov x1, v2.d[0]");

  COMPARE_MACRO(Mov(w2, v7.V4S(), 2), "mov w2, v7.s[2]");
  COMPARE_MACRO(Mov(w0, v8.V2S(), 1), "mov w0, v8.s[1]");
  COMPARE_MACRO(Mov(w1, v9.S(), 0), "mov w1, v9.s[0]");
  COMPARE_MACRO(Mov(x0, v1.V2D(), 1), "mov x0, v1.d[1]");
  COMPARE_MACRO(Mov(x1, v2.D(), 0), "mov x1, v2.d[0]");

  CLEANUP();
}


TEST(neon_table) {
  SETUP();

  COMPARE_MACRO(Tbl(v0.V8B(), v1.V16B(), v2.V8B()),
                "tbl v0.8b, {v1.16b}, v2.8b");
  COMPARE_MACRO(Tbl(v3.V8B(), v4.V16B(), v5.V16B(), v6.V8B()),
                "tbl v3.8b, {v4.16b, v5.16b}, v6.8b");
  COMPARE_MACRO(Tbl(v7.V8B(), v8.V16B(), v9.V16B(), v10.V16B(), v11.V8B()),
                "tbl v7.8b, {v8.16b, v9.16b, v10.16b}, v11.8b");
  COMPARE_MACRO(Tbl(v12.V8B(),
                    v13.V16B(),
                    v14.V16B(),
                    v15.V16B(),
                    v16.V16B(),
                    v17.V8B()),
                "tbl v12.8b, {v13.16b, v14.16b, v15.16b, v16.16b}, v17.8b");
  COMPARE_MACRO(Tbl(v18.V16B(), v19.V16B(), v20.V16B()),
                "tbl v18.16b, {v19.16b}, v20.16b");
  COMPARE_MACRO(Tbl(v21.V16B(), v22.V16B(), v23.V16B(), v24.V16B()),
                "tbl v21.16b, {v22.16b, v23.16b}, v24.16b");
  COMPARE_MACRO(Tbl(v25.V16B(), v26.V16B(), v27.V16B(), v28.V16B(), v29.V16B()),
                "tbl v25.16b, {v26.16b, v27.16b, v28.16b}, v29.16b");
  COMPARE_MACRO(Tbl(v30.V16B(),
                    v31.V16B(),
                    v0.V16B(),
                    v1.V16B(),
                    v2.V16B(),
                    v3.V16B()),
                "tbl v30.16b, {v31.16b, v0.16b, v1.16b, v2.16b}, v3.16b");

  COMPARE_MACRO(Tbx(v0.V8B(), v1.V16B(), v2.V8B()),
                "tbx v0.8b, {v1.16b}, v2.8b");
  COMPARE_MACRO(Tbx(v3.V8B(), v4.V16B(), v5.V16B(), v6.V8B()),
                "tbx v3.8b, {v4.16b, v5.16b}, v6.8b");
  COMPARE_MACRO(Tbx(v7.V8B(), v8.V16B(), v9.V16B(), v10.V16B(), v11.V8B()),
                "tbx v7.8b, {v8.16b, v9.16b, v10.16b}, v11.8b");
  COMPARE_MACRO(Tbx(v12.V8B(),
                    v13.V16B(),
                    v14.V16B(),
                    v15.V16B(),
                    v16.V16B(),
                    v17.V8B()),
                "tbx v12.8b, {v13.16b, v14.16b, v15.16b, v16.16b}, v17.8b");
  COMPARE_MACRO(Tbx(v18.V16B(), v19.V16B(), v20.V16B()),
                "tbx v18.16b, {v19.16b}, v20.16b");
  COMPARE_MACRO(Tbx(v21.V16B(), v22.V16B(), v23.V16B(), v24.V16B()),
                "tbx v21.16b, {v22.16b, v23.16b}, v24.16b");
  COMPARE_MACRO(Tbx(v25.V16B(), v26.V16B(), v27.V16B(), v28.V16B(), v29.V16B()),
                "tbx v25.16b, {v26.16b, v27.16b, v28.16b}, v29.16b");
  COMPARE_MACRO(Tbx(v30.V16B(),
                    v31.V16B(),
                    v0.V16B(),
                    v1.V16B(),
                    v2.V16B(),
                    v3.V16B()),
                "tbx v30.16b, {v31.16b, v0.16b, v1.16b, v2.16b}, v3.16b");

  CLEANUP();
}


TEST(neon_extract) {
  SETUP();

  COMPARE_MACRO(Ext(v4.V8B(), v5.V8B(), v6.V8B(), 0),
                "ext v4.8b, v5.8b, v6.8b, #0");
  COMPARE_MACRO(Ext(v1.V8B(), v2.V8B(), v3.V8B(), 7),
                "ext v1.8b, v2.8b, v3.8b, #7");
  COMPARE_MACRO(Ext(v1.V16B(), v2.V16B(), v3.V16B(), 0),
                "ext v1.16b, v2.16b, v3.16b, #0");
  COMPARE_MACRO(Ext(v1.V16B(), v2.V16B(), v3.V16B(), 15),
                "ext v1.16b, v2.16b, v3.16b, #15");

  CLEANUP();
}


TEST(neon_modimm) {
  SETUP();

  COMPARE_MACRO(Orr(v4.V4H(), 0xaa, 0), "orr v4.4h, #0xaa, lsl #0");
  COMPARE_MACRO(Orr(v1.V8H(), 0xcc, 8), "orr v1.8h, #0xcc, lsl #8");
  COMPARE_MACRO(Orr(v4.V2S(), 0xaa, 0), "orr v4.2s, #0xaa, lsl #0");
  COMPARE_MACRO(Orr(v1.V2S(), 0xcc, 8), "orr v1.2s, #0xcc, lsl #8");
  COMPARE_MACRO(Orr(v4.V4S(), 0xaa, 16), "orr v4.4s, #0xaa, lsl #16");
  COMPARE_MACRO(Orr(v1.V4S(), 0xcc, 24), "orr v1.4s, #0xcc, lsl #24");

  COMPARE_MACRO(Bic(v4.V4H(), 0xaa, 0), "bic v4.4h, #0xaa, lsl #0");
  COMPARE_MACRO(Bic(v1.V8H(), 0xcc, 8), "bic v1.8h, #0xcc, lsl #8");
  COMPARE_MACRO(Bic(v4.V2S(), 0xaa, 0), "bic v4.2s, #0xaa, lsl #0");
  COMPARE_MACRO(Bic(v1.V2S(), 0xcc, 8), "bic v1.2s, #0xcc, lsl #8");
  COMPARE_MACRO(Bic(v4.V4S(), 0xaa, 16), "bic v4.4s, #0xaa, lsl #16");
  COMPARE_MACRO(Bic(v1.V4S(), 0xcc, 24), "bic v1.4s, #0xcc, lsl #24");

  COMPARE_MACRO(Mvni(v4.V4H(), 0xaa, LSL, 0), "mvni v4.4h, #0xaa, lsl #0");
  COMPARE_MACRO(Mvni(v1.V8H(), 0xcc, LSL, 8), "mvni v1.8h, #0xcc, lsl #8");
  COMPARE_MACRO(Mvni(v4.V2S(), 0xaa, LSL, 0), "mvni v4.2s, #0xaa, lsl #0");
  COMPARE_MACRO(Mvni(v1.V2S(), 0xcc, LSL, 8), "mvni v1.2s, #0xcc, lsl #8");
  COMPARE_MACRO(Mvni(v4.V4S(), 0xaa, LSL, 16), "mvni v4.4s, #0xaa, lsl #16");
  COMPARE_MACRO(Mvni(v1.V4S(), 0xcc, LSL, 24), "mvni v1.4s, #0xcc, lsl #24");

  COMPARE_MACRO(Mvni(v4.V2S(), 0xaa, MSL, 8), "mvni v4.2s, #0xaa, msl #8");
  COMPARE_MACRO(Mvni(v1.V2S(), 0xcc, MSL, 16), "mvni v1.2s, #0xcc, msl #16");
  COMPARE_MACRO(Mvni(v4.V4S(), 0xaa, MSL, 8), "mvni v4.4s, #0xaa, msl #8");
  COMPARE_MACRO(Mvni(v1.V4S(), 0xcc, MSL, 16), "mvni v1.4s, #0xcc, msl #16");

  COMPARE_MACRO(Movi(v4.V8B(), 0xaa), "movi v4.8b, #0xaa");
  COMPARE_MACRO(Movi(v1.V16B(), 0xcc), "movi v1.16b, #0xcc");

  COMPARE_MACRO(Movi(v4.V4H(), 0xaa, LSL, 0), "movi v4.4h, #0xaa, lsl #0");
  COMPARE_MACRO(Movi(v1.V8H(), 0xcc, LSL, 8), "movi v1.8h, #0xcc, lsl #8");

  COMPARE_MACRO(Movi(v4.V2S(), 0xaa, LSL, 0), "movi v4.2s, #0xaa, lsl #0");
  COMPARE_MACRO(Movi(v1.V2S(), 0xcc, LSL, 8), "movi v1.2s, #0xcc, lsl #8");
  COMPARE_MACRO(Movi(v4.V4S(), 0xaa, LSL, 16), "movi v4.4s, #0xaa, lsl #16");
  COMPARE_MACRO(Movi(v1.V4S(), 0xcc, LSL, 24), "movi v1.4s, #0xcc, lsl #24");

  COMPARE_MACRO(Movi(v4.V2S(), 0xaa, MSL, 8), "movi v4.2s, #0xaa, msl #8");
  COMPARE_MACRO(Movi(v1.V2S(), 0xcc, MSL, 16), "movi v1.2s, #0xcc, msl #16");
  COMPARE_MACRO(Movi(v4.V4S(), 0xaa, MSL, 8), "movi v4.4s, #0xaa, msl #8");
  COMPARE_MACRO(Movi(v1.V4S(), 0xcc, MSL, 16), "movi v1.4s, #0xcc, msl #16");

  COMPARE_MACRO(Movi(d2, 0xffff0000ffffff), "movi d2, #0xffff0000ffffff");
  COMPARE_MACRO(Movi(v1.V2D(), 0xffff0000ffffff),
                "movi v1.2d, #0xffff0000ffffff");

  COMPARE_MACRO(Fmov(v0.V2S(), 1.0f), "fmov v0.2s, #0x70 (1.0000)");
  COMPARE_MACRO(Fmov(v31.V2S(), -13.0f), "fmov v31.2s, #0xaa (-13.0000)");
  COMPARE_MACRO(Fmov(v0.V4S(), 1.0f), "fmov v0.4s, #0x70 (1.0000)");
  COMPARE_MACRO(Fmov(v31.V4S(), -13.0f), "fmov v31.4s, #0xaa (-13.0000)");
  COMPARE_MACRO(Fmov(v1.V2D(), 1.0), "fmov v1.2d, #0x70 (1.0000)");
  COMPARE_MACRO(Fmov(v29.V2D(), -13.0), "fmov v29.2d, #0xaa (-13.0000)");

  COMPARE_MACRO(Fmov(v0.V4H(), Float16(-5.0f)), "fmov v0.4h, #0x94 (-5.0000)");
  COMPARE_MACRO(Fmov(v31.V8H(), Float16(29.0f)),
                "fmov v31.8h, #0x3d (29.0000)");
  COMPARE_MACRO(Fmov(v0.V4H(), Float16(-5.0)), "fmov v0.4h, #0x94 (-5.0000)");
  COMPARE_MACRO(Fmov(v31.V8H(), Float16(29.0)), "fmov v31.8h, #0x3d (29.0000)");

  COMPARE_MACRO(Fmov(v5.D(), 1, x14), "fmov v5.D[1], x14");
  COMPARE_MACRO(Fmov(x14, v5.D(), 1), "fmov x14, v5.D[1]");
  COMPARE_MACRO(Fmov(v3.D(), 0, x21), "mov v3.d[0], x21");
  COMPARE_MACRO(Fmov(x21, v3.D(), 0), "mov x21, v3.d[0]");

  // An unallocated form of fmov.
  COMPARE(dci(0x2f07ffff), "unallocated (Unallocated)");

  CLEANUP();
}


TEST(neon_2regmisc) {
  SETUP();

  COMPARE_MACRO(Shll(v1.V8H(), v8.V8B(), 8), "shll v1.8h, v8.8b, #8");
  COMPARE_MACRO(Shll(v3.V4S(), v1.V4H(), 16), "shll v3.4s, v1.4h, #16");
  COMPARE_MACRO(Shll(v5.V2D(), v3.V2S(), 32), "shll v5.2d, v3.2s, #32");
  COMPARE_MACRO(Shll2(v2.V8H(), v9.V16B(), 8), "shll2 v2.8h, v9.16b, #8");
  COMPARE_MACRO(Shll2(v4.V4S(), v2.V8H(), 16), "shll2 v4.4s, v2.8h, #16");
  COMPARE_MACRO(Shll2(v6.V2D(), v4.V4S(), 32), "shll2 v6.2d, v4.4s, #32");

  // An unallocated form of shll.
  COMPARE(dci(0x2ee13bff), "unallocated (NEON2RegMisc)");
  // An unallocated form of shll2.
  COMPARE(dci(0x6ee13bff), "unallocated (NEON2RegMisc)");

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Cmeq(v0.M, v1.M, 0), "cmeq v0." S ", v1." S ", #0");
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Cmge(v0.M, v1.M, 0), "cmge v0." S ", v1." S ", #0");
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Cmgt(v0.M, v1.M, 0), "cmgt v0." S ", v1." S ", #0");
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Cmle(v0.M, v1.M, 0), "cmle v0." S ", v1." S ", #0");
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Cmlt(v0.M, v1.M, 0), "cmlt v0." S ", v1." S ", #0");
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Cmeq(v0.D(), v1.D(), 0), "cmeq d0, d1, #0");
  COMPARE_MACRO(Cmge(v3.D(), v4.D(), 0), "cmge d3, d4, #0");
  COMPARE_MACRO(Cmgt(v6.D(), v7.D(), 0), "cmgt d6, d7, #0");
  COMPARE_MACRO(Cmle(v0.D(), v1.D(), 0), "cmle d0, d1, #0");
  COMPARE_MACRO(Cmlt(v3.D(), v4.D(), 0), "cmlt d3, d4, #0");

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fcmeq(v0.M, v1.M, 0), "fcmeq v0." S ", v1." S ", #0.0");
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Fcmeq(v0.S(), v1.S(), 0),
                "fcmeq s0, s1, "
                "#0.0");
  COMPARE_MACRO(Fcmeq(v0.D(), v1.D(), 0),
                "fcmeq d0, d1, "
                "#0.0");

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fcmge(v0.M, v1.M, 0), "fcmge v0." S ", v1." S ", #0.0");
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Fcmge(v0.S(), v1.S(), 0),
                "fcmge s0, s1, "
                "#0.0");
  COMPARE_MACRO(Fcmge(v0.D(), v1.D(), 0),
                "fcmge d0, d1, "
                "#0.0");

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fcmgt(v0.M, v1.M, 0), "fcmgt v0." S ", v1." S ", #0.0");
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Fcmgt(v0.S(), v1.S(), 0),
                "fcmgt s0, s1, "
                "#0.0");
  COMPARE_MACRO(Fcmgt(v0.D(), v1.D(), 0),
                "fcmgt d0, d1, "
                "#0.0");

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fcmle(v0.M, v1.M, 0), "fcmle v0." S ", v1." S ", #0.0");
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Fcmle(v0.S(), v1.S(), 0),
                "fcmle s0, s1, "
                "#0.0");
  COMPARE_MACRO(Fcmle(v0.D(), v1.D(), 0),
                "fcmle d0, d1, "
                "#0.0");

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Fcmlt(v0.M, v1.M, 0), "fcmlt v0." S ", v1." S ", #0.0");
  NEON_FORMAT_LIST_FP(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Fcmlt(v0.S(), v1.S(), 0),
                "fcmlt s0, s1, "
                "#0.0");
  COMPARE_MACRO(Fcmlt(v0.D(), v1.D(), 0),
                "fcmlt d0, d1, "
                "#0.0");

#define DISASM_INST(M, S) COMPARE_MACRO(Neg(v0.M, v1.M), "neg v0." S ", v1." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Neg(v0.D(), v1.D()), "neg d0, d1");

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Sqneg(v0.M, v1.M), "sqneg v0." S ", v1." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Sqneg(b0, b1), "sqneg b0, b1");
  COMPARE_MACRO(Sqneg(h1, h2), "sqneg h1, h2");
  COMPARE_MACRO(Sqneg(s2, s3), "sqneg s2, s3");
  COMPARE_MACRO(Sqneg(d3, d4), "sqneg d3, d4");

#define DISASM_INST(M, S) COMPARE_MACRO(Abs(v0.M, v1.M), "abs v0." S ", v1." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Abs(v0.D(), v1.D()), "abs d0, d1");

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Sqabs(v0.M, v1.M), "sqabs v0." S ", v1." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Sqabs(b0, b1), "sqabs b0, b1");
  COMPARE_MACRO(Sqabs(h1, h2), "sqabs h1, h2");
  COMPARE_MACRO(Sqabs(s2, s3), "sqabs s2, s3");
  COMPARE_MACRO(Sqabs(d3, d4), "sqabs d3, d4");

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Suqadd(v0.M, v1.M), "suqadd v0." S ", v1." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Suqadd(b0, b1), "suqadd b0, b1");
  COMPARE_MACRO(Suqadd(h1, h2), "suqadd h1, h2");
  COMPARE_MACRO(Suqadd(s2, s3), "suqadd s2, s3");
  COMPARE_MACRO(Suqadd(d3, d4), "suqadd d3, d4");

#define DISASM_INST(M, S) \
  COMPARE_MACRO(Usqadd(v0.M, v1.M), "usqadd v0." S ", v1." S);
  NEON_FORMAT_LIST(DISASM_INST)
#undef DISASM_INST

  COMPARE_MACRO(Usqadd(b0, b1), "usqadd b0, b1");
  COMPARE_MACRO(Usqadd(h1, h2), "usqadd h1, h2");
  COMPARE_MACRO(Usqadd(s2, s3), "usqadd s2, s3");
  COMPARE_MACRO(Usqadd(d3, d4), "usqadd d3, d4");

  COMPARE_MACRO(Xtn(v0.V8B(), v1.V8H()),
                "xtn v0.8b, "
                "v1.8h");
  COMPARE_MACRO(Xtn(v1.V4H(), v2.V4S()),
                "xtn v1.4h, "
                "v2.4s");
  COMPARE_MACRO(Xtn(v2.V2S(), v3.V2D()),
                "xtn v2.2s, "
                "v3.2d");
  COMPARE_MACRO(Xtn2(v0.V16B(), v1.V8H()),
                "xtn2 v0.16b, "
                "v1.8h");
  COMPARE_MACRO(Xtn2(v1.V8H(), v2.V4S()),
                "xtn2 v1.8h, "
                "v2.4s");
  COMPARE_MACRO(Xtn2(v2.V4S(), v3.V2D()),
                "xtn2 v2.4s, "
                "v3.2d");

  COMPARE_MACRO(Sqxtn(v0.V8B(), v1.V8H()),
                "sqxtn v0.8b, "
                "v1.8h");
  COMPARE_MACRO(Sqxtn(v1.V4H(), v2.V4S()),
                "sqxtn v1.4h, "
                "v2.4s");
  COMPARE_MACRO(Sqxtn(v2.V2S(), v3.V2D()),
                "sqxtn v2.2s, "
                "v3.2d");
  COMPARE_MACRO(Sqxtn2(v0.V16B(), v1.V8H()),
                "sqxtn2 v0.16b, "
                "v1.8h");
  COMPARE_MACRO(Sqxtn2(v1.V8H(), v2.V4S()),
                "sqxtn2 v1.8h, "
                "v2.4s");
  COMPARE_MACRO(Sqxtn2(v2.V4S(), v3.V2D()),
                "sqxtn2 v2.4s, "
                "v3.2d");
  COMPARE_MACRO(Sqxtn(b19, h0), "sqxtn b19, h0");
  COMPARE_MACRO(Sqxtn(h20, s0), "sqxtn h20, s0");
  COMPARE_MACRO(Sqxtn(s21, d0), "sqxtn s21, d0");

  COMPARE_MACRO(Uqxtn(v0.V8B(), v1.V8H()),
                "uqxtn v0.8b, "
                "v1.8h");
  COMPARE_MACRO(Uqxtn(v1.V4H(), v2.V4S()),
                "uqxtn v1.4h, "
                "v2.4s");
  COMPARE_MACRO(Uqxtn(v2.V2S(), v3.V2D()),
                "uqxtn v2.2s, "
                "v3.2d");
  COMPARE_MACRO(Uqxtn2(v0.V16B(), v1.V8H()),
                "uqxtn2 v0.16b, "
                "v1.8h");
  COMPARE_MACRO(Uqxtn2(v1.V8H(), v2.V4S()),
                "uqxtn2 v1.8h, "
                "v2.4s");
  COMPARE_MACRO(Uqxtn2(v2.V4S(), v3.V2D()),
                "uqxtn2 v2.4s, "
                "v3.2d");
  COMPARE_MACRO(Uqxtn(b19, h0), "uqxtn b19, h0");
  COMPARE_MACRO(Uqxtn(h20, s0), "uqxtn h20, s0");
  COMPARE_MACRO(Uqxtn(s21, d0), "uqxtn s21, d0");

  COMPARE_MACRO(Sqxtun(v0.V8B(), v1.V8H()),
                "sqxtun v0.8b, "
                "v1.8h");
  COMPARE_MACRO(Sqxtun(v1.V4H(), v2.V4S()),
                "sqxtun v1.4h, "
                "v2.4s");
  COMPARE_MACRO(Sqxtun(v2.V2S(), v3.V2D()),
                "sqxtun v2.2s, "
                "v3.2d");
  COMPARE_MACRO(Sqxtun2(v0.V16B(), v1.V8H()),
                "sqxtun2 v0.16b, "
                "v1.8h");
  COMPARE_MACRO(Sqxtun2(v1.V8H(), v2.V4S()),
                "sqxtun2 v1.8h, "
                "v2.4s");
  COMPARE_MACRO(Sqxtun2(v2.V4S(), v3.V2D()),
                "sqxtun2 v2.4s, "
                "v3.2d");
  COMPARE_MACRO(Sqxtun(b19, h0), "sqxtun b19, h0");
  COMPARE_MACRO(Sqxtun(h20, s0), "sqxtun h20, s0");
  COMPARE_MACRO(Sqxtun(s21, d0), "sqxtun s21, d0");

  COMPARE_MACRO(Cls(v1.V8B(), v8.V8B()),
                "cls v1.8b, "
                "v8.8b");
  COMPARE_MACRO(Cls(v2.V16B(), v9.V16B()),
                "cls v2.16b, "
                "v9.16b");
  COMPARE_MACRO(Cls(v3.V4H(), v1.V4H()),
                "cls v3.4h, "
                "v1.4h");
  COMPARE_MACRO(Cls(v4.V8H(), v2.V8H()),
                "cls v4.8h, "
                "v2.8h");
  COMPARE_MACRO(Cls(v5.V2S(), v3.V2S()),
                "cls v5.2s, "
                "v3.2s");
  COMPARE_MACRO(Cls(v6.V4S(), v4.V4S()),
                "cls v6.4s, "
                "v4.4s");

  COMPARE_MACRO(Clz(v1.V8B(), v8.V8B()),
                "clz v1.8b, "
                "v8.8b");
  COMPARE_MACRO(Clz(v2.V16B(), v9.V16B()),
                "clz v2.16b, "
                "v9.16b");
  COMPARE_MACRO(Clz(v3.V4H(), v1.V4H()),
                "clz v3.4h, "
                "v1.4h");
  COMPARE_MACRO(Clz(v4.V8H(), v2.V8H()),
                "clz v4.8h, "
                "v2.8h");
  COMPARE_MACRO(Clz(v5.V2S(), v3.V2S()),
                "clz v5.2s, "
                "v3.2s");
  COMPARE_MACRO(Clz(v6.V4S(), v4.V4S()),
                "clz v6.4s, "
                "v4.4s");

  COMPARE_MACRO(Cnt(v1.V8B(), v8.V8B()),
                "cnt v1.8b, "
                "v8.8b");
  COMPARE_MACRO(Cnt(v2.V16B(), v9.V16B()),
                "cnt v2.16b, "
                "v9.16b");

  COMPARE_MACRO(Mvn(v4.V8B(), v5.V8B()),
                "mvn v4.8b, "
                "v5.8b");
  COMPARE_MACRO(Mvn(v4.V16B(), v5.V16B()),
                "mvn v4.16b, "
                "v5.16b");

  COMPARE_MACRO(Not(v4.V8B(), v5.V8B()),
                "mvn v4.8b, "
                "v5.8b");
  COMPARE_MACRO(Not(v4.V16B(), v5.V16B()),
                "mvn v4.16b, "
                "v5.16b");

  COMPARE_MACRO(Rev64(v1.V8B(), v8.V8B()),
                "rev64 v1.8b, "
                "v8.8b");
  COMPARE_MACRO(Rev64(v2.V16B(), v9.V16B()),
                "rev64 v2.16b, "
                "v9.16b");
  COMPARE_MACRO(Rev64(v3.V4H(), v1.V4H()),
                "rev64 v3.4h, "
                "v1.4h");
  COMPARE_MACRO(Rev64(v4.V8H(), v2.V8H()),
                "rev64 v4.8h, "
                "v2.8h");
  COMPARE_MACRO(Rev64(v5.V2S(), v3.V2S()),
                "rev64 v5.2s, "
                "v3.2s");
  COMPARE_MACRO(Rev64(v6.V4S(), v4.V4S()),
                "rev64 v6.4s, "
                "v4.4s");

  COMPARE_MACRO(Rev32(v1.V8B(), v8.V8B()),
                "rev32 v1.8b, "
                "v8.8b");
  COMPARE_MACRO(Rev32(v2.V16B(), v9.V16B()),
                "rev32 v2.16b, "
                "v9.16b");
  COMPARE_MACRO(Rev32(v3.V4H(), v1.V4H()),
                "rev32 v3.4h, "
                "v1.4h");
  COMPARE_MACRO(Rev32(v4.V8H(), v2.V8H()),
                "rev32 v4.8h, "
                "v2.8h");

  COMPARE_MACRO(Rev16(v1.V8B(), v8.V8B()),
                "rev16 v1.8b, "
                "v8.8b");
  COMPARE_MACRO(Rev16(v2.V16B(), v9.V16B()),
                "rev16 v2.16b, "
                "v9.16b");

  COMPARE_MACRO(Rbit(v1.V8B(), v8.V8B()),
                "rbit v1.8b, "
                "v8.8b");
  COMPARE_MACRO(Rbit(v2.V16B(), v9.V16B()),
                "rbit v2.16b, "
                "v9.16b");

  COMPARE_MACRO(Ursqrte(v2.V2S(), v9.V2S()),
                "ursqrte v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Ursqrte(v16.V4S(), v23.V4S()),
                "ursqrte v16.4s, "
                "v23.4s");

  COMPARE_MACRO(Urecpe(v2.V2S(), v9.V2S()),
                "urecpe v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Urecpe(v16.V4S(), v23.V4S()),
                "urecpe v16.4s, "
                "v23.4s");

  COMPARE_MACRO(Frsqrte(v2.V2S(), v9.V2S()),
                "frsqrte v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Frsqrte(v16.V4S(), v23.V4S()),
                "frsqrte v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Frsqrte(v2.V2D(), v9.V2D()),
                "frsqrte v2.2d, "
                "v9.2d");
  COMPARE_MACRO(Frsqrte(v0.S(), v1.S()), "frsqrte s0, s1");
  COMPARE_MACRO(Frsqrte(v0.D(), v1.D()), "frsqrte d0, d1");

  COMPARE_MACRO(Frecpe(v2.V2S(), v9.V2S()),
                "frecpe v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Frecpe(v16.V4S(), v23.V4S()),
                "frecpe v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Frecpe(v2.V2D(), v9.V2D()),
                "frecpe v2.2d, "
                "v9.2d");
  COMPARE_MACRO(Frecpe(v0.S(), v1.S()), "frecpe s0, s1");
  COMPARE_MACRO(Frecpe(v0.D(), v1.D()), "frecpe d0, d1");

  COMPARE_MACRO(Fabs(v2.V2S(), v9.V2S()),
                "fabs v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Fabs(v16.V4S(), v23.V4S()),
                "fabs v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Fabs(v31.V2D(), v30.V2D()),
                "fabs v31.2d, "
                "v30.2d");

  COMPARE_MACRO(Fneg(v2.V2S(), v9.V2S()),
                "fneg v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Fneg(v16.V4S(), v23.V4S()),
                "fneg v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Fneg(v31.V2D(), v30.V2D()),
                "fneg v31.2d, "
                "v30.2d");

  COMPARE_MACRO(Frint32x(v2.V2S(), v9.V2S()),
                "frint32x v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Frint32x(v16.V4S(), v23.V4S()),
                "frint32x v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Frint32x(v31.V2D(), v30.V2D()),
                "frint32x v31.2d, "
                "v30.2d");

  COMPARE_MACRO(Frint32z(v2.V2S(), v9.V2S()),
                "frint32z v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Frint32z(v16.V4S(), v23.V4S()),
                "frint32z v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Frint32z(v31.V2D(), v30.V2D()),
                "frint32z v31.2d, "
                "v30.2d");

  COMPARE_MACRO(Frint64x(v3.V2S(), v10.V2S()),
                "frint64x v3.2s, "
                "v10.2s");
  COMPARE_MACRO(Frint64x(v17.V4S(), v24.V4S()),
                "frint64x v17.4s, "
                "v24.4s");
  COMPARE_MACRO(Frint64x(v30.V2D(), v29.V2D()),
                "frint64x v30.2d, "
                "v29.2d");

  COMPARE_MACRO(Frint64z(v4.V2S(), v11.V2S()),
                "frint64z v4.2s, "
                "v11.2s");
  COMPARE_MACRO(Frint64z(v18.V4S(), v25.V4S()),
                "frint64z v18.4s, "
                "v25.4s");
  COMPARE_MACRO(Frint64z(v29.V2D(), v28.V2D()),
                "frint64z v29.2d, "
                "v28.2d");

  COMPARE_MACRO(Frintn(v2.V2S(), v9.V2S()),
                "frintn v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Frintn(v16.V4S(), v23.V4S()),
                "frintn v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Frintn(v31.V2D(), v30.V2D()),
                "frintn v31.2d, "
                "v30.2d");

  COMPARE_MACRO(Frinta(v2.V2S(), v9.V2S()),
                "frinta v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Frinta(v16.V4S(), v23.V4S()),
                "frinta v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Frinta(v31.V2D(), v30.V2D()),
                "frinta v31.2d, "
                "v30.2d");

  COMPARE_MACRO(Frintp(v2.V2S(), v9.V2S()),
                "frintp v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Frintp(v16.V4S(), v23.V4S()),
                "frintp v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Frintp(v31.V2D(), v30.V2D()),
                "frintp v31.2d, "
                "v30.2d");

  COMPARE_MACRO(Frintm(v2.V2S(), v9.V2S()),
                "frintm v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Frintm(v16.V4S(), v23.V4S()),
                "frintm v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Frintm(v31.V2D(), v30.V2D()),
                "frintm v31.2d, "
                "v30.2d");

  COMPARE_MACRO(Frintx(v2.V2S(), v9.V2S()),
                "frintx v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Frintx(v16.V4S(), v23.V4S()),
                "frintx v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Frintx(v31.V2D(), v30.V2D()),
                "frintx v31.2d, "
                "v30.2d");

  COMPARE_MACRO(Frintz(v2.V2S(), v9.V2S()),
                "frintz v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Frintz(v16.V4S(), v23.V4S()),
                "frintz v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Frintz(v31.V2D(), v30.V2D()),
                "frintz v31.2d, "
                "v30.2d");

  COMPARE_MACRO(Frinti(v2.V2S(), v9.V2S()),
                "frinti v2.2s, "
                "v9.2s");
  COMPARE_MACRO(Frinti(v16.V4S(), v23.V4S()),
                "frinti v16.4s, "
                "v23.4s");
  COMPARE_MACRO(Frinti(v31.V2D(), v30.V2D()),
                "frinti v31.2d, "
                "v30.2d");

  COMPARE_MACRO(Fsqrt(v3.V2S(), v10.V2S()),
                "fsqrt v3.2s, "
                "v10.2s");
  COMPARE_MACRO(Fsqrt(v22.V4S(), v11.V4S()),
                "fsqrt v22.4s, "
                "v11.4s");
  COMPARE_MACRO(Fsqrt(v31.V2D(), v0.V2D()),
                "fsqrt v31.2d, "
                "v0.2d");

  COMPARE_MACRO(Fcvtns(v4.V2S(), v11.V2S()),
                "fcvtns v4.2s, "
                "v11.2s");
  COMPARE_MACRO(Fcvtns(v23.V4S(), v12.V4S()),
                "fcvtns v23.4s, "
                "v12.4s");
  COMPARE_MACRO(Fcvtns(v30.V2D(), v1.V2D()),
                "fcvtns v30.2d, "
                "v1.2d");
  COMPARE_MACRO(Fcvtnu(v4.V2S(), v11.V2S()),
                "fcvtnu v4.2s, "
                "v11.2s");
  COMPARE_MACRO(Fcvtnu(v23.V4S(), v12.V4S()),
                "fcvtnu v23.4s, "
                "v12.4s");
  COMPARE_MACRO(Fcvtnu(v30.V2D(), v1.V2D()),
                "fcvtnu v30.2d, "
                "v1.2d");

  COMPARE_MACRO(Fcvtps(v4.V2S(), v11.V2S()),
                "fcvtps v4.2s, "
                "v11.2s");
  COMPARE_MACRO(Fcvtps(v23.V4S(), v12.V4S()),
                "fcvtps v23.4s, "
                "v12.4s");
  COMPARE_MACRO(Fcvtps(v30.V2D(), v1.V2D()),
                "fcvtps v30.2d, "
                "v1.2d");
  COMPARE_MACRO(Fcvtpu(v4.V2S(), v11.V2S()),
                "fcvtpu v4.2s, "
                "v11.2s");
  COMPARE_MACRO(Fcvtpu(v23.V4S(), v12.V4S()),
                "fcvtpu v23.4s, "
                "v12.4s");
  COMPARE_MACRO(Fcvtpu(v30.V2D(), v1.V2D()),
                "fcvtpu v30.2d, "
                "v1.2d");

  COMPARE_MACRO(Fcvtms(v4.V2S(), v11.V2S()),
                "fcvtms v4.2s, "
                "v11.2s");
  COMPARE_MACRO(Fcvtms(v23.V4S(), v12.V4S()),
                "fcvtms v23.4s, "
                "v12.4s");
  COMPARE_MACRO(Fcvtms(v30.V2D(), v1.V2D()),
                "fcvtms v30.2d, "
                "v1.2d");
  COMPARE_MACRO(Fcvtmu(v4.V2S(), v11.V2S()),
                "fcvtmu v4.2s, "
                "v11.2s");
  COMPARE_MACRO(Fcvtmu(v23.V4S(), v12.V4S()),
                "fcvtmu v23.4s, "
                "v12.4s");
  COMPARE_MACRO(Fcvtmu(v30.V2D(), v1.V2D()),
                "fcvtmu v30.2d, "
                "v1.2d");

  COMPARE_MACRO(Fcvtzs(v4.V2S(), v11.V2S()),
                "fcvtzs v4.2s, "
                "v11.2s");
  COMPARE_MACRO(Fcvtzs(v23.V4S(), v12.V4S()),
                "fcvtzs v23.4s, "
                "v12.4s");
  COMPARE_MACRO(Fcvtzs(v30.V2D(), v1.V2D()),
                "fcvtzs v30.2d, "
                "v1.2d");
  COMPARE_MACRO(Fcvtzu(v4.V2S(), v11.V2S()),
                "fcvtzu v4.2s, "
                "v11.2s");
  COMPARE_MACRO(Fcvtzu(v23.V4S(), v12.V4S()),
                "fcvtzu v23.4s, "
                "v12.4s");
  COMPARE_MACRO(Fcvtzu(v30.V2D(), v1.V2D()),
                "fcvtzu v30.2d, "
                "v1.2d");

  COMPARE_MACRO(Fcvtas(v4.V2S(), v11.V2S()),
                "fcvtas v4.2s, "
                "v11.2s");
  COMPARE_MACRO(Fcvtas(v23.V4S(), v12.V4S()),
                "fcvtas v23.4s, "
                "v12.4s");
  COMPARE_MACRO(Fcvtas(v30.V2D(), v1.V2D()),
                "fcvtas v30.2d, "
                "v1.2d");
  COMPARE_MACRO(Fcvtau(v4.V2S(), v11.V2S()),
                "fcvtau v4.2s, "
                "v11.2s");
  COMPARE_MACRO(Fcvtau(v23.V4S(), v12.V4S()),
                "fcvtau v23.4s, "
                "v12.4s");
  COMPARE_MACRO(Fcvtau(v30.V2D(), v1.V2D()),
                "fcvtau v30.2d, "
                "v1.2d");

  COMPARE_MACRO(Fcvtns(s0, s1), "fcvtns s0, s1");
  COMPARE_MACRO(Fcvtns(d2, d3), "fcvtns d2, d3");
  COMPARE_MACRO(Fcvtnu(s4, s5), "fcvtnu s4, s5");
  COMPARE_MACRO(Fcvtnu(d6, d7), "fcvtnu d6, d7");
  COMPARE_MACRO(Fcvtps(s8, s9), "fcvtps s8, s9");
  COMPARE_MACRO(Fcvtps(d10, d11), "fcvtps d10, d11");
  COMPARE_MACRO(Fcvtpu(s12, s13), "fcvtpu s12, s13");
  COMPARE_MACRO(Fcvtpu(d14, d15), "fcvtpu d14, d15");
  COMPARE_MACRO(Fcvtms(s16, s17), "fcvtms s16, s17");
  COMPARE_MACRO(Fcvtms(d18, d19), "fcvtms d18, d19");
  COMPARE_MACRO(Fcvtmu(s20, s21), "fcvtmu s20, s21");
  COMPARE_MACRO(Fcvtmu(d22, d23), "fcvtmu d22, d23");
  COMPARE_MACRO(Fcvtzs(s24, s25), "fcvtzs s24, s25");
  COMPARE_MACRO(Fcvtzs(d26, d27), "fcvtzs d26, d27");
  COMPARE_MACRO(Fcvtzu(s28, s29), "fcvtzu s28, s29");
  COMPARE_MACRO(Fcvtzu(d30, d31), "fcvtzu d30, d31");
  COMPARE_MACRO(Fcvtas(s0, s1), "fcvtas s0, s1");
  COMPARE_MACRO(Fcvtas(d2, d3), "fcvtas d2, d3");
  COMPARE_MACRO(Fcvtau(s4, s5), "fcvtau s4, s5");
  COMPARE_MACRO(Fcvtau(d6, d7), "fcvtau d6, d7");

  COMPARE_MACRO(Fcvtl(v3.V4S(), v5.V4H()),
                "fcvtl v3.4s, "
                "v5.4h");
  COMPARE_MACRO(Fcvtl(v7.V2D(), v11.V2S()),
                "fcvtl v7.2d, "
                "v11.2s");
  COMPARE_MACRO(Fcvtl2(v13.V4S(), v17.V8H()),
                "fcvtl2 v13.4s, "
                "v17.8h");
  COMPARE_MACRO(Fcvtl2(v23.V2D(), v29.V4S()),
                "fcvtl2 v23.2d, "
                "v29.4s");

  COMPARE_MACRO(Fcvtn(v3.V4H(), v5.V4S()),
                "fcvtn v3.4h, "
                "v5.4s");
  COMPARE_MACRO(Fcvtn(v7.V2S(), v11.V2D()),
                "fcvtn v7.2s, "
                "v11.2d");
  COMPARE_MACRO(Fcvtn2(v13.V8H(), v17.V4S()),
                "fcvtn2 v13.8h, "
                "v17.4s");
  COMPARE_MACRO(Fcvtn2(v23.V4S(), v29.V2D()),
                "fcvtn2 v23.4s, "
                "v29.2d");

  COMPARE_MACRO(Fcvtxn(v5.V2S(), v7.V2D()),
                "fcvtxn v5.2s, "
                "v7.2d");
  COMPARE_MACRO(Fcvtxn2(v8.V4S(), v13.V2D()),
                "fcvtxn2 v8.4s, "
                "v13.2d");
  COMPARE_MACRO(Fcvtxn(s17, d31), "fcvtxn s17, d31");

  COMPARE_MACRO(Frecpx(s0, s1), "frecpx s0, s1");
  COMPARE_MACRO(Frecpx(s31, s30), "frecpx s31, s30");
  COMPARE_MACRO(Frecpx(d2, d3), "frecpx d2, d3");
  COMPARE_MACRO(Frecpx(d31, d30), "frecpx d31, d30");

  COMPARE_MACRO(Scvtf(v5.V2S(), v3.V2S()),
                "scvtf v5.2s, "
                "v3.2s");
  COMPARE_MACRO(Scvtf(v6.V4S(), v4.V4S()),
                "scvtf v6.4s, "
                "v4.4s");
  COMPARE_MACRO(Scvtf(v7.V2D(), v5.V2D()),
                "scvtf v7.2d, "
                "v5.2d");
  COMPARE_MACRO(Scvtf(s8, s6), "scvtf s8, s6");
  COMPARE_MACRO(Scvtf(d8, d6), "scvtf d8, d6");

  COMPARE_MACRO(Ucvtf(v5.V2S(), v3.V2S()),
                "ucvtf v5.2s, "
                "v3.2s");
  COMPARE_MACRO(Ucvtf(v6.V4S(), v4.V4S()),
                "ucvtf v6.4s, "
                "v4.4s");
  COMPARE_MACRO(Ucvtf(v7.V2D(), v5.V2D()),
                "ucvtf v7.2d, "
                "v5.2d");
  COMPARE_MACRO(Ucvtf(s8, s6), "ucvtf s8, s6");
  COMPARE_MACRO(Ucvtf(d8, d6), "ucvtf d8, d6");

#define DISASM_INST(TA, TAS, TB, TBS) \
  COMPARE_MACRO(Saddlp(v0.TA, v1.TB), "saddlp v0." TAS ", v1." TBS);
  NEON_FORMAT_LIST_LP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS) \
  COMPARE_MACRO(Uaddlp(v0.TA, v1.TB), "uaddlp v0." TAS ", v1." TBS);
  NEON_FORMAT_LIST_LP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS) \
  COMPARE_MACRO(Sadalp(v0.TA, v1.TB), "sadalp v0." TAS ", v1." TBS);
  NEON_FORMAT_LIST_LP(DISASM_INST)
#undef DISASM_INST

#define DISASM_INST(TA, TAS, TB, TBS) \
  COMPARE_MACRO(Uadalp(v0.TA, v1.TB), "uadalp v0." TAS ", v1." TBS);
  NEON_FORMAT_LIST_LP(DISASM_INST)
#undef DISASM_INST

  CLEANUP();
}

#define COMPARE_2REGMISC_FP16(A, B)                            \
  COMPARE_MACRO(A(v2.V4H(), v9.V4H()), B " v2.4h, v9.4h");     \
  COMPARE_MACRO(A(v16.V8H(), v23.V8H()), B " v16.8h, v23.8h"); \
  COMPARE_MACRO(A(h5, h17), B " h5, h17")

#define COMPARE_2REGMISC_CMP_FP16(A, B)                                 \
  COMPARE_MACRO(A(v2.V4H(), v9.V4H(), 0), B " v2.4h, v9.4h, #0.0");     \
  COMPARE_MACRO(A(v16.V8H(), v23.V8H(), 0), B " v16.8h, v23.8h, #0.0"); \
  COMPARE_MACRO(A(h5, h17, 0), B " h5, h17, #0.0")

TEST(neon_2regmisc_fp16) {
  SETUP();

  COMPARE_2REGMISC_FP16(Frintn, "frintn");
  COMPARE_2REGMISC_FP16(Frinta, "frinta");
  COMPARE_2REGMISC_FP16(Frintp, "frintp");
  COMPARE_2REGMISC_FP16(Frintm, "frintm");
  COMPARE_2REGMISC_FP16(Frintx, "frintx");
  COMPARE_2REGMISC_FP16(Frintz, "frintz");
  COMPARE_2REGMISC_FP16(Frinti, "frinti");

  COMPARE_2REGMISC_FP16(Fcvtns, "fcvtns");
  COMPARE_2REGMISC_FP16(Fcvtnu, "fcvtnu");
  COMPARE_2REGMISC_FP16(Fcvtps, "fcvtps");
  COMPARE_2REGMISC_FP16(Fcvtpu, "fcvtpu");
  COMPARE_2REGMISC_FP16(Fcvtms, "fcvtms");
  COMPARE_2REGMISC_FP16(Fcvtmu, "fcvtmu");
  COMPARE_2REGMISC_FP16(Fcvtzs, "fcvtzs");
  COMPARE_2REGMISC_FP16(Fcvtzu, "fcvtzu");
  COMPARE_2REGMISC_FP16(Fcvtas, "fcvtas");
  COMPARE_2REGMISC_FP16(Fcvtau, "fcvtau");

  COMPARE_2REGMISC_FP16(Scvtf, "scvtf");
  COMPARE_2REGMISC_FP16(Ucvtf, "ucvtf");

  COMPARE_2REGMISC_FP16(Fabs, "fabs");
  COMPARE_2REGMISC_FP16(Frecpe, "frecpe");
  COMPARE_2REGMISC_FP16(Fneg, "fneg");
  COMPARE_2REGMISC_FP16(Frsqrte, "frsqrte");
  COMPARE_2REGMISC_FP16(Fsqrt, "fsqrt");

  COMPARE_2REGMISC_CMP_FP16(Fcmeq, "fcmeq");
  COMPARE_2REGMISC_CMP_FP16(Fcmgt, "fcmgt");
  COMPARE_2REGMISC_CMP_FP16(Fcmge, "fcmge");
  COMPARE_2REGMISC_CMP_FP16(Fcmlt, "fcmlt");
  COMPARE_2REGMISC_CMP_FP16(Fcmle, "fcmle");

  COMPARE_MACRO(Frecpx(h5, h17), "frecpx h5, h17");

  CLEANUP();
}

TEST(neon_acrosslanes) {
  SETUP();

  COMPARE_MACRO(Smaxv(b4, v5.V8B()), "smaxv b4, v5.8b");
  COMPARE_MACRO(Smaxv(b4, v5.V16B()), "smaxv b4, v5.16b");
  COMPARE_MACRO(Smaxv(h4, v5.V4H()), "smaxv h4, v5.4h");
  COMPARE_MACRO(Smaxv(h4, v5.V8H()), "smaxv h4, v5.8h");
  COMPARE_MACRO(Smaxv(s4, v5.V4S()), "smaxv s4, v5.4s");

  COMPARE_MACRO(Sminv(b4, v5.V8B()), "sminv b4, v5.8b");
  COMPARE_MACRO(Sminv(b4, v5.V16B()), "sminv b4, v5.16b");
  COMPARE_MACRO(Sminv(h4, v5.V4H()), "sminv h4, v5.4h");
  COMPARE_MACRO(Sminv(h4, v5.V8H()), "sminv h4, v5.8h");
  COMPARE_MACRO(Sminv(s4, v5.V4S()), "sminv s4, v5.4s");

  COMPARE_MACRO(Umaxv(b4, v5.V8B()), "umaxv b4, v5.8b");
  COMPARE_MACRO(Umaxv(b4, v5.V16B()), "umaxv b4, v5.16b");
  COMPARE_MACRO(Umaxv(h4, v5.V4H()), "umaxv h4, v5.4h");
  COMPARE_MACRO(Umaxv(h4, v5.V8H()), "umaxv h4, v5.8h");
  COMPARE_MACRO(Umaxv(s4, v5.V4S()), "umaxv s4, v5.4s");

  COMPARE_MACRO(Uminv(b4, v5.V8B()), "uminv b4, v5.8b");
  COMPARE_MACRO(Uminv(b4, v5.V16B()), "uminv b4, v5.16b");
  COMPARE_MACRO(Uminv(h4, v5.V4H()), "uminv h4, v5.4h");
  COMPARE_MACRO(Uminv(h4, v5.V8H()), "uminv h4, v5.8h");
  COMPARE_MACRO(Uminv(s4, v5.V4S()), "uminv s4, v5.4s");

  COMPARE_MACRO(Addv(b4, v5.V8B()), "addv b4, v5.8b");
  COMPARE_MACRO(Addv(b4, v5.V16B()), "addv b4, v5.16b");
  COMPARE_MACRO(Addv(h4, v5.V4H()), "addv h4, v5.4h");
  COMPARE_MACRO(Addv(h4, v5.V8H()), "addv h4, v5.8h");
  COMPARE_MACRO(Addv(s4, v5.V4S()), "addv s4, v5.4s");

  COMPARE_MACRO(Saddlv(h4, v5.V8B()), "saddlv h4, v5.8b");
  COMPARE_MACRO(Saddlv(h4, v5.V16B()), "saddlv h4, v5.16b");
  COMPARE_MACRO(Saddlv(s4, v5.V4H()), "saddlv s4, v5.4h");
  COMPARE_MACRO(Saddlv(s4, v5.V8H()), "saddlv s4, v5.8h");
  COMPARE_MACRO(Saddlv(d4, v5.V4S()), "saddlv d4, v5.4s");

  COMPARE_MACRO(Uaddlv(h4, v5.V8B()), "uaddlv h4, v5.8b");
  COMPARE_MACRO(Uaddlv(h4, v5.V16B()), "uaddlv h4, v5.16b");
  COMPARE_MACRO(Uaddlv(s4, v5.V4H()), "uaddlv s4, v5.4h");
  COMPARE_MACRO(Uaddlv(s4, v5.V8H()), "uaddlv s4, v5.8h");
  COMPARE_MACRO(Uaddlv(d4, v5.V4S()), "uaddlv d4, v5.4s");

  COMPARE_MACRO(Fmaxv(s4, v5.V4S()), "fmaxv s4, v5.4s");
  COMPARE_MACRO(Fmaxv(h4, v5.V4H()), "fmaxv h4, v5.4h");
  COMPARE_MACRO(Fmaxv(h4, v5.V8H()), "fmaxv h4, v5.8h");
  COMPARE_MACRO(Fminv(s4, v5.V4S()), "fminv s4, v5.4s");
  COMPARE_MACRO(Fminv(h4, v5.V4H()), "fminv h4, v5.4h");
  COMPARE_MACRO(Fminv(h4, v5.V8H()), "fminv h4, v5.8h");
  COMPARE_MACRO(Fmaxnmv(s4, v5.V4S()), "fmaxnmv s4, v5.4s");
  COMPARE_MACRO(Fmaxnmv(h4, v5.V4H()), "fmaxnmv h4, v5.4h");
  COMPARE_MACRO(Fmaxnmv(h4, v5.V8H()), "fmaxnmv h4, v5.8h");
  COMPARE_MACRO(Fminnmv(s4, v5.V4S()), "fminnmv s4, v5.4s");
  COMPARE_MACRO(Fminnmv(h4, v5.V4H()), "fminnmv h4, v5.4h");
  COMPARE_MACRO(Fminnmv(h4, v5.V8H()), "fminnmv h4, v5.8h");

  CLEANUP();
}

TEST(neon_scalar_pairwise) {
  SETUP();

  COMPARE_MACRO(Addp(d0, v1.V2D()), "addp d0, v1.2d");
  COMPARE_MACRO(Faddp(h0, v1.V2H()), "faddp h0, v1.2h");
  COMPARE_MACRO(Faddp(s0, v1.V2S()), "faddp s0, v1.2s");
  COMPARE_MACRO(Faddp(d2, v3.V2D()), "faddp d2, v3.2d");
  COMPARE_MACRO(Fmaxp(h4, v5.V2H()), "fmaxp h4, v5.2h");
  COMPARE_MACRO(Fmaxp(s4, v5.V2S()), "fmaxp s4, v5.2s");
  COMPARE_MACRO(Fmaxp(d6, v7.V2D()), "fmaxp d6, v7.2d");
  COMPARE_MACRO(Fmaxnmp(h8, v9.V2H()), "fmaxnmp h8, v9.2h");
  COMPARE_MACRO(Fmaxnmp(s8, v9.V2S()), "fmaxnmp s8, v9.2s");
  COMPARE_MACRO(Fmaxnmp(d10, v11.V2D()), "fmaxnmp d10, v11.2d");
  COMPARE_MACRO(Fminp(h12, v13.V2H()), "fminp h12, v13.2h");
  COMPARE_MACRO(Fminp(s12, v13.V2S()), "fminp s12, v13.2s");
  COMPARE_MACRO(Fminp(d14, v15.V2D()), "fminp d14, v15.2d");
  COMPARE_MACRO(Fminnmp(h16, v17.V2H()), "fminnmp h16, v17.2h");
  COMPARE_MACRO(Fminnmp(s16, v17.V2S()), "fminnmp s16, v17.2s");
  COMPARE_MACRO(Fminnmp(d18, v19.V2D()), "fminnmp d18, v19.2d");
  CLEANUP();
}

TEST(neon_shift_immediate) {
  SETUP();

  COMPARE_MACRO(Sshr(v0.V8B(), v1.V8B(), 1), "sshr v0.8b, v1.8b, #1");
  COMPARE_MACRO(Sshr(v2.V8B(), v3.V8B(), 8), "sshr v2.8b, v3.8b, #8");
  COMPARE_MACRO(Sshr(v4.V16B(), v5.V16B(), 1), "sshr v4.16b, v5.16b, #1");
  COMPARE_MACRO(Sshr(v6.V16B(), v7.V16B(), 8), "sshr v6.16b, v7.16b, #8");
  COMPARE_MACRO(Sshr(v8.V4H(), v9.V4H(), 1), "sshr v8.4h, v9.4h, #1");
  COMPARE_MACRO(Sshr(v10.V4H(), v11.V4H(), 16), "sshr v10.4h, v11.4h, #16");
  COMPARE_MACRO(Sshr(v12.V8H(), v13.V8H(), 1), "sshr v12.8h, v13.8h, #1");
  COMPARE_MACRO(Sshr(v14.V8H(), v15.V8H(), 16), "sshr v14.8h, v15.8h, #16");
  COMPARE_MACRO(Sshr(v16.V2S(), v17.V2S(), 1), "sshr v16.2s, v17.2s, #1");
  COMPARE_MACRO(Sshr(v18.V2S(), v19.V2S(), 32), "sshr v18.2s, v19.2s, #32");
  COMPARE_MACRO(Sshr(v20.V4S(), v21.V4S(), 1), "sshr v20.4s, v21.4s, #1");
  COMPARE_MACRO(Sshr(v22.V4S(), v23.V4S(), 32), "sshr v22.4s, v23.4s, #32");
  COMPARE_MACRO(Sshr(v28.V2D(), v29.V2D(), 1), "sshr v28.2d, v29.2d, #1");
  COMPARE_MACRO(Sshr(v30.V2D(), v31.V2D(), 64), "sshr v30.2d, v31.2d, #64");
  COMPARE_MACRO(Sshr(d0, d1, 7), "sshr d0, d1, #7");

  COMPARE_MACRO(Ushr(v0.V8B(), v1.V8B(), 1), "ushr v0.8b, v1.8b, #1");
  COMPARE_MACRO(Ushr(v2.V8B(), v3.V8B(), 8), "ushr v2.8b, v3.8b, #8");
  COMPARE_MACRO(Ushr(v4.V16B(), v5.V16B(), 1), "ushr v4.16b, v5.16b, #1");
  COMPARE_MACRO(Ushr(v6.V16B(), v7.V16B(), 8), "ushr v6.16b, v7.16b, #8");
  COMPARE_MACRO(Ushr(v8.V4H(), v9.V4H(), 1), "ushr v8.4h, v9.4h, #1");
  COMPARE_MACRO(Ushr(v10.V4H(), v11.V4H(), 16), "ushr v10.4h, v11.4h, #16");
  COMPARE_MACRO(Ushr(v12.V8H(), v13.V8H(), 1), "ushr v12.8h, v13.8h, #1");
  COMPARE_MACRO(Ushr(v14.V8H(), v15.V8H(), 16), "ushr v14.8h, v15.8h, #16");
  COMPARE_MACRO(Ushr(v16.V2S(), v17.V2S(), 1), "ushr v16.2s, v17.2s, #1");
  COMPARE_MACRO(Ushr(v18.V2S(), v19.V2S(), 32), "ushr v18.2s, v19.2s, #32");
  COMPARE_MACRO(Ushr(v20.V4S(), v21.V4S(), 1), "ushr v20.4s, v21.4s, #1");
  COMPARE_MACRO(Ushr(v22.V4S(), v23.V4S(), 32), "ushr v22.4s, v23.4s, #32");
  COMPARE_MACRO(Ushr(v28.V2D(), v29.V2D(), 1), "ushr v28.2d, v29.2d, #1");
  COMPARE_MACRO(Ushr(v30.V2D(), v31.V2D(), 64), "ushr v30.2d, v31.2d, #64");
  COMPARE_MACRO(Ushr(d0, d1, 7), "ushr d0, d1, #7");

  COMPARE_MACRO(Srshr(v0.V8B(), v1.V8B(), 1), "srshr v0.8b, v1.8b, #1");
  COMPARE_MACRO(Srshr(v2.V8B(), v3.V8B(), 8), "srshr v2.8b, v3.8b, #8");
  COMPARE_MACRO(Srshr(v4.V16B(), v5.V16B(), 1), "srshr v4.16b, v5.16b, #1");
  COMPARE_MACRO(Srshr(v6.V16B(), v7.V16B(), 8), "srshr v6.16b, v7.16b, #8");
  COMPARE_MACRO(Srshr(v8.V4H(), v9.V4H(), 1), "srshr v8.4h, v9.4h, #1");
  COMPARE_MACRO(Srshr(v10.V4H(), v11.V4H(), 16), "srshr v10.4h, v11.4h, #16");
  COMPARE_MACRO(Srshr(v12.V8H(), v13.V8H(), 1), "srshr v12.8h, v13.8h, #1");
  COMPARE_MACRO(Srshr(v14.V8H(), v15.V8H(), 16), "srshr v14.8h, v15.8h, #16");
  COMPARE_MACRO(Srshr(v16.V2S(), v17.V2S(), 1), "srshr v16.2s, v17.2s, #1");
  COMPARE_MACRO(Srshr(v18.V2S(), v19.V2S(), 32), "srshr v18.2s, v19.2s, #32");
  COMPARE_MACRO(Srshr(v20.V4S(), v21.V4S(), 1), "srshr v20.4s, v21.4s, #1");
  COMPARE_MACRO(Srshr(v22.V4S(), v23.V4S(), 32), "srshr v22.4s, v23.4s, #32");
  COMPARE_MACRO(Srshr(v28.V2D(), v29.V2D(), 1), "srshr v28.2d, v29.2d, #1");
  COMPARE_MACRO(Srshr(v30.V2D(), v31.V2D(), 64), "srshr v30.2d, v31.2d, #64");
  COMPARE_MACRO(Srshr(d0, d1, 7), "srshr d0, d1, #7");

  COMPARE_MACRO(Urshr(v0.V8B(), v1.V8B(), 1), "urshr v0.8b, v1.8b, #1");
  COMPARE_MACRO(Urshr(v2.V8B(), v3.V8B(), 8), "urshr v2.8b, v3.8b, #8");
  COMPARE_MACRO(Urshr(v4.V16B(), v5.V16B(), 1), "urshr v4.16b, v5.16b, #1");
  COMPARE_MACRO(Urshr(v6.V16B(), v7.V16B(), 8), "urshr v6.16b, v7.16b, #8");
  COMPARE_MACRO(Urshr(v8.V4H(), v9.V4H(), 1), "urshr v8.4h, v9.4h, #1");
  COMPARE_MACRO(Urshr(v10.V4H(), v11.V4H(), 16), "urshr v10.4h, v11.4h, #16");
  COMPARE_MACRO(Urshr(v12.V8H(), v13.V8H(), 1), "urshr v12.8h, v13.8h, #1");
  COMPARE_MACRO(Urshr(v14.V8H(), v15.V8H(), 16), "urshr v14.8h, v15.8h, #16");
  COMPARE_MACRO(Urshr(v16.V2S(), v17.V2S(), 1), "urshr v16.2s, v17.2s, #1");
  COMPARE_MACRO(Urshr(v18.V2S(), v19.V2S(), 32), "urshr v18.2s, v19.2s, #32");
  COMPARE_MACRO(Urshr(v20.V4S(), v21.V4S(), 1), "urshr v20.4s, v21.4s, #1");
  COMPARE_MACRO(Urshr(v22.V4S(), v23.V4S(), 32), "urshr v22.4s, v23.4s, #32");
  COMPARE_MACRO(Urshr(v28.V2D(), v29.V2D(), 1), "urshr v28.2d, v29.2d, #1");
  COMPARE_MACRO(Urshr(v30.V2D(), v31.V2D(), 64), "urshr v30.2d, v31.2d, #64");
  COMPARE_MACRO(Urshr(d0, d1, 7), "urshr d0, d1, #7");

  COMPARE_MACRO(Srsra(v0.V8B(), v1.V8B(), 1), "srsra v0.8b, v1.8b, #1");
  COMPARE_MACRO(Srsra(v2.V8B(), v3.V8B(), 8), "srsra v2.8b, v3.8b, #8");
  COMPARE_MACRO(Srsra(v4.V16B(), v5.V16B(), 1), "srsra v4.16b, v5.16b, #1");
  COMPARE_MACRO(Srsra(v6.V16B(), v7.V16B(), 8), "srsra v6.16b, v7.16b, #8");
  COMPARE_MACRO(Srsra(v8.V4H(), v9.V4H(), 1), "srsra v8.4h, v9.4h, #1");
  COMPARE_MACRO(Srsra(v10.V4H(), v11.V4H(), 16), "srsra v10.4h, v11.4h, #16");
  COMPARE_MACRO(Srsra(v12.V8H(), v13.V8H(), 1), "srsra v12.8h, v13.8h, #1");
  COMPARE_MACRO(Srsra(v14.V8H(), v15.V8H(), 16), "srsra v14.8h, v15.8h, #16");
  COMPARE_MACRO(Srsra(v16.V2S(), v17.V2S(), 1), "srsra v16.2s, v17.2s, #1");
  COMPARE_MACRO(Srsra(v18.V2S(), v19.V2S(), 32), "srsra v18.2s, v19.2s, #32");
  COMPARE_MACRO(Srsra(v20.V4S(), v21.V4S(), 1), "srsra v20.4s, v21.4s, #1");
  COMPARE_MACRO(Srsra(v22.V4S(), v23.V4S(), 32), "srsra v22.4s, v23.4s, #32");
  COMPARE_MACRO(Srsra(v28.V2D(), v29.V2D(), 1), "srsra v28.2d, v29.2d, #1");
  COMPARE_MACRO(Srsra(v30.V2D(), v31.V2D(), 64), "srsra v30.2d, v31.2d, #64");
  COMPARE_MACRO(Srsra(d0, d1, 7), "srsra d0, d1, #7");

  COMPARE_MACRO(Ssra(v0.V8B(), v1.V8B(), 1), "ssra v0.8b, v1.8b, #1");
  COMPARE_MACRO(Ssra(v2.V8B(), v3.V8B(), 8), "ssra v2.8b, v3.8b, #8");
  COMPARE_MACRO(Ssra(v4.V16B(), v5.V16B(), 1), "ssra v4.16b, v5.16b, #1");
  COMPARE_MACRO(Ssra(v6.V16B(), v7.V16B(), 8), "ssra v6.16b, v7.16b, #8");
  COMPARE_MACRO(Ssra(v8.V4H(), v9.V4H(), 1), "ssra v8.4h, v9.4h, #1");
  COMPARE_MACRO(Ssra(v10.V4H(), v11.V4H(), 16), "ssra v10.4h, v11.4h, #16");
  COMPARE_MACRO(Ssra(v12.V8H(), v13.V8H(), 1), "ssra v12.8h, v13.8h, #1");
  COMPARE_MACRO(Ssra(v14.V8H(), v15.V8H(), 16), "ssra v14.8h, v15.8h, #16");
  COMPARE_MACRO(Ssra(v16.V2S(), v17.V2S(), 1), "ssra v16.2s, v17.2s, #1");
  COMPARE_MACRO(Ssra(v18.V2S(), v19.V2S(), 32), "ssra v18.2s, v19.2s, #32");
  COMPARE_MACRO(Ssra(v20.V4S(), v21.V4S(), 1), "ssra v20.4s, v21.4s, #1");
  COMPARE_MACRO(Ssra(v22.V4S(), v23.V4S(), 32), "ssra v22.4s, v23.4s, #32");
  COMPARE_MACRO(Ssra(v28.V2D(), v29.V2D(), 1), "ssra v28.2d, v29.2d, #1");
  COMPARE_MACRO(Ssra(v30.V2D(), v31.V2D(), 64), "ssra v30.2d, v31.2d, #64");
  COMPARE_MACRO(Ssra(d0, d1, 7), "ssra d0, d1, #7");

  COMPARE_MACRO(Ursra(v0.V8B(), v1.V8B(), 1), "ursra v0.8b, v1.8b, #1");
  COMPARE_MACRO(Ursra(v2.V8B(), v3.V8B(), 8), "ursra v2.8b, v3.8b, #8");
  COMPARE_MACRO(Ursra(v4.V16B(), v5.V16B(), 1), "ursra v4.16b, v5.16b, #1");
  COMPARE_MACRO(Ursra(v6.V16B(), v7.V16B(), 8), "ursra v6.16b, v7.16b, #8");
  COMPARE_MACRO(Ursra(v8.V4H(), v9.V4H(), 1), "ursra v8.4h, v9.4h, #1");
  COMPARE_MACRO(Ursra(v10.V4H(), v11.V4H(), 16), "ursra v10.4h, v11.4h, #16");
  COMPARE_MACRO(Ursra(v12.V8H(), v13.V8H(), 1), "ursra v12.8h, v13.8h, #1");
  COMPARE_MACRO(Ursra(v14.V8H(), v15.V8H(), 16), "ursra v14.8h, v15.8h, #16");
  COMPARE_MACRO(Ursra(v16.V2S(), v17.V2S(), 1), "ursra v16.2s, v17.2s, #1");
  COMPARE_MACRO(Ursra(v18.V2S(), v19.V2S(), 32), "ursra v18.2s, v19.2s, #32");
  COMPARE_MACRO(Ursra(v20.V4S(), v21.V4S(), 1), "ursra v20.4s, v21.4s, #1");
  COMPARE_MACRO(Ursra(v22.V4S(), v23.V4S(), 32), "ursra v22.4s, v23.4s, #32");
  COMPARE_MACRO(Ursra(v28.V2D(), v29.V2D(), 1), "ursra v28.2d, v29.2d, #1");
  COMPARE_MACRO(Ursra(v30.V2D(), v31.V2D(), 64), "ursra v30.2d, v31.2d, #64");
  COMPARE_MACRO(Ursra(d0, d1, 7), "ursra d0, d1, #7");

  COMPARE_MACRO(Usra(v0.V8B(), v1.V8B(), 1), "usra v0.8b, v1.8b, #1");
  COMPARE_MACRO(Usra(v2.V8B(), v3.V8B(), 8), "usra v2.8b, v3.8b, #8");
  COMPARE_MACRO(Usra(v4.V16B(), v5.V16B(), 1), "usra v4.16b, v5.16b, #1");
  COMPARE_MACRO(Usra(v6.V16B(), v7.V16B(), 8), "usra v6.16b, v7.16b, #8");
  COMPARE_MACRO(Usra(v8.V4H(), v9.V4H(), 1), "usra v8.4h, v9.4h, #1");
  COMPARE_MACRO(Usra(v10.V4H(), v11.V4H(), 16), "usra v10.4h, v11.4h, #16");
  COMPARE_MACRO(Usra(v12.V8H(), v13.V8H(), 1), "usra v12.8h, v13.8h, #1");
  COMPARE_MACRO(Usra(v14.V8H(), v15.V8H(), 16), "usra v14.8h, v15.8h, #16");
  COMPARE_MACRO(Usra(v16.V2S(), v17.V2S(), 1), "usra v16.2s, v17.2s, #1");
  COMPARE_MACRO(Usra(v18.V2S(), v19.V2S(), 32), "usra v18.2s, v19.2s, #32");
  COMPARE_MACRO(Usra(v20.V4S(), v21.V4S(), 1), "usra v20.4s, v21.4s, #1");
  COMPARE_MACRO(Usra(v22.V4S(), v23.V4S(), 32), "usra v22.4s, v23.4s, #32");
  COMPARE_MACRO(Usra(v28.V2D(), v29.V2D(), 1), "usra v28.2d, v29.2d, #1");
  COMPARE_MACRO(Usra(v30.V2D(), v31.V2D(), 64), "usra v30.2d, v31.2d, #64");
  COMPARE_MACRO(Usra(d0, d1, 7), "usra d0, d1, #7");

  COMPARE_MACRO(Sli(v1.V8B(), v8.V8B(), 1), "sli v1.8b, v8.8b, #1");
  COMPARE_MACRO(Sli(v2.V16B(), v9.V16B(), 2), "sli v2.16b, v9.16b, #2");
  COMPARE_MACRO(Sli(v3.V4H(), v1.V4H(), 3), "sli v3.4h, v1.4h, #3");
  COMPARE_MACRO(Sli(v4.V8H(), v2.V8H(), 4), "sli v4.8h, v2.8h, #4");
  COMPARE_MACRO(Sli(v5.V2S(), v3.V2S(), 5), "sli v5.2s, v3.2s, #5");
  COMPARE_MACRO(Sli(v6.V4S(), v4.V4S(), 6), "sli v6.4s, v4.4s, #6");
  COMPARE_MACRO(Sli(v7.V2D(), v5.V2D(), 7), "sli v7.2d, v5.2d, #7");
  COMPARE_MACRO(Sli(d8, d6, 8), "sli d8, d6, #8");

  COMPARE_MACRO(Shl(v1.V8B(), v8.V8B(), 1), "shl v1.8b, v8.8b, #1");
  COMPARE_MACRO(Shl(v2.V16B(), v9.V16B(), 2), "shl v2.16b, v9.16b, #2");
  COMPARE_MACRO(Shl(v3.V4H(), v1.V4H(), 3), "shl v3.4h, v1.4h, #3");
  COMPARE_MACRO(Shl(v4.V8H(), v2.V8H(), 4), "shl v4.8h, v2.8h, #4");
  COMPARE_MACRO(Shl(v5.V2S(), v3.V2S(), 5), "shl v5.2s, v3.2s, #5");
  COMPARE_MACRO(Shl(v6.V4S(), v4.V4S(), 6), "shl v6.4s, v4.4s, #6");
  COMPARE_MACRO(Shl(v7.V2D(), v5.V2D(), 7), "shl v7.2d, v5.2d, #7");
  COMPARE_MACRO(Shl(d8, d6, 8), "shl d8, d6, #8");

  COMPARE_MACRO(Sqshl(v1.V8B(), v8.V8B(), 1), "sqshl v1.8b, v8.8b, #1");
  COMPARE_MACRO(Sqshl(v2.V16B(), v9.V16B(), 2), "sqshl v2.16b, v9.16b, #2");
  COMPARE_MACRO(Sqshl(v3.V4H(), v1.V4H(), 3), "sqshl v3.4h, v1.4h, #3");
  COMPARE_MACRO(Sqshl(v4.V8H(), v2.V8H(), 4), "sqshl v4.8h, v2.8h, #4");
  COMPARE_MACRO(Sqshl(v5.V2S(), v3.V2S(), 5), "sqshl v5.2s, v3.2s, #5");
  COMPARE_MACRO(Sqshl(v6.V4S(), v4.V4S(), 6), "sqshl v6.4s, v4.4s, #6");
  COMPARE_MACRO(Sqshl(v7.V2D(), v5.V2D(), 7), "sqshl v7.2d, v5.2d, #7");
  COMPARE_MACRO(Sqshl(b8, b7, 1), "sqshl b8, b7, #1");
  COMPARE_MACRO(Sqshl(h9, h8, 2), "sqshl h9, h8, #2");
  COMPARE_MACRO(Sqshl(s10, s9, 3), "sqshl s10, s9, #3");
  COMPARE_MACRO(Sqshl(d11, d10, 4), "sqshl d11, d10, #4");

  COMPARE_MACRO(Sqshlu(v1.V8B(), v8.V8B(), 1), "sqshlu v1.8b, v8.8b, #1");
  COMPARE_MACRO(Sqshlu(v2.V16B(), v9.V16B(), 2), "sqshlu v2.16b, v9.16b, #2");
  COMPARE_MACRO(Sqshlu(v3.V4H(), v1.V4H(), 3), "sqshlu v3.4h, v1.4h, #3");
  COMPARE_MACRO(Sqshlu(v4.V8H(), v2.V8H(), 4), "sqshlu v4.8h, v2.8h, #4");
  COMPARE_MACRO(Sqshlu(v5.V2S(), v3.V2S(), 5), "sqshlu v5.2s, v3.2s, #5");
  COMPARE_MACRO(Sqshlu(v6.V4S(), v4.V4S(), 6), "sqshlu v6.4s, v4.4s, #6");
  COMPARE_MACRO(Sqshlu(v7.V2D(), v5.V2D(), 7), "sqshlu v7.2d, v5.2d, #7");
  COMPARE_MACRO(Sqshlu(b8, b7, 1), "sqshlu b8, b7, #1");
  COMPARE_MACRO(Sqshlu(h9, h8, 2), "sqshlu h9, h8, #2");
  COMPARE_MACRO(Sqshlu(s10, s9, 3), "sqshlu s10, s9, #3");
  COMPARE_MACRO(Sqshlu(d11, d10, 4), "sqshlu d11, d10, #4");

  COMPARE_MACRO(Uqshl(v1.V8B(), v8.V8B(), 1), "uqshl v1.8b, v8.8b, #1");
  COMPARE_MACRO(Uqshl(v2.V16B(), v9.V16B(), 2), "uqshl v2.16b, v9.16b, #2");
  COMPARE_MACRO(Uqshl(v3.V4H(), v1.V4H(), 3), "uqshl v3.4h, v1.4h, #3");
  COMPARE_MACRO(Uqshl(v4.V8H(), v2.V8H(), 4), "uqshl v4.8h, v2.8h, #4");
  COMPARE_MACRO(Uqshl(v5.V2S(), v3.V2S(), 5), "uqshl v5.2s, v3.2s, #5");
  COMPARE_MACRO(Uqshl(v6.V4S(), v4.V4S(), 6), "uqshl v6.4s, v4.4s, #6");
  COMPARE_MACRO(Uqshl(v7.V2D(), v5.V2D(), 7), "uqshl v7.2d, v5.2d, #7");
  COMPARE_MACRO(Uqshl(b8, b7, 1), "uqshl b8, b7, #1");
  COMPARE_MACRO(Uqshl(h9, h8, 2), "uqshl h9, h8, #2");
  COMPARE_MACRO(Uqshl(s10, s9, 3), "uqshl s10, s9, #3");
  COMPARE_MACRO(Uqshl(d11, d10, 4), "uqshl d11, d10, #4");

  COMPARE_MACRO(Sshll(v1.V8H(), v8.V8B(), 1), "sshll v1.8h, v8.8b, #1");
  COMPARE_MACRO(Sshll(v3.V4S(), v1.V4H(), 3), "sshll v3.4s, v1.4h, #3");
  COMPARE_MACRO(Sshll(v5.V2D(), v3.V2S(), 5), "sshll v5.2d, v3.2s, #5");
  COMPARE_MACRO(Sshll2(v2.V8H(), v9.V16B(), 2), "sshll2 v2.8h, v9.16b, #2");
  COMPARE_MACRO(Sshll2(v4.V4S(), v2.V8H(), 4), "sshll2 v4.4s, v2.8h, #4");
  COMPARE_MACRO(Sshll2(v6.V2D(), v4.V4S(), 6), "sshll2 v6.2d, v4.4s, #6");

  // Translate to shll if shift(immediate) equal to the element size
  COMPARE_MACRO(Sshll(v1.V8H(), v8.V8B(), 8), "shll v1.8h, v8.8b, #8");
  COMPARE_MACRO(Sshll(v3.V4S(), v1.V4H(), 16), "shll v3.4s, v1.4h, #16");
  COMPARE_MACRO(Sshll(v5.V2D(), v3.V2S(), 32), "shll v5.2d, v3.2s, #32");
  COMPARE_MACRO(Sshll2(v2.V8H(), v9.V16B(), 8), "shll2 v2.8h, v9.16b, #8");
  COMPARE_MACRO(Sshll2(v4.V4S(), v2.V8H(), 16), "shll2 v4.4s, v2.8h, #16");
  COMPARE_MACRO(Sshll2(v6.V2D(), v4.V4S(), 32), "shll2 v6.2d, v4.4s, #32");
  COMPARE_MACRO(Ushll(v3.V8H(), v10.V8B(), 8), "shll v3.8h, v10.8b, #8");
  COMPARE_MACRO(Ushll(v5.V4S(), v3.V4H(), 16), "shll v5.4s, v3.4h, #16");
  COMPARE_MACRO(Ushll(v7.V2D(), v5.V2S(), 32), "shll v7.2d, v5.2s, #32");
  COMPARE_MACRO(Ushll2(v4.V8H(), v9.V16B(), 8), "shll2 v4.8h, v9.16b, #8");
  COMPARE_MACRO(Ushll2(v6.V4S(), v4.V8H(), 16), "shll2 v6.4s, v4.8h, #16");
  COMPARE_MACRO(Ushll2(v8.V2D(), v6.V4S(), 32), "shll2 v8.2d, v6.4s, #32");

  COMPARE_MACRO(Sshll(v1.V8H(), v8.V8B(), 0), "sxtl v1.8h, v8.8b");
  COMPARE_MACRO(Sshll(v3.V4S(), v1.V4H(), 0), "sxtl v3.4s, v1.4h");
  COMPARE_MACRO(Sshll(v5.V2D(), v3.V2S(), 0), "sxtl v5.2d, v3.2s");
  COMPARE_MACRO(Sshll2(v2.V8H(), v9.V16B(), 0), "sxtl2 v2.8h, v9.16b");
  COMPARE_MACRO(Sshll2(v4.V4S(), v2.V8H(), 0), "sxtl2 v4.4s, v2.8h");
  COMPARE_MACRO(Sshll2(v6.V2D(), v4.V4S(), 0), "sxtl2 v6.2d, v4.4s");

  COMPARE_MACRO(Sxtl(v1.V8H(), v8.V8B()), "sxtl v1.8h, v8.8b");
  COMPARE_MACRO(Sxtl(v3.V4S(), v1.V4H()), "sxtl v3.4s, v1.4h");
  COMPARE_MACRO(Sxtl(v5.V2D(), v3.V2S()), "sxtl v5.2d, v3.2s");
  COMPARE_MACRO(Sxtl2(v2.V8H(), v9.V16B()), "sxtl2 v2.8h, v9.16b");
  COMPARE_MACRO(Sxtl2(v4.V4S(), v2.V8H()), "sxtl2 v4.4s, v2.8h");
  COMPARE_MACRO(Sxtl2(v6.V2D(), v4.V4S()), "sxtl2 v6.2d, v4.4s");

  COMPARE_MACRO(Ushll(v1.V8H(), v8.V8B(), 1), "ushll v1.8h, v8.8b, #1");
  COMPARE_MACRO(Ushll(v3.V4S(), v1.V4H(), 3), "ushll v3.4s, v1.4h, #3");
  COMPARE_MACRO(Ushll(v5.V2D(), v3.V2S(), 5), "ushll v5.2d, v3.2s, #5");
  COMPARE_MACRO(Ushll2(v2.V8H(), v9.V16B(), 2), "ushll2 v2.8h, v9.16b, #2");
  COMPARE_MACRO(Ushll2(v4.V4S(), v2.V8H(), 4), "ushll2 v4.4s, v2.8h, #4");
  COMPARE_MACRO(Ushll2(v6.V2D(), v4.V4S(), 6), "ushll2 v6.2d, v4.4s, #6");

  COMPARE_MACRO(Ushll(v1.V8H(), v8.V8B(), 0), "uxtl v1.8h, v8.8b");
  COMPARE_MACRO(Ushll(v3.V4S(), v1.V4H(), 0), "uxtl v3.4s, v1.4h");
  COMPARE_MACRO(Ushll(v5.V2D(), v3.V2S(), 0), "uxtl v5.2d, v3.2s");
  COMPARE_MACRO(Ushll2(v2.V8H(), v9.V16B(), 0), "uxtl2 v2.8h, v9.16b");
  COMPARE_MACRO(Ushll2(v4.V4S(), v2.V8H(), 0), "uxtl2 v4.4s, v2.8h");
  COMPARE_MACRO(Ushll2(v6.V2D(), v4.V4S(), 0), "uxtl2 v6.2d, v4.4s");

  COMPARE_MACRO(Uxtl(v1.V8H(), v8.V8B()), "uxtl v1.8h, v8.8b");
  COMPARE_MACRO(Uxtl(v3.V4S(), v1.V4H()), "uxtl v3.4s, v1.4h");
  COMPARE_MACRO(Uxtl(v5.V2D(), v3.V2S()), "uxtl v5.2d, v3.2s");
  COMPARE_MACRO(Uxtl2(v2.V8H(), v9.V16B()), "uxtl2 v2.8h, v9.16b");
  COMPARE_MACRO(Uxtl2(v4.V4S(), v2.V8H()), "uxtl2 v4.4s, v2.8h");
  COMPARE_MACRO(Uxtl2(v6.V2D(), v4.V4S()), "uxtl2 v6.2d, v4.4s");

  COMPARE_MACRO(Sri(v1.V8B(), v8.V8B(), 1), "sri v1.8b, v8.8b, #1");
  COMPARE_MACRO(Sri(v2.V16B(), v9.V16B(), 2), "sri v2.16b, v9.16b, #2");
  COMPARE_MACRO(Sri(v3.V4H(), v1.V4H(), 3), "sri v3.4h, v1.4h, #3");
  COMPARE_MACRO(Sri(v4.V8H(), v2.V8H(), 4), "sri v4.8h, v2.8h, #4");
  COMPARE_MACRO(Sri(v5.V2S(), v3.V2S(), 5), "sri v5.2s, v3.2s, #5");
  COMPARE_MACRO(Sri(v6.V4S(), v4.V4S(), 6), "sri v6.4s, v4.4s, #6");
  COMPARE_MACRO(Sri(v7.V2D(), v5.V2D(), 7), "sri v7.2d, v5.2d, #7");
  COMPARE_MACRO(Sri(d8, d6, 8), "sri d8, d6, #8");

  COMPARE_MACRO(Shrn(v0.V8B(), v1.V8H(), 1), "shrn v0.8b, v1.8h, #1");
  COMPARE_MACRO(Shrn(v1.V4H(), v2.V4S(), 2), "shrn v1.4h, v2.4s, #2");
  COMPARE_MACRO(Shrn(v2.V2S(), v3.V2D(), 3), "shrn v2.2s, v3.2d, #3");
  COMPARE_MACRO(Shrn2(v0.V16B(), v1.V8H(), 4), "shrn2 v0.16b, v1.8h, #4");
  COMPARE_MACRO(Shrn2(v1.V8H(), v2.V4S(), 5), "shrn2 v1.8h, v2.4s, #5");
  COMPARE_MACRO(Shrn2(v2.V4S(), v3.V2D(), 6), "shrn2 v2.4s, v3.2d, #6");

  COMPARE_MACRO(Rshrn(v0.V8B(), v1.V8H(), 1), "rshrn v0.8b, v1.8h, #1");
  COMPARE_MACRO(Rshrn(v1.V4H(), v2.V4S(), 2), "rshrn v1.4h, v2.4s, #2");
  COMPARE_MACRO(Rshrn(v2.V2S(), v3.V2D(), 3), "rshrn v2.2s, v3.2d, #3");
  COMPARE_MACRO(Rshrn2(v0.V16B(), v1.V8H(), 4), "rshrn2 v0.16b, v1.8h, #4");
  COMPARE_MACRO(Rshrn2(v1.V8H(), v2.V4S(), 5), "rshrn2 v1.8h, v2.4s, #5");
  COMPARE_MACRO(Rshrn2(v2.V4S(), v3.V2D(), 6), "rshrn2 v2.4s, v3.2d, #6");

  COMPARE_MACRO(Uqshrn(v0.V8B(), v1.V8H(), 1), "uqshrn v0.8b, v1.8h, #1");
  COMPARE_MACRO(Uqshrn(v1.V4H(), v2.V4S(), 2), "uqshrn v1.4h, v2.4s, #2");
  COMPARE_MACRO(Uqshrn(v2.V2S(), v3.V2D(), 3), "uqshrn v2.2s, v3.2d, #3");
  COMPARE_MACRO(Uqshrn2(v0.V16B(), v1.V8H(), 4), "uqshrn2 v0.16b, v1.8h, #4");
  COMPARE_MACRO(Uqshrn2(v1.V8H(), v2.V4S(), 5), "uqshrn2 v1.8h, v2.4s, #5");
  COMPARE_MACRO(Uqshrn2(v2.V4S(), v3.V2D(), 6), "uqshrn2 v2.4s, v3.2d, #6");
  COMPARE_MACRO(Uqshrn(b0, h1, 1), "uqshrn b0, h1, #1");
  COMPARE_MACRO(Uqshrn(h1, s2, 2), "uqshrn h1, s2, #2");
  COMPARE_MACRO(Uqshrn(s2, d3, 3), "uqshrn s2, d3, #3");

  COMPARE_MACRO(Uqrshrn(v0.V8B(), v1.V8H(), 1), "uqrshrn v0.8b, v1.8h, #1");
  COMPARE_MACRO(Uqrshrn(v1.V4H(), v2.V4S(), 2), "uqrshrn v1.4h, v2.4s, #2");
  COMPARE_MACRO(Uqrshrn(v2.V2S(), v3.V2D(), 3), "uqrshrn v2.2s, v3.2d, #3");
  COMPARE_MACRO(Uqrshrn2(v0.V16B(), v1.V8H(), 4), "uqrshrn2 v0.16b, v1.8h, #4");
  COMPARE_MACRO(Uqrshrn2(v1.V8H(), v2.V4S(), 5), "uqrshrn2 v1.8h, v2.4s, #5");
  COMPARE_MACRO(Uqrshrn2(v2.V4S(), v3.V2D(), 6), "uqrshrn2 v2.4s, v3.2d, #6");
  COMPARE_MACRO(Uqrshrn(b0, h1, 1), "uqrshrn b0, h1, #1");
  COMPARE_MACRO(Uqrshrn(h1, s2, 2), "uqrshrn h1, s2, #2");
  COMPARE_MACRO(Uqrshrn(s2, d3, 3), "uqrshrn s2, d3, #3");

  COMPARE_MACRO(Sqshrn(v0.V8B(), v1.V8H(), 1), "sqshrn v0.8b, v1.8h, #1");
  COMPARE_MACRO(Sqshrn(v1.V4H(), v2.V4S(), 2), "sqshrn v1.4h, v2.4s, #2");
  COMPARE_MACRO(Sqshrn(v2.V2S(), v3.V2D(), 3), "sqshrn v2.2s, v3.2d, #3");
  COMPARE_MACRO(Sqshrn2(v0.V16B(), v1.V8H(), 4), "sqshrn2 v0.16b, v1.8h, #4");
  COMPARE_MACRO(Sqshrn2(v1.V8H(), v2.V4S(), 5), "sqshrn2 v1.8h, v2.4s, #5");
  COMPARE_MACRO(Sqshrn2(v2.V4S(), v3.V2D(), 6), "sqshrn2 v2.4s, v3.2d, #6");
  COMPARE_MACRO(Sqshrn(b0, h1, 1), "sqshrn b0, h1, #1");
  COMPARE_MACRO(Sqshrn(h1, s2, 2), "sqshrn h1, s2, #2");
  COMPARE_MACRO(Sqshrn(s2, d3, 3), "sqshrn s2, d3, #3");

  COMPARE_MACRO(Sqrshrn(v0.V8B(), v1.V8H(), 1), "sqrshrn v0.8b, v1.8h, #1");
  COMPARE_MACRO(Sqrshrn(v1.V4H(), v2.V4S(), 2), "sqrshrn v1.4h, v2.4s, #2");
  COMPARE_MACRO(Sqrshrn(v2.V2S(), v3.V2D(), 3), "sqrshrn v2.2s, v3.2d, #3");
  COMPARE_MACRO(Sqrshrn2(v0.V16B(), v1.V8H(), 4), "sqrshrn2 v0.16b, v1.8h, #4");
  COMPARE_MACRO(Sqrshrn2(v1.V8H(), v2.V4S(), 5), "sqrshrn2 v1.8h, v2.4s, #5");
  COMPARE_MACRO(Sqrshrn2(v2.V4S(), v3.V2D(), 6), "sqrshrn2 v2.4s, v3.2d, #6");
  COMPARE_MACRO(Sqrshrn(b0, h1, 1), "sqrshrn b0, h1, #1");
  COMPARE_MACRO(Sqrshrn(h1, s2, 2), "sqrshrn h1, s2, #2");
  COMPARE_MACRO(Sqrshrn(s2, d3, 3), "sqrshrn s2, d3, #3");

  COMPARE_MACRO(Sqshrun(v0.V8B(), v1.V8H(), 1), "sqshrun v0.8b, v1.8h, #1");
  COMPARE_MACRO(Sqshrun(v1.V4H(), v2.V4S(), 2), "sqshrun v1.4h, v2.4s, #2");
  COMPARE_MACRO(Sqshrun(v2.V2S(), v3.V2D(), 3), "sqshrun v2.2s, v3.2d, #3");
  COMPARE_MACRO(Sqshrun2(v0.V16B(), v1.V8H(), 4), "sqshrun2 v0.16b, v1.8h, #4");
  COMPARE_MACRO(Sqshrun2(v1.V8H(), v2.V4S(), 5), "sqshrun2 v1.8h, v2.4s, #5");
  COMPARE_MACRO(Sqshrun2(v2.V4S(), v3.V2D(), 6), "sqshrun2 v2.4s, v3.2d, #6");
  COMPARE_MACRO(Sqshrun(b0, h1, 1), "sqshrun b0, h1, #1");
  COMPARE_MACRO(Sqshrun(h1, s2, 2), "sqshrun h1, s2, #2");
  COMPARE_MACRO(Sqshrun(s2, d3, 3), "sqshrun s2, d3, #3");

  COMPARE_MACRO(Sqrshrun(v0.V8B(), v1.V8H(), 1), "sqrshrun v0.8b, v1.8h, #1");
  COMPARE_MACRO(Sqrshrun(v1.V4H(), v2.V4S(), 2), "sqrshrun v1.4h, v2.4s, #2");
  COMPARE_MACRO(Sqrshrun(v2.V2S(), v3.V2D(), 3), "sqrshrun v2.2s, v3.2d, #3");
  COMPARE_MACRO(Sqrshrun2(v0.V16B(), v1.V8H(), 4),
                "sqrshrun2 v0.16b, v1.8h, #4");
  COMPARE_MACRO(Sqrshrun2(v1.V8H(), v2.V4S(), 5), "sqrshrun2 v1.8h, v2.4s, #5");
  COMPARE_MACRO(Sqrshrun2(v2.V4S(), v3.V2D(), 6), "sqrshrun2 v2.4s, v3.2d, #6");
  COMPARE_MACRO(Sqrshrun(b0, h1, 1), "sqrshrun b0, h1, #1");
  COMPARE_MACRO(Sqrshrun(h1, s2, 2), "sqrshrun h1, s2, #2");
  COMPARE_MACRO(Sqrshrun(s2, d3, 3), "sqrshrun s2, d3, #3");

  COMPARE_MACRO(Scvtf(v5.V4H(), v3.V4H(), 11), "scvtf v5.4h, v3.4h, #11");
  COMPARE_MACRO(Scvtf(v6.V8H(), v4.V8H(), 12), "scvtf v6.8h, v4.8h, #12");
  COMPARE_MACRO(Scvtf(v5.V2S(), v3.V2S(), 11), "scvtf v5.2s, v3.2s, #11");
  COMPARE_MACRO(Scvtf(v6.V4S(), v4.V4S(), 12), "scvtf v6.4s, v4.4s, #12");
  COMPARE_MACRO(Scvtf(v7.V2D(), v5.V2D(), 33), "scvtf v7.2d, v5.2d, #33");
  COMPARE_MACRO(Scvtf(h8, h6, 13), "scvtf h8, h6, #13");
  COMPARE_MACRO(Scvtf(s8, s6, 13), "scvtf s8, s6, #13");
  COMPARE_MACRO(Scvtf(d8, d6, 34), "scvtf d8, d6, #34");

  COMPARE_MACRO(Ucvtf(v5.V4H(), v3.V4H(), 11), "ucvtf v5.4h, v3.4h, #11");
  COMPARE_MACRO(Ucvtf(v6.V8H(), v4.V8H(), 12), "ucvtf v6.8h, v4.8h, #12");
  COMPARE_MACRO(Ucvtf(v5.V2S(), v3.V2S(), 11), "ucvtf v5.2s, v3.2s, #11");
  COMPARE_MACRO(Ucvtf(v6.V4S(), v4.V4S(), 12), "ucvtf v6.4s, v4.4s, #12");
  COMPARE_MACRO(Ucvtf(v7.V2D(), v5.V2D(), 33), "ucvtf v7.2d, v5.2d, #33");
  COMPARE_MACRO(Ucvtf(h8, h6, 13), "ucvtf h8, h6, #13");
  COMPARE_MACRO(Ucvtf(s8, s6, 13), "ucvtf s8, s6, #13");
  COMPARE_MACRO(Ucvtf(d8, d6, 34), "ucvtf d8, d6, #34");

  COMPARE_MACRO(Fcvtzs(v3.V4H(), v1.V4H(), 5), "fcvtzs v3.4h, v1.4h, #5");
  COMPARE_MACRO(Fcvtzs(v4.V8H(), v2.V8H(), 6), "fcvtzs v4.8h, v2.8h, #6");
  COMPARE_MACRO(Fcvtzs(v5.V2S(), v3.V2S(), 11), "fcvtzs v5.2s, v3.2s, #11");
  COMPARE_MACRO(Fcvtzs(v6.V4S(), v4.V4S(), 12), "fcvtzs v6.4s, v4.4s, #12");
  COMPARE_MACRO(Fcvtzs(v7.V2D(), v5.V2D(), 33), "fcvtzs v7.2d, v5.2d, #33");
  COMPARE_MACRO(Fcvtzs(h8, h6, 13), "fcvtzs h8, h6, #13");
  COMPARE_MACRO(Fcvtzs(s8, s6, 13), "fcvtzs s8, s6, #13");
  COMPARE_MACRO(Fcvtzs(d8, d6, 34), "fcvtzs d8, d6, #34");

  COMPARE_MACRO(Fcvtzu(v3.V4H(), v1.V4H(), 5), "fcvtzu v3.4h, v1.4h, #5");
  COMPARE_MACRO(Fcvtzu(v4.V8H(), v2.V8H(), 6), "fcvtzu v4.8h, v2.8h, #6");
  COMPARE_MACRO(Fcvtzu(v5.V2S(), v3.V2S(), 11), "fcvtzu v5.2s, v3.2s, #11");
  COMPARE_MACRO(Fcvtzu(v6.V4S(), v4.V4S(), 12), "fcvtzu v6.4s, v4.4s, #12");
  COMPARE_MACRO(Fcvtzu(v7.V2D(), v5.V2D(), 33), "fcvtzu v7.2d, v5.2d, #33");
  COMPARE_MACRO(Fcvtzu(h8, h6, 13), "fcvtzu h8, h6, #13");
  COMPARE_MACRO(Fcvtzu(s8, s6, 13), "fcvtzu s8, s6, #13");
  COMPARE_MACRO(Fcvtzu(d8, d6, 34), "fcvtzu d8, d6, #34");
  CLEANUP();
}

}  // namespace aarch64
}  // namespace vixl
