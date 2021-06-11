// Copyright 2017, VIXL authors
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

extern "C" {
#ifdef __arm__
#include <linux/perf_event.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
}
#include <iostream>

#include "aarch32/assembler-aarch32.h"
#include "aarch32/constants-aarch32.h"
#include "aarch32/disasm-aarch32.h"
#include "aarch32/instructions-aarch32.h"
#include "aarch32/operands-aarch32.h"

// This benchmark tests the speed of the assembler and the disassembler.
// 7210 instructions (19340 bytes):
// The command is:
// asm-disasm-speed-test 2> /dev/null
// We redirect the output to check only the disassembler speed and not the time
// to display the disassembly.
// Example of output:
// T32 assembler: time: 0.001333s, cycles: 850453
// T32 disassembler: time: 0.166865s, cycles: 86770518

using namespace vixl;
using namespace vixl::aarch32;

#define __ assembler->

const int kBufferSize = 512 * KBytes;
const Register sl(10);
const Register fp(11);

class TestAssembler : public Assembler {
 public:
  TestAssembler() : Assembler(kBufferSize) {
    UseInstructionSet(T32);
    SetAllowAssembler(true);
  }
  void Emit8(uint8_t data) { GetBuffer()->Emit8(data); }
  void Emit16(uint16_t data) { GetBuffer()->Emit16(data); }
  void Emit32(uint32_t data) { GetBuffer()->Emit32(data); }
};

class Generator {
  Label l_0000, l_007c, l_0080, l_008a, l_009e, l_00a2, l_00a8, l_00ac;
  Label l_00b0, l_00d2, l_00dc, l_00e2, l_00e6, l_00ee, l_00f2, l_0100;
  Label l_0126, l_012a, l_0160, l_0166, l_016e, l_0172, l_018c, l_0196;
  Label l_019a, l_01a8, l_01b4, l_01c0, l_01ce, l_0212, l_0226, l_0238;
  Label l_028a, l_028e, l_02a4, l_02a8, l_02ac, l_02c6, l_02cc, l_02dc;
  Label l_030c, l_0334, l_035c, l_03ba, l_03be, l_03d4, l_03d8, l_03dc;
  Label l_0494, l_049a, l_04aa, l_04f8, l_04fc, l_0512, l_0516, l_051a;
  Label l_05d2, l_05d8, l_05f0, l_05f8, l_05fc, l_0600, l_0604, l_0622;
  Label l_066c, l_0670, l_0686, l_068a, l_068e, l_06b2, l_06b8, l_06c6;
  Label l_06f2, l_071a, l_077e, l_0782, l_0798, l_079c, l_07a8, l_07d0;
  Label l_07dc, l_088e, l_0894, l_08a4, l_08cc, l_096c, l_09d6, l_09dc;
  Label l_09e0, l_09e4, l_09e8, l_09fa, l_0a00, l_0aa2, l_0ad4, l_0aec;
  Label l_0afc, l_0b00, l_0b16, l_0b1a, l_0b1c, l_0b20, l_0b2e, l_0b36;
  Label l_0b42, l_0b6c, l_0b98, l_0c2e, l_0c60, l_0c8c, l_0cb8, l_0d10;
  Label l_0d18, l_0d1c, l_0d20, l_0d24, l_0d28, l_0df8, l_0e04, l_0e18;
  Label l_0e2e, l_0e44, l_0e88, l_0e8c, l_0e9e, l_0ea4, l_0ec0, l_0ec6;
  Label l_0ed8, l_0efc, l_0f4a, l_0f4e, l_0f60, l_0f66, l_0f94, l_0f9a;
  Label l_0fac, l_0fbe, l_1006, l_100a, l_101c, l_1022, l_1040, l_1048;
  Label l_1052, l_1064, l_1068, l_106c, l_10a6, l_10aa, l_10bc, l_10c2;
  Label l_10e4, l_10ec, l_10fe, l_1152, l_1156, l_1168, l_116e, l_118e;
  Label l_1196, l_11a0, l_11ac, l_11c4, l_11ce, l_120e, l_1212, l_1224;
  Label l_122a, l_12e2, l_12ea, l_12fc, l_133a, l_133e, l_1350, l_1356;
  Label l_13fc, l_1400, l_1404, l_1416, l_141e, l_1430, l_1472, l_1476;
  Label l_1488, l_148e, l_14b0, l_14b6, l_14c0, l_1552, l_1556, l_1568;
  Label l_156e, l_158a, l_1590, l_159a, l_15ae, l_15dc, l_15e8, l_15f2;
  Label l_1648, l_164c, l_165e, l_1664, l_1694, l_16a2, l_1724, l_1728;
  Label l_172c, l_175a, l_1760, l_1772, l_178c, l_17a0, l_17ce, l_17da;
  Label l_17f2, l_17f6, l_17f8, l_17fc, l_1826, l_1842, l_1856, l_1868;
  Label l_18ac, l_18b0, l_18c6, l_18ca, l_18ce, l_18e6, l_18ec, l_18f6;
  Label l_1938, l_193c, l_1952, l_1956, l_195a, l_1a18, l_1a1e, l_1a2e;
  Label l_1a36, l_1a54, l_1a72, l_1a90, l_1a98, l_1aa0, l_1aa8, l_1ab0;
  Label l_1ab8, l_1ac2, l_1b00, l_1b04, l_1b3a, l_1b3e, l_1b44, l_1b4a;
  Label l_1b68, l_1b88, l_1ba8, l_1bc8, l_1be8, l_1bec, l_1bf0, l_1c0e;
  Label l_1c2e, l_1c48, l_1c4e, l_1c54, l_1c5a, l_1c60, l_1c6a, l_1c72;
  Label l_1c78, l_1c80, l_1c86, l_1c8e, l_1c94, l_1c9c, l_1ca2, l_1caa;
  Label l_1cb0, l_1cb8, l_1cbe, l_1cc6, l_1ccc, l_1cd4, l_1cda, l_1ce2;
  Label l_1d00, l_1d24, l_1d2a, l_1d30, l_1d3a, l_1d42, l_1d4a, l_1d50;
  Label l_1d56, l_1d5c, l_1d62, l_1d6a, l_1d72, l_1d78, l_1d80, l_1d88;
  Label l_1d8a, l_1d92, l_1d9c, l_1dba, l_1dbe, l_1dc4, l_1dc8, l_1dd0;
  Label l_1dd4, l_1dda, l_1dde, l_1de8, l_1df2, l_1df6, l_1dfc, l_1e00;
  Label l_1e06, l_1e0a, l_1e10, l_1e16, l_1e1a, l_1e20, l_1e26, l_1e2c;
  Label l_1e38, l_1e3c, l_1e42, l_1e48, l_1e4e, l_1e52, l_1e5e, l_1e64;
  Label l_1e6a, l_1e70, l_1e76, l_1e7c, l_1e82, l_1e86, l_1e8c, l_1e96;
  Label l_1e9a, l_1ea0, l_1eaa, l_1eb0, l_1eb4, l_1eb8, l_1ebe, l_1ec8;
  Label l_1ed0, l_1ed6, l_1edc, l_1ee0, l_1ee6, l_1eec, l_1ef2, l_1ef6;
  Label l_1efc, l_1f02, l_1f0a, l_1f12, l_1f1a, l_1f22, l_1f26, l_1f2c;
  Label l_1f36, l_1f40, l_1f46, l_1f4c, l_1f50, l_1f56, l_1f5e, l_1f62;
  Label l_1f68, l_1f74, l_1f7e, l_1f84, l_1fb0, l_1fc2, l_1fcc, l_1fda;
  Label l_1fe0, l_1fe8, l_1ff4, l_1ffc, l_2028, l_202c, l_2044, l_2054;
  Label l_2058, l_2070, l_2080, l_2082, l_208a, l_20a4, l_20b4, l_20b6;
  Label l_20be, l_20d8, l_20dc, l_20f4, l_20f8, l_20fa, l_20fe, l_212c;
  Label l_213c, l_2158, l_2178, l_21a6, l_21ae, l_21f6, l_21fc, l_2220;
  Label l_2248, l_2256, l_225c, l_2282, l_2284, l_22a8, l_22b0, l_22c8;
  Label l_22de, l_22e0, l_22e6, l_2304, l_230c, l_2316, l_2334, l_233a;
  Label l_2374, l_2388, l_238c, l_238e, l_2394, l_23a2, l_23a8, l_23ac;
  Label l_23b4, l_23c8, l_23d8, l_23de, l_23e2, l_23ee, l_23f4, l_23fa;
  Label l_2400, l_2404, l_240a, l_2432, l_2446, l_2466, l_246a, l_2474;
  Label l_2484, l_248a, l_2490, l_24ba, l_24c4, l_24ca, l_24e6, l_2500;
  Label l_2510, l_2522, l_2528, l_255c, l_2564, l_256a, l_2592, l_25be;
  Label l_25c8, l_25ce, l_25d8, l_25da, l_25dc, l_25ec, l_25f4, l_25fa;
  Label l_2630, l_2660, l_2680, l_268e, l_2694, l_26b0, l_26b2, l_26c4;
  Label l_26e2, l_26f8, l_272a, l_2734, l_274a, l_2776, l_277e, l_2782;
  Label l_2794, l_279a, l_27a4, l_27dc, l_27e8, l_27f0, l_2808, l_280e;
  Label l_283c, l_2840, l_2846, l_284a, l_2864, l_286c, l_28a2, l_28f8;
  Label l_28fc, l_2906, l_2910, l_294c, l_2972, l_297c, l_2982, l_29a2;
  Label l_29d4, l_29e4, l_29f0, l_2a00, l_2a30, l_2a62, l_2a66, l_2a86;
  Label l_2aba, l_2ad2, l_2afc, l_2b02, l_2b2e, l_2b30, l_2b4e, l_2b76;
  Label l_2b7c, l_2b8a, l_2ba6, l_2bbc, l_2bda, l_2be2, l_2bfc, l_2c2e;
  Label l_2c44, l_2ca8, l_2cac, l_2ce2, l_2cf0, l_2d0c, l_2d16, l_2d6c;
  Label l_2d7a, l_2db8, l_2dfa, l_2e32, l_2e38, l_2e4a, l_2e9c, l_2ea4;
  Label l_2ebe, l_2ed0, l_2ee0, l_2f0a, l_2f20, l_2f26, l_2f3a, l_2f3e;
  Label l_2f9a, l_2fa4, l_2fce, l_2ff8, l_3020, l_302a, l_3030, l_3058;
  Label l_3072, l_3080, l_30a0, l_30b6, l_30ba, l_30e2, l_30ee, l_30f2;
  Label l_3124, l_315a, l_3164, l_3192, l_31a0, l_31ae, l_31be, l_31ce;
  Label l_31d2, l_31e8, l_31ec, l_31f2, l_31fe, l_3216, l_322c, l_3230;
  Label l_323c, l_3242, l_3292, l_329a, l_32a0, l_32a4, l_32ae, l_32b2;
  Label l_32da, l_32e8, l_3310, l_3316, l_3340, l_3346, l_3350, l_3354;
  Label l_3366, l_3384, l_33c8, l_33d2, l_33e0, l_33ee, l_33f2, l_33fa;
  Label l_340c, l_3410, l_342c, l_3430, l_343a, l_343e, l_3444, l_3448;
  Label l_3460, l_3466, l_346e, l_3480, l_3484, l_3492, l_34a6, l_34da;
  Label l_34e8, l_34f4, l_34f8, l_3506, l_350a, l_350c, l_3510, l_3518;
  Label l_3520, l_3536, l_3542, l_3550, l_3554, l_3558, l_35ac, l_35b0;
  Label l_35c6, l_35ca, l_35d0, l_3646, l_3652, l_3678, l_3688, l_3694;
  Label l_3698, l_36a4, l_36a8, l_36aa, l_36ae, l_36b6, l_36bc, l_36d6;
  Label l_36de, l_36e8, l_36f0, l_3706, l_3712, l_3718, l_371e, l_3722;
  Label l_3728, l_372c, l_3730, l_3736, l_3740, l_3744, l_3748, l_374c;
  Label l_37a0, l_37a4, l_37ba, l_37be, l_37c4, l_383a, l_384a, l_3870;
  Label l_3880, l_388c, l_3890, l_389c, l_38a0, l_38a2, l_38a6, l_38ae;
  Label l_38b4, l_38ce, l_38d6, l_38e0, l_38e8, l_38fe, l_390a, l_3910;
  Label l_3916, l_391a, l_3920, l_3924, l_3928, l_392e, l_3938, l_393c;
  Label l_3940, l_3944, l_3998, l_399c, l_39b2, l_39b6, l_39bc, l_3a32;
  Label l_3a42, l_3a68, l_3a78, l_3a84, l_3a88, l_3a94, l_3a98, l_3a9a;
  Label l_3a9e, l_3aa6, l_3aac, l_3ac6, l_3ace, l_3ad8, l_3ae0, l_3af6;
  Label l_3b02, l_3b08, l_3b0e, l_3b12, l_3b18, l_3b1c, l_3b20, l_3b26;
  Label l_3b30, l_3b34, l_3b38, l_3b3c, l_3ba0, l_3ba4, l_3bba, l_3bbe;
  Label l_3bc4, l_3c46, l_3c6c, l_3c7a, l_3c86, l_3c8a, l_3c96, l_3c9a;
  Label l_3c9c, l_3ca0, l_3ca8, l_3cae, l_3cc8, l_3cd0, l_3ce6, l_3cf2;
  Label l_3cf8, l_3d02, l_3d06, l_3d0c, l_3d10, l_3d16, l_3d1c, l_3d20;
  Label l_3d24, l_3d28, l_3d7e, l_3d82, l_3d98, l_3d9c, l_3da2, l_3e18;
  Label l_3e26, l_3e4c, l_3e5c, l_3e68, l_3e6c, l_3e78, l_3e7c, l_3e7e;
  Label l_3e82, l_3e8a, l_3e90, l_3eaa, l_3eb2, l_3ebc, l_3ec4, l_3eda;
  Label l_3ee6, l_3eec, l_3ef2, l_3ef6, l_3efc, l_3f00, l_3f04, l_3f0a;
  Label l_3f14, l_3f18, l_3f1c, l_3f20, l_3f78, l_3f7c, l_3f92, l_3f96;
  Label l_3f9c, l_3fd0, l_3fe6, l_4000, l_400a, l_400c, l_407a, l_4084;
  Label l_40ac, l_40bc, l_40c8, l_40cc, l_40d8, l_40dc, l_40de, l_40e2;
  Label l_40ec, l_40f2, l_4104, l_410c, l_4118, l_4126, l_4142, l_4146;
  Label l_4160, l_4164, l_416c, l_4172, l_417a, l_4184, l_419a, l_41a6;
  Label l_41ac, l_41b4, l_41c4, l_41c8, l_41ce, l_41d2, l_41d6, l_41dc;
  Label l_41e4, l_41e8, l_41ec, l_41f0, l_424a, l_424e, l_4264, l_4268;
  Label l_426e, l_4278, l_4282, l_429c, l_42a0, l_42b6, l_42e0, l_4304;
  Label l_4310, l_4312, l_4384, l_4390, l_43b6, l_43c6, l_43d2, l_43d6;
  Label l_43e2, l_43e6, l_43e8, l_43ec, l_43f6, l_43fc, l_4404, l_4424;
  Label l_4436, l_443e, l_444c, l_4454, l_445e, l_4462, l_446a, l_4470;
  Label l_4478, l_4482, l_4498, l_44a4, l_44aa, l_44b2, l_44d4, l_44f6;
  Label l_44fc, l_4500, l_4508, l_450c, l_4512, l_451c, l_4520, l_4524;
  Label l_4528, l_4542, l_454e, l_4554, l_4558, l_455c, l_4560, l_4564;
  Label l_456c, l_4578, l_457c, l_4580, l_4584, l_458c, l_4590, l_4594;
  Label l_4598, l_45a0, l_45b4, l_45f6, l_4684, l_468a, l_469e, l_46be;
  Label l_46c8, l_46d0, l_46ea, l_46f4, l_46fa, l_4704, l_470a, l_4710;
  Label l_4714, l_471c, l_4724, l_4728, l_472c, l_4730, l_4b0c, l_4b22;
  Label l_4b2e, l_4b34, l_4b38, l_4b3c, l_4b40, l_4b44, l_4b4c, l_4b52;
  Label l_4b5e, l_4b62, l_4b66, l_4b6a, l_4b72, l_4b76, l_4b7c, l_4b80;
  void Generate_1(TestAssembler* assembler) {
    // --------------
    __ bind(&l_0000);
    __ push(Narrow, RegisterList(0x40f8));
    __ mov(Narrow, r7, r2);
    __ ldr(Narrow, r6, MemOperand(r7));
    __ mov(Narrow, r4, r0);
    __ movs(Narrow, r3, 0U);
    __ movs(Narrow, r5, 1U);
    __ ldr(Narrow, r2, &l_00a8);
    __ movs(Narrow, r0, 4U);
    __ cmp(Narrow, r6, 14U);
    __ stm(r4, NO_WRITE_BACK, RegisterList(r1, r6));
    __ str(Narrow, r0, MemOperand(r4, 12));
    __ str(Narrow, r5, MemOperand(r4, 16));
    __ str(Narrow, r5, MemOperand(r4, 20));
    __ strb(r5, MemOperand(r4, 48));
    __ str(Narrow, r5, MemOperand(r4, 64));
    __ str(Narrow, r5, MemOperand(r4, 88));
    __ str(Narrow, r5, MemOperand(r4, 112));
    __ str(r5, MemOperand(r4, 136));
    __ strb(Narrow, r3, MemOperand(r4, 24));
    __ str(Narrow, r3, MemOperand(r4, 28));
    __ strb(r3, MemOperand(r4, 32));
    __ strb(r3, MemOperand(r4, 36));
    __ str(Narrow, r3, MemOperand(r4, 52));
    __ str(Narrow, r3, MemOperand(r4, 56));
    __ str(Narrow, r3, MemOperand(r4, 60));
    __ str(Narrow, r3, MemOperand(r4, 68));
    __ str(Narrow, r3, MemOperand(r4, 72));
    __ str(Narrow, r3, MemOperand(r4, 76));
    __ str(Narrow, r3, MemOperand(r4, 80));
    __ str(Narrow, r3, MemOperand(r4, 84));
    __ str(Narrow, r3, MemOperand(r4, 92));
    __ str(Narrow, r3, MemOperand(r4, 96));
    __ str(Narrow, r3, MemOperand(r4, 100));
    __ str(Narrow, r3, MemOperand(r4, 104));
    __ str(Narrow, r3, MemOperand(r4, 108));
    __ str(Narrow, r3, MemOperand(r4, 116));
    __ str(Narrow, r3, MemOperand(r4, 120));
    __ str(Narrow, r3, MemOperand(r4, 124));
    __ str(r3, MemOperand(r4, 128));
    __ str(r3, MemOperand(r4, 132));
    __ str(r3, MemOperand(r4, 140));
    __ str(r3, MemOperand(r4, 144));
    __ str(r3, MemOperand(r4, 148));
    __ str(r3, MemOperand(r4, 152));
    __ strb(r3, MemOperand(r4, 160));
    __ str(Narrow, r2, MemOperand(r4, 8));
    __ b(eq, Narrow, &l_007c);
    __ ldr(Narrow, r2, MemOperand(r1, 28));
    __ cmp(Narrow, r2, r5);
    __ b(eq, Narrow, &l_0080);
    // --------------
    __ bind(&l_007c);
    __ mov(Narrow, r0, r4);
    __ pop(Narrow, RegisterList(0x80f8));
    // --------------
    __ bind(&l_0080);
    __ mov(Narrow, r0, r1);
    __ add(r3, r4, 8U);
    __ eor(r1, r6, 0x1);
    // --------------
    __ bind(&l_008a);
    __ bl(&l_008a);
    __ movs(Narrow, r3, 14U);
    __ mov(Narrow, r0, r4);
    __ str(Narrow, r3, MemOperand(r7));
    __ pop(Narrow, RegisterList(0x80f8));
    __ ldr(Narrow, r3, &l_00ac);
    __ add(r0, r4, 36U);
    __ str(Narrow, r3, MemOperand(r4, 8));
    // --------------
    __ bind(&l_009e);
    __ bl(&l_009e);
    // --------------
    __ bind(&l_00a2);
    __ bl(&l_00a2);
    __ nop(Narrow);
    // --------------
    __ bind(&l_00a8);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_00ac);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_00b0);
    __ push(RegisterList(0x4ff8));
    __ mov(Narrow, r6, r1);
    __ ldr(Narrow, r4, MemOperand(r0));
    __ mov(Narrow, r7, r0);
    __ mov(Narrow, r5, r2);
    __ ldr(Narrow, r1, MemOperand(r0, 4));
    __ cmp(Narrow, r4, r1);
    __ b(eq, &l_01ce);
    __ mov(Narrow, r9, r3);
    __ ldr(Narrow, r3, MemOperand(sp, 40));
    __ cmp(Narrow, r3, 0U);
    __ b(eq, Narrow, &l_01a8);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ cmp(Narrow, r2, r3);
    __ b(lt, Narrow, &l_01c0);
    // --------------
    __ bind(&l_00d2);
    __ ldr(Narrow, r3, MemOperand(r4));
    __ cbz(r3, &l_00e6);
    __ add(r3, r4, 24U);
    __ b(Narrow, &l_00e2);
    // --------------
    __ bind(&l_00dc);
    __ ldr(r2, MemOperand(r3, 24, PostIndex));
    __ cbz(r2, &l_00e6);
    // --------------
    __ bind(&l_00e2);
    __ cmp(Narrow, r1, r3);
    __ b(ne, Narrow, &l_00dc);
    // --------------
    __ bind(&l_00e6);
    __ movw(sl, 0U);
    __ movt(sl, 0U);
    // --------------
    __ bind(&l_00ee);
    __ cmp(Narrow, r1, r4);
    __ b(eq, Narrow, &l_016e);
    // --------------
    __ bind(&l_00f2);
    __ ldr(Narrow, r3, MemOperand(r7, 40));
    __ add(r0, r5, r9);
    __ ldr(Narrow, r2, MemOperand(r4, 8));
    __ add(Narrow, r3, r3, r0);
    __ cmp(Narrow, r2, r3);
  }
  void Generate_2(TestAssembler* assembler) {
    __ b(gt, Narrow, &l_018c);
    // --------------
    __ bind(&l_0100);
    __ ldr(Narrow, r3, MemOperand(r4, 12));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r2, MemOperand(r6));
    __ ldr(fp, MemOperand(r4));
    __ subs(r3, 1U);
    __ add(r8, r5, r3);
    __ ldr(Narrow, r2, MemOperand(r2, 44));
    __ bic(r8, r8, r3);
    __ rsb(r1, r5, r8);
    __ blx(r2);
    __ ldr(Narrow, r3, MemOperand(r6));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3));
    __ cmp(Narrow, r3, sl);
    __ b(ne, Narrow, &l_0196);
    // --------------
    __ bind(&l_0126);
    __ bl(&l_0126);
    // --------------
    __ bind(&l_012a);
    __ ldr(r3, MemOperand(fp));
    __ mov(Narrow, r1, r0);
    __ str(r8, MemOperand(fp, 20));
    __ mov(Narrow, r0, fp);
    __ ldr(Narrow, r3, MemOperand(r3, 20));
    __ blx(r3);
    __ ldr(r3, MemOperand(fp));
    __ mov(Narrow, r0, fp);
    __ mov(Narrow, r1, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 16));
    __ blx(r3);
    __ ldr(r3, MemOperand(fp));
    __ mov(Narrow, r0, fp);
    __ ldr(r5, MemOperand(fp, 4));
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ cbz(r0, &l_019a);
    __ movs(Narrow, r3, 1U);
    __ mov(Narrow, r1, r4);
    __ strb(r3, MemOperand(fp, 16));
    __ mov(Narrow, r0, r7);
    // --------------
    __ bind(&l_0160);
    __ bl(&l_0160);
    __ mov(Narrow, r4, r0);
    // --------------
    __ bind(&l_0166);
    __ ldr(Narrow, r1, MemOperand(r7, 4));
    __ add(Narrow, r5, r5, r8);
    __ cmp(Narrow, r1, r4);
    __ b(ne, Narrow, &l_00f2);
    // --------------
    __ bind(&l_016e);
    __ mov(Narrow, r0, r7);
    __ movs(Narrow, r1, 0U);
    // --------------
    __ bind(&l_0172);
    __ bl(&l_0172);
    __ ldr(Narrow, r3, MemOperand(r6));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 40));
    __ blx(r3);
    __ ldr(Narrow, r0, MemOperand(r7, 28));
    __ subs(r0, 1U);
    __ add(Narrow, r5, r5, r0);
    __ bic(r0, r5, r0);
    __ pop(RegisterList(0x8ff8));
    // --------------
    __ bind(&l_018c);
    __ ldr(Narrow, r3, MemOperand(r4, 16));
    __ cmp(Narrow, r5, r3);
    __ b(ge, Narrow, &l_0100);
    __ adds(Narrow, r4, r4, 24U);
    __ b(Narrow, &l_00ee);
    // --------------
    __ bind(&l_0196);
    __ blx(r3);
    __ b(Narrow, &l_012a);
    // --------------
    __ bind(&l_019a);
    __ ldr(Narrow, r0, MemOperand(r4));
    __ mov(Narrow, r1, r4);
    __ adds(Narrow, r4, r4, 24U);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_0166);
    // --------------
    __ bind(&l_01a8);
    __ ldr(Narrow, r3, MemOperand(r6));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 36));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r7, 24));
    __ add(Narrow, r5, r5, r3);
    // --------------
    __ bind(&l_01b4);
    __ ldr(Narrow, r4, MemOperand(r7));
    __ ldr(Narrow, r1, MemOperand(r7, 4));
    __ cmp(Narrow, r4, r1);
    __ b(ne, Narrow, &l_00d2);
    __ mov(Narrow, r1, r4);
    __ b(Narrow, &l_00e6);
    // --------------
    __ bind(&l_01c0);
    __ ldr(Narrow, r2, MemOperand(r6));
    __ subs(Narrow, r1, r3, r5);
    __ mov(Narrow, r0, r6);
    __ mov(Narrow, r5, r3);
    __ ldr(Narrow, r3, MemOperand(r2, 48));
    __ blx(r3);
    __ b(Narrow, &l_01b4);
    // --------------
    __ bind(&l_01ce);
    __ mov(Narrow, r0, r2);
    __ pop(RegisterList(0x8ff8));
    __ push(RegisterList(0x4ff0));
    __ movw(r7, 0U);
    __ movt(r7, 0U);
    __ sub(sp, sp, 2476U);
    __ mov(Narrow, r4, r0);
    __ ldr(Narrow, r3, MemOperand(r7, 4));
    __ ldr(Narrow, r5, MemOperand(r7, 24));
    __ ldr(Narrow, r0, MemOperand(r7, 28));
    __ str(Narrow, r3, MemOperand(sp, 64));
  }
  void Generate_3(TestAssembler* assembler) {
    __ ldr(Narrow, r3, MemOperand(r7, 8));
    __ ubfx(r2, r5, 5U, 4U);
    __ ldr(Narrow, r1, MemOperand(r7, 32));
    __ str(Narrow, r3, MemOperand(sp, 36));
    __ ldr(Narrow, r3, MemOperand(r7, 12));
    __ str(Narrow, r3, MemOperand(sp, 60));
    __ ldr(Narrow, r3, MemOperand(r7, 16));
    __ str(Narrow, r3, MemOperand(sp, 56));
    __ ldr(Narrow, r3, MemOperand(r7, 20));
    __ str(Narrow, r3, MemOperand(sp, 52));
    __ cbz(r2, &l_0212);
    __ and_(r5, r5, 0x1f);
    __ mov(r8, 1U);
    __ lsl(r2, r8, r5);
    // --------------
    __ bind(&l_0212);
    __ ubfx(r3, r0, 5U, 4U);
    __ mov(Narrow, r8, r3);
    __ cbz(r3, &l_0226);
    __ and_(r0, r0, 0x1f);
    __ mov(r8, 1U);
    __ lsl(r8, r8, r0);
    // --------------
    __ bind(&l_0226);
    __ ubfx(r3, r1, 5U, 4U);
    __ orr(r8, r8, r2);
    __ cbz(r3, &l_0238);
    __ and_(r1, r1, 0x1f);
    __ movs(Narrow, r3, 1U);
    __ lsls(Narrow, r3, r3, r1);
    // --------------
    __ bind(&l_0238);
    __ movs(Narrow, r1, 14U);
    __ ldr(r9, &l_0600);
    __ str(Narrow, r1, MemOperand(sp, 84));
    __ adds(r1, r4, 44U);
    __ add(r5, sp, 2136U);
    __ mov(r2, 0U);
    __ orr(r8, r8, r3);
    __ str(r9, MemOperand(r5));
    __ str(Narrow, r2, MemOperand(r5, 4));
    __ mov(Narrow, r6, r1);
    __ strb(Narrow, r2, MemOperand(r5, 17));
    __ str(Narrow, r1, MemOperand(sp, 20));
    __ b(eq, Narrow, &l_02ac);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r1, MemOperand(r5, 20));
    __ mov(Narrow, r0, r1);
    __ str(Narrow, r2, MemOperand(r5, 24));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1aa8);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_028a);
    __ bl(&l_028a);
    // --------------
    __ bind(&l_028e);
    __ str(Narrow, r0, MemOperand(r5, 4));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_02a8);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_02a4);
    __ bl(&l_02a4);
    // --------------
    __ bind(&l_02a8);
    __ movs(Narrow, r3, 1U);
    __ strb(Narrow, r3, MemOperand(r5, 17));
    // --------------
    __ bind(&l_02ac);
    __ ldr(Narrow, r3, &l_05f8);
    __ add(fp, sp, 2304U);
    __ mov(Narrow, r0, fp);
    __ mov(Narrow, r1, r4);
    __ add(Narrow, r2, sp, 84U);
    __ str(Narrow, r3, MemOperand(r5));
    __ bl(&l_0000);
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r1, MemOperand(sp, 84));
    __ mov(Narrow, r3, r8);
    __ movs(Narrow, r2, 0U);
    // --------------
    __ bind(&l_02c6);
    __ bl(&l_02c6);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_02cc);
    __ bl(&l_02cc);
    __ ldrb(Narrow, r3, MemOperand(r5, 17));
    __ str(r9, MemOperand(r5));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1a72);
    // --------------
    __ bind(&l_02dc);
    __ ldr(Narrow, r3, MemOperand(r7));
    __ add(r6, sp, 2304U);
    __ vmov(F64, d7, 0.12500);
    __ mov(sl, 832U);
    __ mov(Narrow, r0, r4);
    __ str(fp, MemOperand(sp));
    __ str(r3, MemOperand(sp, 2320));
    __ movs(Narrow, r1, 14U);
    __ vstr(d7, MemOperand(r6));
  }
  void Generate_4(TestAssembler* assembler) {
    __ mov(r2, 800U);
    __ ldr(Narrow, r3, MemOperand(sp, 60));
    __ add(r6, sp, 2304U);
    __ str(sl, MemOperand(sp, 2312));
    __ mov(r8, 0U);
    // --------------
    __ bind(&l_030c);
    __ bl(&l_030c);
    __ vldr(d7, &l_05f0);
    __ mov(Narrow, r0, r4);
    __ str(fp, MemOperand(sp));
    __ movs(Narrow, r1, 14U);
    __ str(sl, MemOperand(sp, 2312));
    __ mov(r2, 800U);
    __ vstr(d7, MemOperand(r6));
    __ add(sl, sp, 1968U);
    __ ldr(Narrow, r6, MemOperand(r7));
    __ ldr(Narrow, r3, MemOperand(sp, 56));
    __ str(r6, MemOperand(sp, 2320));
    // --------------
    __ bind(&l_0334);
    __ bl(&l_0334);
    __ ldr(Narrow, r3, MemOperand(r7, 36));
    __ movs(Narrow, r1, 14U);
    __ ldr(lr, MemOperand(r7, 40));
    __ mov(Narrow, r0, r4);
    __ str(r8, MemOperand(sp, 2304));
    __ mov(Narrow, r2, r3);
    __ str(Narrow, r3, MemOperand(sp, 72));
    __ mov(Narrow, r3, fp);
    __ str(r8, MemOperand(sp, 2312));
    __ str(lr, MemOperand(sp, 2308));
    __ str(lr, MemOperand(sp, 2320));
    __ str(r8, MemOperand(sp, 2316));
    // --------------
    __ bind(&l_035c);
    __ bl(&l_035c);
    __ mov(r3, 1056U);
    __ ldr(Narrow, r1, MemOperand(r7));
    __ str(r3, MemOperand(sl, 8));
    __ movs(Narrow, r2, 42U);
    __ ldr(Narrow, r3, MemOperand(r7, 44));
    __ str(r8, MemOperand(r5, 4));
    __ strb(r8, MemOperand(r5, 17));
    __ str(Narrow, r3, MemOperand(sp, 48));
    __ ldr(Narrow, r3, MemOperand(r7, 48));
    __ str(r9, MemOperand(r5));
    __ str(r1, MemOperand(sl, 16));
    __ str(Narrow, r3, MemOperand(sp, 44));
    __ ldr(Narrow, r3, MemOperand(sp, 20));
    __ str(r2, MemOperand(sl));
    __ cbz(r3, &l_03dc);
    __ mov(Narrow, r6, r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r6, MemOperand(r5, 20));
    __ str(Narrow, r2, MemOperand(r5, 24));
    __ mov(Narrow, r0, r6);
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1ab0);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_03ba);
    __ bl(&l_03ba);
    // --------------
    __ bind(&l_03be);
    __ str(Narrow, r0, MemOperand(r5, 4));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_03d8);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_03d4);
    __ bl(&l_03d4);
    // --------------
    __ bind(&l_03d8);
    __ movs(Narrow, r3, 1U);
    __ strb(Narrow, r3, MemOperand(r5, 17));
    // --------------
    __ bind(&l_03dc);
    __ ldr(Narrow, r1, &l_05f8);
    __ mov(lr, 0U);
    __ ldr(Narrow, r6, &l_05fc);
    __ mov(ip, 1U);
    __ movs(Narrow, r3, 14U);
    __ movs(Narrow, r2, 4U);
    __ str(Narrow, r1, MemOperand(r5));
    __ mov(Narrow, r0, r4);
    __ str(r3, MemOperand(sp, 2308));
    __ mov(Narrow, r1, r3);
    __ str(r2, MemOperand(sp, 2316));
    __ mov(r2, 1032U);
    __ str(r4, MemOperand(sp, 2304));
    __ str(ip, MemOperand(sp, 2320));
    __ ldr(Narrow, r3, MemOperand(sp, 48));
    __ str(ip, MemOperand(sp, 2324));
    __ strb(ip, MemOperand(sp, 2352));
    __ str(ip, MemOperand(sp, 2368));
    __ str(ip, MemOperand(sp, 2392));
  }
  void Generate_5(TestAssembler* assembler) {
    __ str(ip, MemOperand(sp, 2416));
    __ str(ip, MemOperand(sp, 2440));
    __ strb(lr, MemOperand(sp, 2328));
    __ str(lr, MemOperand(sp, 2332));
    __ strb(lr, MemOperand(sp, 2336));
    __ strb(lr, MemOperand(sp, 2340));
    __ str(lr, MemOperand(sp, 2356));
    __ str(lr, MemOperand(sp, 2360));
    __ str(lr, MemOperand(sp, 2364));
    __ str(lr, MemOperand(sp, 2372));
    __ str(lr, MemOperand(sp, 2376));
    __ str(lr, MemOperand(sp, 2380));
    __ str(lr, MemOperand(sp, 2384));
    __ str(lr, MemOperand(sp, 2388));
    __ str(lr, MemOperand(sp, 2396));
    __ str(lr, MemOperand(sp, 2400));
    __ str(lr, MemOperand(sp, 2404));
    __ str(lr, MemOperand(sp, 2408));
    __ str(lr, MemOperand(sp, 2412));
    __ str(lr, MemOperand(sp, 2420));
    __ str(lr, MemOperand(sp, 2424));
    __ str(lr, MemOperand(sp, 2428));
    __ str(lr, MemOperand(sp, 2432));
    __ str(lr, MemOperand(sp, 2436));
    __ str(sl, MemOperand(sp));
    __ str(lr, MemOperand(sp, 2444));
    __ str(lr, MemOperand(sp, 2448));
    __ str(lr, MemOperand(sp, 2452));
    __ str(lr, MemOperand(sp, 2456));
    __ strb(lr, MemOperand(sp, 2464));
    __ str(r6, MemOperand(sp, 2312));
    // --------------
    __ bind(&l_0494);
    __ bl(&l_0494);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_049a);
    __ bl(&l_049a);
    __ ldrb(Narrow, r3, MemOperand(r5, 17));
    __ str(r9, MemOperand(r5));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1a54);
    // --------------
    __ bind(&l_04aa);
    __ movs(Narrow, r3, 0U);
    __ ldr(Narrow, r0, MemOperand(r7));
    __ str(Narrow, r3, MemOperand(r5, 4));
    __ movs(Narrow, r1, 32U);
    __ strb(Narrow, r3, MemOperand(r5, 17));
    __ mov(r2, 1056U);
    __ ldr(Narrow, r3, MemOperand(sp, 20));
    __ str(r9, MemOperand(r5));
    __ str(r1, MemOperand(sl));
    __ str(r0, MemOperand(sl, 16));
    __ str(r2, MemOperand(sl, 8));
    __ cbz(r3, &l_051a);
    __ mov(Narrow, r6, r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r2, MemOperand(r5, 24));
    __ str(Narrow, r6, MemOperand(r5, 20));
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1a98);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_04f8);
    __ bl(&l_04f8);
    // --------------
    __ bind(&l_04fc);
    __ str(Narrow, r0, MemOperand(r5, 4));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_0516);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_0512);
    __ bl(&l_0512);
    // --------------
    __ bind(&l_0516);
    __ movs(Narrow, r3, 1U);
    __ strb(Narrow, r3, MemOperand(r5, 17));
    // --------------
    __ bind(&l_051a);
    __ ldr(Narrow, r1, &l_05f8);
    __ mov(lr, 0U);
    __ ldr(Narrow, r6, &l_05fc);
    __ mov(ip, 1U);
    __ movs(Narrow, r3, 14U);
    __ movs(Narrow, r2, 4U);
    __ str(Narrow, r1, MemOperand(r5));
    __ mov(Narrow, r0, r4);
    __ str(r3, MemOperand(sp, 2308));
    __ mov(Narrow, r1, r3);
    __ str(r2, MemOperand(sp, 2316));
    __ mov(r2, 1032U);
    __ str(r4, MemOperand(sp, 2304));
    __ str(ip, MemOperand(sp, 2320));
    __ ldr(Narrow, r3, MemOperand(sp, 44));
    __ str(ip, MemOperand(sp, 2324));
  }
  void Generate_6(TestAssembler* assembler) {
    __ strb(ip, MemOperand(sp, 2352));
    __ str(ip, MemOperand(sp, 2368));
    __ str(ip, MemOperand(sp, 2392));
    __ str(ip, MemOperand(sp, 2416));
    __ str(ip, MemOperand(sp, 2440));
    __ strb(lr, MemOperand(sp, 2328));
    __ str(lr, MemOperand(sp, 2332));
    __ strb(lr, MemOperand(sp, 2336));
    __ strb(lr, MemOperand(sp, 2340));
    __ str(lr, MemOperand(sp, 2356));
    __ str(lr, MemOperand(sp, 2360));
    __ str(lr, MemOperand(sp, 2364));
    __ str(lr, MemOperand(sp, 2372));
    __ str(lr, MemOperand(sp, 2376));
    __ str(lr, MemOperand(sp, 2380));
    __ str(lr, MemOperand(sp, 2384));
    __ str(lr, MemOperand(sp, 2388));
    __ str(lr, MemOperand(sp, 2396));
    __ str(lr, MemOperand(sp, 2400));
    __ str(lr, MemOperand(sp, 2404));
    __ str(lr, MemOperand(sp, 2408));
    __ str(lr, MemOperand(sp, 2412));
    __ str(lr, MemOperand(sp, 2420));
    __ str(lr, MemOperand(sp, 2424));
    __ str(lr, MemOperand(sp, 2428));
    __ str(lr, MemOperand(sp, 2432));
    __ str(lr, MemOperand(sp, 2436));
    __ str(sl, MemOperand(sp));
    __ str(lr, MemOperand(sp, 2444));
    __ str(lr, MemOperand(sp, 2448));
    __ str(lr, MemOperand(sp, 2452));
    __ str(lr, MemOperand(sp, 2456));
    __ strb(lr, MemOperand(sp, 2464));
    __ str(r6, MemOperand(sp, 2312));
    // --------------
    __ bind(&l_05d2);
    __ bl(&l_05d2);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_05d8);
    __ bl(&l_05d8);
    __ ldrb(Narrow, r3, MemOperand(r5, 17));
    __ str(r9, MemOperand(r5));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1a36);
    __ b(Narrow, &l_0604);
    __ nop(Narrow);
    __ nop(Wide);
    // --------------
    __ bind(&l_05f0);
    __ Emit32(0x00000000);
    __ Emit32(0x3fb00000);
    // --------------
    __ bind(&l_05f8);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_05fc);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_0600);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_0604);
    __ ldr(Narrow, r3, MemOperand(r7, 48));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r1, MemOperand(r7, 52));
    __ add(r8, sp, 120U);
    __ vmov(F32, s0, -2.0000);
    __ and_(r3, r3, 0x1f);
    __ orr(r3, r3, 0x8000);
    __ orr(r3, r3, 0x60);
    __ mov(Narrow, r6, r3);
    __ str(Narrow, r3, MemOperand(sp, 76));
    // --------------
    __ bind(&l_0622);
    __ bl(&l_0622);
    __ movs(Narrow, r3, 0U);
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r3, MemOperand(r5, 4));
    __ movs(Narrow, r1, 14U);
    __ strb(Narrow, r3, MemOperand(r5, 17));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(sp, 20));
    __ str(r9, MemOperand(r5));
    __ stm(r8, NO_WRITE_BACK, RegisterList(r0, r2));
    __ str(Narrow, r1, MemOperand(sp, 88));
    __ cbz(r3, &l_068e);
    __ mov(Narrow, r6, r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ str(Narrow, r2, MemOperand(r5, 24));
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r6, MemOperand(r5, 20));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1a90);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_066c);
    __ bl(&l_066c);
    // --------------
    __ bind(&l_0670);
    __ str(Narrow, r0, MemOperand(r5, 4));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_068a);
    __ lsls(Narrow, r1, r1, 1U);
  }
  void Generate_7(TestAssembler* assembler) {
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_0686);
    __ bl(&l_0686);
    // --------------
    __ bind(&l_068a);
    __ movs(Narrow, r3, 1U);
    __ strb(Narrow, r3, MemOperand(r5, 17));
    // --------------
    __ bind(&l_068e);
    __ ldr(Narrow, r3, &l_09dc);
    __ mov(Narrow, r0, fp);
    __ mov(Narrow, r1, r4);
    __ add(Narrow, r2, sp, 88U);
    __ str(Narrow, r3, MemOperand(r5));
    __ bl(&l_0000);
    __ ldm(r8, NO_WRITE_BACK, RegisterList(r0, r1));
    __ mov(r2, 1568U);
    __ ldr(lr, MemOperand(sp, 88));
    __ ldr(Narrow, r3, MemOperand(sp, 36));
    __ stm(sp, NO_WRITE_BACK, RegisterList(r0, r1));
    __ mov(Narrow, r0, r4);
    __ mov(Narrow, r1, lr);
    // --------------
    __ bind(&l_06b2);
    __ bl(&l_06b2);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_06b8);
    __ bl(&l_06b8);
    __ ldr(Narrow, r3, &l_09dc);
    __ mov(Narrow, r0, r5);
    __ mov(r8, 0U);
    __ str(Narrow, r3, MemOperand(r5));
    // --------------
    __ bind(&l_06c6);
    __ bl(&l_06c6);
    __ ldr(Narrow, r3, MemOperand(r7));
    __ mov(lr, 832U);
    __ vmov(F64, d7, 4.0000);
    __ mov(Narrow, r0, r4);
    __ str(fp, MemOperand(sp));
    __ movs(Narrow, r1, 14U);
    __ str(r3, MemOperand(sp, 2320));
    __ add(r3, sp, 2304U);
    __ vstr(d7, MemOperand(r3));
    __ mov(r2, 800U);
    __ ldr(Narrow, r3, MemOperand(sp, 52));
    __ str(lr, MemOperand(sp, 2312));
    // --------------
    __ bind(&l_06f2);
    __ bl(&l_06f2);
    __ ldr(lr, MemOperand(r7, 40));
    __ mov(ip, 64U);
    __ ldr(Narrow, r2, MemOperand(r7, 24));
    __ mov(Narrow, r3, fp);
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 14U);
    __ str(r8, MemOperand(sp, 2312));
    __ str(r8, MemOperand(sp, 2316));
    __ str(lr, MemOperand(sp, 2308));
    __ str(lr, MemOperand(sp, 2320));
    __ str(ip, MemOperand(sp, 2304));
    // --------------
    __ bind(&l_071a);
    __ bl(&l_071a);
    __ movs(Narrow, r2, 132U);
    __ ldr(Narrow, r3, MemOperand(r7, 40));
    __ str(r2, MemOperand(sl));
    __ ldr(Narrow, r2, MemOperand(sp, 20));
    __ str(r8, MemOperand(sl, 8));
    __ str(r8, MemOperand(sl, 12));
    __ str(r8, MemOperand(r5, 4));
    __ strb(r8, MemOperand(r5, 17));
    __ str(r3, MemOperand(sl, 4));
    __ str(r3, MemOperand(sl, 16));
    __ str(r9, MemOperand(r5));
    __ ldr(Narrow, r3, &l_09dc);
    __ ldr(r8, MemOperand(r7, 56));
    __ cmp(Narrow, r2, 0U);
    __ b(eq, &l_1ab8);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r6, r2);
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r6, MemOperand(r5, 20));
    __ str(Narrow, r2, MemOperand(r5, 24));
    __ mov(Narrow, r0, r6);
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1aa0);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_077e);
    __ bl(&l_077e);
    // --------------
    __ bind(&l_0782);
    __ str(Narrow, r0, MemOperand(r5, 4));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_079c);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_0798);
    __ bl(&l_0798);
    // --------------
    __ bind(&l_079c);
    __ movs(Narrow, r3, 1U);
  }
  void Generate_8(TestAssembler* assembler) {
    __ ldr(Narrow, r2, &l_09dc);
    __ strb(Narrow, r3, MemOperand(r5, 17));
    __ and_(r3, r8, 0x1f);
    __ str(Narrow, r2, MemOperand(r5));
    // --------------
    __ bind(&l_07a8);
    __ ldr(r0, MemOperand(sl, 4));
    __ ubfx(r1, r0, 5U, 4U);
    __ cmp(Narrow, r1, 0U);
    __ b(ne, &l_1ac2);
    __ ldr(r2, MemOperand(sl));
    __ cmp(Narrow, r2, 0U);
    __ b(eq, &l_1d88);
    __ cmp(Narrow, r2, 7U);
    __ b(hi, &l_1b3e);
    __ cmp(Narrow, r3, 7U);
    __ it(ls);
    __ mov(ls, ip, 1U);
    __ b(ls, Narrow, &l_07dc);
    // --------------
    __ bind(&l_07d0);
    __ cmp(Narrow, r3, 7U);
    __ ite(hi);
    __ mov(hi, ip, 0U);
    __ mov(ls, ip, 1U);
    // --------------
    __ bind(&l_07dc);
    __ ldr(Narrow, r6, &l_09e0);
    __ mov(lr, 0U);
    __ movs(Narrow, r3, 1U);
    __ movs(Narrow, r2, 4U);
    __ movs(Narrow, r1, 14U);
    __ str(r2, MemOperand(sp, 2316));
    __ str(r3, MemOperand(sp, 2320));
    __ mov(Narrow, r2, lr);
    __ str(r3, MemOperand(sp, 2324));
    __ mov(Narrow, r0, r4);
    __ strb(r3, MemOperand(sp, 2352));
    __ str(r3, MemOperand(sp, 2368));
    __ str(r3, MemOperand(sp, 2392));
    __ str(r3, MemOperand(sp, 2416));
    __ str(r3, MemOperand(sp, 2440));
    __ mov(Narrow, r3, r8);
    __ str(r4, MemOperand(sp, 2304));
    __ str(r1, MemOperand(sp, 2308));
    __ strb(lr, MemOperand(sp, 2328));
    __ str(lr, MemOperand(sp, 2332));
    __ strb(lr, MemOperand(sp, 2336));
    __ strb(lr, MemOperand(sp, 2340));
    __ str(lr, MemOperand(sp, 2356));
    __ str(lr, MemOperand(sp, 2360));
    __ str(lr, MemOperand(sp, 2364));
    __ str(lr, MemOperand(sp, 2372));
    __ str(lr, MemOperand(sp, 2376));
    __ str(lr, MemOperand(sp, 2380));
    __ str(lr, MemOperand(sp, 2384));
    __ str(lr, MemOperand(sp, 2388));
    __ str(lr, MemOperand(sp, 2396));
    __ str(lr, MemOperand(sp, 2400));
    __ str(lr, MemOperand(sp, 2404));
    __ str(lr, MemOperand(sp, 2408));
    __ str(lr, MemOperand(sp, 2412));
    __ str(lr, MemOperand(sp, 2420));
    __ str(lr, MemOperand(sp, 2424));
    __ str(lr, MemOperand(sp, 2428));
    __ str(lr, MemOperand(sp, 2432));
    __ str(lr, MemOperand(sp, 2436));
    __ str(lr, MemOperand(sp, 2444));
    __ str(lr, MemOperand(sp, 2448));
    __ str(r8, MemOperand(sp));
    __ str(sl, MemOperand(sp, 4));
    __ str(lr, MemOperand(sp, 2452));
    __ str(lr, MemOperand(sp, 2456));
    __ str(r6, MemOperand(sp, 2312));
    __ strb(ip, MemOperand(sp, 2464));
    // --------------
    __ bind(&l_088e);
    __ bl(&l_088e);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_0894);
    __ bl(&l_0894);
    __ ldrb(Narrow, r3, MemOperand(r5, 17));
    __ str(r9, MemOperand(r5));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1ce2);
    // --------------
    __ bind(&l_08a4);
    __ ldr(Narrow, r3, MemOperand(r7, 32));
    __ mov(r8, 0U);
    __ ldr(Narrow, r6, MemOperand(r7, 40));
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 14U);
    __ str(r8, MemOperand(sp, 2304));
    __ mov(Narrow, r2, r3);
    __ str(Narrow, r3, MemOperand(sp, 68));
    __ ldr(Narrow, r3, MemOperand(r7, 56));
    __ str(r6, MemOperand(sp, 2320));
    __ str(r8, MemOperand(sp, 2312));
    __ str(r3, MemOperand(sp, 2308));
    __ mov(Narrow, r3, fp);
    __ str(r8, MemOperand(sp, 2316));
    // --------------
    __ bind(&l_08cc);
    __ bl(&l_08cc);
    __ movs(Narrow, r2, 4U);
    __ movs(Narrow, r3, 1U);
    __ str(Narrow, r2, MemOperand(sp, 476));
    __ add(Narrow, r1, sp, 472U);
    __ ldr(Narrow, r2, &l_09e0);
    __ mov(Narrow, r0, r4);
    __ str(Narrow, r3, MemOperand(sp, 480));
    __ str(Narrow, r3, MemOperand(sp, 484));
    __ strb(r3, MemOperand(sp, 512));
  }
  void Generate_9(TestAssembler* assembler) {
    __ str(Narrow, r3, MemOperand(sp, 528));
    __ str(Narrow, r3, MemOperand(sp, 552));
    __ str(Narrow, r3, MemOperand(sp, 576));
    __ str(Narrow, r3, MemOperand(sp, 600));
    __ strb(r8, MemOperand(sp, 488));
    __ str(r8, MemOperand(sp, 492));
    __ strb(r8, MemOperand(sp, 496));
    __ strb(r8, MemOperand(sp, 500));
    __ str(r8, MemOperand(sp, 516));
    __ str(r8, MemOperand(sp, 520));
    __ str(r8, MemOperand(sp, 524));
    __ str(r8, MemOperand(sp, 532));
    __ str(r8, MemOperand(sp, 536));
    __ str(r8, MemOperand(sp, 540));
    __ str(r8, MemOperand(sp, 544));
    __ str(r8, MemOperand(sp, 548));
    __ str(r8, MemOperand(sp, 556));
    __ str(r8, MemOperand(sp, 560));
    __ str(r8, MemOperand(sp, 564));
    __ str(r8, MemOperand(sp, 568));
    __ str(r8, MemOperand(sp, 572));
    __ str(r8, MemOperand(sp, 580));
    __ str(r8, MemOperand(sp, 584));
    __ str(r8, MemOperand(sp, 588));
    __ str(r8, MemOperand(sp, 592));
    __ str(r8, MemOperand(sp, 596));
    __ str(r8, MemOperand(sp, 604));
    __ str(r8, MemOperand(sp, 608));
    __ str(r8, MemOperand(sp, 612));
    __ str(r8, MemOperand(sp, 616));
    __ ldr(Narrow, r3, MemOperand(r4));
    __ str(Narrow, r2, MemOperand(sp, 472));
    __ ldr(Narrow, r2, &l_09e4);
    __ ldr(Narrow, r3, MemOperand(r3, 8));
    __ str(Narrow, r2, MemOperand(sp, 24));
    __ blx(r3);
    __ ldr(Narrow, r3, &l_09e4);
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r1, MemOperand(r7, 60));
    __ vmov(F32, s0, -2.0000);
    __ str(Narrow, r3, MemOperand(sp, 24));
    // --------------
    __ bind(&l_096c);
    __ bl(&l_096c);
    __ movs(Narrow, r3, 0U);
    __ add(r8, sp, 136U);
    __ movt(r3, 49148U);
    __ mov(r1, 800U);
    __ str(r1, MemOperand(r8, 8));
    __ str(r3, MemOperand(r8));
    __ movs(Narrow, r3, 0U);
    __ ldr(Narrow, r1, MemOperand(sp, 20));
    __ ldr(Narrow, r2, MemOperand(r7));
    __ str(Narrow, r3, MemOperand(sp, 168));
    __ strb(r3, MemOperand(sp, 181));
    __ ldr(Narrow, r3, MemOperand(r7, 64));
    __ str(r2, MemOperand(r8, 16));
    __ str(r9, MemOperand(sp, 164));
    __ mov(Narrow, r6, r3);
    __ cmp(Narrow, r1, 0U);
    __ b(eq, Narrow, &l_0a00);
    __ movs(Narrow, r2, 1U);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ str(Narrow, r2, MemOperand(sp, 188));
    __ mov(Narrow, r0, r1);
    __ ldr(Narrow, r2, &l_09e4);
    __ str(Narrow, r1, MemOperand(sp, 184));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ str(Narrow, r2, MemOperand(sp, 24));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ ldr(Narrow, r2, &l_09e4);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ str(Narrow, r2, MemOperand(sp, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d30);
    // --------------
    __ bind(&l_09d6);
    __ bl(&l_09d6);
    __ b(Narrow, &l_09e8);
    // --------------
    __ bind(&l_09dc);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_09e0);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_09e4);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_09e8);
    __ str(Narrow, r0, MemOperand(sp, 168));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(ls, &l_1c60);
    // --------------
    __ bind(&l_09fa);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 181));
    // --------------
    __ bind(&l_0a00);
    __ ldr(Narrow, r3, &l_0d18);
    __ mov(lr, 0U);
    __ movs(Narrow, r2, 1U);
  }
  void Generate_10(TestAssembler* assembler) {
    __ movs(Narrow, r0, 14U);
    __ movs(Narrow, r1, 4U);
    __ str(Narrow, r0, MemOperand(sp, 628));
    __ str(Narrow, r3, MemOperand(sp, 164));
    __ mov(Narrow, r3, r6);
    __ ldr(Narrow, r6, &l_0d1c);
    __ str(Narrow, r1, MemOperand(sp, 636));
    __ mov(Narrow, r1, r0);
    __ str(Narrow, r2, MemOperand(sp, 640));
    __ mov(Narrow, r0, r4);
    __ str(Narrow, r2, MemOperand(sp, 644));
    __ strb(r2, MemOperand(sp, 672));
    __ str(Narrow, r2, MemOperand(sp, 688));
    __ str(Narrow, r2, MemOperand(sp, 712));
    __ str(Narrow, r2, MemOperand(sp, 736));
    __ str(Narrow, r2, MemOperand(sp, 760));
    __ mov(r2, 800U);
    __ str(Narrow, r4, MemOperand(sp, 624));
    __ strb(lr, MemOperand(sp, 648));
    __ str(lr, MemOperand(sp, 652));
    __ strb(lr, MemOperand(sp, 656));
    __ strb(lr, MemOperand(sp, 660));
    __ str(lr, MemOperand(sp, 676));
    __ str(lr, MemOperand(sp, 680));
    __ str(lr, MemOperand(sp, 684));
    __ str(lr, MemOperand(sp, 692));
    __ str(lr, MemOperand(sp, 696));
    __ str(lr, MemOperand(sp, 700));
    __ str(lr, MemOperand(sp, 704));
    __ str(lr, MemOperand(sp, 708));
    __ str(lr, MemOperand(sp, 716));
    __ str(lr, MemOperand(sp, 720));
    __ str(lr, MemOperand(sp, 724));
    __ str(lr, MemOperand(sp, 728));
    __ str(lr, MemOperand(sp, 732));
    __ str(lr, MemOperand(sp, 740));
    __ str(lr, MemOperand(sp, 744));
    __ str(lr, MemOperand(sp, 748));
    __ str(lr, MemOperand(sp, 752));
    __ str(lr, MemOperand(sp, 756));
    __ str(lr, MemOperand(sp, 764));
    __ str(r8, MemOperand(sp));
    __ str(lr, MemOperand(sp, 768));
    __ str(lr, MemOperand(sp, 772));
    __ str(lr, MemOperand(sp, 776));
    __ str(Narrow, r6, MemOperand(sp, 632));
    __ strb(lr, MemOperand(sp, 784));
    // --------------
    __ bind(&l_0aa2);
    __ bl(&l_0aa2);
    __ ldrb(r3, MemOperand(sp, 656));
    __ cbz(r3, &l_0b00);
    __ ldr(r8, MemOperand(sp, 624));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(r2, MemOperand(r8));
    __ ldr(Narrow, r2, MemOperand(r2, 8));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d92);
    __ ldr(r2, MemOperand(r8, 12));
    __ add(Narrow, r0, sp, 632U);
    __ ldr(r3, MemOperand(r8, 4));
    __ subs(Narrow, r3, r2, r3);
    __ mov(Narrow, r1, r3);
    __ str(Narrow, r3, MemOperand(sp, 28));
    // --------------
    __ bind(&l_0ad4);
    __ bl(&l_0ad4);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, &l_1d00);
    __ add(r3, r8, 96U);
    __ mov(Narrow, r6, r3);
    __ add(r3, r8, 44U);
    __ mov(Narrow, r1, r3);
    __ str(Narrow, r3, MemOperand(sp, 24));
    // --------------
    __ bind(&l_0aec);
    __ ldr(lr, MemOperand(r8, 12));
    __ mov(Narrow, r0, r6);
    __ ldr(r3, MemOperand(r8, 4));
    __ add(Narrow, r2, sp, 632U);
    __ rsb(r3, r3, lr);
    // --------------
    __ bind(&l_0afc);
    __ bl(&l_0afc);
    // --------------
    __ bind(&l_0b00);
    __ ldr(Narrow, r3, &l_0d20);
    __ ldr(r8, MemOperand(sp, 776));
    __ str(Narrow, r3, MemOperand(sp, 24));
    __ str(Narrow, r3, MemOperand(sp, 632));
    __ cmp(r8, 0U);
    __ b(eq, Narrow, &l_0b20);
    __ ldr(r0, MemOperand(r8));
    __ cbz(r0, &l_0b1a);
    // --------------
    __ bind(&l_0b16);
    __ bl(&l_0b16);
    // --------------
    __ bind(&l_0b1a);
    __ mov(Narrow, r0, r8);
    // --------------
    __ bind(&l_0b1c);
    __ bl(&l_0b1c);
    // --------------
    __ bind(&l_0b20);
    __ ldrb(r3, MemOperand(sp, 181));
    __ str(r9, MemOperand(sp, 164));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1c0e);
    // --------------
    __ bind(&l_0b2e);
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r1, MemOperand(r7, 68));
    __ vmov(F32, s0, -1.9375);
    // --------------
    __ bind(&l_0b36);
    __ bl(&l_0b36);
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r1, MemOperand(r7, 72));
    __ vldr(s0, &l_0d24);
    // --------------
    __ bind(&l_0b42);
    __ bl(&l_0b42);
  }
  void Generate_11(TestAssembler* assembler) {
    __ ldr(ip, MemOperand(r7, 40));
    __ movs(Narrow, r3, 32U);
    __ mov(lr, 0U);
    __ str(r3, MemOperand(sp, 2304));
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 14U);
    __ ldr(Narrow, r2, MemOperand(r7, 28));
    __ mov(Narrow, r3, fp);
    __ str(ip, MemOperand(sp, 2308));
    __ str(ip, MemOperand(sp, 2320));
    __ str(lr, MemOperand(sp, 2312));
    __ str(lr, MemOperand(sp, 2316));
    // --------------
    __ bind(&l_0b6c);
    __ bl(&l_0b6c);
    __ ldr(Narrow, r3, MemOperand(r7, 76));
    __ mov(lr, 0U);
    __ ldr(Narrow, r6, MemOperand(sp, 68));
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 14U);
    __ str(lr, MemOperand(sp, 2304));
    __ mov(Narrow, r2, r3);
    __ str(Narrow, r3, MemOperand(sp, 40));
    __ ldr(Narrow, r3, MemOperand(r7, 40));
    __ str(r6, MemOperand(sp, 2308));
    __ str(lr, MemOperand(sp, 2312));
    __ str(r3, MemOperand(sp, 2320));
    __ mov(Narrow, r3, fp);
    __ str(lr, MemOperand(sp, 2316));
    // --------------
    __ bind(&l_0b98);
    __ bl(&l_0b98);
    __ movs(Narrow, r2, 1U);
    __ movs(Narrow, r3, 0U);
    __ str(Narrow, r2, MemOperand(sp, 632));
    __ movs(Narrow, r1, 4U);
    __ str(Narrow, r2, MemOperand(sp, 636));
    __ mov(Narrow, r0, r4);
    __ strb(r2, MemOperand(sp, 664));
    __ str(Narrow, r2, MemOperand(sp, 680));
    __ str(Narrow, r2, MemOperand(sp, 704));
    __ str(Narrow, r2, MemOperand(sp, 728));
    __ str(Narrow, r2, MemOperand(sp, 752));
    __ ldr(Narrow, r2, MemOperand(r7, 80));
    __ str(Narrow, r1, MemOperand(sp, 628));
    __ add(Narrow, r1, sp, 624U);
    __ strb(r3, MemOperand(sp, 640));
    __ str(Narrow, r3, MemOperand(sp, 644));
    __ strb(r3, MemOperand(sp, 648));
    __ strb(r3, MemOperand(sp, 652));
    __ str(Narrow, r3, MemOperand(sp, 668));
    __ str(Narrow, r3, MemOperand(sp, 672));
    __ str(Narrow, r3, MemOperand(sp, 676));
    __ str(Narrow, r3, MemOperand(sp, 684));
    __ str(Narrow, r3, MemOperand(sp, 688));
    __ str(Narrow, r3, MemOperand(sp, 692));
    __ str(Narrow, r3, MemOperand(sp, 696));
    __ str(Narrow, r3, MemOperand(sp, 700));
    __ str(Narrow, r3, MemOperand(sp, 708));
    __ str(Narrow, r3, MemOperand(sp, 712));
    __ str(Narrow, r3, MemOperand(sp, 716));
    __ str(Narrow, r3, MemOperand(sp, 720));
    __ str(Narrow, r3, MemOperand(sp, 724));
    __ str(Narrow, r3, MemOperand(sp, 732));
    __ str(Narrow, r3, MemOperand(sp, 736));
    __ str(Narrow, r3, MemOperand(sp, 740));
    __ str(Narrow, r3, MemOperand(sp, 744));
    __ str(Narrow, r3, MemOperand(sp, 748));
    __ str(Narrow, r3, MemOperand(sp, 756));
    __ str(Narrow, r2, MemOperand(sp, 28));
    __ ldr(Narrow, r6, MemOperand(r7, 84));
    __ ldr(Narrow, r2, MemOperand(r4));
    __ str(Narrow, r3, MemOperand(sp, 760));
    __ str(Narrow, r6, MemOperand(sp, 32));
    __ ldr(Narrow, r6, &l_0d1c);
    __ str(Narrow, r3, MemOperand(sp, 764));
    __ str(Narrow, r3, MemOperand(sp, 768));
    __ str(Narrow, r6, MemOperand(sp, 624));
    __ ldr(Narrow, r3, MemOperand(r2, 8));
    __ blx(r3);
    __ ldr(ip, MemOperand(r7, 40));
    __ movs(Narrow, r1, 1U);
    __ mov(lr, 0U);
    __ ldr(Narrow, r2, MemOperand(r7, 88));
    __ ldr(Narrow, r3, MemOperand(r7, 92));
    __ mov(Narrow, r0, r4);
    __ str(r1, MemOperand(sp, 2304));
    __ movs(Narrow, r1, 14U);
    __ str(fp, MemOperand(sp));
    __ str(ip, MemOperand(sp, 2308));
    __ str(ip, MemOperand(sp, 2320));
    __ str(lr, MemOperand(sp, 2312));
    __ str(lr, MemOperand(sp, 2316));
    // --------------
    __ bind(&l_0c2e);
    __ bl(&l_0c2e);
    __ vldr(d7, &l_0d10);
    __ add(r6, sp, 2304U);
    __ ldr(r8, MemOperand(r7, 96));
    __ mov(lr, 832U);
    __ str(fp, MemOperand(sp));
    __ mov(Narrow, r0, r4);
    __ vstr(d7, MemOperand(r6));
    __ movs(Narrow, r1, 14U);
    __ ldr(Narrow, r6, MemOperand(r7));
    __ mov(Narrow, r3, r8);
  }
  void Generate_12(TestAssembler* assembler) {
    __ mov(r2, 800U);
    __ str(lr, MemOperand(sp, 2312));
    __ str(r6, MemOperand(sp, 2320));
    __ ldr(Narrow, r6, MemOperand(r7, 100));
    // --------------
    __ bind(&l_0c60);
    __ bl(&l_0c60);
    __ movs(Narrow, r2, 0U);
    __ movs(Narrow, r3, 0U);
    __ str(fp, MemOperand(sp));
    __ add(ip, sp, 2304U);
    __ strd(r2, r3, MemOperand(ip));
    __ mov(lr, 832U);
    __ ldr(Narrow, r3, MemOperand(r7));
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 14U);
    __ mov(r2, 800U);
    __ str(lr, MemOperand(sp, 2312));
    __ str(r3, MemOperand(sp, 2320));
    __ mov(Narrow, r3, r6);
    // --------------
    __ bind(&l_0c8c);
    __ bl(&l_0c8c);
    __ movs(Narrow, r2, 0U);
    __ movs(Narrow, r3, 0U);
    __ str(fp, MemOperand(sp));
    __ add(ip, sp, 2304U);
    __ strd(r2, r3, MemOperand(ip));
    __ mov(lr, 832U);
    __ ldr(Narrow, r3, MemOperand(r7));
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 14U);
    __ mov(r2, 800U);
    __ str(lr, MemOperand(sp, 2312));
    __ str(r3, MemOperand(sp, 2320));
    __ ldr(Narrow, r3, MemOperand(sp, 28));
    // --------------
    __ bind(&l_0cb8);
    __ bl(&l_0cb8);
    __ movs(Narrow, r3, 0U);
    __ movs(Narrow, r2, 1U);
    __ movs(Narrow, r1, 4U);
    __ str(Narrow, r2, MemOperand(r5, 8));
    __ str(Narrow, r1, MemOperand(r5, 4));
    __ mov(Narrow, r0, r4);
    __ str(Narrow, r2, MemOperand(r5, 12));
    __ strb(r2, MemOperand(r5, 40));
    __ str(Narrow, r2, MemOperand(r5, 56));
    __ str(Narrow, r2, MemOperand(r5, 80));
    __ str(Narrow, r2, MemOperand(r5, 104));
    __ str(r2, MemOperand(r5, 128));
    __ strb(Narrow, r3, MemOperand(r5, 16));
    __ str(Narrow, r3, MemOperand(r5, 20));
    __ strb(Narrow, r3, MemOperand(r5, 24));
    __ strb(Narrow, r3, MemOperand(r5, 28));
    __ str(Narrow, r3, MemOperand(r5, 44));
    __ str(Narrow, r3, MemOperand(r5, 48));
    __ str(Narrow, r3, MemOperand(r5, 52));
    __ str(Narrow, r3, MemOperand(r5, 60));
    __ str(Narrow, r3, MemOperand(r5, 64));
    __ str(Narrow, r3, MemOperand(r5, 68));
    __ str(Narrow, r3, MemOperand(r5, 72));
    __ str(Narrow, r3, MemOperand(r5, 76));
    __ str(Narrow, r3, MemOperand(r5, 84));
    __ str(Narrow, r3, MemOperand(r5, 88));
    __ str(Narrow, r3, MemOperand(r5, 92));
    __ str(Narrow, r3, MemOperand(r5, 96));
    __ str(Narrow, r3, MemOperand(r5, 100));
    __ str(Narrow, r3, MemOperand(r5, 108));
    __ str(Narrow, r3, MemOperand(r5, 112));
    __ str(Narrow, r3, MemOperand(r5, 116));
    __ str(Narrow, r3, MemOperand(r5, 120));
    __ str(Narrow, r3, MemOperand(r5, 124));
    __ str(r3, MemOperand(r5, 132));
    __ b(Narrow, &l_0d28);
    __ nop(Narrow);
    __ nop(Wide);
    // --------------
    __ bind(&l_0d10);
    __ nop(Wide);
    __ nop(Wide);
    // --------------
    __ bind(&l_0d18);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_0d1c);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_0d20);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_0d24);
    __ Emit32(0xbff40000);
    // --------------
    __ bind(&l_0d28);
    __ str(r3, MemOperand(r5, 136));
    __ str(r3, MemOperand(r5, 140));
    __ str(r3, MemOperand(r5, 144));
    __ str(r2, MemOperand(sp, 2312));
    __ str(r2, MemOperand(sp, 2316));
    __ strb(r2, MemOperand(sp, 2344));
    __ str(r2, MemOperand(sp, 2360));
    __ str(r2, MemOperand(sp, 2384));
    __ str(r2, MemOperand(sp, 2408));
    __ str(r2, MemOperand(sp, 2432));
    __ ldr(Narrow, r2, &l_1064);
    __ str(r1, MemOperand(sp, 2308));
    __ mov(Narrow, r1, r5);
    __ strb(r3, MemOperand(sp, 2320));
    __ str(r3, MemOperand(sp, 2324));
    __ strb(r3, MemOperand(sp, 2328));
    __ strb(r3, MemOperand(sp, 2332));
    __ str(r3, MemOperand(sp, 2348));
    __ str(r3, MemOperand(sp, 2352));
    __ str(r3, MemOperand(sp, 2356));
    __ str(r3, MemOperand(sp, 2364));
    __ str(r3, MemOperand(sp, 2368));
    __ str(r3, MemOperand(sp, 2372));
  }
  void Generate_13(TestAssembler* assembler) {
    __ str(r3, MemOperand(sp, 2376));
    __ str(r3, MemOperand(sp, 2380));
    __ str(r3, MemOperand(sp, 2388));
    __ str(r3, MemOperand(sp, 2392));
    __ str(r3, MemOperand(sp, 2396));
    __ str(r3, MemOperand(sp, 2400));
    __ str(r3, MemOperand(sp, 2404));
    __ str(r3, MemOperand(sp, 2412));
    __ str(r3, MemOperand(sp, 2416));
    __ str(r3, MemOperand(sp, 2420));
    __ str(r3, MemOperand(sp, 2424));
    __ str(r3, MemOperand(sp, 2428));
    __ str(r3, MemOperand(sp, 2436));
    __ str(r3, MemOperand(sp, 2440));
    __ str(Narrow, r2, MemOperand(r5));
    __ str(r3, MemOperand(sp, 2444));
    __ str(r3, MemOperand(sp, 2448));
    __ ldr(Narrow, r2, MemOperand(r4));
    __ ldr(Narrow, r3, &l_1064);
    __ str(r3, MemOperand(sp, 2304));
    __ ldr(Narrow, r3, MemOperand(r2, 8));
    __ blx(r3);
    __ ldr(Narrow, r1, MemOperand(r7, 40));
    __ mov(lr, 0U);
    __ ldr(Narrow, r2, MemOperand(r7, 88));
    __ mov(ip, 1U);
    __ str(sl, MemOperand(sp));
    __ mov(Narrow, r0, r4);
    __ str(r1, MemOperand(sl, 4));
    __ str(r1, MemOperand(sl, 16));
    __ mov(Narrow, r3, r2);
    __ movs(Narrow, r1, 14U);
    __ str(ip, MemOperand(sl));
    __ str(lr, MemOperand(sl, 8));
    __ str(lr, MemOperand(sl, 12));
    // --------------
    __ bind(&l_0df8);
    __ bl(&l_0df8);
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 13U);
    __ mov(Narrow, r2, fp);
    __ movs(Narrow, r3, 2U);
    // --------------
    __ bind(&l_0e04);
    __ bl(&l_0e04);
    __ ldr(Narrow, r3, MemOperand(r7, 104));
    __ mov(Narrow, r0, r4);
    __ str(r8, MemOperand(sp));
    __ movs(Narrow, r1, 14U);
    __ str(Narrow, r6, MemOperand(sp, 4));
    __ mov(r2, 800U);
    // --------------
    __ bind(&l_0e18);
    __ bl(&l_0e18);
    __ str(r8, MemOperand(sp));
    __ mov(Narrow, r0, r4);
    __ str(r8, MemOperand(sp, 4));
    __ movs(Narrow, r1, 14U);
    __ mov(r2, 800U);
    __ mov(Narrow, r3, r8);
    // --------------
    __ bind(&l_0e2e);
    __ bl(&l_0e2e);
    __ ldr(Narrow, r3, MemOperand(sp, 64));
    __ mov(Narrow, r0, r4);
    __ str(r8, MemOperand(sp, 4));
    __ movs(Narrow, r1, 14U);
    __ mov(r2, 800U);
    __ str(Narrow, r3, MemOperand(sp));
    __ mov(Narrow, r3, r8);
    // --------------
    __ bind(&l_0e44);
    __ bl(&l_0e44);
    __ ldr(Narrow, r1, MemOperand(sp, 20));
    __ movs(Narrow, r3, 0U);
    __ movs(Narrow, r2, 14U);
    __ str(r9, MemOperand(sp, 192));
    __ str(Narrow, r2, MemOperand(sp, 92));
    __ str(Narrow, r3, MemOperand(sp, 196));
    __ strb(r3, MemOperand(sp, 209));
    __ cbz(r1, &l_0ea4);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r1, MemOperand(sp, 212));
    __ mov(Narrow, r0, r1);
    __ str(Narrow, r2, MemOperand(sp, 216));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d56);
    // --------------
    __ bind(&l_0e88);
    __ bl(&l_0e88);
    // --------------
    __ bind(&l_0e8c);
    __ str(Narrow, r0, MemOperand(sp, 196));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(ls, &l_1c72);
    // --------------
    __ bind(&l_0e9e);
    __ movs(Narrow, r3, 1U);
  }
  void Generate_14(TestAssembler* assembler) {
    __ strb(r3, MemOperand(sp, 209));
    // --------------
    __ bind(&l_0ea4);
    __ ldr(Narrow, r3, &l_1068);
    __ mov(Narrow, r1, r4);
    __ add(Narrow, r0, sp, 792U);
    __ add(Narrow, r2, sp, 92U);
    __ str(Narrow, r3, MemOperand(sp, 192));
    __ bl(&l_0000);
    __ ldr(Narrow, r1, MemOperand(sp, 92));
    __ mov(Narrow, r0, r4);
    __ str(Narrow, r6, MemOperand(sp));
    __ mov(r2, 800U);
    __ str(Narrow, r6, MemOperand(sp, 4));
    __ mov(Narrow, r3, r8);
    // --------------
    __ bind(&l_0ec0);
    __ bl(&l_0ec0);
    __ add(Narrow, r0, sp, 792U);
    // --------------
    __ bind(&l_0ec6);
    __ bl(&l_0ec6);
    __ ldrb(r3, MemOperand(sp, 209));
    __ str(r9, MemOperand(sp, 192));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1ba8);
    // --------------
    __ bind(&l_0ed8);
    __ ldr(Narrow, r3, MemOperand(sp, 36));
    __ mov(ip, 0U);
    __ mov(lr, 1056U);
    __ str(sl, MemOperand(sp));
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 14U);
    __ str(r3, MemOperand(sl, 16));
    __ mov(r2, 800U);
    __ mov(Narrow, r3, r6);
    __ str(ip, MemOperand(sl));
    __ str(lr, MemOperand(sl, 8));
    // --------------
    __ bind(&l_0efc);
    __ bl(&l_0efc);
    __ ldr(Narrow, r1, MemOperand(sp, 76));
    __ movs(Narrow, r3, 14U);
    __ str(Narrow, r3, MemOperand(sp, 96));
    __ movs(Narrow, r3, 0U);
    __ str(Narrow, r3, MemOperand(sp, 224));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r1, MemOperand(sp, 128));
    __ ldr(Narrow, r1, MemOperand(sp, 20));
    __ strb(r3, MemOperand(sp, 237));
    __ ldr(Narrow, r3, MemOperand(r7, 104));
    __ str(Narrow, r2, MemOperand(sp, 132));
    __ str(r9, MemOperand(sp, 220));
    __ str(Narrow, r3, MemOperand(sp, 76));
    __ cbz(r1, &l_0f66);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r1);
    __ str(Narrow, r1, MemOperand(sp, 240));
    __ movs(Narrow, r1, 32U);
    __ str(Narrow, r2, MemOperand(sp, 244));
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d5c);
    // --------------
    __ bind(&l_0f4a);
    __ bl(&l_0f4a);
    // --------------
    __ bind(&l_0f4e);
    __ str(Narrow, r0, MemOperand(sp, 224));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(ls, &l_1c80);
    // --------------
    __ bind(&l_0f60);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 237));
    // --------------
    __ bind(&l_0f66);
    __ ldr(Narrow, r3, &l_1068);
    __ mov(Narrow, r1, r4);
    __ add(Narrow, r0, sp, 960U);
    __ add(Narrow, r2, sp, 96U);
    __ str(Narrow, r3, MemOperand(sp, 220));
    __ bl(&l_0000);
    __ add(Narrow, r2, sp, 128U);
    __ ldr(lr, MemOperand(sp, 96));
    __ ldm(r2, NO_WRITE_BACK, RegisterList(r0, r1));
    __ mov(Narrow, r3, sp);
    __ ldr(Narrow, r2, MemOperand(sp, 76));
    __ str(r2, MemOperand(r3, 4, PostIndex));
    __ mov(r2, 800U);
    __ stm(r3, NO_WRITE_BACK, RegisterList(r0, r1));
    __ mov(Narrow, r3, r6);
    __ mov(Narrow, r1, lr);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_0f94);
    __ bl(&l_0f94);
    __ add(Narrow, r0, sp, 960U);
    // --------------
    __ bind(&l_0f9a);
    __ bl(&l_0f9a);
    __ ldrb(r3, MemOperand(sp, 237));
    __ str(r9, MemOperand(sp, 220));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1b88);
    // --------------
    __ bind(&l_0fac);
    __ ldr(Narrow, r3, MemOperand(r7, 104));
  }
  void Generate_15(TestAssembler* assembler) {
    __ mov(Narrow, r0, r4);
    __ str(r8, MemOperand(sp));
    __ movs(Narrow, r1, 14U);
    __ str(r8, MemOperand(sp, 4));
    __ mov(r2, 800U);
    // --------------
    __ bind(&l_0fbe);
    __ bl(&l_0fbe);
    __ ldr(Narrow, r1, MemOperand(sp, 20));
    __ movs(Narrow, r3, 0U);
    __ movs(Narrow, r2, 14U);
    __ str(r9, MemOperand(sp, 248));
    __ str(Narrow, r2, MemOperand(sp, 100));
    __ str(Narrow, r3, MemOperand(sp, 252));
    __ strb(r3, MemOperand(sp, 265));
    __ ldr(r8, MemOperand(r7, 104));
    __ cbz(r1, &l_1022);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r1, MemOperand(sp, 268));
    __ mov(Narrow, r0, r1);
    __ str(Narrow, r2, MemOperand(sp, 272));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d72);
    // --------------
    __ bind(&l_1006);
    __ bl(&l_1006);
    // --------------
    __ bind(&l_100a);
    __ str(Narrow, r0, MemOperand(sp, 252));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(ls, &l_1c8e);
    // --------------
    __ bind(&l_101c);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 265));
    // --------------
    __ bind(&l_1022);
    __ ldr(Narrow, r3, &l_1068);
    __ mov(Narrow, r1, r4);
    __ add(r0, sp, 1128U);
    __ add(Narrow, r2, sp, 100U);
    __ str(Narrow, r3, MemOperand(sp, 248));
    __ bl(&l_0000);
    __ ldr(Narrow, r1, MemOperand(sp, 100));
    __ mov(Narrow, r3, r8);
    __ str(Narrow, r6, MemOperand(sp));
    __ mov(Narrow, r0, r4);
    __ str(Narrow, r6, MemOperand(sp, 4));
    __ mov(r2, 800U);
    // --------------
    __ bind(&l_1040);
    __ bl(&l_1040);
    __ add(r0, sp, 1128U);
    // --------------
    __ bind(&l_1048);
    __ bl(&l_1048);
    __ ldr(Narrow, r3, &l_1068);
    __ add(Narrow, r0, sp, 248U);
    __ str(Narrow, r3, MemOperand(sp, 248));
    // --------------
    __ bind(&l_1052);
    __ bl(&l_1052);
    __ movs(Narrow, r3, 0U);
    __ movs(Narrow, r2, 14U);
    __ str(Narrow, r3, MemOperand(sp, 280));
    __ strb(r3, MemOperand(sp, 293));
    __ ldr(Narrow, r3, MemOperand(sp, 20));
    __ b(Narrow, &l_106c);
    // --------------
    __ bind(&l_1064);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_1068);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_106c);
    __ str(Narrow, r2, MemOperand(sp, 104));
    __ str(r9, MemOperand(sp, 276));
    __ ldr(r8, MemOperand(r7, 104));
    __ cbz(r3, &l_10c2);
    __ mov(Narrow, r6, r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r6, MemOperand(sp, 296));
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r2, MemOperand(sp, 300));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d3a);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_10a6);
    __ bl(&l_10a6);
    // --------------
    __ bind(&l_10aa);
    __ str(Narrow, r0, MemOperand(sp, 280));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
  }
  void Generate_16(TestAssembler* assembler) {
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(ls, &l_1cc6);
    // --------------
    __ bind(&l_10bc);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 293));
    // --------------
    __ bind(&l_10c2);
    __ ldr(Narrow, r3, &l_13fc);
    __ mov(Narrow, r1, r4);
    __ add(r0, sp, 1296U);
    __ add(Narrow, r2, sp, 104U);
    __ str(Narrow, r3, MemOperand(sp, 276));
    __ bl(&l_0000);
    __ ldr(Narrow, r2, MemOperand(sp, 52));
    __ mov(Narrow, r3, r8);
    __ ldr(Narrow, r1, MemOperand(sp, 104));
    __ mov(Narrow, r0, r4);
    __ str(r8, MemOperand(sp));
    __ str(Narrow, r2, MemOperand(sp, 4));
    __ mov(r2, 800U);
    // --------------
    __ bind(&l_10e4);
    __ bl(&l_10e4);
    __ add(r0, sp, 1296U);
    // --------------
    __ bind(&l_10ec);
    __ bl(&l_10ec);
    __ ldrb(r3, MemOperand(sp, 293));
    __ str(r9, MemOperand(sp, 276));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1b68);
    // --------------
    __ bind(&l_10fe);
    __ movs(Narrow, r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r7, 104));
    __ str(r3, MemOperand(sl));
    __ mov(r1, 1056U);
    __ str(Narrow, r3, MemOperand(sp, 308));
    __ strb(r3, MemOperand(sp, 321));
    __ ldr(Narrow, r3, MemOperand(sp, 20));
    __ str(r2, MemOperand(sl, 16));
    __ movs(Narrow, r2, 14U);
    __ str(r9, MemOperand(sp, 304));
    __ str(r1, MemOperand(sl, 8));
    __ str(Narrow, r2, MemOperand(sp, 108));
    __ cbz(r3, &l_116e);
    __ mov(Narrow, r6, r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r6, MemOperand(sp, 324));
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r2, MemOperand(sp, 328));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d42);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_1152);
    __ bl(&l_1152);
    // --------------
    __ bind(&l_1156);
    __ str(Narrow, r0, MemOperand(sp, 308));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(ls, &l_1cd4);
    // --------------
    __ bind(&l_1168);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 321));
    // --------------
    __ bind(&l_116e);
    __ ldr(Narrow, r3, &l_13fc);
    __ mov(Narrow, r1, r4);
    __ add(r0, sp, 1464U);
    __ add(Narrow, r2, sp, 108U);
    __ str(Narrow, r3, MemOperand(sp, 304));
    __ bl(&l_0000);
    __ ldr(Narrow, r3, MemOperand(sp, 28));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r1, MemOperand(sp, 108));
    __ mov(r2, 800U);
    __ str(sl, MemOperand(sp, 4));
    __ str(Narrow, r3, MemOperand(sp));
    // --------------
    __ bind(&l_118e);
    __ bl(&l_118e);
    __ add(r0, sp, 1464U);
    // --------------
    __ bind(&l_1196);
    __ bl(&l_1196);
    __ ldr(Narrow, r3, &l_13fc);
    __ add(Narrow, r0, sp, 304U);
    __ str(Narrow, r3, MemOperand(sp, 304));
    // --------------
    __ bind(&l_11a0);
    __ bl(&l_11a0);
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 14U);
    __ mov(Narrow, r2, r5);
    __ movs(Narrow, r3, 2U);
    // --------------
    __ bind(&l_11ac);
    __ bl(&l_11ac);
    __ ldr(Narrow, r3, MemOperand(r4));
    __ mov(Narrow, r0, r4);
    __ mov(Narrow, r1, fp);
    __ ldr(Narrow, r3, MemOperand(r3, 8));
    __ blx(r3);
  }
  void Generate_17(TestAssembler* assembler) {
    __ ldr(Narrow, r6, MemOperand(sp, 24));
    __ add(r0, sp, 2332U);
    __ str(r6, MemOperand(sp, 2304));
    // --------------
    __ bind(&l_11c4);
    __ bl(&l_11c4);
    __ str(Narrow, r6, MemOperand(r5));
    __ add(r0, sp, 2164U);
    // --------------
    __ bind(&l_11ce);
    __ bl(&l_11ce);
    __ ldr(Narrow, r6, MemOperand(sp, 20));
    __ movs(Narrow, r3, 0U);
    __ str(r9, MemOperand(sp, 332));
    __ str(Narrow, r3, MemOperand(sp, 336));
    __ strb(r3, MemOperand(sp, 349));
    __ cbz(r6, &l_122a);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r6, MemOperand(sp, 352));
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r2, MemOperand(sp, 356));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d6a);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_120e);
    __ bl(&l_120e);
    // --------------
    __ bind(&l_1212);
    __ str(Narrow, r0, MemOperand(sp, 336));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(ls, &l_1cb8);
    // --------------
    __ bind(&l_1224);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 349));
    // --------------
    __ bind(&l_122a);
    __ ldr(Narrow, r1, &l_13fc);
    __ mov(lr, 0U);
    __ mov(ip, 1U);
    __ movs(Narrow, r3, 14U);
    __ movs(Narrow, r2, 4U);
    __ str(r3, MemOperand(sp, 1636));
    __ str(Narrow, r1, MemOperand(sp, 332));
    __ mov(Narrow, r1, r3);
    __ str(r2, MemOperand(sp, 1644));
    __ mov(Narrow, r0, r4);
    __ str(r4, MemOperand(sp, 1632));
    __ mov(r2, 1056U);
    __ str(ip, MemOperand(sp, 1648));
    __ ldr(Narrow, r3, MemOperand(sp, 32));
    __ str(ip, MemOperand(sp, 1652));
    __ strb(ip, MemOperand(sp, 1680));
    __ str(ip, MemOperand(sp, 1696));
    __ str(ip, MemOperand(sp, 1720));
    __ str(ip, MemOperand(sp, 1744));
    __ str(ip, MemOperand(sp, 1768));
    __ strb(lr, MemOperand(sp, 1656));
    __ str(lr, MemOperand(sp, 1660));
    __ strb(lr, MemOperand(sp, 1664));
    __ strb(lr, MemOperand(sp, 1668));
    __ str(lr, MemOperand(sp, 1684));
    __ str(lr, MemOperand(sp, 1688));
    __ str(lr, MemOperand(sp, 1692));
    __ str(lr, MemOperand(sp, 1700));
    __ str(lr, MemOperand(sp, 1704));
    __ str(lr, MemOperand(sp, 1708));
    __ str(lr, MemOperand(sp, 1712));
    __ str(lr, MemOperand(sp, 1716));
    __ str(lr, MemOperand(sp, 1724));
    __ str(lr, MemOperand(sp, 1728));
    __ str(lr, MemOperand(sp, 1732));
    __ str(lr, MemOperand(sp, 1736));
    __ str(lr, MemOperand(sp, 1740));
    __ str(lr, MemOperand(sp, 1748));
    __ str(lr, MemOperand(sp, 1752));
    __ str(lr, MemOperand(sp, 1756));
    __ str(lr, MemOperand(sp, 1760));
    __ str(lr, MemOperand(sp, 1764));
    __ ldr(Narrow, r6, MemOperand(sp, 28));
    __ str(lr, MemOperand(sp, 1772));
    __ str(lr, MemOperand(sp, 1776));
    __ str(Narrow, r6, MemOperand(sp));
    __ ldr(Narrow, r6, &l_1400);
    __ str(lr, MemOperand(sp, 1780));
    __ str(lr, MemOperand(sp, 1784));
    __ str(r6, MemOperand(sp, 1640));
    __ strb(lr, MemOperand(sp, 1792));
    // --------------
    __ bind(&l_12e2);
    __ bl(&l_12e2);
    __ add(r0, sp, 1632U);
    // --------------
    __ bind(&l_12ea);
    __ bl(&l_12ea);
    __ ldrb(r3, MemOperand(sp, 349));
    __ str(r9, MemOperand(sp, 332));
    __ cmp(Narrow, r3, 0U);
  }
  void Generate_18(TestAssembler* assembler) {
    __ b(ne, &l_1bc8);
    // --------------
    __ bind(&l_12fc);
    __ movs(Narrow, r3, 0U);
    __ str(r9, MemOperand(sp, 360));
    __ str(Narrow, r3, MemOperand(sp, 364));
    __ strb(r3, MemOperand(sp, 377));
    __ ldr(Narrow, r3, MemOperand(sp, 20));
    __ cbz(r3, &l_1356);
    __ mov(Narrow, r6, r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r6, MemOperand(sp, 380));
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r2, MemOperand(sp, 384));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d62);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_133a);
    __ bl(&l_133a);
    // --------------
    __ bind(&l_133e);
    __ str(Narrow, r0, MemOperand(sp, 364));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(ls, &l_1caa);
    // --------------
    __ bind(&l_1350);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 377));
    // --------------
    __ bind(&l_1356);
    __ ldr(Narrow, r0, &l_13fc);
    __ mov(lr, 0U);
    __ movs(Narrow, r3, 1U);
    __ movs(Narrow, r2, 14U);
    __ movs(Narrow, r1, 4U);
    __ str(r2, MemOperand(sp, 1804));
    __ str(Narrow, r0, MemOperand(sp, 360));
    __ mov(Narrow, r0, r4);
    __ str(r1, MemOperand(sp, 1812));
    __ mov(Narrow, r1, r2);
    __ str(r3, MemOperand(sp, 1816));
    __ mov(r2, 1040U);
    __ str(r3, MemOperand(sp, 1820));
    __ strb(r3, MemOperand(sp, 1848));
    __ str(r3, MemOperand(sp, 1864));
    __ str(r3, MemOperand(sp, 1888));
    __ str(r3, MemOperand(sp, 1912));
    __ str(r3, MemOperand(sp, 1936));
    __ str(r4, MemOperand(sp, 1800));
    __ ldr(Narrow, r3, MemOperand(sp, 32));
    __ strb(lr, MemOperand(sp, 1824));
    __ str(lr, MemOperand(sp, 1828));
    __ strb(lr, MemOperand(sp, 1832));
    __ strb(lr, MemOperand(sp, 1836));
    __ str(lr, MemOperand(sp, 1852));
    __ str(lr, MemOperand(sp, 1856));
    __ str(lr, MemOperand(sp, 1860));
    __ str(lr, MemOperand(sp, 1868));
    __ str(lr, MemOperand(sp, 1872));
    __ str(lr, MemOperand(sp, 1876));
    __ str(lr, MemOperand(sp, 1880));
    __ str(lr, MemOperand(sp, 1884));
    __ str(lr, MemOperand(sp, 1892));
    __ str(lr, MemOperand(sp, 1896));
    __ str(lr, MemOperand(sp, 1900));
    __ str(lr, MemOperand(sp, 1904));
    __ str(lr, MemOperand(sp, 1908));
    __ str(lr, MemOperand(sp, 1916));
    __ str(lr, MemOperand(sp, 1920));
    __ str(lr, MemOperand(sp, 1924));
    __ str(lr, MemOperand(sp, 1928));
    __ str(lr, MemOperand(sp, 1932));
    __ ldr(Narrow, r6, MemOperand(sp, 28));
    __ str(lr, MemOperand(sp, 1940));
    __ str(lr, MemOperand(sp, 1944));
    __ str(Narrow, r6, MemOperand(sp));
    __ b(Narrow, &l_1404);
    // --------------
    __ bind(&l_13fc);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_1400);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_1404);
    __ ldr(Narrow, r6, &l_1724);
    __ str(lr, MemOperand(sp, 1948));
    __ str(lr, MemOperand(sp, 1952));
    __ str(r6, MemOperand(sp, 1808));
    __ strb(lr, MemOperand(sp, 1960));
    // --------------
    __ bind(&l_1416);
    __ bl(&l_1416);
    __ add(r0, sp, 1800U);
    // --------------
    __ bind(&l_141e);
    __ bl(&l_141e);
    __ ldrb(r3, MemOperand(sp, 377));
    __ str(r9, MemOperand(sp, 360));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1bf0);
    // --------------
    __ bind(&l_1430);
    __ movs(Narrow, r3, 0U);
  }
  void Generate_19(TestAssembler* assembler) {
    __ movs(Narrow, r2, 14U);
    __ str(Narrow, r3, MemOperand(sp, 392));
    __ strb(r3, MemOperand(sp, 405));
    __ ldr(Narrow, r3, MemOperand(sp, 20));
    __ str(Narrow, r2, MemOperand(sp, 112));
    __ str(r9, MemOperand(sp, 388));
    __ cbz(r3, &l_148e);
    __ mov(Narrow, r6, r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r6, MemOperand(sp, 408));
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r2, MemOperand(sp, 412));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d78);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_1472);
    __ bl(&l_1472);
    // --------------
    __ bind(&l_1476);
    __ str(Narrow, r0, MemOperand(sp, 392));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(ls, &l_1c9c);
    // --------------
    __ bind(&l_1488);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 405));
    // --------------
    __ bind(&l_148e);
    __ ldr(Narrow, r3, &l_1728);
    __ mov(Narrow, r0, sl);
    __ mov(Narrow, r1, r4);
    __ add(Narrow, r2, sp, 112U);
    __ str(Narrow, r3, MemOperand(sp, 388));
    __ bl(&l_0000);
    __ ldr(Narrow, r3, MemOperand(sp, 44));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r6, MemOperand(sp, 32));
    __ mov(r2, 1568U);
    __ ldr(Narrow, r1, MemOperand(sp, 112));
    __ str(Narrow, r3, MemOperand(sp));
    __ ldr(Narrow, r3, MemOperand(sp, 48));
    __ str(Narrow, r3, MemOperand(sp, 4));
    __ mov(Narrow, r3, r6);
    // --------------
    __ bind(&l_14b0);
    __ bl(&l_14b0);
    __ mov(Narrow, r0, sl);
    // --------------
    __ bind(&l_14b6);
    __ bl(&l_14b6);
    __ ldr(Narrow, r3, &l_1728);
    __ add(Narrow, r0, sp, 388U);
    __ str(Narrow, r3, MemOperand(sp, 388));
    // --------------
    __ bind(&l_14c0);
    __ bl(&l_14c0);
    __ and_(r3, r6, 0x1f);
    __ ldr(Narrow, r1, MemOperand(r7, 40));
    __ orr(r3, r3, 0x8000);
    __ movs(Narrow, r2, 0U);
    __ orr(r3, r3, 0x60);
    __ mov(lr, 1U);
    __ str(r3, MemOperand(sl));
    __ mov(r0, 4098U);
    __ ldr(Narrow, r3, MemOperand(sp, 40));
    __ str(r1, MemOperand(sp, 2316));
    __ str(r2, MemOperand(sl, 4));
    __ str(r3, MemOperand(sp, 2304));
    __ movs(Narrow, r3, 14U);
    __ str(Narrow, r3, MemOperand(sp, 116));
    __ ldr(Narrow, r3, MemOperand(sp, 20));
    __ str(r2, MemOperand(sl, 12));
    __ str(r2, MemOperand(sp, 2308));
    __ str(r2, MemOperand(sp, 2312));
    __ str(r2, MemOperand(sp, 2320));
    __ str(r2, MemOperand(sp, 2324));
    __ str(Narrow, r2, MemOperand(sp, 420));
    __ strb(r2, MemOperand(sp, 433));
    __ str(r9, MemOperand(sp, 416));
    __ str(lr, MemOperand(sl, 8));
    __ str(lr, MemOperand(sl, 16));
    __ str(lr, MemOperand(sp, 2332));
    __ str(r0, MemOperand(sp, 2328));
    __ cbz(r3, &l_156e);
    __ mov(Narrow, r6, r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r6, MemOperand(sp, 436));
    __ movs(Narrow, r1, 32U);
    __ str(lr, MemOperand(sp, 440));
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
  }
  void Generate_20(TestAssembler* assembler) {
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d2a);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_1552);
    __ bl(&l_1552);
    // --------------
    __ bind(&l_1556);
    __ str(Narrow, r0, MemOperand(sp, 420));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(ls, &l_1c48);
    // --------------
    __ bind(&l_1568);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 433));
    // --------------
    __ bind(&l_156e);
    __ ldr(Narrow, r3, &l_1728);
    __ mov(Narrow, r0, r5);
    __ mov(Narrow, r1, r4);
    __ add(Narrow, r2, sp, 116U);
    __ str(Narrow, r3, MemOperand(sp, 416));
    __ bl(&l_0000);
    __ ldr(Narrow, r1, MemOperand(sp, 116));
    __ mov(Narrow, r0, r4);
    __ str(fp, MemOperand(sp));
    __ mov(r2, 1568U);
    __ mov(Narrow, r3, sl);
    // --------------
    __ bind(&l_158a);
    __ bl(&l_158a);
    __ mov(Narrow, r0, r5);
    // --------------
    __ bind(&l_1590);
    __ bl(&l_1590);
    __ ldr(Narrow, r3, &l_1728);
    __ add(Narrow, r0, sp, 416U);
    __ str(Narrow, r3, MemOperand(sp, 416));
    // --------------
    __ bind(&l_159a);
    __ bl(&l_159a);
    __ ldr(Narrow, r2, MemOperand(sp, 60));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r3, MemOperand(sp, 64));
    __ movs(Narrow, r1, 14U);
    __ str(Narrow, r2, MemOperand(sp, 4));
    __ mov(r2, 800U);
    __ str(Narrow, r3, MemOperand(sp));
    // --------------
    __ bind(&l_15ae);
    __ bl(&l_15ae);
    __ ldr(Narrow, r1, MemOperand(r7, 40));
    __ mov(lr, 0U);
    __ ldr(Narrow, r2, MemOperand(r7, 28));
    __ mov(ip, 1U);
    __ str(fp, MemOperand(sp));
    __ mov(Narrow, r0, r4);
    __ str(r1, MemOperand(sp, 2308));
    __ str(r1, MemOperand(sp, 2320));
    __ mov(Narrow, r3, r2);
    __ movs(Narrow, r1, 14U);
    __ str(ip, MemOperand(sp, 2304));
    __ str(lr, MemOperand(sp, 2312));
    __ str(lr, MemOperand(sp, 2316));
    // --------------
    __ bind(&l_15dc);
    __ bl(&l_15dc);
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 1U);
    __ add(Narrow, r2, sp, 624U);
    __ movs(Narrow, r3, 2U);
    // --------------
    __ bind(&l_15e8);
    __ bl(&l_15e8);
    __ ldr(Narrow, r3, MemOperand(sp, 24));
    __ add(Narrow, r0, sp, 652U);
    __ str(Narrow, r3, MemOperand(sp, 624));
    // --------------
    __ bind(&l_15f2);
    __ bl(&l_15f2);
    __ ldr(Narrow, r2, MemOperand(sp, 40));
    __ movs(Narrow, r3, 0U);
    __ ldr(Narrow, r6, MemOperand(sp, 20));
    __ ldr(Narrow, r1, MemOperand(r7, 40));
    __ stm(r5, NO_WRITE_BACK, RegisterList(r2, r3));
    __ mov(r2, 4096U);
    __ str(Narrow, r3, MemOperand(r5, 8));
    __ str(Narrow, r1, MemOperand(r5, 12));
    __ str(Narrow, r3, MemOperand(r5, 16));
    __ str(Narrow, r3, MemOperand(r5, 20));
    __ str(Narrow, r3, MemOperand(sp, 448));
    __ strb(r3, MemOperand(sp, 461));
    __ str(r9, MemOperand(sp, 444));
    __ str(Narrow, r2, MemOperand(r5, 24));
    __ cbz(r6, &l_1664);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r6, MemOperand(sp, 464));
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r2, MemOperand(sp, 468));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d24);
  }
  void Generate_21(TestAssembler* assembler) {
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_1648);
    __ bl(&l_1648);
    // --------------
    __ bind(&l_164c);
    __ str(Narrow, r0, MemOperand(sp, 448));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(ls, &l_1c54);
    // --------------
    __ bind(&l_165e);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 461));
    // --------------
    __ bind(&l_1664);
    __ ldr(Narrow, r3, &l_1728);
    __ ldr(Narrow, r2, MemOperand(r5, 12));
    __ str(Narrow, r3, MemOperand(sp, 444));
    __ ubfx(r3, r2, 5U, 4U);
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1b04);
    __ ldr(Narrow, r2, MemOperand(sp, 72));
    __ and_(r2, r2, 0x1f);
    __ cmp(Narrow, r2, 7U);
    __ b(hi, &l_1b00);
    __ ldr(Narrow, r2, MemOperand(r5));
    __ and_(r2, r2, 0x1f);
    __ cmp(Narrow, r2, 7U);
    __ b(hi, &l_1b00);
    __ ldr(Narrow, r2, MemOperand(r5, 4));
    __ cmp(Narrow, r2, 31U);
    __ b(hi, &l_1b00);
    // --------------
    __ bind(&l_1694);
    __ ldr(Narrow, r3, MemOperand(r5, 24));
    __ ubfx(r3, r3, 0U, 12U);
    __ clz(ip, r3);
    __ lsr(ip, ip, 5U);
    // --------------
    __ bind(&l_16a2);
    __ ldr(Narrow, r6, &l_1724);
    __ mov(lr, 0U);
    __ movs(Narrow, r0, 1U);
    __ movs(Narrow, r2, 4U);
    __ movs(Narrow, r1, 14U);
    __ str(r2, MemOperand(sp, 2316));
    __ str(r0, MemOperand(sp, 2320));
    __ mov(Narrow, r2, lr);
    __ str(r0, MemOperand(sp, 2324));
    __ strb(r0, MemOperand(sp, 2352));
    __ str(r0, MemOperand(sp, 2368));
    __ str(r0, MemOperand(sp, 2392));
    __ str(r0, MemOperand(sp, 2416));
    __ str(r0, MemOperand(sp, 2440));
    __ mov(Narrow, r0, r4);
    __ str(r4, MemOperand(sp, 2304));
    __ ldr(Narrow, r3, MemOperand(sp, 72));
    __ str(r1, MemOperand(sp, 2308));
    __ strb(lr, MemOperand(sp, 2328));
    __ str(lr, MemOperand(sp, 2332));
    __ strb(lr, MemOperand(sp, 2336));
    __ strb(lr, MemOperand(sp, 2340));
    __ str(lr, MemOperand(sp, 2356));
    __ str(lr, MemOperand(sp, 2360));
    __ str(lr, MemOperand(sp, 2364));
    __ str(lr, MemOperand(sp, 2372));
    __ str(lr, MemOperand(sp, 2376));
    __ str(lr, MemOperand(sp, 2380));
    __ str(lr, MemOperand(sp, 2384));
    __ str(lr, MemOperand(sp, 2388));
    __ str(lr, MemOperand(sp, 2396));
    __ str(lr, MemOperand(sp, 2400));
    __ str(lr, MemOperand(sp, 2404));
    __ str(lr, MemOperand(sp, 2408));
    __ str(lr, MemOperand(sp, 2412));
    __ b(Narrow, &l_172c);
    __ nop(Narrow);
    // --------------
    __ bind(&l_1724);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_1728);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_172c);
    __ str(lr, MemOperand(sp, 2420));
    __ str(lr, MemOperand(sp, 2424));
    __ str(lr, MemOperand(sp, 2428));
    __ str(lr, MemOperand(sp, 2432));
    __ str(lr, MemOperand(sp, 2436));
    __ str(lr, MemOperand(sp, 2444));
    __ str(Narrow, r5, MemOperand(sp));
    __ str(lr, MemOperand(sp, 2448));
    __ str(lr, MemOperand(sp, 2452));
    __ str(lr, MemOperand(sp, 2456));
    __ str(r6, MemOperand(sp, 2312));
    __ strb(ip, MemOperand(sp, 2464));
    // --------------
    __ bind(&l_175a);
    __ bl(&l_175a);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_1760);
    __ bl(&l_1760);
    __ ldrb(r3, MemOperand(sp, 461));
    __ str(r9, MemOperand(sp, 444));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1b4a);
    // --------------
    __ bind(&l_1772);
    __ ldr(lr, MemOperand(r7, 40));
    __ movw(r1, 0U);
    __ ldr(Narrow, r2, MemOperand(sp, 68));
    __ movt(r1, 0U);
    __ mov(Narrow, r0, r4);
    __ str(lr, MemOperand(sp));
    __ mov(Narrow, r3, lr);
    __ str(lr, MemOperand(sp, 4));
    // --------------
    __ bind(&l_178c);
    __ bl(&l_178c);
  }
  void Generate_22(TestAssembler* assembler) {
    __ ldr(Narrow, r2, MemOperand(sp, 56));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r3, MemOperand(sp, 36));
    __ movs(Narrow, r1, 14U);
    __ str(Narrow, r2, MemOperand(sp, 4));
    __ mov(r2, 800U);
    __ str(Narrow, r3, MemOperand(sp));
    // --------------
    __ bind(&l_17a0);
    __ bl(&l_17a0);
    __ ldr(Narrow, r1, MemOperand(r7, 40));
    __ mov(lr, 0U);
    __ ldr(Narrow, r2, MemOperand(r7, 24));
    __ mov(ip, 1U);
    __ str(fp, MemOperand(sp));
    __ mov(Narrow, r0, r4);
    __ str(r1, MemOperand(sp, 2308));
    __ str(r1, MemOperand(sp, 2320));
    __ mov(Narrow, r3, r2);
    __ movs(Narrow, r1, 14U);
    __ str(ip, MemOperand(sp, 2304));
    __ str(lr, MemOperand(sp, 2312));
    __ str(lr, MemOperand(sp, 2316));
    // --------------
    __ bind(&l_17ce);
    __ bl(&l_17ce);
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 1U);
    __ add(Narrow, r2, sp, 472U);
    __ movs(Narrow, r3, 2U);
    // --------------
    __ bind(&l_17da);
    __ bl(&l_17da);
    __ ldr(Narrow, r3, MemOperand(sp, 24));
    __ ldr(r8, MemOperand(sp, 616));
    __ str(Narrow, r3, MemOperand(sp, 472));
    __ cmp(r8, 0U);
    __ b(eq, Narrow, &l_17fc);
    __ ldr(r0, MemOperand(r8));
    __ cbz(r0, &l_17f6);
    // --------------
    __ bind(&l_17f2);
    __ bl(&l_17f2);
    // --------------
    __ bind(&l_17f6);
    __ mov(Narrow, r0, r8);
    // --------------
    __ bind(&l_17f8);
    __ bl(&l_17f8);
    // --------------
    __ bind(&l_17fc);
    __ ldr(Narrow, r1, MemOperand(r7, 40));
    __ mov(lr, 0U);
    __ ldr(Narrow, r2, MemOperand(r7, 56));
    __ mov(ip, 132U);
    __ mov(Narrow, r0, r4);
    __ str(fp, MemOperand(sp));
    __ str(r1, MemOperand(sp, 2308));
    __ mov(Narrow, r3, r2);
    __ str(r1, MemOperand(sp, 2320));
    __ movs(Narrow, r1, 14U);
    __ str(ip, MemOperand(sp, 2304));
    __ str(lr, MemOperand(sp, 2312));
    __ str(lr, MemOperand(sp, 2316));
    // --------------
    __ bind(&l_1826);
    __ bl(&l_1826);
    __ ldr(Narrow, r3, MemOperand(r7, 24));
    __ ldr(Narrow, r0, MemOperand(r7, 28));
    __ ldr(Narrow, r2, MemOperand(r7, 32));
    __ ubfx(r1, r3, 5U, 4U);
    __ cbz(r1, &l_1842);
    __ and_(lr, r3, 0x1f);
    __ mov(r8, 1U);
    __ lsl(r1, r8, lr);
    // --------------
    __ bind(&l_1842);
    __ ubfx(r3, r0, 5U, 4U);
    __ mov(Narrow, r8, r3);
    __ cbz(r3, &l_1856);
    __ and_(r0, r0, 0x1f);
    __ mov(r8, 1U);
    __ lsl(r8, r8, r0);
    // --------------
    __ bind(&l_1856);
    __ ubfx(r3, r2, 5U, 4U);
    __ orr(r8, r8, r1);
    __ cbz(r3, &l_1868);
    __ and_(r2, r2, 0x1f);
    __ movs(Narrow, r3, 1U);
    __ lsls(Narrow, r3, r3, r2);
    // --------------
    __ bind(&l_1868);
    __ ldr(Narrow, r6, MemOperand(sp, 20));
    __ movs(Narrow, r2, 0U);
    __ movs(Narrow, r1, 14U);
    __ orr(r8, r8, r3);
    __ str(r9, MemOperand(r5));
    __ str(r1, MemOperand(sl));
    __ str(Narrow, r2, MemOperand(r5, 4));
    __ strb(Narrow, r2, MemOperand(r5, 17));
    __ cbz(r6, &l_18ce);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r2, MemOperand(r5, 24));
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r6, MemOperand(r5, 20));
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d50);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_18ac);
    __ bl(&l_18ac);
  }
  void Generate_23(TestAssembler* assembler) {
    // --------------
    __ bind(&l_18b0);
    __ str(Narrow, r0, MemOperand(r5, 4));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_18ca);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_18c6);
    __ bl(&l_18c6);
    // --------------
    __ bind(&l_18ca);
    __ movs(Narrow, r3, 1U);
    __ strb(Narrow, r3, MemOperand(r5, 17));
    // --------------
    __ bind(&l_18ce);
    __ ldr(Narrow, r3, &l_1be8);
    __ mov(Narrow, r0, fp);
    __ mov(Narrow, r1, r4);
    __ mov(Narrow, r2, sl);
    __ str(Narrow, r3, MemOperand(r5));
    __ bl(&l_0000);
    __ mov(Narrow, r0, r4);
    __ ldr(r1, MemOperand(sl));
    __ mov(Narrow, r3, r8);
    __ movs(Narrow, r2, 0U);
    // --------------
    __ bind(&l_18e6);
    __ bl(&l_18e6);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_18ec);
    __ bl(&l_18ec);
    __ ldr(Narrow, r3, &l_1be8);
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r3, MemOperand(r5));
    // --------------
    __ bind(&l_18f6);
    __ bl(&l_18f6);
    __ movs(Narrow, r3, 0U);
    __ ldr(Narrow, r7, MemOperand(r7, 108));
    __ str(Narrow, r3, MemOperand(r5, 4));
    __ strb(Narrow, r3, MemOperand(r5, 17));
    __ ldr(Narrow, r3, MemOperand(sp, 20));
    __ str(r9, MemOperand(r5));
    __ cbz(r3, &l_195a);
    __ mov(Narrow, r6, r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r6, MemOperand(r5, 20));
    __ str(Narrow, r2, MemOperand(r5, 24));
    __ mov(Narrow, r0, r6);
    __ movs(Narrow, r1, 32U);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_1d4a);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_1938);
    __ bl(&l_1938);
    // --------------
    __ bind(&l_193c);
    __ str(Narrow, r0, MemOperand(r5, 4));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_1956);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_1952);
    __ bl(&l_1952);
    // --------------
    __ bind(&l_1956);
    __ movs(Narrow, r3, 1U);
    __ strb(Narrow, r3, MemOperand(r5, 17));
    // --------------
    __ bind(&l_195a);
    __ movs(Narrow, r3, 1U);
    __ and_(r6, r7, 0x1f);
    __ subs(r8, r6, 15U);
    __ str(r3, MemOperand(sp, 2320));
    __ str(r3, MemOperand(sp, 2324));
    __ mov(lr, 14U);
    __ strb(r3, MemOperand(sp, 2352));
    __ mov(ip, 4U);
    __ str(r3, MemOperand(sp, 2368));
    __ it(ne);
    __ mov(ne, r8, 1U);
    __ str(r3, MemOperand(sp, 2392));
    __ mov(Narrow, r2, r7);
    __ str(r3, MemOperand(sp, 2416));
    __ movs(Narrow, r7, 0U);
    __ str(r3, MemOperand(sp, 2440));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r6, &l_1be8);
    __ mov(Narrow, r1, lr);
    __ ldr(Narrow, r3, &l_1bec);
    __ str(r4, MemOperand(sp, 2304));
    __ str(Narrow, r6, MemOperand(r5));
    __ str(lr, MemOperand(sp, 2308));
    __ str(ip, MemOperand(sp, 2316));
    __ strb(r7, MemOperand(sp, 2328));
    __ str(r7, MemOperand(sp, 2332));
    __ strb(r7, MemOperand(sp, 2336));
  }
  void Generate_24(TestAssembler* assembler) {
    __ strb(r7, MemOperand(sp, 2340));
    __ str(r7, MemOperand(sp, 2356));
    __ str(r7, MemOperand(sp, 2360));
    __ str(r7, MemOperand(sp, 2364));
    __ str(r7, MemOperand(sp, 2372));
    __ str(r7, MemOperand(sp, 2376));
    __ str(r7, MemOperand(sp, 2380));
    __ str(r7, MemOperand(sp, 2384));
    __ str(r7, MemOperand(sp, 2388));
    __ str(r7, MemOperand(sp, 2396));
    __ str(r7, MemOperand(sp, 2400));
    __ str(r7, MemOperand(sp, 2404));
    __ str(r7, MemOperand(sp, 2408));
    __ str(r7, MemOperand(sp, 2412));
    __ str(r7, MemOperand(sp, 2420));
    __ str(r7, MemOperand(sp, 2424));
    __ str(r7, MemOperand(sp, 2428));
    __ str(r7, MemOperand(sp, 2432));
    __ str(r7, MemOperand(sp, 2436));
    __ str(r7, MemOperand(sp, 2444));
    __ str(r7, MemOperand(sp, 2448));
    __ str(r7, MemOperand(sp, 2452));
    __ str(r7, MemOperand(sp, 2456));
    __ str(r3, MemOperand(sp, 2312));
    __ strb(r8, MemOperand(sp, 2464));
    // --------------
    __ bind(&l_1a18);
    __ bl(&l_1a18);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_1a1e);
    __ bl(&l_1a1e);
    __ ldrb(Narrow, r3, MemOperand(r5, 17));
    __ str(r9, MemOperand(r5));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_1c2e);
    // --------------
    __ bind(&l_1a2e);
    __ add(sp, sp, 2476U);
    __ pop(RegisterList(0x8ff0));
    // --------------
    __ bind(&l_1a36);
    __ ldr(Narrow, r0, MemOperand(r5, 20));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_0604);
    __ ldr(Narrow, r3, MemOperand(r5, 24));
    __ movs(Narrow, r2, 0U);
    __ strb(Narrow, r2, MemOperand(r5, 17));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_0604);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(&l_0604);
    // --------------
    __ bind(&l_1a54);
    __ ldr(Narrow, r0, MemOperand(r5, 20));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_04aa);
    __ ldr(Narrow, r3, MemOperand(r5, 24));
    __ movs(Narrow, r2, 0U);
    __ strb(Narrow, r2, MemOperand(r5, 17));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_04aa);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(&l_04aa);
    // --------------
    __ bind(&l_1a72);
    __ ldr(Narrow, r0, MemOperand(r5, 20));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_02dc);
    __ ldr(Narrow, r3, MemOperand(r5, 24));
    __ movs(Narrow, r2, 0U);
    __ strb(Narrow, r2, MemOperand(r5, 17));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_02dc);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(&l_02dc);
    // --------------
    __ bind(&l_1a90);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(&l_0670);
    // --------------
    __ bind(&l_1a98);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(&l_04fc);
    // --------------
    __ bind(&l_1aa0);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(&l_0782);
    // --------------
    __ bind(&l_1aa8);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(&l_028e);
    // --------------
    __ bind(&l_1ab0);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(&l_03be);
    // --------------
    __ bind(&l_1ab8);
    __ str(Narrow, r3, MemOperand(r5));
    __ and_(r3, r8, 0x1f);
    __ b(&l_07a8);
    // --------------
    __ bind(&l_1ac2);
    __ ldr(r2, MemOperand(sl, 8));
    __ cmp(Narrow, r2, 4U);
    __ b(eq, &l_1d80);
    __ ldr(r2, MemOperand(sl, 16));
    __ ubfx(r2, r2, 5U, 4U);
    __ cmp(Narrow, r2, 0U);
    __ b(ne, &l_1d80);
    __ ldr(r1, MemOperand(sl, 12));
    __ mov(Narrow, ip, r2);
    __ cmp(Narrow, r1, 0U);
    __ b(ne, &l_07dc);
    __ cmp(Narrow, r3, 7U);
  }
  void Generate_25(TestAssembler* assembler) {
    __ b(hi, Narrow, &l_1b44);
    __ and_(ip, r0, 0x1f);
    __ cmp(ip, 7U);
    __ ite(hi);
    __ mov(hi, ip, 0U);
    __ mov(ls, ip, 1U);
    __ b(&l_07dc);
    // --------------
    __ bind(&l_1b00);
    __ mov(Narrow, ip, r3);
    __ b(Narrow, &l_16a2);
    // --------------
    __ bind(&l_1b04);
    __ ldr(Narrow, r3, MemOperand(r5, 16));
    __ mov(ip, 0U);
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_16a2);
    __ ldr(Narrow, r1, MemOperand(r5, 20));
    __ cmp(Narrow, r1, 0U);
    __ b(ne, Narrow, &l_1b00);
    __ ldr(Narrow, r3, MemOperand(sp, 72));
    __ and_(r3, r3, 0x1f);
    __ cmp(Narrow, r3, 7U);
    __ b(hi, Narrow, &l_1b3a);
    __ ldr(Narrow, r3, MemOperand(r5));
    __ and_(r3, r3, 0x1f);
    __ cmp(Narrow, r3, 7U);
    __ b(hi, Narrow, &l_1b3a);
    __ and_(r2, r2, 0x1f);
    __ cmp(Narrow, r2, 7U);
    __ b(hi, Narrow, &l_1b3a);
    __ ldr(Narrow, r3, MemOperand(r5, 8));
    __ cmp(Narrow, r3, 0U);
    __ b(eq, &l_1694);
    // --------------
    __ bind(&l_1b3a);
    __ mov(Narrow, ip, r1);
    __ b(Narrow, &l_16a2);
    // --------------
    __ bind(&l_1b3e);
    __ cmp(Narrow, r2, 255U);
    __ b(ls, &l_07d0);
    // --------------
    __ bind(&l_1b44);
    __ mov(Narrow, ip, r1);
    __ b(&l_07dc);
    // --------------
    __ bind(&l_1b4a);
    __ ldr(Narrow, r0, MemOperand(sp, 464));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_1772);
    __ ldr(Narrow, r3, MemOperand(sp, 468));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 461));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_1772);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_1772);
    // --------------
    __ bind(&l_1b68);
    __ ldr(Narrow, r0, MemOperand(sp, 296));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_10fe);
    __ ldr(Narrow, r3, MemOperand(sp, 300));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 293));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_10fe);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(&l_10fe);
    // --------------
    __ bind(&l_1b88);
    __ ldr(Narrow, r0, MemOperand(sp, 240));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_0fac);
    __ ldr(Narrow, r3, MemOperand(sp, 244));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 237));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_0fac);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(&l_0fac);
    // --------------
    __ bind(&l_1ba8);
    __ ldr(Narrow, r0, MemOperand(sp, 212));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_0ed8);
    __ ldr(Narrow, r3, MemOperand(sp, 216));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 209));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_0ed8);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(&l_0ed8);
    // --------------
    __ bind(&l_1bc8);
    __ ldr(Narrow, r0, MemOperand(sp, 352));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_12fc);
    __ ldr(Narrow, r3, MemOperand(sp, 356));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 349));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_12fc);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(&l_12fc);
    // --------------
    __ bind(&l_1be8);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_1bec);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_1bf0);
    __ ldr(Narrow, r0, MemOperand(sp, 380));
    __ cmp(Narrow, r0, 0U);
  }
  void Generate_26(TestAssembler* assembler) {
    __ b(eq, &l_1430);
    __ ldr(Narrow, r3, MemOperand(sp, 384));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 377));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_1430);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_1430);
    // --------------
    __ bind(&l_1c0e);
    __ ldr(Narrow, r0, MemOperand(sp, 184));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_0b2e);
    __ ldr(Narrow, r3, MemOperand(sp, 188));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 181));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_0b2e);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(&l_0b2e);
    // --------------
    __ bind(&l_1c2e);
    __ ldr(Narrow, r0, MemOperand(r5, 20));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_1a2e);
    __ ldr(Narrow, r3, MemOperand(r5, 24));
    __ strb(Narrow, r7, MemOperand(r5, 17));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_1a2e);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_1a2e);
    // --------------
    __ bind(&l_1c48);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_1c4e);
    __ bl(&l_1c4e);
    __ b(Narrow, &l_1568);
    // --------------
    __ bind(&l_1c54);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_1c5a);
    __ bl(&l_1c5a);
    __ b(Narrow, &l_165e);
    // --------------
    __ bind(&l_1c60);
    __ lsls(Narrow, r1, r1, 1U);
    __ ldr(Narrow, r3, &l_1f4c);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    __ str(Narrow, r3, MemOperand(sp, 24));
    // --------------
    __ bind(&l_1c6a);
    __ bl(&l_1c6a);
    __ b(&l_09fa);
    // --------------
    __ bind(&l_1c72);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_1c78);
    __ bl(&l_1c78);
    __ b(&l_0e9e);
    // --------------
    __ bind(&l_1c80);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_1c86);
    __ bl(&l_1c86);
    __ b(&l_0f60);
    // --------------
    __ bind(&l_1c8e);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_1c94);
    __ bl(&l_1c94);
    __ b(&l_101c);
    // --------------
    __ bind(&l_1c9c);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_1ca2);
    __ bl(&l_1ca2);
    __ b(&l_1488);
    // --------------
    __ bind(&l_1caa);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_1cb0);
    __ bl(&l_1cb0);
    __ b(&l_1350);
    // --------------
    __ bind(&l_1cb8);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_1cbe);
    __ bl(&l_1cbe);
    __ b(&l_1224);
    // --------------
    __ bind(&l_1cc6);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_1ccc);
    __ bl(&l_1ccc);
    __ b(&l_10bc);
    // --------------
    __ bind(&l_1cd4);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_1cda);
    __ bl(&l_1cda);
    __ b(&l_1168);
    // --------------
    __ bind(&l_1ce2);
    __ ldr(Narrow, r0, MemOperand(r5, 20));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_08a4);
    __ ldr(Narrow, r3, MemOperand(r5, 24));
    __ movs(Narrow, r2, 0U);
    __ strb(Narrow, r2, MemOperand(r5, 17));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_08a4);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
  }
  void Generate_27(TestAssembler* assembler) {
    __ blx(r3);
    __ b(&l_08a4);
    // --------------
    __ bind(&l_1d00);
    __ ldr(Narrow, r1, MemOperand(sp, 28));
    __ add(r2, r8, 96U);
    __ ldr(r3, MemOperand(r8, 132));
    __ mov(Narrow, r6, r2);
    __ mov(Narrow, r2, r1);
    __ cmp(Narrow, r1, r3);
    __ b(eq, Narrow, &l_1d9c);
    __ adds(r2, 2U);
    __ cmp(Narrow, r3, r2);
    __ b(lt, Narrow, &l_1d9c);
    __ add(r3, r8, 44U);
    __ str(Narrow, r3, MemOperand(sp, 24));
    __ mov(Narrow, r1, r3);
    __ b(&l_0aec);
    // --------------
    __ bind(&l_1d24);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(Narrow, &l_164c);
    // --------------
    __ bind(&l_1d2a);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(Narrow, &l_1556);
    // --------------
    __ bind(&l_1d30);
    __ ldr(Narrow, r3, &l_1f4c);
    __ str(Narrow, r3, MemOperand(sp, 24));
    __ blx(r2);
    __ b(&l_09e8);
    // --------------
    __ bind(&l_1d3a);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(&l_10aa);
    // --------------
    __ bind(&l_1d42);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(&l_1156);
    // --------------
    __ bind(&l_1d4a);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(Narrow, &l_193c);
    // --------------
    __ bind(&l_1d50);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(Narrow, &l_18b0);
    // --------------
    __ bind(&l_1d56);
    __ blx(r2);
    __ b(&l_0e8c);
    // --------------
    __ bind(&l_1d5c);
    __ blx(r2);
    __ b(&l_0f4e);
    // --------------
    __ bind(&l_1d62);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(&l_133e);
    // --------------
    __ bind(&l_1d6a);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(&l_1212);
    // --------------
    __ bind(&l_1d72);
    __ blx(r2);
    __ b(&l_100a);
    // --------------
    __ bind(&l_1d78);
    __ ldr(Narrow, r0, MemOperand(sp, 20));
    __ blx(r2);
    __ b(&l_1476);
    // --------------
    __ bind(&l_1d80);
    __ mov(ip, 0U);
    __ b(&l_07dc);
    // --------------
    __ bind(&l_1d88);
    __ mov(Narrow, r0, r5);
    // --------------
    __ bind(&l_1d8a);
    __ bl(&l_1d8a);
    __ b(&l_08a4);
    // --------------
    __ bind(&l_1d92);
    __ mov(Narrow, r0, r8);
    __ add(Narrow, r1, sp, 632U);
    __ blx(r2);
    __ b(&l_0b00);
    // --------------
    __ bind(&l_1d9c);
    __ add(r3, r8, 44U);
    __ ldr(Narrow, r2, MemOperand(sp, 28));
    __ mov(Narrow, r1, r3);
    __ str(Narrow, r3, MemOperand(sp, 24));
    __ movs(Narrow, r3, 0U);
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r3, MemOperand(sp));
    __ movs(Narrow, r3, 2U);
    __ bl(&l_00b0);
    __ ldr(Narrow, r1, MemOperand(sp, 24));
    __ b(&l_0aec);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_1dba);
    __ bl(&l_1dba);
    // --------------
    __ bind(&l_1dbe);
    __ ldr(Narrow, r3, &l_1f50);
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r3, MemOperand(r5));
    // --------------
    __ bind(&l_1dc4);
    __ bl(&l_1dc4);
    // --------------
    __ bind(&l_1dc8);
    __ bl(&l_1dc8);
    __ add(r0, sp, 1296U);
    // --------------
    __ bind(&l_1dd0);
    __ bl(&l_1dd0);
    // --------------
    __ bind(&l_1dd4);
    __ ldr(Narrow, r3, &l_1f50);
    __ add(Narrow, r0, sp, 276U);
    __ str(Narrow, r3, MemOperand(sp, 276));
    // --------------
    __ bind(&l_1dda);
    __ bl(&l_1dda);
    // --------------
    __ bind(&l_1dde);
    __ ldr(Narrow, r4, MemOperand(sp, 24));
    __ add(r0, sp, 2332U);
    __ str(r4, MemOperand(sp, 2304));
    // --------------
    __ bind(&l_1de8);
    __ bl(&l_1de8);
    __ add(r0, sp, 2164U);
    __ str(Narrow, r4, MemOperand(r5));
    // --------------
    __ bind(&l_1df2);
    __ bl(&l_1df2);
    // --------------
    __ bind(&l_1df6);
    __ ldr(Narrow, r3, MemOperand(sp, 24));
    __ add(Narrow, r0, sp, 652U);
    __ str(Narrow, r3, MemOperand(sp, 624));
    // --------------
    __ bind(&l_1dfc);
    __ bl(&l_1dfc);
    // --------------
    __ bind(&l_1e00);
    __ ldr(Narrow, r3, MemOperand(sp, 24));
    __ add(Narrow, r0, sp, 500U);
    __ str(Narrow, r3, MemOperand(sp, 472));
  }
  void Generate_28(TestAssembler* assembler) {
    // --------------
    __ bind(&l_1e06);
    __ bl(&l_1e06);
    // --------------
    __ bind(&l_1e0a);
    __ bl(&l_1e0a);
    __ b(Narrow, &l_1dd4);
    // --------------
    __ bind(&l_1e10);
    __ ldr(Narrow, r3, &l_1f50);
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r3, MemOperand(r5));
    // --------------
    __ bind(&l_1e16);
    __ bl(&l_1e16);
    // --------------
    __ bind(&l_1e1a);
    __ bl(&l_1e1a);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_1e20);
    __ bl(&l_1e20);
    __ b(Narrow, &l_1e10);
    // --------------
    __ bind(&l_1e26);
    __ ldr(Narrow, r3, &l_1f50);
    __ add(Narrow, r0, sp, 388U);
    __ str(Narrow, r3, MemOperand(sp, 388));
    // --------------
    __ bind(&l_1e2c);
    __ bl(&l_1e2c);
    __ b(Narrow, &l_1df6);
    __ ldr(Narrow, r3, &l_1f4c);
    __ add(Narrow, r0, sp, 624U);
    __ str(Narrow, r3, MemOperand(sp, 24));
    // --------------
    __ bind(&l_1e38);
    __ bl(&l_1e38);
    // --------------
    __ bind(&l_1e3c);
    __ ldr(Narrow, r3, &l_1f50);
    __ add(Narrow, r0, sp, 164U);
    __ str(Narrow, r3, MemOperand(sp, 164));
    // --------------
    __ bind(&l_1e42);
    __ bl(&l_1e42);
    __ b(Narrow, &l_1e00);
    // --------------
    __ bind(&l_1e48);
    __ ldr(Narrow, r3, &l_1f50);
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r3, MemOperand(r5));
    // --------------
    __ bind(&l_1e4e);
    __ bl(&l_1e4e);
    // --------------
    __ bind(&l_1e52);
    __ bl(&l_1e52);
    __ ldr(Narrow, r3, &l_1f4c);
    __ add(Narrow, r0, sp, 660U);
    __ str(Narrow, r3, MemOperand(sp, 24));
    __ str(Narrow, r3, MemOperand(sp, 632));
    // --------------
    __ bind(&l_1e5e);
    __ bl(&l_1e5e);
    __ b(Narrow, &l_1e3c);
    // --------------
    __ bind(&l_1e64);
    __ ldr(Narrow, r3, &l_1f50);
    __ add(Narrow, r0, sp, 220U);
    __ str(Narrow, r3, MemOperand(sp, 220));
    // --------------
    __ bind(&l_1e6a);
    __ bl(&l_1e6a);
    __ b(Narrow, &l_1dde);
    // --------------
    __ bind(&l_1e70);
    __ ldr(Narrow, r3, &l_1f50);
    __ add(Narrow, r0, sp, 332U);
    __ str(Narrow, r3, MemOperand(sp, 332));
    // --------------
    __ bind(&l_1e76);
    __ bl(&l_1e76);
    __ b(Narrow, &l_1df6);
    // --------------
    __ bind(&l_1e7c);
    __ ldr(Narrow, r3, &l_1f50);
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r3, MemOperand(r5));
    // --------------
    __ bind(&l_1e82);
    __ bl(&l_1e82);
    // --------------
    __ bind(&l_1e86);
    __ bl(&l_1e86);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_1e8c);
    __ bl(&l_1e8c);
    __ b(Narrow, &l_1e7c);
    __ add(r0, sp, 1128U);
    // --------------
    __ bind(&l_1e96);
    __ bl(&l_1e96);
    // --------------
    __ bind(&l_1e9a);
    __ ldr(Narrow, r3, &l_1f50);
    __ add(Narrow, r0, sp, 248U);
    __ str(Narrow, r3, MemOperand(sp, 248));
    // --------------
    __ bind(&l_1ea0);
    __ bl(&l_1ea0);
    __ b(Narrow, &l_1dde);
    __ b(Narrow, &l_1e9a);
    __ b(Narrow, &l_1e00);
    // --------------
    __ bind(&l_1eaa);
    __ ldr(Narrow, r3, &l_1f50);
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r3, MemOperand(r5));
    // --------------
    __ bind(&l_1eb0);
    __ bl(&l_1eb0);
    // --------------
    __ bind(&l_1eb4);
    __ bl(&l_1eb4);
    // --------------
    __ bind(&l_1eb8);
    __ ldr(Narrow, r3, &l_1f50);
    __ add(Narrow, r0, sp, 360U);
    __ str(Narrow, r3, MemOperand(sp, 360));
    // --------------
    __ bind(&l_1ebe);
    __ bl(&l_1ebe);
    __ b(Narrow, &l_1df6);
    __ add(r0, sp, 1800U);
    // --------------
    __ bind(&l_1ec8);
    __ bl(&l_1ec8);
    __ b(Narrow, &l_1eb8);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_1ed0);
    __ bl(&l_1ed0);
    __ b(Narrow, &l_1e48);
    // --------------
    __ bind(&l_1ed6);
    __ ldr(Narrow, r3, &l_1f50);
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r3, MemOperand(r5));
    // --------------
    __ bind(&l_1edc);
    __ bl(&l_1edc);
    // --------------
    __ bind(&l_1ee0);
    __ bl(&l_1ee0);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_1ee6);
    __ bl(&l_1ee6);
    __ b(Narrow, &l_1eaa);
    // --------------
    __ bind(&l_1eec);
    __ ldr(Narrow, r3, &l_1f50);
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r3, MemOperand(r5));
    // --------------
    __ bind(&l_1ef2);
    __ bl(&l_1ef2);
    // --------------
    __ bind(&l_1ef6);
    __ bl(&l_1ef6);
    __ b(Narrow, &l_1dbe);
    // --------------
    __ bind(&l_1efc);
    __ ldr(Narrow, r3, &l_1f50);
    __ add(Narrow, r0, sp, 444U);
    __ str(Narrow, r3, MemOperand(sp, 444));
    // --------------
    __ bind(&l_1f02);
    __ bl(&l_1f02);
    __ b(Narrow, &l_1e00);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_1f0a);
    __ bl(&l_1f0a);
  }
  void Generate_29(TestAssembler* assembler) {
    __ b(Narrow, &l_1eec);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_1f12);
    __ bl(&l_1f12);
    __ b(Narrow, &l_1ed6);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_1f1a);
    __ bl(&l_1f1a);
    __ b(Narrow, &l_1efc);
    __ mov(Narrow, r0, r5);
    // --------------
    __ bind(&l_1f22);
    __ bl(&l_1f22);
    // --------------
    __ bind(&l_1f26);
    __ ldr(Narrow, r3, &l_1f50);
    __ add(Narrow, r0, sp, 416U);
    __ str(Narrow, r3, MemOperand(sp, 416));
    // --------------
    __ bind(&l_1f2c);
    __ bl(&l_1f2c);
    __ b(Narrow, &l_1df6);
    __ b(Narrow, &l_1f26);
    __ mov(Narrow, r0, sl);
    // --------------
    __ bind(&l_1f36);
    __ bl(&l_1f36);
    __ b(Narrow, &l_1e26);
    __ b(Narrow, &l_1dde);
    __ b(Narrow, &l_1df6);
    // --------------
    __ bind(&l_1f40);
    __ ldr(Narrow, r3, &l_1f50);
    __ add(Narrow, r0, sp, 304U);
    __ str(Narrow, r3, MemOperand(sp, 304));
    // --------------
    __ bind(&l_1f46);
    __ bl(&l_1f46);
    __ b(Narrow, &l_1dde);
    // --------------
    __ bind(&l_1f4c);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_1f50);
    __ Emit32(0x00000008);
    __ add(Narrow, r0, sp, 960U);
    // --------------
    __ bind(&l_1f56);
    __ bl(&l_1f56);
    __ b(Narrow, &l_1e64);
    __ add(Narrow, r0, sp, 792U);
    // --------------
    __ bind(&l_1f5e);
    __ bl(&l_1f5e);
    // --------------
    __ bind(&l_1f62);
    __ ldr(Narrow, r3, &l_1f84);
    __ add(Narrow, r0, sp, 192U);
    __ str(Narrow, r3, MemOperand(sp, 192));
    // --------------
    __ bind(&l_1f68);
    __ bl(&l_1f68);
    __ b(Narrow, &l_1dde);
    __ b(Narrow, &l_1f62);
    __ add(r0, sp, 1632U);
    // --------------
    __ bind(&l_1f74);
    __ bl(&l_1f74);
    __ b(Narrow, &l_1e70);
    __ add(r0, sp, 1464U);
    // --------------
    __ bind(&l_1f7e);
    __ bl(&l_1f7e);
    __ b(Narrow, &l_1f40);
    // --------------
    __ bind(&l_1f84);
    __ Emit32(0x00000008);
    __ bx(lr);
    __ nop(Narrow);
    __ movs(Narrow, r0, 1U);
    __ bx(lr);
    __ bx(lr);
    __ nop(Narrow);
    __ movs(Narrow, r0, 1U);
    __ bx(lr);
    __ movs(Narrow, r0, 0U);
    __ bx(lr);
    __ movs(Narrow, r0, 0U);
    __ bx(lr);
    __ movs(Narrow, r0, 0U);
    __ bx(lr);
    __ movs(Narrow, r0, 0U);
    __ bx(lr);
    __ movs(Narrow, r0, 0U);
    __ bx(lr);
    __ push(Narrow, RegisterList(r4, lr));
    __ mov(Narrow, r4, r0);
    // --------------
    __ bind(&l_1fb0);
    __ bl(&l_1fb0);
    __ mov(Narrow, r0, r4);
    __ pop(Narrow, RegisterList(r4, pc));
    __ ldr(Narrow, r2, &l_1fcc);
    __ push(Narrow, RegisterList(r4, lr));
    __ mov(Narrow, r4, r0);
    __ str(r2, MemOperand(r0, 4, PostIndex));
    // --------------
    __ bind(&l_1fc2);
    __ bl(&l_1fc2);
    __ mov(Narrow, r0, r4);
    __ pop(Narrow, RegisterList(r4, pc));
    __ nop(Narrow);
    // --------------
    __ bind(&l_1fcc);
    __ Emit32(0x00000008);
    __ ldr(Narrow, r3, &l_1fe8);
    __ push(Narrow, RegisterList(r4, lr));
    __ mov(Narrow, r4, r0);
    __ str(r3, MemOperand(r0, 4, PostIndex));
    // --------------
    __ bind(&l_1fda);
    __ bl(&l_1fda);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_1fe0);
    __ bl(&l_1fe0);
    __ mov(Narrow, r0, r4);
    __ pop(Narrow, RegisterList(r4, pc));
    // --------------
    __ bind(&l_1fe8);
    __ Emit32(0x00000008);
    __ ldr(Narrow, r2, MemOperand(r0, 8));
    __ ldr(Narrow, r3, MemOperand(r1, 8));
    __ cmp(Narrow, r2, r3);
    __ b(eq, Narrow, &l_1ffc);
    // --------------
    __ bind(&l_1ff4);
    __ ite(lt);
    __ mov(lt, Narrow, r0, 1U);
    __ mov(ge, Narrow, r0, 0U);
    __ bx(lr);
    // --------------
    __ bind(&l_1ffc);
    __ ldr(Narrow, r2, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r1));
    __ ldr(Narrow, r2, MemOperand(r2, 4));
    __ ldr(Narrow, r3, MemOperand(r3, 4));
    __ cmp(Narrow, r2, r3);
  }
  void Generate_30(TestAssembler* assembler) {
    __ b(ne, Narrow, &l_1ff4);
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ ldr(Narrow, r3, MemOperand(r1, 12));
    __ cmp(Narrow, r2, r3);
    __ b(ne, Narrow, &l_1ff4);
    __ ldr(Narrow, r2, MemOperand(r0, 4));
    __ ldr(Narrow, r3, MemOperand(r1, 4));
    __ cmp(Narrow, r2, r3);
    __ b(ne, Narrow, &l_1ff4);
    __ movs(Narrow, r0, 0U);
    __ bx(lr);
    __ ldrb(Narrow, r3, MemOperand(r0, 17));
    __ ldr(Narrow, r2, &l_2044);
    __ push(Narrow, RegisterList(r4, lr));
    __ mov(Narrow, r4, r0);
    __ str(Narrow, r2, MemOperand(r0));
    __ cbnz(r3, &l_202c);
    // --------------
    __ bind(&l_2028);
    __ mov(Narrow, r0, r4);
    __ pop(Narrow, RegisterList(r4, pc));
    // --------------
    __ bind(&l_202c);
    __ ldr(Narrow, r0, MemOperand(r0, 20));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_2028);
    __ ldr(Narrow, r3, MemOperand(r4, 24));
    __ movs(Narrow, r2, 0U);
    __ strb(Narrow, r2, MemOperand(r4, 17));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_2028);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_2028);
    // --------------
    __ bind(&l_2044);
    __ Emit32(0x00000008);
    __ ldrb(Narrow, r3, MemOperand(r0, 17));
    __ ldr(Narrow, r2, &l_2070);
    __ push(Narrow, RegisterList(r4, lr));
    __ mov(Narrow, r4, r0);
    __ str(Narrow, r2, MemOperand(r0));
    __ cbnz(r3, &l_2058);
    // --------------
    __ bind(&l_2054);
    __ mov(Narrow, r0, r4);
    __ pop(Narrow, RegisterList(r4, pc));
    // --------------
    __ bind(&l_2058);
    __ ldr(Narrow, r0, MemOperand(r0, 20));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_2054);
    __ ldr(Narrow, r3, MemOperand(r4, 24));
    __ movs(Narrow, r2, 0U);
    __ strb(Narrow, r2, MemOperand(r4, 17));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_2054);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_2054);
    // --------------
    __ bind(&l_2070);
    __ Emit32(0x00000008);
    __ ldrb(Narrow, r3, MemOperand(r0, 17));
    __ ldr(Narrow, r2, &l_20a4);
    __ push(Narrow, RegisterList(r4, lr));
    __ mov(Narrow, r4, r0);
    __ str(Narrow, r2, MemOperand(r0));
    __ cbnz(r3, &l_208a);
    // --------------
    __ bind(&l_2080);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_2082);
    __ bl(&l_2082);
    __ mov(Narrow, r0, r4);
    __ pop(Narrow, RegisterList(r4, pc));
    // --------------
    __ bind(&l_208a);
    __ ldr(Narrow, r0, MemOperand(r0, 20));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_2080);
    __ ldr(Narrow, r3, MemOperand(r4, 24));
    __ movs(Narrow, r2, 0U);
    __ strb(Narrow, r2, MemOperand(r4, 17));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_2080);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_2080);
    __ nop(Narrow);
    // --------------
    __ bind(&l_20a4);
    __ Emit32(0x00000008);
    __ ldrb(Narrow, r3, MemOperand(r0, 17));
    __ ldr(Narrow, r2, &l_20d8);
    __ push(Narrow, RegisterList(r4, lr));
    __ mov(Narrow, r4, r0);
    __ str(Narrow, r2, MemOperand(r0));
    __ cbnz(r3, &l_20be);
    // --------------
    __ bind(&l_20b4);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_20b6);
    __ bl(&l_20b6);
    __ mov(Narrow, r0, r4);
    __ pop(Narrow, RegisterList(r4, pc));
    // --------------
    __ bind(&l_20be);
    __ ldr(Narrow, r0, MemOperand(r0, 20));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_20b4);
    __ ldr(Narrow, r3, MemOperand(r4, 24));
    __ movs(Narrow, r2, 0U);
    __ strb(Narrow, r2, MemOperand(r4, 17));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_20b4);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_20b4);
    __ nop(Narrow);
  }
  void Generate_31(TestAssembler* assembler) {
    // --------------
    __ bind(&l_20d8);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_20dc);
    __ bx(lr);
    __ nop(Narrow);
    __ sub(r0, r0, 44U);
    __ b(Narrow, &l_20dc);
    __ nop(Narrow);
    __ push(Narrow, RegisterList(r3, r4, r5, lr));
    __ mov(Narrow, r5, r0);
    __ ldr(Narrow, r4, MemOperand(r0, 116));
    __ cbz(r4, &l_20fe);
    __ ldr(Narrow, r0, MemOperand(r4));
    __ cbz(r0, &l_20f8);
    // --------------
    __ bind(&l_20f4);
    __ bl(&l_20f4);
    // --------------
    __ bind(&l_20f8);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_20fa);
    __ bl(&l_20fa);
    // --------------
    __ bind(&l_20fe);
    __ mov(Narrow, r0, r5);
    __ pop(Narrow, RegisterList(r3, r4, r5, pc));
    __ nop(Narrow);
    __ push(RegisterList(0x4ff0));
    __ sub(Narrow, sp, sp, 12U);
    __ ldr(Narrow, r6, MemOperand(r0, 36));
    __ ldr(Narrow, r4, MemOperand(sp, 48));
    __ cmp(Narrow, r6, r1);
    __ b(eq, &l_22de);
    __ adds(Narrow, r5, r1, r2);
    __ cmp(Narrow, r6, r5);
    __ b(lt, &l_22de);
    __ cmp(Narrow, r3, 0U);
    __ b(eq, &l_2304);
    __ ldr(Narrow, r7, MemOperand(r0));
    __ ldr(Narrow, r6, MemOperand(r0, 4));
    __ cmp(Narrow, r7, r6);
    __ b(eq, &l_2304);
    // --------------
    __ bind(&l_212c);
    __ ldr(Narrow, r1, MemOperand(r7));
    __ cmp(Narrow, r4, r1);
    __ b(eq, &l_22e6);
    __ adds(Narrow, r7, r7, 24U);
    __ cmp(Narrow, r6, r7);
    __ b(ne, Narrow, &l_212c);
    __ movs(Narrow, r7, 0U);
    // --------------
    __ bind(&l_213c);
    __ mov(Narrow, r9, r3);
    __ ldr(Narrow, r3, MemOperand(r4));
    __ mov(Narrow, fp, r0);
    __ str(Narrow, r2, MemOperand(sp, 4));
    __ mov(Narrow, r0, r4);
    __ ldr(sl, MemOperand(r4, 8));
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ cbz(r0, &l_2158);
    __ ldr(Narrow, r3, MemOperand(r4));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r3, MemOperand(r3, 36));
    __ blx(r3);
    // --------------
    __ bind(&l_2158);
    __ ldr(r3, MemOperand(r9, 12));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r2, MemOperand(r4));
    __ ldr(r1, MemOperand(r9, 16));
    __ ldr(Narrow, r2, MemOperand(r2, 32));
    __ bic(r8, r3, Operand(r3, ASR, 31));
    __ str(Narrow, r1, MemOperand(sp));
    __ blx(r2);
    __ cbz(r0, &l_2178);
    __ ldr(Narrow, r3, MemOperand(r4));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r3, MemOperand(r3, 36));
    __ blx(r3);
    // --------------
    __ bind(&l_2178);
    __ ldr(r3, MemOperand(r9, 8));
    __ cmp(Narrow, sl, r3);
    __ it(lt);
    __ mov(lt, Narrow, sl, r3);
    __ cbz(r7, &l_21ae);
    __ ldr(Narrow, r2, MemOperand(r7, 4));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r3, MemOperand(r7, 8));
    __ ldr(Narrow, r1, MemOperand(r4));
    __ cmp(Narrow, r8, r2);
    __ it(lt);
    __ mov(lt, Narrow, r8, r2);
    __ ldr(Narrow, r2, MemOperand(sp));
    __ ldr(Narrow, r1, MemOperand(r1, 32));
    __ cmp(Narrow, r2, r3);
    __ it(ge);
    __ mov(ge, Narrow, r2, r3);
    __ str(Narrow, r2, MemOperand(sp));
    __ blx(r1);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, &l_230c);
    // --------------
    __ bind(&l_21a6);
    __ ldr(Narrow, r3, MemOperand(r7, 12));
    __ cmp(Narrow, sl, r3);
    __ it(lt);
    __ mov(lt, Narrow, sl, r3);
    // --------------
    __ bind(&l_21ae);
    __ ldr(r1, MemOperand(fp, 4));
    __ rsb(lr, sl, 0U);
    __ ldr(Narrow, r3, MemOperand(sp));
    __ ldr(r7, MemOperand(r1, -12));
    __ and_(r2, lr, r3);
    __ ldr(r3, MemOperand(r1, -24));
    __ ldr(r0, MemOperand(r1, -16));
    __ ldr(ip, MemOperand(r3, 4));
    __ rsbs(Narrow, r3, r7, 0U);
  }
  void Generate_32(TestAssembler* assembler) {
    __ ands(Narrow, r3, r3, r0);
    __ add(Narrow, r3, r3, ip);
    __ cmp(Narrow, r3, r2);
    __ b(le, &l_2304);
    __ ldr(r3, MemOperand(fp, 40));
    __ mov(Narrow, r6, r3);
    __ ldr(Narrow, r3, MemOperand(sp, 4));
    __ add(Narrow, r3, r3, r5);
    __ mov(Narrow, r9, r3);
    __ mov(Narrow, r3, r6);
    __ add(Narrow, r3, r3, r9);
    __ cmp(Narrow, r3, r2);
    __ b(le, &l_2304);
    __ ldr(Narrow, r3, MemOperand(sp));
    __ cmp(Narrow, r0, r3);
    __ b(eq, &l_233a);
    // --------------
    __ bind(&l_21f6);
    __ ite(lt);
    __ mov(lt, Narrow, r3, 1U);
    __ mov(ge, Narrow, r3, 0U);
    // --------------
    __ bind(&l_21fc);
    __ cmp(Narrow, r3, 0U);
    __ b(eq, &l_2334);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ mov(r2, 65535U);
    __ movt(r2, 32767U);
    __ subs(Narrow, r2, r2, r3);
    __ ldr(Narrow, r3, MemOperand(sp));
    __ cmp(Narrow, r3, r2);
    __ ite(le);
    __ and_(le, r2, lr, r3);
    __ and_(gt, r2, lr, r2);
    __ cmp(Narrow, r2, r8);
    __ b(lt, Narrow, &l_22de);
    // --------------
    __ bind(&l_2220);
    __ ldr(r0, MemOperand(fp));
    __ mov(r3, 43691U);
    __ movt(r3, 43690U);
    __ subs(Narrow, r1, r1, r0);
    __ asrs(Narrow, r1, r1, 3U);
    __ mul(r1, r3, r1);
    __ subs(Narrow, r3, r1, 1U);
    __ b(mi, Narrow, &l_22c8);
    __ lsls(Narrow, r7, r3, 5U);
    __ sub(ip, r0, 112U);
    __ movs(Narrow, r1, 1U);
    __ sub(r3, r7, Operand(r3, LSL, 3));
    __ subs(Narrow, r3, r3, 88U);
    __ add(Narrow, r3, r3, r0);
    // --------------
    __ bind(&l_2248);
    __ cbz(r1, &l_2282);
    __ ldr(Narrow, r0, MemOperand(r3, 96));
    __ ldr(Narrow, r6, MemOperand(sp));
    __ pld(MemOperand(r3));
    __ cmp(Narrow, r0, r6);
    __ b(eq, Narrow, &l_2316);
    // --------------
    __ bind(&l_2256);
    __ ite(lt);
    __ mov(lt, Narrow, r0, 1U);
    __ mov(ge, Narrow, r0, 0U);
    // --------------
    __ bind(&l_225c);
    __ cbz(r0, &l_2282);
    __ ldr(Narrow, r1, MemOperand(r4, 4));
    __ subs(Narrow, r2, r2, r1);
    __ ldr(Narrow, r1, MemOperand(sp));
    __ cmp(Narrow, r2, r1);
    __ ite(le);
    __ and_(le, r2, lr, r2);
    __ and_(gt, r2, lr, r1);
    __ cmp(Narrow, r5, r2);
    __ ite(le);
    __ mov(le, Narrow, r1, 0U);
    __ mov(gt, Narrow, r1, 1U);
    __ cmp(Narrow, r2, r8);
    __ it(lt);
    __ orr(lt, r1, r1, 0x1);
    __ cbnz(r1, &l_22de);
    // --------------
    __ bind(&l_2282);
    __ ldr(Narrow, r0, MemOperand(r3, 88));
    // --------------
    __ bind(&l_2284);
    __ cmp(Narrow, r4, r0);
    __ b(eq, Narrow, &l_22a8);
    __ ldr(Narrow, r0, MemOperand(r0, 4));
    __ ldr(Narrow, r7, MemOperand(r3, 100));
    __ subs(Narrow, r2, r2, r0);
    __ ldr(Narrow, r0, MemOperand(r3, 92));
    __ rsbs(Narrow, r7, r7, 0U);
    __ mov(Narrow, r6, r0);
    __ ldr(Narrow, r0, MemOperand(r3, 96));
    __ cmp(Narrow, r2, r0);
    __ ite(le);
    __ and_(le, Narrow, r2, r2, r7);
    __ and_(gt, r2, r7, r0);
    __ cmp(Narrow, r5, r2);
    __ it(le);
    __ cmp(le, Narrow, r6, r2);
    __ b(gt, Narrow, &l_22de);
    // --------------
    __ bind(&l_22a8);
    __ subs(Narrow, r3, r3, 24U);
    __ cmp(Narrow, r3, ip);
    __ b(ne, Narrow, &l_2248);
    __ cbnz(r1, &l_22c8);
    // --------------
    __ bind(&l_22b0);
    __ ldr(r3, MemOperand(fp, 24));
    __ ldr(r0, MemOperand(fp, 28));
    __ subs(Narrow, r2, r2, r3);
    __ rsbs(Narrow, r0, r0, 0U);
    __ ands(Narrow, r2, r2, r0);
    __ cmp(Narrow, r5, r2);
    __ ite(le);
  }
  void Generate_33(TestAssembler* assembler) {
    __ mov(le, Narrow, r0, 0U);
    __ mov(gt, Narrow, r0, 1U);
    __ b(Narrow, &l_22e0);
    // --------------
    __ bind(&l_22c8);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ subs(Narrow, r2, r2, r3);
    __ ldr(Narrow, r3, MemOperand(sp));
    __ cmp(Narrow, r2, r3);
    __ ite(le);
    __ and_(le, r2, lr, r2);
    __ and_(gt, r2, lr, r3);
    __ cmp(Narrow, r2, r8);
    __ b(ge, Narrow, &l_22b0);
    // --------------
    __ bind(&l_22de);
    __ movs(Narrow, r0, 1U);
    // --------------
    __ bind(&l_22e0);
    __ add(Narrow, sp, sp, 12U);
    __ pop(RegisterList(0x8ff0));
    // --------------
    __ bind(&l_22e6);
    __ ldr(Narrow, r6, MemOperand(r3, 12));
    __ ldr(Narrow, r1, MemOperand(r7, 4));
    __ cmp(Narrow, r6, r1);
    __ b(gt, &l_213c);
    __ ldr(Narrow, r6, MemOperand(r3, 16));
    __ ldr(Narrow, r1, MemOperand(r7, 8));
    __ cmp(Narrow, r6, r1);
    __ b(lt, &l_213c);
    __ ldr(Narrow, r6, MemOperand(r3, 8));
    __ ldr(Narrow, r1, MemOperand(r7, 12));
    __ cmp(Narrow, r6, r1);
    __ b(gt, &l_213c);
    // --------------
    __ bind(&l_2304);
    __ movs(Narrow, r0, 0U);
    __ add(Narrow, sp, sp, 12U);
    __ pop(RegisterList(0x8ff0));
    // --------------
    __ bind(&l_230c);
    __ ldr(Narrow, r3, MemOperand(r4));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r3, MemOperand(r3, 36));
    __ blx(r3);
    __ b(Narrow, &l_21a6);
    // --------------
    __ bind(&l_2316);
    __ ldr(Narrow, r0, MemOperand(r3, 88));
    __ ldr(Narrow, r7, MemOperand(r4, 4));
    __ ldr(Narrow, r6, MemOperand(r0, 4));
    __ cmp(Narrow, r6, r7);
    __ b(ne, Narrow, &l_2256);
    __ ldr(Narrow, r7, MemOperand(r3, 100));
    __ cmp(Narrow, r7, sl);
    __ b(ne, Narrow, &l_2256);
    __ ldr(Narrow, r7, MemOperand(r3, 92));
    __ cmp(Narrow, r8, r7);
    __ b(eq, Narrow, &l_2284);
    __ ite(gt);
    __ mov(gt, Narrow, r0, 1U);
    __ mov(le, Narrow, r0, 0U);
    __ b(Narrow, &l_225c);
    // --------------
    __ bind(&l_2334);
    __ mvn(r2, 2147483648U);
    __ b(Narrow, &l_2220);
    // --------------
    __ bind(&l_233a);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ cmp(Narrow, ip, r3);
    __ b(ne, &l_21f6);
    __ cmp(Narrow, r7, sl);
    __ b(ne, &l_21f6);
    __ ldr(r3, MemOperand(r1, -20));
    __ cmp(Narrow, r8, r3);
    __ b(eq, Narrow, &l_2334);
    __ ite(gt);
    __ mov(gt, Narrow, r3, 1U);
    __ mov(le, Narrow, r3, 0U);
    __ b(Narrow, &l_21fc);
    __ push(RegisterList(0x43f0));
    __ mov(Narrow, r5, r0);
    __ mov(Narrow, r6, r1);
    __ ldr(Narrow, r0, MemOperand(r0, 4));
    __ sub(Narrow, sp, sp, 12U);
    __ ldr(Narrow, r1, MemOperand(r5, 8));
    __ cmp(Narrow, r0, r1);
    __ b(eq, Narrow, &l_2394);
    __ cbz(r0, &l_2374);
    __ ldr(r1, MemOperand(r0, -4));
    __ str(Narrow, r1, MemOperand(r0));
    __ ldr(Narrow, r0, MemOperand(r5, 4));
    // --------------
    __ bind(&l_2374);
    __ subs(Narrow, r4, r0, 4U);
    __ adds(Narrow, r1, r0, 4U);
    __ subs(Narrow, r4, r4, r6);
    __ str(Narrow, r1, MemOperand(r5, 4));
    __ lsrs(Narrow, r3, r4, 2U);
    __ ldr(Narrow, r5, MemOperand(r2));
    __ b(eq, Narrow, &l_238c);
    __ subs(Narrow, r0, r0, r4);
    __ mov(Narrow, r2, r4);
    __ mov(Narrow, r1, r6);
    // --------------
    __ bind(&l_2388);
    __ bl(&l_2388);
    // --------------
    __ bind(&l_238c);
    __ str(Narrow, r5, MemOperand(r6));
    // --------------
    __ bind(&l_238e);
    __ add(Narrow, sp, sp, 12U);
    __ pop(RegisterList(0x83f0));
    // --------------
    __ bind(&l_2394);
    __ ldr(Narrow, r1, MemOperand(r5));
    __ subs(Narrow, r3, r0, r1);
    __ asrs(Narrow, r3, r3, 2U);
    __ b(eq, Narrow, &l_23ee);
    __ lsls(Narrow, r0, r3, 1U);
    __ cmp(Narrow, r3, r0);
    __ b(ls, Narrow, &l_240a);
    // --------------
    __ bind(&l_23a2);
    __ subs(Narrow, r7, r6, r1);
    __ mvn(r4, 3U);
    // --------------
    __ bind(&l_23a8);
    __ mov(Narrow, r0, r4);
  }
  void Generate_34(TestAssembler* assembler) {
    __ str(Narrow, r2, MemOperand(sp, 4));
    // --------------
    __ bind(&l_23ac);
    __ bl(&l_23ac);
    __ ldr(Narrow, r2, MemOperand(sp, 4));
    __ mov(Narrow, r9, r0);
    // --------------
    __ bind(&l_23b4);
    __ cmn(r9, r7);
    __ itt(ne);
    __ ldr(ne, Narrow, r3, MemOperand(r2));
    __ str(ne, r3, MemOperand(r9, r7));
    __ ldr(Narrow, r1, MemOperand(r5));
    __ subs(Narrow, r7, r6, r1);
    __ lsrs(Narrow, r3, r7, 2U);
    __ b(ne, Narrow, &l_2400);
    // --------------
    __ bind(&l_23c8);
    __ ldr(Narrow, r3, MemOperand(r5, 4));
    __ adds(r7, 4U);
    __ add(Narrow, r7, r7, r9);
    __ rsb(r8, r6, r3);
    __ lsrs(r3, r8, 2U);
    __ b(ne, Narrow, &l_23f4);
    // --------------
    __ bind(&l_23d8);
    __ ldr(Narrow, r0, MemOperand(r5));
    __ add(Narrow, r7, r7, r8);
    __ cbz(r0, &l_23e2);
    // --------------
    __ bind(&l_23de);
    __ bl(&l_23de);
    // --------------
    __ bind(&l_23e2);
    __ add(Narrow, r4, r4, r9);
    __ str(r9, MemOperand(r5));
    __ str(Narrow, r4, MemOperand(r5, 8));
    __ str(Narrow, r7, MemOperand(r5, 4));
    __ b(Narrow, &l_238e);
    // --------------
    __ bind(&l_23ee);
    __ subs(Narrow, r7, r6, r1);
    __ movs(Narrow, r4, 4U);
    __ b(Narrow, &l_23a8);
    // --------------
    __ bind(&l_23f4);
    __ mov(Narrow, r1, r6);
    __ mov(Narrow, r0, r7);
    __ mov(Narrow, r2, r8);
    // --------------
    __ bind(&l_23fa);
    __ bl(&l_23fa);
    __ b(Narrow, &l_23d8);
    // --------------
    __ bind(&l_2400);
    __ mov(Narrow, r0, r9);
    __ mov(Narrow, r2, r7);
    // --------------
    __ bind(&l_2404);
    __ bl(&l_2404);
    __ b(Narrow, &l_23c8);
    // --------------
    __ bind(&l_240a);
    __ cmp(r0, 1073741824U);
    __ b(cs, Narrow, &l_23a2);
    __ lsls(Narrow, r4, r3, 3U);
    __ subs(Narrow, r7, r6, r1);
    __ mov(Narrow, r9, r0);
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_23b4);
    __ b(Narrow, &l_23a8);
    __ ldr(Narrow, r3, MemOperand(r1));
    __ push(Narrow, RegisterList(r4, r5, r6, lr));
    __ sub(Narrow, sp, sp, 8U);
    __ mov(Narrow, r6, r0);
    __ mov(Narrow, r5, r1);
    __ mov(Narrow, r0, r3);
    __ ldr(Narrow, r2, MemOperand(r3));
    __ str(Narrow, r3, MemOperand(sp, 4));
    __ ldr(Narrow, r3, MemOperand(r2, 12));
    __ blx(r3);
    __ cbnz(r0, &l_2474);
    // --------------
    __ bind(&l_2432);
    __ ldr(Narrow, r0, MemOperand(sp, 4));
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 8));
    __ blx(r3);
    __ cbz(r0, &l_2446);
    __ ldr(Narrow, r0, MemOperand(sp, 4));
    __ cbz(r0, &l_2446);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 4));
    __ blx(r3);
    // --------------
    __ bind(&l_2446);
    __ ldr(Narrow, r4, MemOperand(r6, 4));
    __ add(r1, r5, 24U);
    __ cmp(Narrow, r1, r4);
    __ b(eq, Narrow, &l_246a);
    __ subs(Narrow, r2, r4, r1);
    __ mov(r3, 43691U);
    __ asrs(Narrow, r0, r2, 3U);
    __ movt(r3, 43690U);
    __ mul(r3, r3, r0);
    __ cmp(Narrow, r3, 0U);
    __ b(le, Narrow, &l_246a);
    __ mov(Narrow, r0, r5);
    // --------------
    __ bind(&l_2466);
    __ bl(&l_2466);
    // --------------
    __ bind(&l_246a);
    __ subs(Narrow, r4, r4, 24U);
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r4, MemOperand(r6, 4));
    __ add(Narrow, sp, sp, 8U);
    __ pop(Narrow, RegisterList(r4, r5, r6, pc));
    // --------------
    __ bind(&l_2474);
    __ ldr(Narrow, r1, MemOperand(r6, 16));
    __ ldr(Narrow, r3, MemOperand(r6, 20));
    __ cmp(Narrow, r1, r3);
    __ b(eq, Narrow, &l_248a);
    __ cbz(r1, &l_2484);
    __ ldr(Narrow, r3, MemOperand(sp, 4));
    __ str(Narrow, r3, MemOperand(r1));
    __ ldr(Narrow, r1, MemOperand(r6, 16));
    // --------------
    __ bind(&l_2484);
    __ adds(r1, 4U);
    __ str(Narrow, r1, MemOperand(r6, 16));
    __ b(Narrow, &l_2432);
    // --------------
    __ bind(&l_248a);
    __ add(r0, r6, 12U);
    __ add(Narrow, r2, sp, 4U);
    // --------------
    __ bind(&l_2490);
    __ bl(&l_2490);
  }
  void Generate_35(TestAssembler* assembler) {
    __ b(Narrow, &l_2432);
    __ nop(Narrow);
    __ push(RegisterList(0x4ff8));
    __ mov(Narrow, r5, r0);
    __ mov(Narrow, r6, r1);
    __ ldr(Narrow, r0, MemOperand(r0));
    __ mov(Narrow, r8, r2);
    __ mov(Narrow, fp, r3);
    __ ldr(Narrow, r1, MemOperand(r5, 4));
    __ cmp(Narrow, r0, r1);
    __ b(eq, Narrow, &l_24ca);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ cmp(Narrow, r2, r3);
    __ it(ne);
    __ add(ne, r4, r0, 24U);
    __ b(ne, Narrow, &l_24c4);
    __ b(Narrow, &l_25ce);
    // --------------
    __ bind(&l_24ba);
    __ ldr(r2, MemOperand(r4, 24, PostIndex));
    __ cmp(Narrow, r8, r2);
    __ b(eq, &l_25ce);
    // --------------
    __ bind(&l_24c4);
    __ cmp(Narrow, r4, r1);
    __ mov(Narrow, r0, r4);
    __ b(ne, Narrow, &l_24ba);
    // --------------
    __ bind(&l_24ca);
    __ ldr(r3, MemOperand(r8));
    __ mov(Narrow, r0, r8);
    __ mov(sl, 0U);
    __ ldr(Narrow, r3, MemOperand(r3, 40));
    __ blx(r3);
    __ ldr(r3, MemOperand(r8));
    __ mov(Narrow, r7, r0);
    __ mov(Narrow, r0, r8);
    __ ldr(Narrow, r3, MemOperand(r3, 44));
    __ blx(r3);
    __ mov(Narrow, r9, r0);
    // --------------
    __ bind(&l_24e6);
    __ subs(Narrow, r4, r7, 1U);
    __ ldr(Narrow, r2, MemOperand(r6));
    __ add(r3, r4, fp);
    __ mov(Narrow, r0, r6);
    __ bic(r4, r3, r4);
    __ rsb(r1, fp, r4);
    __ ldr(Narrow, r3, MemOperand(r2, 48));
    __ blx(r3);
    __ cmp(Narrow, r9, r4);
    __ b(le, Narrow, &l_2510);
    // --------------
    __ bind(&l_2500);
    __ ldr(Narrow, r3, MemOperand(r6));
    __ add(Narrow, r4, r4, r7);
    __ mov(Narrow, r0, r6);
    __ mov(Narrow, r1, r7);
    __ ldr(Narrow, r3, MemOperand(r3, 48));
    __ blx(r3);
    __ cmp(Narrow, r9, r4);
    __ b(gt, Narrow, &l_2500);
    // --------------
    __ bind(&l_2510);
    __ ldr(Narrow, r2, MemOperand(r6));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, Narrow, &l_25f4);
    // --------------
    __ bind(&l_2522);
    __ bl(&l_2522);
    __ mov(Narrow, r1, r0);
    // --------------
    __ bind(&l_2528);
    __ ldr(r3, MemOperand(r8));
    __ mov(Narrow, r0, r8);
    __ str(r4, MemOperand(r8, 20));
    __ ldr(Narrow, r3, MemOperand(r3, 20));
    __ blx(r3);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(r8, 16));
    __ cmp(sl, 0U);
    __ b(eq, Narrow, &l_25c8);
    __ ldr(Narrow, r7, MemOperand(r5));
    __ ldr(Narrow, r0, MemOperand(r5, 4));
    __ cmp(Narrow, r7, r0);
    __ b(eq, Narrow, &l_25fa);
    __ ldr(Narrow, r3, MemOperand(r7));
    __ ldr(r6, MemOperand(sl));
    __ cmp(Narrow, r3, r6);
    __ it(ne);
    __ add(ne, r3, r7, 24U);
    __ b(ne, Narrow, &l_2564);
    __ b(Narrow, &l_25d8);
    // --------------
    __ bind(&l_255c);
    __ ldr(r2, MemOperand(r3, 24, PostIndex));
    __ cmp(Narrow, r2, r6);
    __ b(eq, Narrow, &l_25da);
    // --------------
    __ bind(&l_2564);
    __ cmp(Narrow, r3, r0);
    __ mov(Narrow, r1, r3);
    __ b(ne, Narrow, &l_255c);
    // --------------
    __ bind(&l_256a);
    __ subs(Narrow, r2, r3, r7);
    __ mov(r0, 43691U);
    __ asrs(Narrow, r2, r2, 3U);
    __ movt(r0, 43690U);
    __ ldr(Narrow, r1, MemOperand(r5, 28));
    __ mvn(r6, 2147483648U);
    __ mul(r2, r0, r2);
    __ ldr(ip, MemOperand(r5, 24));
    __ add(lr, r1, 4294967295U);
    __ add(r0, ip, Operand(lr, LSL, 1));
    __ cmp(Narrow, r2, 0U);
    __ str(Narrow, r0, MemOperand(r5, 40));
    __ b(le, Narrow, &l_25be);
  }
  void Generate_36(TestAssembler* assembler) {
    // --------------
    __ bind(&l_2592);
    __ ldr(r1, MemOperand(r3, -24));
    __ subs(Narrow, r3, r3, 24U);
    __ ldr(Narrow, r2, MemOperand(r3, 12));
    __ ldr(r8, MemOperand(r3, 8));
    __ ldr(Narrow, r1, MemOperand(r1, 4));
    __ subs(r2, 1U);
    __ mvn(r9, r2);
    __ add(Narrow, r2, r2, r1);
    __ subs(Narrow, r1, r6, r1);
    __ cmp(Narrow, r1, r8);
    __ ite(le);
    __ and_(le, r6, r9, r1);
    __ and_(gt, r6, r9, r8);
    __ cmp(Narrow, r3, r7);
    __ add(Narrow, r0, r0, r2);
    __ str(Narrow, r0, MemOperand(r5, 40));
    __ b(ne, Narrow, &l_2592);
    // --------------
    __ bind(&l_25be);
    __ rsb(r1, ip, r6);
    __ bic(r1, r1, lr);
    __ str(Narrow, r1, MemOperand(r5, 36));
    // --------------
    __ bind(&l_25c8);
    __ mov(Narrow, r0, r4);
    __ pop(RegisterList(0x8ff8));
    // --------------
    __ bind(&l_25ce);
    __ ldr(Narrow, r7, MemOperand(r0, 12));
    __ mov(Narrow, sl, r0);
    __ ldr(r9, MemOperand(r0, 4));
    __ b(Narrow, &l_24e6);
    // --------------
    __ bind(&l_25d8);
    __ mov(Narrow, r1, r7);
    // --------------
    __ bind(&l_25da);
    __ mov(Narrow, r0, r5);
    // --------------
    __ bind(&l_25dc);
    __ bl(&l_25dc);
    __ ldr(Narrow, r7, MemOperand(r5));
    __ movs(Narrow, r2, 0U);
    __ ldr(Narrow, r3, MemOperand(r5, 4));
    __ str(Narrow, r2, MemOperand(r5, 40));
    __ cmp(Narrow, r7, r3);
    __ b(ne, Narrow, &l_256a);
    // --------------
    __ bind(&l_25ec);
    __ mvn(r3, 2147483648U);
    __ str(Narrow, r3, MemOperand(r5, 36));
    __ b(Narrow, &l_25c8);
    // --------------
    __ bind(&l_25f4);
    __ blx(r2);
    __ mov(Narrow, r1, r0);
    __ b(Narrow, &l_2528);
    // --------------
    __ bind(&l_25fa);
    __ movs(Narrow, r3, 0U);
    __ str(Narrow, r3, MemOperand(r5, 40));
    __ b(Narrow, &l_25ec);
    __ push(RegisterList(0x4ff0));
    __ mov(Narrow, r8, r0);
    __ ldr(Narrow, r4, MemOperand(r0, 4));
    __ sub(Narrow, sp, sp, 36U);
    __ mov(Narrow, r5, r1);
    __ mov(Narrow, r6, r2);
    __ ldr(Narrow, r3, MemOperand(r0, 8));
    __ cmp(Narrow, r4, r3);
    __ b(eq, Narrow, &l_2694);
    __ mvn(ip, 23U);
    __ cbz(r4, &l_2630);
    __ sub(ip, r4, 24U);
    __ mov(Narrow, r7, r4);
    __ mov(Narrow, lr, ip);
    __ ldm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r7, NO_WRITE_BACK, RegisterList(r0, r1));
    // --------------
    __ bind(&l_2630);
    __ rsb(lr, r5, ip);
    __ mov(r9, 43691U);
    __ asr(lr, lr, 3U);
    __ ldm(Narrow, r6, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ movt(r9, 43690U);
    __ mul(lr, r9, lr);
    __ add(Narrow, r7, sp, 8U);
    __ stm(Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r6, NO_WRITE_BACK, RegisterList(r0, r1));
    __ add(r3, r4, 24U);
    __ cmp(lr, 0U);
    __ str(r3, MemOperand(r8, 4));
    __ stm(r7, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(le, Narrow, &l_2680);
    // --------------
    __ bind(&l_2660);
    __ sub(r7, ip, 24U);
    __ sub(r6, r4, 24U);
    __ ldm(Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, r4, r6);
    __ subs(lr, lr, 1U);
    __ sub(ip, ip, 24U);
    __ stm(Narrow, r6, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r7, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r6, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(ne, Narrow, &l_2660);
    // --------------
    __ bind(&l_2680);
    __ add(Narrow, r4, sp, 8U);
    __ ldm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    // --------------
    __ bind(&l_268e);
    __ add(Narrow, sp, sp, 36U);
    __ pop(RegisterList(0x8ff0));
    // --------------
    __ bind(&l_2694);
    __ ldr(Narrow, r1, MemOperand(r0));
    __ mov(r3, 43691U);
    __ movt(r3, 43690U);
    __ subs(Narrow, r2, r4, r1);
    __ asrs(Narrow, r2, r2, 3U);
    __ mul(r3, r3, r2);
    __ cmp(Narrow, r3, 0U);
  }
  void Generate_37(TestAssembler* assembler) {
    __ b(ne, Narrow, &l_2794);
    __ subs(Narrow, r7, r5, r1);
    __ movs(Narrow, r3, 24U);
    __ str(Narrow, r3, MemOperand(sp, 4));
    // --------------
    __ bind(&l_26b0);
    __ ldr(Narrow, r0, MemOperand(sp, 4));
    // --------------
    __ bind(&l_26b2);
    __ bl(&l_26b2);
    __ ldr(r4, MemOperand(r8, 4));
    __ mov(Narrow, sl, r0);
    __ ldr(fp, MemOperand(r8));
    __ add(r9, r0, 24U);
    // --------------
    __ bind(&l_26c4);
    __ adds(r7, sl, r7);
    __ itttt(ne);
    __ ldm(ne, Narrow, r6, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(ne, Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(ne, r6, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(ne, r7, NO_WRITE_BACK, RegisterList(r0, r1));
    __ cmp(Narrow, r5, fp);
    __ b(eq, Narrow, &l_272a);
    __ mov(Narrow, r3, r4);
    __ mov(Narrow, ip, fp);
    __ mov(Narrow, r4, sl);
    __ mov(Narrow, lr, r3);
    // --------------
    __ bind(&l_26e2);
    __ mov(Narrow, r7, ip);
    __ mov(Narrow, r6, r4);
    __ add(ip, ip, 24U);
    __ cbz(r4, &l_26f8);
    __ ldm(Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r6, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r7, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r6, NO_WRITE_BACK, RegisterList(r0, r1));
    // --------------
    __ bind(&l_26f8);
    __ cmp(Narrow, r5, ip);
    __ add(r4, r4, 24U);
    __ b(ne, Narrow, &l_26e2);
    __ add(r2, fp, 24U);
    __ mov(r3, 43691U);
    __ subs(Narrow, r2, r5, r2);
    __ movt(r3, 2730U);
    __ lsrs(Narrow, r2, r2, 3U);
    __ mov(Narrow, r4, lr);
    __ mul(r3, r3, r2);
    __ bic(r3, r3, 0xe0000000);
    __ adds(r3, 1U);
    __ lsl(r9, r3, 5U);
    __ sub(r9, r9, Operand(r3, LSL, 3));
    __ add(r9, r9, 24U);
    __ add(Narrow, r9, r9, sl);
    // --------------
    __ bind(&l_272a);
    __ cmp(Narrow, r5, r4);
    __ b(eq, Narrow, &l_2776);
    __ mov(Narrow, ip, r4);
    __ mov(Narrow, lr, r5);
    __ mov(Narrow, r4, r9);
    // --------------
    __ bind(&l_2734);
    __ mov(Narrow, r7, lr);
    __ mov(Narrow, r6, r4);
    __ add(lr, lr, 24U);
    __ cbz(r4, &l_274a);
    __ ldm(Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r6, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r7, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r6, NO_WRITE_BACK, RegisterList(r0, r1));
    // --------------
    __ bind(&l_274a);
    __ cmp(Narrow, lr, ip);
    __ add(r4, r4, 24U);
    __ b(ne, Narrow, &l_2734);
    __ add(r3, r5, 24U);
    __ mov(r2, 43691U);
    __ rsb(r3, r3, lr);
    __ movt(r2, 2730U);
    __ lsrs(Narrow, r3, r3, 3U);
    __ mul(r3, r2, r3);
    __ bic(r3, r3, 0xe0000000);
    __ adds(r3, 1U);
    __ lsls(Narrow, r2, r3, 5U);
    __ sub(r3, r2, Operand(r3, LSL, 3));
    __ add(Narrow, r9, r9, r3);
    // --------------
    __ bind(&l_2776);
    __ cmp(fp, 0U);
    __ b(eq, Narrow, &l_2782);
    __ mov(Narrow, r0, fp);
    // --------------
    __ bind(&l_277e);
    __ bl(&l_277e);
    // --------------
    __ bind(&l_2782);
    __ ldr(Narrow, r3, MemOperand(sp, 4));
    __ str(sl, MemOperand(r8));
    __ str(r9, MemOperand(r8, 4));
    __ add(Narrow, r3, r3, sl);
    __ str(r3, MemOperand(r8, 8));
    __ b(Narrow, &l_268e);
    // --------------
    __ bind(&l_2794);
    __ lsls(Narrow, r2, r3, 1U);
    __ cmp(Narrow, r3, r2);
    __ b(ls, Narrow, &l_27a4);
    // --------------
    __ bind(&l_279a);
    __ mvn(r3, 15U);
    __ subs(Narrow, r7, r5, r1);
    __ str(Narrow, r3, MemOperand(sp, 4));
    __ b(Narrow, &l_26b0);
    // --------------
    __ bind(&l_27a4);
    __ mov(r0, 43690U);
    __ movt(r0, 2730U);
    __ cmp(Narrow, r2, r0);
    __ b(hi, Narrow, &l_279a);
    __ lsls(Narrow, r0, r3, 6U);
    __ subs(Narrow, r7, r5, r1);
    __ mov(Narrow, fp, r1);
    __ sub(r3, r0, Operand(r3, LSL, 4));
    __ mov(Narrow, sl, r2);
    __ mov(r9, 24U);
  }
  void Generate_38(TestAssembler* assembler) {
    __ str(Narrow, r3, MemOperand(sp, 4));
    __ cmp(Narrow, r2, 0U);
    __ b(eq, &l_26c4);
    __ b(Narrow, &l_26b0);
    __ nop(Narrow);
    __ push(Narrow, RegisterList(0x40f8));
    __ mov(Narrow, r2, r1);
    __ ldm(r0, NO_WRITE_BACK, RegisterList(r3, r5));
    __ mov(Narrow, r7, r0);
    __ cmp(Narrow, r3, r5);
    __ b(eq, Narrow, &l_27f0);
    __ ldr(Narrow, r1, MemOperand(r1, 8));
    // --------------
    __ bind(&l_27dc);
    __ ldr(Narrow, r4, MemOperand(r3, 8));
    __ cmp(Narrow, r4, r1);
    __ ite(lt);
    __ mov(lt, Narrow, r4, 1U);
    __ mov(ge, Narrow, r4, 0U);
    __ b(eq, Narrow, &l_280e);
    // --------------
    __ bind(&l_27e8);
    __ cbz(r4, &l_283c);
    __ adds(Narrow, r3, r3, 24U);
    __ cmp(Narrow, r3, r5);
    __ b(ne, Narrow, &l_27dc);
    // --------------
    __ bind(&l_27f0);
    __ ldr(Narrow, r3, MemOperand(r7, 8));
    __ cmp(Narrow, r5, r3);
    __ b(eq, Narrow, &l_2846);
    __ cbz(r5, &l_2808);
    __ mov(Narrow, r6, r2);
    __ mov(Narrow, r4, r5);
    __ ldm(Narrow, r6, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r6, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    // --------------
    __ bind(&l_2808);
    __ adds(Narrow, r5, r5, 24U);
    __ str(Narrow, r5, MemOperand(r7, 4));
    __ pop(Narrow, RegisterList(0x80f8));
    // --------------
    __ bind(&l_280e);
    __ ldr(Narrow, r0, MemOperand(r3));
    __ ldr(Narrow, r4, MemOperand(r2));
    __ ldr(Narrow, r6, MemOperand(r0, 4));
    __ ldr(Narrow, r4, MemOperand(r4, 4));
    __ cmp(Narrow, r6, r4);
    __ ite(lt);
    __ mov(lt, Narrow, r4, 1U);
    __ mov(ge, Narrow, r4, 0U);
    __ b(ne, Narrow, &l_27e8);
    __ ldr(Narrow, r4, MemOperand(r3, 12));
    __ ldr(Narrow, r0, MemOperand(r2, 12));
    __ cmp(Narrow, r4, r0);
    __ ite(lt);
    __ mov(lt, Narrow, r4, 1U);
    __ mov(ge, Narrow, r4, 0U);
    __ b(ne, Narrow, &l_27e8);
    __ ldr(Narrow, r4, MemOperand(r3, 4));
    __ ldr(Narrow, r0, MemOperand(r2, 4));
    __ cmp(Narrow, r4, r0);
    __ ite(lt);
    __ mov(lt, Narrow, r4, 1U);
    __ mov(ge, Narrow, r4, 0U);
    __ b(ne, Narrow, &l_27e8);
    // --------------
    __ bind(&l_283c);
    __ mov(Narrow, r0, r7);
    __ mov(Narrow, r1, r3);
    // --------------
    __ bind(&l_2840);
    __ bl(&l_2840);
    __ pop(Narrow, RegisterList(0x80f8));
    // --------------
    __ bind(&l_2846);
    __ mov(Narrow, r0, r7);
    __ mov(Narrow, r1, r5);
    // --------------
    __ bind(&l_284a);
    __ bl(&l_284a);
    __ pop(Narrow, RegisterList(0x80f8));
    __ push(RegisterList(0x41f0));
    __ mov(Narrow, r5, r0);
    __ ldr(Narrow, r4, MemOperand(r0));
    __ sub(Narrow, sp, sp, 24U);
    __ mov(Narrow, r7, r1);
    __ ldr(Narrow, r0, MemOperand(r0, 4));
    __ cmp(Narrow, r4, r0);
    __ b(ne, Narrow, &l_286c);
    __ b(Narrow, &l_2982);
    // --------------
    __ bind(&l_2864);
    __ adds(Narrow, r4, r4, 24U);
    __ cmp(Narrow, r4, r0);
    __ b(eq, &l_2982);
    // --------------
    __ bind(&l_286c);
    __ ldr(Narrow, r3, MemOperand(r4));
    __ cmp(Narrow, r2, r3);
    __ b(ne, Narrow, &l_2864);
    __ ldr(Narrow, r2, MemOperand(r7, 16));
    __ mov(Narrow, r0, r3);
    __ ldr(Narrow, r1, MemOperand(r3));
    __ ldr(Narrow, r3, MemOperand(r4, 8));
    __ ldr(Narrow, r6, MemOperand(r7, 12));
    __ cmp(Narrow, r2, r3);
    __ it(ge);
    __ mov(ge, Narrow, r2, r3);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ str(Narrow, r2, MemOperand(r4, 8));
    __ cmp(Narrow, r3, r6);
    __ it(lt);
    __ mov(lt, Narrow, r3, r6);
    __ str(Narrow, r3, MemOperand(r4, 4));
    __ ldr(Narrow, r3, MemOperand(r1, 32));
    __ blx(r3);
    __ cbz(r0, &l_28a2);
    __ ldr(Narrow, r0, MemOperand(r4));
    __ ldr(Narrow, r6, MemOperand(r4, 8));
  }
  void Generate_39(TestAssembler* assembler) {
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 36));
    __ blx(r3);
    __ subs(Narrow, r0, r6, r0);
    __ str(Narrow, r0, MemOperand(r4, 16));
    // --------------
    __ bind(&l_28a2);
    __ ldr(lr, MemOperand(r5));
    __ mov(ip, 43691U);
    __ ldr(Narrow, r6, MemOperand(r5, 4));
    __ movt(ip, 43690U);
    __ ldr(Narrow, r1, MemOperand(r7, 8));
    __ ldr(Narrow, r2, MemOperand(r4, 12));
    __ rsb(r3, lr, r6);
    __ asrs(Narrow, r3, r3, 3U);
    __ cmp(Narrow, r2, r1);
    __ mul(r3, ip, r3);
    __ it(lt);
    __ mov(lt, Narrow, r2, r1);
    __ str(Narrow, r2, MemOperand(r4, 12));
    __ cmp(Narrow, r3, 1U);
    __ b(eq, Narrow, &l_2910);
    __ mov(Narrow, lr, r4);
    __ mov(Narrow, r7, sp);
    __ ldm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ add(r8, r4, 24U);
    __ cmp(Narrow, r6, r8);
    __ stm(Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r7, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(eq, Narrow, &l_28fc);
    __ rsb(r2, r8, r6);
    __ asrs(Narrow, r3, r2, 3U);
    __ mul(ip, ip, r3);
    __ cmp(ip, 0U);
    __ b(le, Narrow, &l_28fc);
    __ mov(Narrow, r0, r4);
    __ mov(Narrow, r1, r8);
    // --------------
    __ bind(&l_28f8);
    __ bl(&l_28f8);
    // --------------
    __ bind(&l_28fc);
    __ sub(r2, r6, 24U);
    __ mov(Narrow, r1, sp);
    __ str(Narrow, r2, MemOperand(r5, 4));
    __ mov(Narrow, r0, r5);
    // --------------
    __ bind(&l_2906);
    __ bl(&l_2906);
    __ ldr(lr, MemOperand(r5));
    __ ldr(Narrow, r6, MemOperand(r5, 4));
    // --------------
    __ bind(&l_2910);
    __ movs(Narrow, r3, 0U);
    __ cmp(Narrow, r6, lr);
    __ str(Narrow, r3, MemOperand(r5, 40));
    __ itt(eq);
    __ mvn(eq, r3, 2147483648U);
    __ str(eq, Narrow, r3, MemOperand(r5, 36));
    __ b(eq, Narrow, &l_297c);
    __ rsb(r3, lr, r6);
    __ mov(r2, 43691U);
    __ asrs(Narrow, r3, r3, 3U);
    __ movt(r2, 43690U);
    __ ldr(Narrow, r1, MemOperand(r5, 28));
    __ ldr(r8, MemOperand(r5, 24));
    __ mul(r3, r2, r3);
    __ add(ip, r1, 4294967295U);
    __ add(r0, r8, Operand(ip, LSL, 1));
    __ cmp(Narrow, r3, 0U);
    __ str(Narrow, r0, MemOperand(r5, 40));
    __ b(le, Narrow, &l_2a00);
    __ mov(Narrow, r2, r6);
    __ mvn(r1, 2147483648U);
    // --------------
    __ bind(&l_294c);
    __ ldr(r4, MemOperand(r2, -24));
    __ subs(Narrow, r2, r2, 24U);
    __ ldr(Narrow, r3, MemOperand(r2, 12));
    __ ldr(Narrow, r7, MemOperand(r2, 8));
    __ ldr(Narrow, r4, MemOperand(r4, 4));
    __ subs(r3, 1U);
    __ mvns(Narrow, r6, r3);
    __ subs(Narrow, r1, r1, r4);
    __ add(Narrow, r3, r3, r4);
    __ cmp(Narrow, r1, r7);
    __ ite(le);
    __ and_(le, Narrow, r1, r1, r6);
    __ and_(gt, r1, r6, r7);
    __ cmp(Narrow, r2, lr);
    __ add(Narrow, r0, r0, r3);
    __ str(Narrow, r0, MemOperand(r5, 40));
    __ b(ne, Narrow, &l_294c);
    // --------------
    __ bind(&l_2972);
    __ rsb(r1, r8, r1);
    __ bic(r1, r1, ip);
    __ str(Narrow, r1, MemOperand(r5, 36));
    // --------------
    __ bind(&l_297c);
    __ add(Narrow, sp, sp, 24U);
    __ pop(RegisterList(0x81f0));
    // --------------
    __ bind(&l_2982);
    __ ldr(Narrow, r0, MemOperand(r2, 12));
    __ movs(Narrow, r3, 0U);
    __ ldr(Narrow, r6, MemOperand(r2, 8));
    __ mvn(r1, 2147483648U);
    __ ldr(Narrow, r4, MemOperand(r2));
    __ str(Narrow, r0, MemOperand(sp, 20));
    __ mov(Narrow, r0, r2);
    __ str(Narrow, r2, MemOperand(sp));
    __ str(Narrow, r6, MemOperand(sp, 12));
    __ ldr(Narrow, r2, MemOperand(r4, 32));
    __ str(Narrow, r3, MemOperand(sp, 4));
    __ str(Narrow, r3, MemOperand(sp, 16));
    __ str(Narrow, r1, MemOperand(sp, 8));
  }
  void Generate_40(TestAssembler* assembler) {
    __ blx(r2);
    __ cbnz(r0, &l_29f0);
    // --------------
    __ bind(&l_29a2);
    __ ldr(Narrow, r2, MemOperand(r7, 16));
    __ ldr(Narrow, r6, MemOperand(sp, 8));
    __ ldr(Narrow, r1, MemOperand(sp));
    __ ldr(Narrow, r4, MemOperand(sp, 4));
    __ ldr(Narrow, r3, MemOperand(r7, 12));
    __ cmp(Narrow, r2, r6);
    __ mov(Narrow, r0, r1);
    __ ldr(Narrow, r1, MemOperand(r1));
    __ it(ge);
    __ mov(ge, Narrow, r2, r6);
    __ cmp(Narrow, r3, r4);
    __ str(Narrow, r2, MemOperand(sp, 8));
    __ it(lt);
    __ mov(lt, Narrow, r3, r4);
    __ ldr(Narrow, r2, MemOperand(r1, 32));
    __ str(Narrow, r3, MemOperand(sp, 4));
    __ blx(r2);
    __ cbz(r0, &l_29d4);
    __ ldr(Narrow, r0, MemOperand(sp));
    __ ldr(Narrow, r4, MemOperand(sp, 8));
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 36));
    __ blx(r3);
    __ subs(Narrow, r0, r4, r0);
    __ str(Narrow, r0, MemOperand(sp, 16));
    // --------------
    __ bind(&l_29d4);
    __ ldr(Narrow, r3, MemOperand(r7, 8));
    __ mov(Narrow, r0, r5);
    __ ldr(Narrow, r2, MemOperand(sp, 12));
    __ mov(Narrow, r1, sp);
    __ cmp(Narrow, r3, r2);
    __ it(lt);
    __ mov(lt, Narrow, r3, r2);
    __ str(Narrow, r3, MemOperand(sp, 12));
    // --------------
    __ bind(&l_29e4);
    __ bl(&l_29e4);
    __ ldr(lr, MemOperand(r5));
    __ ldr(Narrow, r6, MemOperand(r5, 4));
    __ b(Narrow, &l_2910);
    // --------------
    __ bind(&l_29f0);
    __ ldr(Narrow, r0, MemOperand(sp));
    __ ldr(Narrow, r4, MemOperand(sp, 8));
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 36));
    __ blx(r3);
    __ subs(Narrow, r0, r4, r0);
    __ str(Narrow, r0, MemOperand(sp, 16));
    __ b(Narrow, &l_29a2);
    // --------------
    __ bind(&l_2a00);
    __ mvn(r1, 2147483648U);
    __ b(Narrow, &l_2972);
    __ nop(Narrow);
    __ cmp(Narrow, r0, r1);
    __ b(eq, Narrow, &l_2b02);
    __ push(RegisterList(0x4ff0));
    __ add(r6, r0, 24U);
    __ cmp(Narrow, r6, r1);
    __ sub(Narrow, sp, sp, 44U);
    __ b(eq, Narrow, &l_2afc);
    __ mov(r3, 43691U);
    __ str(Narrow, r1, MemOperand(sp, 8));
    __ movt(r3, 43690U);
    __ mov(Narrow, r9, r0);
    __ str(Narrow, r3, MemOperand(sp, 12));
    __ add(r8, r0, 48U);
    __ str(Narrow, r2, MemOperand(sp, 4));
    __ b(Narrow, &l_2a86);
    // --------------
    __ bind(&l_2a30);
    __ ldr(Narrow, r7, MemOperand(sp, 12));
    __ rsb(lr, r9, r6);
    __ sub(r5, r8, 24U);
    __ asr(ip, lr, 3U);
    __ ldm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ add(Narrow, r4, sp, 16U);
    __ mul(ip, r7, ip);
    __ stm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ cmp(ip, 0U);
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(le, Narrow, &l_2a66);
    __ rsb(r0, lr, 24U);
    __ rsb(r1, lr, r6);
    __ add(Narrow, r0, r0, r6);
    __ mov(Narrow, r2, lr);
    // --------------
    __ bind(&l_2a62);
    __ bl(&l_2a62);
    // --------------
    __ bind(&l_2a66);
    __ add(Narrow, r5, sp, 16U);
    __ mov(Narrow, r4, r9);
    __ ldm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, lr, r8);
    __ add(sl, r8, 24U);
    __ adds(Narrow, r6, r6, 24U);
    __ mov(Narrow, r8, sl);
    __ stm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldr(Narrow, r3, MemOperand(sp, 8));
    __ ldm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ cmp(Narrow, lr, r3);
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(eq, Narrow, &l_2afc);
    // --------------
    __ bind(&l_2a86);
    __ mov(Narrow, r0, r6);
    __ mov(Narrow, r1, r9);
    __ ldr(Narrow, r3, MemOperand(sp, 4));
    __ blx(r3);
    __ cmp(Narrow, r0, 0U);
  }
  void Generate_41(TestAssembler* assembler) {
    __ b(ne, Narrow, &l_2a30);
    __ sub(lr, r8, 24U);
    __ add(sl, r8, 24U);
    __ ldm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ add(Narrow, r4, sp, 16U);
    __ sub(fp, r8, 48U);
    __ sub(r5, r8, 24U);
    __ pld(MemOperand(sl));
    __ ldr(Narrow, r7, MemOperand(sp, 4));
    __ stm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(Narrow, &l_2ad2);
    // --------------
    __ bind(&l_2aba);
    __ add(r5, fp, 24U);
    __ add(r4, fp, 48U);
    __ mov(Narrow, lr, r5);
    __ ldm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    // --------------
    __ bind(&l_2ad2);
    __ mov(Narrow, r1, fp);
    __ add(Narrow, r0, sp, 16U);
    __ blx(r7);
    __ sub(fp, fp, 24U);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_2aba);
    __ add(Narrow, r4, sp, 16U);
    __ mov(Narrow, lr, r8);
    __ ldm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ adds(Narrow, r6, r6, 24U);
    __ mov(Narrow, r8, sl);
    __ str(Narrow, r7, MemOperand(sp, 4));
    __ stm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldr(Narrow, r3, MemOperand(sp, 8));
    __ ldm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ cmp(Narrow, lr, r3);
    __ stm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(ne, Narrow, &l_2a86);
    // --------------
    __ bind(&l_2afc);
    __ add(Narrow, sp, sp, 44U);
    __ pop(RegisterList(0x8ff0));
    // --------------
    __ bind(&l_2b02);
    __ bx(lr);
    __ sub(Narrow, sp, sp, 8U);
    __ push(RegisterList(0x4ff0));
    __ add(r8, r2, 4294967295U);
    __ add(r8, r8, Operand(r8, LSR, 31));
    __ sub(Narrow, sp, sp, 36U);
    __ mov(Narrow, fp, r0);
    __ asr(r8, r8, 1U);
    __ stm(sp, NO_WRITE_BACK, RegisterList(r1, r2));
    __ str(Narrow, r3, MemOperand(sp, 76));
    __ cmp(Narrow, r1, r8);
    __ ldr(r9, MemOperand(sp, 100));
    __ b(ge, &l_2c2e);
    __ mov(Narrow, r7, r1);
    __ b(Narrow, &l_2b4e);
    // --------------
    __ bind(&l_2b2e);
    __ mov(Narrow, r6, sl);
    // --------------
    __ bind(&l_2b30);
    __ lsls(Narrow, r5, r7, 5U);
    __ mov(Narrow, lr, r4);
    __ ldm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ cmp(Narrow, r6, r8);
    __ sub(r5, r5, Operand(r7, LSL, 3));
    __ mov(Narrow, r7, r6);
    __ add(Narrow, r5, r5, fp);
    __ stm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(ge, Narrow, &l_2b76);
    // --------------
    __ bind(&l_2b4e);
    __ adds(Narrow, r5, r7, 1U);
    __ lsls(Narrow, r6, r5, 1U);
    __ lsls(Narrow, r2, r5, 6U);
    __ add(sl, r6, 4294967295U);
    __ sub(r5, r2, Operand(r5, LSL, 4));
    __ lsl(r4, sl, 5U);
    __ add(Narrow, r5, r5, fp);
    __ sub(r4, r4, Operand(sl, LSL, 3));
    __ mov(Narrow, r0, r5);
    __ add(Narrow, r4, r4, fp);
    __ mov(Narrow, r1, r4);
    __ blx(r9);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_2b2e);
    __ mov(Narrow, r4, r5);
    __ b(Narrow, &l_2b30);
    // --------------
    __ bind(&l_2b76);
    __ ldr(Narrow, r3, MemOperand(sp, 4));
    __ lsls(Narrow, r2, r3, 31U);
    __ b(mi, Narrow, &l_2b8a);
    // --------------
    __ bind(&l_2b7c);
    __ sub(sl, r3, 2U);
    __ add(sl, sl, Operand(sl, LSR, 31));
    __ cmp(r6, Operand(sl, ASR, 1));
    __ b(eq, Narrow, &l_2bbc);
    // --------------
    __ bind(&l_2b8a);
    __ add(lr, sp, 76U);
    __ add(Narrow, r5, sp, 8U);
    __ ldm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, r7, r5);
    __ stm(Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldr(Narrow, r3, MemOperand(sp));
    __ ldm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ cmp(Narrow, r6, r3);
    __ stm(r7, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(gt, Narrow, &l_2bda);
  }
  void Generate_42(TestAssembler* assembler) {
    // --------------
    __ bind(&l_2ba6);
    __ ldm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ add(Narrow, sp, sp, 36U);
    __ pop(RegisterList(0x4ff0));
    __ add(Narrow, sp, sp, 8U);
    __ bx(lr);
    // --------------
    __ bind(&l_2bbc);
    __ lsls(Narrow, r6, r6, 1U);
    __ mov(Narrow, r7, r4);
    __ adds(r6, 1U);
    __ lsls(Narrow, r5, r6, 5U);
    __ sub(r5, r5, Operand(r6, LSL, 3));
    __ add(Narrow, r5, r5, fp);
    __ mov(Narrow, r4, r5);
    __ ldm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r7, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(Narrow, &l_2b8a);
    // --------------
    __ bind(&l_2bda);
    __ subs(Narrow, r7, r6, 1U);
    __ add(r7, r7, Operand(r7, LSR, 31));
    __ asrs(Narrow, r7, r7, 1U);
    // --------------
    __ bind(&l_2be2);
    __ lsls(Narrow, r4, r7, 5U);
    __ mov(Narrow, r1, r5);
    __ sub(r4, r4, Operand(r7, LSL, 3));
    __ add(Narrow, r4, r4, fp);
    __ mov(Narrow, r0, r4);
    __ blx(r9);
    __ cbnz(r0, &l_2bfc);
    __ lsls(Narrow, r4, r6, 5U);
    __ sub(r4, r4, Operand(r6, LSL, 3));
    __ add(Narrow, r4, r4, fp);
    __ b(Narrow, &l_2ba6);
    // --------------
    __ bind(&l_2bfc);
    __ lsl(sl, r6, 5U);
    __ mov(Narrow, lr, r4);
    __ ldm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ add(r8, r7, 4294967295U);
    __ sub(r6, sl, Operand(r6, LSL, 3));
    __ add(r8, r8, Operand(r8, LSR, 31));
    __ add(Narrow, r6, r6, fp);
    __ asr(r8, r8, 1U);
    __ stm(Narrow, r6, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldr(Narrow, r3, MemOperand(sp));
    __ ldm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ cmp(Narrow, r3, r7);
    __ stm(r6, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(ge, Narrow, &l_2ba6);
    __ mov(Narrow, r6, r7);
    __ mov(Narrow, r7, r8);
    __ b(Narrow, &l_2be2);
    // --------------
    __ bind(&l_2c2e);
    __ mov(Narrow, r3, r2);
    __ lsls(Narrow, r3, r3, 31U);
    __ b(mi, Narrow, &l_2c44);
    __ ldr(Narrow, r3, MemOperand(sp));
    __ mov(Narrow, r6, r3);
    __ lsls(Narrow, r4, r3, 5U);
    __ sub(r4, r4, Operand(r3, LSL, 3));
    __ ldr(Narrow, r3, MemOperand(sp, 4));
    __ add(Narrow, r4, r4, r0);
    __ b(Narrow, &l_2b7c);
    // --------------
    __ bind(&l_2c44);
    __ ldr(Narrow, r4, MemOperand(sp));
    __ add(Narrow, r7, sp, 76U);
    __ ldm(Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ add(Narrow, r5, sp, 8U);
    __ mov(Narrow, r6, r5);
    __ lsl(lr, r4, 5U);
    __ stm(Narrow, r6, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r7, NO_WRITE_BACK, RegisterList(r0, r1));
    __ sub(r4, lr, Operand(r4, LSL, 3));
    __ add(Narrow, r4, r4, fp);
    __ stm(r6, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(Narrow, &l_2ba6);
    __ push(RegisterList(0x4ff0));
    __ sub(Narrow, sp, sp, 68U);
    __ str(Narrow, r1, MemOperand(sp, 28));
    __ subs(Narrow, r1, r1, r0);
    __ cmp(Narrow, r1, 47U);
    __ mov(Narrow, r8, r0);
    __ str(Narrow, r1, MemOperand(sp, 32));
    __ mov(Narrow, sl, r2);
    __ mov(Narrow, r9, r3);
    __ b(le, Narrow, &l_2cf0);
    __ asr(fp, r1, 3U);
    __ mov(r3, 43691U);
    __ movt(r3, 43690U);
    __ str(sl, MemOperand(sp, 36));
    __ mul(r3, r3, fp);
    __ sub(ip, r3, 2U);
    __ asr(fp, ip, 1U);
    __ lsl(r7, fp, 5U);
    __ mov(Narrow, sl, fp);
    __ sub(r7, r7, Operand(fp, LSL, 3));
    __ mov(Narrow, fp, r3);
    __ subs(Narrow, r7, r7, 168U);
    __ add(Narrow, r7, r7, r0);
    __ b(Narrow, &l_2cac);
    // --------------
    __ bind(&l_2ca8);
    __ add(sl, sl, 4294967295U);
    // --------------
    __ bind(&l_2cac);
    __ add(r5, r7, 168U);
    __ add(Narrow, r4, sp, 40U);
  }
  void Generate_43(TestAssembler* assembler) {
    __ ldm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ add(lr, sp, 44U);
    __ mov(Narrow, r6, sp);
    __ pld(MemOperand(r7));
    __ subs(Narrow, r7, r7, 24U);
    __ stm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ str(r9, MemOperand(sp, 20));
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ ldm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldr(r4, MemOperand(lr));
    __ stm(Narrow, r6, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, r0, r8);
    __ ldr(Narrow, r3, MemOperand(sp, 40));
    __ mov(Narrow, r1, sl);
    __ mov(Narrow, r2, fp);
    __ str(Narrow, r4, MemOperand(r6));
    // --------------
    __ bind(&l_2ce2);
    __ bl(&l_2ce2);
    __ cmp(sl, 0U);
    __ b(ne, Narrow, &l_2ca8);
    __ ldr(sl, MemOperand(sp, 36));
    // --------------
    __ bind(&l_2cf0);
    __ ldr(Narrow, r4, MemOperand(sp, 28));
    __ cmp(Narrow, r4, sl);
    __ b(cs, Narrow, &l_2d7a);
    __ ldr(Narrow, r3, MemOperand(sp, 32));
    __ mov(r2, 43691U);
    __ movt(r2, 43690U);
    __ adds(Narrow, r4, r4, 96U);
    __ asrs(Narrow, r3, r3, 3U);
    __ mul(r3, r2, r3);
    __ str(Narrow, r3, MemOperand(sp, 28));
    __ b(Narrow, &l_2d16);
    // --------------
    __ bind(&l_2d0c);
    __ sub(r3, r4, 72U);
    __ adds(Narrow, r4, r4, 24U);
    __ cmp(Narrow, sl, r3);
    __ b(ls, Narrow, &l_2d7a);
    // --------------
    __ bind(&l_2d16);
    __ sub(r5, r4, 96U);
    __ mov(Narrow, r1, r8);
    __ mov(Narrow, r0, r5);
    __ blx(r9);
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_2d0c);
    __ mov(Narrow, r6, r5);
    __ add(lr, sp, 40U);
    __ ldm(Narrow, r6, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, r7, r8);
    __ add(fp, sp, 44U);
    __ mov(Narrow, ip, sp);
    __ pld(MemOperand(r4));
    __ stm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r6, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ ldm(Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r7, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r6, NO_WRITE_BACK, RegisterList(r0, r1));
    __ ldm(fp, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ str(r9, MemOperand(sp, 20));
    __ stm(ip, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, r0, r8);
    __ ldr(r5, MemOperand(fp));
    __ movs(Narrow, r1, 0U);
    __ ldr(Narrow, r3, MemOperand(sp, 40));
    __ ldr(Narrow, r2, MemOperand(sp, 28));
    __ str(r5, MemOperand(ip));
    // --------------
    __ bind(&l_2d6c);
    __ bl(&l_2d6c);
    __ sub(r3, r4, 72U);
    __ adds(Narrow, r4, r4, 24U);
    __ cmp(Narrow, sl, r3);
    __ b(hi, Narrow, &l_2d16);
    // --------------
    __ bind(&l_2d7a);
    __ add(Narrow, sp, sp, 68U);
    __ pop(RegisterList(0x8ff0));
    __ push(RegisterList(0x4ff0));
    __ mov(Narrow, r6, r0);
    __ sub(Narrow, sp, sp, 76U);
    __ mov(Narrow, r4, r2);
    __ str(Narrow, r2, MemOperand(sp, 36));
    __ subs(Narrow, r2, r1, r6);
    __ cmp(r2, 408U);
    __ str(Narrow, r1, MemOperand(sp, 32));
    __ b(lt, &l_2fa4);
    __ mov(Narrow, r7, r3);
    __ cmp(Narrow, r4, 0U);
    __ b(eq, &l_2f20);
    __ mov(Narrow, r3, r1);
    __ mov(r2, 43691U);
    __ add(r1, r6, 24U);
    __ movt(r2, 43690U);
    __ mov(Narrow, r0, r1);
    __ str(Narrow, r2, MemOperand(sp, 44));
    __ add(Narrow, r2, sp, 48U);
    __ str(Narrow, r1, MemOperand(sp, 40));
    __ str(Narrow, r2, MemOperand(sp, 28));
    // --------------
    __ bind(&l_2db8);
    __ ldr(Narrow, r2, MemOperand(sp, 36));
    __ subs(Narrow, r3, r3, r6);
    __ asrs(Narrow, r3, r3, 3U);
    __ subs(r2, 1U);
    __ str(Narrow, r2, MemOperand(sp, 36));
    __ ldr(Narrow, r2, MemOperand(sp, 32));
    __ sub(r5, r2, 24U);
  }
  void Generate_44(TestAssembler* assembler) {
    __ ldr(Narrow, r2, MemOperand(sp, 44));
    __ mul(r3, r2, r3);
    __ add(r3, r3, Operand(r3, LSR, 31));
    __ asrs(Narrow, r3, r3, 1U);
    __ lsls(Narrow, r4, r3, 5U);
    __ sub(r3, r4, Operand(r3, LSL, 3));
    __ adds(Narrow, r4, r6, r3);
    __ mov(Narrow, r1, r4);
    __ blx(r7);
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_2ebe);
    __ mov(Narrow, r0, r4);
    __ mov(Narrow, r1, r5);
    __ blx(r7);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_2ed0);
    __ ldr(Narrow, r0, MemOperand(sp, 40));
    __ mov(Narrow, r1, r5);
    __ blx(r7);
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_2f0a);
    // --------------
    __ bind(&l_2dfa);
    __ mov(Narrow, ip, r6);
    __ ldr(r8, MemOperand(sp, 28));
    __ ldm(ip, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, r4, r5);
    __ mov(Narrow, r9, r6);
    __ mov(Narrow, lr, r8);
    __ stm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(ip, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ ldm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(r9, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(ip, NO_WRITE_BACK, RegisterList(r0, r1));
    __ ldm(r8, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    // --------------
    __ bind(&l_2e32);
    __ ldr(r8, MemOperand(sp, 32));
    __ ldr(Narrow, r5, MemOperand(sp, 40));
    // --------------
    __ bind(&l_2e38);
    __ mov(Narrow, r0, r5);
    __ mov(Narrow, r1, r6);
    __ mov(Narrow, r4, r5);
    __ adds(Narrow, r5, r5, 24U);
    __ blx(r7);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_2e38);
    __ sub(r5, r8, 24U);
    // --------------
    __ bind(&l_2e4a);
    __ mov(Narrow, r1, r5);
    __ mov(Narrow, r0, r6);
    __ mov(Narrow, r8, r5);
    __ subs(Narrow, r5, r5, 24U);
    __ blx(r7);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_2e4a);
    __ cmp(Narrow, r8, r4);
    __ b(ls, Narrow, &l_2e9c);
    __ mov(Narrow, r9, r4);
    __ ldr(fp, MemOperand(sp, 28));
    __ ldm(r9, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, lr, r8);
    __ add(r5, r4, 24U);
    __ mov(Narrow, sl, r8);
    __ mov(Narrow, ip, fp);
    __ stm(ip, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r9, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(ip, NO_WRITE_BACK, RegisterList(r0, r1));
    __ ldm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r9, NO_WRITE_BACK, RegisterList(r0, r1));
    __ ldm(fp, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(sl, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(ip, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(Narrow, &l_2e38);
    // --------------
    __ bind(&l_2e9c);
    __ ldr(Narrow, r2, MemOperand(sp, 36));
    __ mov(Narrow, r3, r7);
    __ ldr(Narrow, r1, MemOperand(sp, 32));
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_2ea4);
    __ bl(&l_2ea4);
    __ subs(Narrow, r2, r4, r6);
    __ mov(Narrow, r3, r4);
    __ cmp(r2, 408U);
    __ b(lt, Narrow, &l_2fa4);
    __ ldr(Narrow, r2, MemOperand(sp, 36));
    __ cmp(Narrow, r2, 0U);
    __ b(eq, Narrow, &l_2f26);
    __ str(Narrow, r3, MemOperand(sp, 32));
    __ ldr(Narrow, r0, MemOperand(sp, 40));
    __ b(Narrow, &l_2db8);
    // --------------
    __ bind(&l_2ebe);
    __ ldr(Narrow, r0, MemOperand(sp, 40));
    __ mov(Narrow, r1, r5);
    __ blx(r7);
    __ cbnz(r0, &l_2f0a);
    __ mov(Narrow, r0, r4);
    __ mov(Narrow, r1, r5);
    __ blx(r7);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_2dfa);
  }
  void Generate_45(TestAssembler* assembler) {
    // --------------
    __ bind(&l_2ed0);
    __ mov(Narrow, lr, r6);
    __ ldr(r8, MemOperand(sp, 28));
    __ ldm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, r9, r6);
    __ mov(Narrow, ip, r4);
    __ mov(Narrow, r5, r8);
    // --------------
    __ bind(&l_2ee0);
    __ stm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ ldm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(r9, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(lr, NO_WRITE_BACK, RegisterList(r0, r1));
    __ ldm(r8, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(ip, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(Narrow, &l_2e32);
    // --------------
    __ bind(&l_2f0a);
    __ ldr(r8, MemOperand(sp, 28));
    __ mov(Narrow, lr, r6);
    __ ldr(ip, MemOperand(sp, 40));
    __ mov(Narrow, r9, r6);
    __ ldm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, r5, r8);
    __ mov(Narrow, r4, ip);
    __ b(Narrow, &l_2ee0);
    // --------------
    __ bind(&l_2f20);
    __ mov(Narrow, r4, r1);
    __ add(Narrow, r3, sp, 48U);
    __ str(Narrow, r3, MemOperand(sp, 28));
    // --------------
    __ bind(&l_2f26);
    __ mov(fp, 43691U);
    __ mov(Narrow, r0, r6);
    __ mov(Narrow, r1, r4);
    __ mov(Narrow, r2, r4);
    __ mov(Narrow, r3, r7);
    __ sub(r8, r4, 144U);
    __ movt(fp, 43690U);
    // --------------
    __ bind(&l_2f3a);
    __ bl(&l_2f3a);
    // --------------
    __ bind(&l_2f3e);
    __ add(r4, r8, 120U);
    __ ldr(r9, MemOperand(sp, 28));
    __ ldm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, r5, r6);
    __ add(sl, r8, 120U);
    __ add(ip, sp, 52U);
    __ pld(MemOperand(r8));
    __ mov(Narrow, lr, sp);
    __ sub(r8, r8, 24U);
    __ stm(r9, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r9, NO_WRITE_BACK, RegisterList(r0, r1));
    __ rsb(r9, r6, sl);
    __ ldm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(sl, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ asr(sl, r9, 3U);
    __ ldm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ ldm(ip, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ str(Narrow, r7, MemOperand(sp, 20));
    __ stm(lr, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mul(r2, fp, sl);
    __ ldr(r4, MemOperand(ip));
    __ mov(Narrow, r0, r6);
    __ movs(Narrow, r1, 0U);
    __ ldr(Narrow, r3, MemOperand(sp, 48));
    __ str(r4, MemOperand(lr));
    // --------------
    __ bind(&l_2f9a);
    __ bl(&l_2f9a);
    __ cmp(r9, 47U);
    __ b(gt, Narrow, &l_2f3e);
    // --------------
    __ bind(&l_2fa4);
    __ add(Narrow, sp, sp, 76U);
    __ pop(RegisterList(0x8ff0));
    __ nop(Narrow);
    __ push(RegisterList(0x47f0));
    __ movs(Narrow, r4, 0U);
    __ ldm(r0, NO_WRITE_BACK, RegisterList(r5, sl));
    __ sub(Narrow, sp, sp, 24U);
    __ str(Narrow, r4, MemOperand(r0, 40));
    __ cmp(Narrow, r5, sl);
    __ itt(eq);
    __ mvn(eq, r3, 2147483648U);
    __ str(eq, Narrow, r3, MemOperand(r0, 36));
    __ b(eq, Narrow, &l_302a);
    __ mov(Narrow, r7, r0);
    __ mov(Narrow, r2, sl);
    __ mov(Narrow, r0, r5);
    __ cbz(r1, &l_3030);
    // --------------
    __ bind(&l_2fce);
    __ subs(Narrow, r3, r2, r0);
    __ mov(r5, 43691U);
    __ asrs(Narrow, r3, r3, 3U);
    __ ldr(Narrow, r1, MemOperand(r7, 28));
    __ ldr(r8, MemOperand(r7, 24));
    __ movt(r5, 43690U);
    __ mul(r3, r5, r3);
    __ add(ip, r1, 4294967295U);
    __ add(r1, r8, Operand(ip, LSL, 1));
    __ cmp(Narrow, r3, 0U);
    __ add(Narrow, r4, r4, r1);
    __ mvn(r1, 2147483648U);
    __ str(Narrow, r4, MemOperand(r7, 40));
    __ b(le, Narrow, &l_3020);
    // --------------
    __ bind(&l_2ff8);
    __ ldr(r5, MemOperand(r2, -24));
    __ subs(Narrow, r2, r2, 24U);
  }
  void Generate_46(TestAssembler* assembler) {
    __ ldr(Narrow, r3, MemOperand(r2, 12));
    __ ldr(lr, MemOperand(r2, 8));
    __ ldr(Narrow, r5, MemOperand(r5, 4));
    __ subs(r3, 1U);
    __ mvns(Narrow, r6, r3);
    __ subs(Narrow, r1, r1, r5);
    __ add(Narrow, r3, r3, r5);
    __ cmp(Narrow, r1, lr);
    __ ite(le);
    __ and_(le, Narrow, r1, r1, r6);
    __ and_(gt, r1, r6, lr);
    __ cmp(Narrow, r2, r0);
    __ add(Narrow, r4, r4, r3);
    __ str(Narrow, r4, MemOperand(r7, 40));
    __ b(ne, Narrow, &l_2ff8);
    // --------------
    __ bind(&l_3020);
    __ rsb(r1, r8, r1);
    __ bic(r1, r1, ip);
    __ str(Narrow, r1, MemOperand(r7, 36));
    // --------------
    __ bind(&l_302a);
    __ add(Narrow, sp, sp, 24U);
    __ pop(RegisterList(0x87f0));
    // --------------
    __ bind(&l_3030);
    __ rsb(r4, r5, sl);
    __ mov(r2, 43691U);
    __ asrs(Narrow, r1, r4, 3U);
    __ movt(r2, 43690U);
    __ movw(r3, 0U);
    __ mul(r2, r2, r1);
    __ movt(r3, 0U);
    __ mov(Narrow, r0, r5);
    __ mov(Narrow, r1, sl);
    __ clz(r2, r2);
    __ rsb(r2, r2, 31U);
    __ lsls(Narrow, r2, r2, 1U);
    // --------------
    __ bind(&l_3058);
    __ bl(&l_3058);
    __ cmp(r4, 408U);
    __ b(lt, Narrow, &l_30e2);
    __ add(r4, r5, 384U);
    __ movw(r2, 0U);
    __ mov(Narrow, r1, r4);
    __ movt(r2, 0U);
    __ mov(Narrow, r0, r5);
    // --------------
    __ bind(&l_3072);
    __ bl(&l_3072);
    __ cmp(Narrow, sl, r4);
    __ it(ne);
    __ add(ne, r9, r5, 504U);
    __ b(eq, Narrow, &l_30f2);
    // --------------
    __ bind(&l_3080);
    __ sub(r5, r9, 120U);
    __ mov(Narrow, r4, sp);
    __ ldm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ sub(r6, r9, 144U);
    __ sub(r8, r9, 120U);
    __ pld(MemOperand(r9));
    __ stm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(Narrow, &l_30b6);
    // --------------
    __ bind(&l_30a0);
    __ add(r8, r6, 24U);
    __ add(r4, r6, 48U);
    __ mov(Narrow, r5, r8);
    __ ldm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ stm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    // --------------
    __ bind(&l_30b6);
    __ mov(Narrow, r1, r6);
    __ mov(Narrow, r0, sp);
    // --------------
    __ bind(&l_30ba);
    __ bl(&l_30ba);
    __ subs(Narrow, r6, r6, 24U);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_30a0);
    __ mov(Narrow, r4, sp);
    __ sub(r5, r9, 96U);
    __ ldm(Narrow, r4, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ cmp(Narrow, sl, r5);
    __ add(r9, r9, 24U);
    __ stm(r8, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ ldm(r4, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r8, NO_WRITE_BACK, RegisterList(r0, r1));
    __ b(ne, Narrow, &l_3080);
    __ b(Narrow, &l_30f2);
    // --------------
    __ bind(&l_30e2);
    __ movw(r2, 0U);
    __ mov(Narrow, r0, r5);
    __ mov(Narrow, r1, sl);
    __ movt(r2, 0U);
    // --------------
    __ bind(&l_30ee);
    __ bl(&l_30ee);
    // --------------
    __ bind(&l_30f2);
    __ ldr(Narrow, r4, MemOperand(r7, 40));
    __ ldm(r7, NO_WRITE_BACK, RegisterList(r0, r2));
    __ b(Narrow, &l_2fce);
    __ nop(Narrow);
    __ push(RegisterList(0x4ff0));
    __ mov(Narrow, r9, r2);
    __ ldrb(Narrow, r2, MemOperand(r2, 16));
    __ sub(Narrow, sp, sp, 92U);
    __ mov(Narrow, r8, r0);
    __ mov(sl, 0U);
    __ str(Narrow, r1, MemOperand(sp, 16));
    __ cmp(Narrow, r2, 0U);
    __ b(ne, &l_3384);
    __ clz(sl, r3);
    __ lsr(sl, sl, 5U);
    __ add(Narrow, r3, sp, 36U);
    __ str(Narrow, r3, MemOperand(sp));
  }
  void Generate_47(TestAssembler* assembler) {
    __ mov(Narrow, r2, sl);
    __ mov(Narrow, r3, r9);
    // --------------
    __ bind(&l_3124);
    __ bl(&l_3124);
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_3410);
    __ ldr(r3, MemOperand(r8, 4));
    __ ldr(r1, MemOperand(r8, 12));
    __ ldr(Narrow, r4, MemOperand(sp, 36));
    __ subs(Narrow, r1, r1, r3);
    __ ldr(Narrow, r3, MemOperand(sp, 16));
    __ ldr(Narrow, r5, MemOperand(r4));
    __ cmp(Narrow, r3, 14U);
    __ ldr(r3, MemOperand(r8, 28));
    __ b(eq, &l_33e0);
    __ cmp(Narrow, r3, 1U);
    __ ittte(eq);
    __ add(eq, Narrow, r2, r5, 4U);
    __ add(eq, Narrow, r6, r1, 4U);
    __ mov(eq, Narrow, r3, 4U);
    __ mov(ne, Narrow, r2, r5);
    __ itt(ne);
    __ mov(ne, Narrow, r6, r1);
    __ mov(ne, Narrow, r3, 8U);
    // --------------
    __ bind(&l_315a);
    __ ldr(Narrow, r0, MemOperand(r4, 16));
    __ add(Narrow, r3, r3, r6);
    __ cbz(r0, &l_3164);
    __ bic(r3, r3, 0x3);
    // --------------
    __ bind(&l_3164);
    __ add(r0, r8, 96U);
    __ ldr(Narrow, r7, MemOperand(r4, 4));
    __ str(Narrow, r0, MemOperand(sp, 24));
    __ ldr(Narrow, r0, MemOperand(r4, 8));
    __ ldr(fp, MemOperand(r4, 12));
    __ add(ip, r3, r7);
    __ str(r9, MemOperand(sp));
    __ add(lr, r3, r0);
    __ ldr(Narrow, r0, MemOperand(sp, 24));
    __ add(Narrow, r3, sp, 60U);
    __ str(ip, MemOperand(sp, 72));
    __ str(lr, MemOperand(sp, 76));
    __ str(fp, MemOperand(sp, 68));
    __ str(Narrow, r6, MemOperand(sp, 60));
    __ str(Narrow, r5, MemOperand(sp, 64));
    // --------------
    __ bind(&l_3192);
    __ bl(&l_3192);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_3242);
    __ add(r3, r8, 44U);
    __ str(Narrow, r3, MemOperand(sp, 8));
    // --------------
    __ bind(&l_31a0);
    __ ldr(Narrow, r2, MemOperand(sp, 8));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ str(Narrow, r3, MemOperand(sp, 20));
    __ str(Narrow, r2, MemOperand(sp, 28));
    // --------------
    __ bind(&l_31ae);
    __ ldr(Narrow, r4, &l_3444);
    __ movs(Narrow, r3, 0U);
    __ str(Narrow, r2, MemOperand(sp, 80));
    __ str(Narrow, r3, MemOperand(sp, 64));
    __ str(Narrow, r4, MemOperand(sp, 60));
    __ strb(r3, MemOperand(sp, 77));
    __ str(Narrow, r3, MemOperand(sp, 84));
    // --------------
    __ bind(&l_31be);
    __ ldr(r3, MemOperand(r8, 44));
    __ ldr(Narrow, r2, MemOperand(sp, 20));
    __ ldr(Narrow, r0, MemOperand(sp, 28));
    __ ldr(Narrow, r3, MemOperand(r3));
    __ cmp(Narrow, r3, r2);
    __ b(ne, &l_33ee);
    // --------------
    __ bind(&l_31ce);
    __ bl(&l_31ce);
    // --------------
    __ bind(&l_31d2);
    __ str(Narrow, r0, MemOperand(sp, 64));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_31ec);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_31e8);
    __ bl(&l_31e8);
    // --------------
    __ bind(&l_31ec);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 77));
    // --------------
    __ bind(&l_31f2);
    __ ldr(Narrow, r6, &l_3448);
    __ mov(Narrow, r2, sl);
    __ ldr(Narrow, r1, MemOperand(sp, 16));
    __ mov(Narrow, r0, r8);
    __ mov(Narrow, r3, r9);
    __ str(Narrow, r6, MemOperand(sp, 60));
    // --------------
    __ bind(&l_31fe);
    __ bl(&l_31fe);
    __ ldrb(r3, MemOperand(r9, 16));
    __ cbnz(r3, &l_3230);
    __ ldr(r3, MemOperand(r9, 144));
    __ cmp(Narrow, r3, 0U);
    __ b(eq, &l_33fa);
    __ ldr(Narrow, r5, MemOperand(r3, 4));
    __ subs(Narrow, r5, r5, 24U);
    // --------------
    __ bind(&l_3216);
    __ ldm(Narrow, r5, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ add(Narrow, r7, sp, 36U);
    __ stm(Narrow, r7, WRITE_BACK, RegisterList(r0, r1, r2, r3));
    __ mov(Narrow, r2, r9);
    __ ldm(r5, NO_WRITE_BACK, RegisterList(r0, r1));
    __ stm(r7, NO_WRITE_BACK, RegisterList(r0, r1));
  }
  void Generate_48(TestAssembler* assembler) {
    __ add(r0, r8, 96U);
    __ add(Narrow, r1, sp, 36U);
    // --------------
    __ bind(&l_322c);
    __ bl(&l_322c);
    // --------------
    __ bind(&l_3230);
    __ ldrb(r3, MemOperand(sp, 77));
    __ str(Narrow, r4, MemOperand(sp, 60));
    __ cmp(Narrow, r3, 0U);
    __ b(ne, &l_3366);
    // --------------
    __ bind(&l_323c);
    __ add(Narrow, sp, sp, 92U);
    __ pop(RegisterList(0x8ff0));
    // --------------
    __ bind(&l_3242);
    __ add(r3, r8, 44U);
    __ ldr(r2, MemOperand(r8, 96));
    __ mov(Narrow, r5, r3);
    __ str(Narrow, r3, MemOperand(sp, 8));
    __ str(Narrow, r3, MemOperand(sp, 28));
    __ ldr(r3, MemOperand(r8, 100));
    __ ldr(Narrow, r4, MemOperand(r4));
    __ ldr(r0, MemOperand(r8, 12));
    __ ldr(r1, MemOperand(r8, 4));
    __ cmp(Narrow, r2, r3);
    __ str(Narrow, r4, MemOperand(sp, 12));
    __ sub(r4, r0, r1);
    __ b(eq, Narrow, &l_31a0);
    __ ldr(r3, MemOperand(r8, 44));
    __ mov(Narrow, r0, r5);
    __ ldr(Narrow, r3, MemOperand(r3, 36));
    __ blx(r3);
    __ ldr(r7, MemOperand(r8, 96));
    __ ldr(r2, MemOperand(r8, 100));
    __ ldr(r1, MemOperand(r8, 120));
    __ cmp(Narrow, r7, r2);
    __ add(Narrow, r1, r1, r4);
    __ b(eq, Narrow, &l_32a0);
    __ ldr(Narrow, r3, MemOperand(r7));
    __ cmp(Narrow, r9, r3);
    __ it(ne);
    __ add(ne, r3, r7, 24U);
    __ b(ne, Narrow, &l_329a);
    __ b(Narrow, &l_340c);
    // --------------
    __ bind(&l_3292);
    __ ldr(r0, MemOperand(r3, 24, PostIndex));
    __ cmp(Narrow, r9, r0);
    __ b(eq, Narrow, &l_32a4);
    // --------------
    __ bind(&l_329a);
    __ cmp(Narrow, r3, r2);
    __ mov(Narrow, fp, r3);
    __ b(ne, Narrow, &l_3292);
    // --------------
    __ bind(&l_32a0);
    __ mov(fp, 0U);
    // --------------
    __ bind(&l_32a4);
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ str(Narrow, r3, MemOperand(sp, 20));
    // --------------
    __ bind(&l_32ae);
    __ cmp(Narrow, r7, r2);
    __ b(eq, Narrow, &l_3350);
    // --------------
    __ bind(&l_32b2);
    __ ldr(Narrow, r3, MemOperand(sp, 12));
    __ ldr(r6, MemOperand(r9, 8));
    __ ldr(r4, MemOperand(r8, 136));
    __ adds(Narrow, r5, r3, r1);
    __ ldr(Narrow, r3, MemOperand(sp, 68));
    __ ldr(Narrow, r0, MemOperand(r7, 8));
    __ add(Narrow, r4, r4, r5);
    __ cmp(Narrow, r3, r6);
    __ it(lt);
    __ mov(lt, Narrow, r3, r6);
    __ cmp(fp, 0U);
    __ b(eq, Narrow, &l_32da);
    __ ldr(r6, MemOperand(fp, 12));
    __ cmp(Narrow, r3, r6);
    __ it(lt);
    __ mov(lt, Narrow, r3, r6);
    // --------------
    __ bind(&l_32da);
    __ ldr(r5, MemOperand(r9, 4));
    __ add(Narrow, r5, r5, r3);
    __ subs(r5, 1U);
    __ add(Narrow, r4, r4, r5);
    __ cmp(Narrow, r0, r4);
    __ b(gt, Narrow, &l_33c8);
    // --------------
    __ bind(&l_32e8);
    __ ldr(Narrow, r4, MemOperand(r7, 12));
    __ ldr(Narrow, r6, MemOperand(sp, 8));
    __ ldr(r3, MemOperand(r8, 44));
    __ subs(r4, 1U);
    __ ldr(Narrow, r5, MemOperand(r7));
    __ adds(Narrow, r2, r1, r4);
    __ mov(Narrow, r0, r6);
    __ bic(r4, r2, r4);
    __ ldr(Narrow, r3, MemOperand(r3, 44));
    __ subs(Narrow, r1, r4, r1);
    __ blx(r3);
    __ ldr(r3, MemOperand(r8, 44));
    __ ldr(Narrow, r2, MemOperand(sp, 20));
    __ ldr(Narrow, r3, MemOperand(r3));
    __ cmp(Narrow, r3, r2);
    __ b(ne, Narrow, &l_33f2);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_3310);
    __ bl(&l_3310);
    __ mov(Narrow, r1, r0);
    // --------------
    __ bind(&l_3316);
    __ ldr(Narrow, r3, MemOperand(r5));
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r4, MemOperand(r5, 20));
    __ ldr(Narrow, r3, MemOperand(r3, 20));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r5));
    __ mov(Narrow, r0, r5);
    __ ldr(Narrow, r1, MemOperand(sp, 8));
    __ ldr(Narrow, r3, MemOperand(r3, 16));
  }
  void Generate_49(TestAssembler* assembler) {
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r5));
    __ mov(Narrow, r0, r5);
    __ ldr(Narrow, r6, MemOperand(r5, 4));
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_33d2);
    __ movs(Narrow, r3, 1U);
    __ mov(Narrow, r1, r7);
    __ strb(Narrow, r3, MemOperand(r5, 16));
    __ ldr(Narrow, r0, MemOperand(sp, 24));
    // --------------
    __ bind(&l_3340);
    __ bl(&l_3340);
    __ mov(Narrow, r7, r0);
    // --------------
    __ bind(&l_3346);
    __ ldr(r2, MemOperand(r8, 100));
    __ adds(Narrow, r1, r4, r6);
    __ cmp(Narrow, r7, r2);
    __ b(ne, Narrow, &l_32b2);
    // --------------
    __ bind(&l_3350);
    __ ldr(Narrow, r0, MemOperand(sp, 24));
    __ movs(Narrow, r1, 0U);
    // --------------
    __ bind(&l_3354);
    __ bl(&l_3354);
    __ ldr(r3, MemOperand(r8, 44));
    __ ldr(Narrow, r0, MemOperand(sp, 8));
    __ ldr(Narrow, r3, MemOperand(r3, 40));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(sp, 8));
    __ b(Narrow, &l_31ae);
    // --------------
    __ bind(&l_3366);
    __ ldr(Narrow, r0, MemOperand(sp, 80));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, &l_323c);
    __ ldr(Narrow, r3, MemOperand(sp, 84));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 77));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, &l_323c);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_323c);
    // --------------
    __ bind(&l_3384);
    __ ldr(Narrow, r4, &l_3444);
    __ adds(r3, r0, 44U);
    __ str(sl, MemOperand(sp, 64));
    __ strb(sl, MemOperand(sp, 77));
    __ str(Narrow, r4, MemOperand(sp, 60));
    __ str(Narrow, r3, MemOperand(sp, 28));
    __ b(eq, &l_31f2);
    __ ldr(Narrow, r6, MemOperand(sp, 28));
    __ movs(Narrow, r2, 1U);
    __ ldr(r3, MemOperand(r8, 44));
    __ movs(Narrow, r1, 32U);
    __ str(Narrow, r2, MemOperand(sp, 84));
    __ movw(r2, 0U);
    __ str(Narrow, r6, MemOperand(sp, 80));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ mov(Narrow, r5, r2);
    __ blx(r3);
    __ ldr(r3, MemOperand(r8, 44));
    __ mov(Narrow, r2, r5);
    __ movt(r2, 0U);
    __ mov(Narrow, r0, r6);
    __ str(Narrow, r2, MemOperand(sp, 20));
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ b(Narrow, &l_31be);
    // --------------
    __ bind(&l_33c8);
    __ ldr(Narrow, r3, MemOperand(r7, 16));
    __ cmp(Narrow, r1, r3);
    __ b(ge, Narrow, &l_32e8);
    __ adds(Narrow, r7, r7, 24U);
    __ b(Narrow, &l_32ae);
    // --------------
    __ bind(&l_33d2);
    __ ldr(Narrow, r0, MemOperand(r7));
    __ mov(Narrow, r1, r7);
    __ adds(Narrow, r7, r7, 24U);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_3346);
    // --------------
    __ bind(&l_33e0);
    __ cmp(Narrow, r3, 1U);
    __ mov(Narrow, r2, r5);
    __ mov(Narrow, r6, r1);
    __ ite(eq);
    __ mov(eq, Narrow, r3, 4U);
    __ mov(ne, Narrow, r3, 8U);
    __ b(Narrow, &l_315a);
    // --------------
    __ bind(&l_33ee);
    __ blx(r3);
    __ b(Narrow, &l_31d2);
    // --------------
    __ bind(&l_33f2);
    __ ldr(Narrow, r0, MemOperand(sp, 8));
    __ blx(r3);
    __ mov(Narrow, r1, r0);
    __ b(Narrow, &l_3316);
    // --------------
    __ bind(&l_33fa);
    __ ldr(r5, MemOperand(r9, 44));
    __ subs(r5, 1U);
    __ lsls(Narrow, r3, r5, 5U);
    __ sub(r5, r3, Operand(r5, LSL, 3));
    __ add(Narrow, r5, r5, r9);
    __ adds(Narrow, r5, r5, 48U);
    __ b(Narrow, &l_3216);
    // --------------
    __ bind(&l_340c);
    __ mov(Narrow, fp, r7);
    __ b(Narrow, &l_32a4);
    // --------------
    __ bind(&l_3410);
    __ movw(r0, 0U);
  }
  void Generate_50(TestAssembler* assembler) {
    __ movw(r1, 0U);
    __ movw(r2, 0U);
    __ movt(r1, 0U);
    __ movt(r2, 0U);
    __ mov(r3, 1306U);
    __ movt(r0, 0U);
    // --------------
    __ bind(&l_342c);
    __ bl(&l_342c);
    // --------------
    __ bind(&l_3430);
    __ bl(&l_3430);
    __ add(Narrow, r0, sp, 88U);
    __ str(r6, MemOperand(r0, -28, PreIndex));
    // --------------
    __ bind(&l_343a);
    __ bl(&l_343a);
    // --------------
    __ bind(&l_343e);
    __ bl(&l_343e);
    __ nop(Narrow);
    // --------------
    __ bind(&l_3444);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3448);
    __ Emit32(0x00000008);
    __ push(RegisterList(0x41f0));
    __ mov(Narrow, r4, r0);
    __ ldr(Narrow, r3, MemOperand(r0, 12));
    __ sub(Narrow, sp, sp, 8U);
    __ mov(Narrow, r6, r1);
    __ mov(Narrow, r0, r1);
    __ ldr(Narrow, r5, MemOperand(r4, 4));
    __ subs(Narrow, r5, r3, r5);
    __ mov(Narrow, r1, r5);
    // --------------
    __ bind(&l_3460);
    __ bl(&l_3460);
    __ cbnz(r0, &l_3484);
    // --------------
    __ bind(&l_3466);
    __ add(r8, r4, 96U);
    __ add(r7, r4, 44U);
    // --------------
    __ bind(&l_346e);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ mov(Narrow, r0, r8);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ mov(Narrow, r1, r7);
    __ mov(Narrow, r2, r6);
    __ subs(Narrow, r3, r5, r3);
    __ add(Narrow, sp, sp, 8U);
    __ pop(RegisterList(0x41f0));
    // --------------
    __ bind(&l_3480);
    __ b(Wide, &l_3480);
    // --------------
    __ bind(&l_3484);
    __ ldr(r3, MemOperand(r4, 132));
    __ cmp(Narrow, r5, r3);
    __ b(eq, Narrow, &l_3492);
    __ adds(Narrow, r2, r5, 2U);
    __ cmp(Narrow, r3, r2);
    __ b(ge, Narrow, &l_3466);
    // --------------
    __ bind(&l_3492);
    __ add(r8, r4, 96U);
    __ add(r7, r4, 44U);
    __ movs(Narrow, r3, 0U);
    __ mov(Narrow, r2, r5);
    __ str(Narrow, r3, MemOperand(sp));
    __ mov(Narrow, r0, r8);
    __ mov(Narrow, r1, r7);
    __ movs(Narrow, r3, 2U);
    // --------------
    __ bind(&l_34a6);
    __ bl(&l_34a6);
    __ b(Narrow, &l_346e);
    __ ldrb(r3, MemOperand(r0, 32));
    __ push(RegisterList(0x43f0));
    __ mov(Narrow, r4, r0);
    __ sub(Narrow, sp, sp, 12U);
    __ cbz(r3, &l_34f8);
    __ mov(Narrow, r6, r0);
    __ movw(r3, 0U);
    __ ldr(r5, MemOperand(r6, 8, PostIndex));
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r5));
    __ ldr(Narrow, r2, MemOperand(r2, 8));
    __ cmp(Narrow, r2, r3);
    __ b(ne, Narrow, &l_3518);
    __ ldr(Narrow, r7, MemOperand(r5, 12));
    __ mov(Narrow, r0, r6);
    __ ldr(Narrow, r3, MemOperand(r5, 4));
    __ subs(Narrow, r7, r7, r3);
    __ mov(Narrow, r1, r7);
    // --------------
    __ bind(&l_34da);
    __ bl(&l_34da);
    __ add(r9, r5, 96U);
    __ add(r8, r5, 44U);
    __ cbnz(r0, &l_3520);
    // --------------
    __ bind(&l_34e8);
    __ ldr(Narrow, r7, MemOperand(r5, 12));
    __ mov(Narrow, r0, r9);
    __ ldr(Narrow, r3, MemOperand(r5, 4));
    __ mov(Narrow, r1, r8);
    __ mov(Narrow, r2, r6);
    __ subs(Narrow, r3, r7, r3);
    // --------------
    __ bind(&l_34f4);
    __ bl(&l_34f4);
    // --------------
    __ bind(&l_34f8);
    __ ldr(r5, MemOperand(r4, 152));
    __ ldr(Narrow, r3, &l_3558);
    __ str(Narrow, r3, MemOperand(r4, 8));
    __ cbz(r5, &l_3510);
    __ ldr(Narrow, r0, MemOperand(r5));
    __ cbz(r0, &l_350a);
    // --------------
    __ bind(&l_3506);
    __ bl(&l_3506);
    // --------------
    __ bind(&l_350a);
    __ mov(Narrow, r0, r5);
    // --------------
    __ bind(&l_350c);
    __ bl(&l_350c);
    // --------------
    __ bind(&l_3510);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, sp, sp, 12U);
    __ pop(RegisterList(0x83f0));
    // --------------
    __ bind(&l_3518);
    __ mov(Narrow, r0, r5);
    __ mov(Narrow, r1, r6);
    __ blx(r2);
    __ b(Narrow, &l_34f8);
    // --------------
    __ bind(&l_3520);
    __ ldr(r3, MemOperand(r5, 132));
    __ add(r9, r5, 96U);
  }
  void Generate_51(TestAssembler* assembler) {
    __ add(r8, r5, 44U);
    __ cmp(Narrow, r7, r3);
    __ b(eq, Narrow, &l_3536);
    __ adds(Narrow, r2, r7, 2U);
    __ cmp(Narrow, r3, r2);
    __ b(ge, Narrow, &l_34e8);
    // --------------
    __ bind(&l_3536);
    __ movs(Narrow, r3, 0U);
    __ mov(Narrow, r2, r7);
    __ str(Narrow, r3, MemOperand(sp));
    __ mov(Narrow, r1, r8);
    __ mov(Narrow, r0, r9);
    __ movs(Narrow, r3, 2U);
    // --------------
    __ bind(&l_3542);
    __ bl(&l_3542);
    __ b(Narrow, &l_34e8);
    __ ldr(Narrow, r3, &l_3558);
    __ add(r0, r4, 36U);
    __ str(Narrow, r3, MemOperand(r4, 8));
    // --------------
    __ bind(&l_3550);
    __ bl(&l_3550);
    // --------------
    __ bind(&l_3554);
    __ bl(&l_3554);
    // --------------
    __ bind(&l_3558);
    __ Emit32(0x00000008);
    __ push(RegisterList(0x4ff0));
    __ adds(r7, r0, 44U);
    __ ldr(Narrow, r6, &l_3740);
    __ sub(Narrow, sp, sp, 212U);
    __ mov(Narrow, r5, r1);
    __ mov(r1, 0U);
    __ mov(Narrow, r4, r0);
    __ mov(Narrow, sl, r2);
    __ mov(Narrow, fp, r3);
    __ mov(Narrow, r8, r5);
    __ str(Narrow, r6, MemOperand(sp, 12));
    __ str(Narrow, r1, MemOperand(sp, 16));
    __ strb(r1, MemOperand(sp, 29));
    __ b(eq, Narrow, &l_35d0);
    __ ldr(Narrow, r3, MemOperand(r0, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r2, MemOperand(sp, 36));
    __ mov(Narrow, r0, r7);
    __ movs(Narrow, r1, 32U);
    __ str(Narrow, r7, MemOperand(sp, 32));
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r7);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ mov(Narrow, r0, r7);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_3718);
    // --------------
    __ bind(&l_35ac);
    __ bl(&l_35ac);
    // --------------
    __ bind(&l_35b0);
    __ str(Narrow, r0, MemOperand(sp, 16));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_35ca);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_35c6);
    __ bl(&l_35c6);
    // --------------
    __ bind(&l_35ca);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 29));
    // --------------
    __ bind(&l_35d0);
    __ ldr(r9, &l_374c);
    __ cmp(Narrow, r5, 14U);
    __ ldr(Narrow, r1, &l_3744);
    __ mov(r3, 0U);
    __ mov(r2, 1U);
    __ mov(r0, 4U);
    __ str(Narrow, r4, MemOperand(sp, 40));
    __ str(Narrow, r5, MemOperand(sp, 44));
    __ str(r9, MemOperand(sp, 12));
    __ str(Narrow, r0, MemOperand(sp, 52));
    __ str(Narrow, r2, MemOperand(sp, 56));
    __ str(Narrow, r2, MemOperand(sp, 60));
    __ strb(r2, MemOperand(sp, 88));
    __ str(Narrow, r2, MemOperand(sp, 104));
    __ str(Narrow, r2, MemOperand(sp, 128));
    __ str(Narrow, r2, MemOperand(sp, 152));
    __ str(Narrow, r2, MemOperand(sp, 176));
    __ strb(r3, MemOperand(sp, 64));
    __ str(Narrow, r3, MemOperand(sp, 68));
    __ strb(r3, MemOperand(sp, 72));
    __ strb(r3, MemOperand(sp, 76));
    __ str(Narrow, r3, MemOperand(sp, 92));
    __ str(Narrow, r3, MemOperand(sp, 96));
    __ str(Narrow, r3, MemOperand(sp, 100));
    __ str(Narrow, r3, MemOperand(sp, 108));
    __ str(Narrow, r3, MemOperand(sp, 112));
    __ str(Narrow, r3, MemOperand(sp, 116));
    __ str(Narrow, r3, MemOperand(sp, 120));
    __ str(Narrow, r3, MemOperand(sp, 124));
    __ str(Narrow, r3, MemOperand(sp, 132));
    __ str(Narrow, r3, MemOperand(sp, 136));
    __ str(Narrow, r3, MemOperand(sp, 140));
  }
  void Generate_52(TestAssembler* assembler) {
    __ str(Narrow, r3, MemOperand(sp, 144));
    __ str(Narrow, r3, MemOperand(sp, 148));
    __ str(Narrow, r3, MemOperand(sp, 156));
    __ str(Narrow, r3, MemOperand(sp, 160));
    __ str(Narrow, r3, MemOperand(sp, 164));
    __ str(Narrow, r3, MemOperand(sp, 168));
    __ str(Narrow, r3, MemOperand(sp, 172));
    __ str(Narrow, r3, MemOperand(sp, 180));
    __ str(Narrow, r3, MemOperand(sp, 184));
    __ str(Narrow, r3, MemOperand(sp, 188));
    __ str(Narrow, r3, MemOperand(sp, 192));
    __ strb(r3, MemOperand(sp, 200));
    __ str(Narrow, r1, MemOperand(sp, 48));
    __ b(eq, Narrow, &l_3646);
    __ ldr(Narrow, r2, MemOperand(r4, 28));
    __ cmp(Narrow, r2, 1U);
    __ b(eq, Narrow, &l_36d6);
    // --------------
    __ bind(&l_3646);
    __ ldr(Narrow, r3, MemOperand(sp, 248));
    __ mov(Narrow, r0, r4);
    __ mov(Narrow, r1, r8);
    __ mov(Narrow, r2, sl);
    __ str(Narrow, r3, MemOperand(sp));
    __ mov(Narrow, r3, fp);
    // --------------
    __ bind(&l_3652);
    __ bl(&l_3652);
    __ ldrb(r3, MemOperand(sp, 72));
    __ cbz(r3, &l_3698);
    __ ldr(Narrow, r4, MemOperand(sp, 40));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r4));
    __ ldr(Narrow, r2, MemOperand(r2, 8));
    __ cmp(Narrow, r2, r3);
    __ b(ne, Narrow, &l_36e8);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ add(Narrow, r0, sp, 48U);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ subs(Narrow, r5, r5, r3);
    __ mov(Narrow, r1, r5);
    // --------------
    __ bind(&l_3678);
    __ bl(&l_3678);
    __ add(r8, r4, 96U);
    __ add(r7, r4, 44U);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_36f0);
    // --------------
    __ bind(&l_3688);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ mov(Narrow, r0, r8);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ mov(Narrow, r1, r7);
    __ add(Narrow, r2, sp, 48U);
    __ subs(Narrow, r3, r5, r3);
    // --------------
    __ bind(&l_3694);
    __ bl(&l_3694);
    // --------------
    __ bind(&l_3698);
    __ ldr(Narrow, r4, MemOperand(sp, 192));
    __ ldr(Narrow, r3, &l_3748);
    __ str(Narrow, r3, MemOperand(sp, 48));
    __ cbz(r4, &l_36ae);
    __ ldr(Narrow, r0, MemOperand(r4));
    __ cbz(r0, &l_36a8);
    // --------------
    __ bind(&l_36a4);
    __ bl(&l_36a4);
    // --------------
    __ bind(&l_36a8);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_36aa);
    __ bl(&l_36aa);
    // --------------
    __ bind(&l_36ae);
    __ ldrb(r3, MemOperand(sp, 29));
    __ str(Narrow, r6, MemOperand(sp, 12));
    __ cbnz(r3, &l_36bc);
    // --------------
    __ bind(&l_36b6);
    __ add(Narrow, sp, sp, 212U);
    __ pop(RegisterList(0x8ff0));
    // --------------
    __ bind(&l_36bc);
    __ ldr(Narrow, r0, MemOperand(sp, 32));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_36b6);
    __ ldr(Narrow, r3, MemOperand(sp, 36));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 29));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_36b6);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_36b6);
    // --------------
    __ bind(&l_36d6);
    __ eor(r1, r5, 0x1);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, r3, sp, 48U);
    // --------------
    __ bind(&l_36de);
    __ bl(&l_36de);
    __ mov(r8, 14U);
    __ b(Narrow, &l_3646);
    // --------------
    __ bind(&l_36e8);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, r1, sp, 48U);
    __ blx(r2);
    __ b(Narrow, &l_3698);
    // --------------
    __ bind(&l_36f0);
    __ ldr(r3, MemOperand(r4, 132));
    __ add(r8, r4, 96U);
    __ add(r7, r4, 44U);
    __ cmp(Narrow, r5, r3);
    __ b(eq, Narrow, &l_3706);
    __ adds(Narrow, r2, r5, 2U);
    __ cmp(Narrow, r3, r2);
    __ b(ge, Narrow, &l_3688);
    // --------------
    __ bind(&l_3706);
    __ movs(Narrow, r3, 0U);
    __ mov(Narrow, r2, r5);
    __ str(Narrow, r3, MemOperand(sp));
    __ mov(Narrow, r1, r7);
    __ mov(Narrow, r0, r8);
    __ movs(Narrow, r3, 2U);
  }
  void Generate_53(TestAssembler* assembler) {
    // --------------
    __ bind(&l_3712);
    __ bl(&l_3712);
    __ b(Narrow, &l_3688);
    // --------------
    __ bind(&l_3718);
    __ blx(r2);
    __ b(Narrow, &l_35b0);
    __ add(Narrow, r0, sp, 40U);
    // --------------
    __ bind(&l_371e);
    __ bl(&l_371e);
    // --------------
    __ bind(&l_3722);
    __ add(Narrow, r0, sp, 208U);
    __ str(r9, MemOperand(r0, -196, PreIndex));
    // --------------
    __ bind(&l_3728);
    __ bl(&l_3728);
    // --------------
    __ bind(&l_372c);
    __ bl(&l_372c);
    // --------------
    __ bind(&l_3730);
    __ ldr(Narrow, r3, &l_3748);
    __ add(Narrow, r0, sp, 76U);
    __ str(Narrow, r3, MemOperand(sp, 48));
    // --------------
    __ bind(&l_3736);
    __ bl(&l_3736);
    __ b(Narrow, &l_3722);
    __ b(Narrow, &l_3730);
    __ nop(Narrow);
    // --------------
    __ bind(&l_3740);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3744);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3748);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_374c);
    __ Emit32(0x00000008);
    __ push(RegisterList(0x4ff0));
    __ adds(r7, r0, 44U);
    __ ldr(Narrow, r6, &l_3938);
    __ sub(Narrow, sp, sp, 212U);
    __ mov(Narrow, r5, r1);
    __ mov(r1, 0U);
    __ mov(Narrow, r4, r0);
    __ mov(Narrow, sl, r2);
    __ mov(Narrow, fp, r3);
    __ mov(Narrow, r8, r5);
    __ str(Narrow, r6, MemOperand(sp, 12));
    __ str(Narrow, r1, MemOperand(sp, 16));
    __ strb(r1, MemOperand(sp, 29));
    __ b(eq, Narrow, &l_37c4);
    __ ldr(Narrow, r3, MemOperand(r0, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r2, MemOperand(sp, 36));
    __ mov(Narrow, r0, r7);
    __ movs(Narrow, r1, 32U);
    __ str(Narrow, r7, MemOperand(sp, 32));
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r7);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ mov(Narrow, r0, r7);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_3910);
    // --------------
    __ bind(&l_37a0);
    __ bl(&l_37a0);
    // --------------
    __ bind(&l_37a4);
    __ str(Narrow, r0, MemOperand(sp, 16));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_37be);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_37ba);
    __ bl(&l_37ba);
    // --------------
    __ bind(&l_37be);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 29));
    // --------------
    __ bind(&l_37c4);
    __ ldr(r9, &l_3944);
    __ cmp(Narrow, r5, 14U);
    __ ldr(Narrow, r1, &l_393c);
    __ mov(r3, 0U);
    __ mov(r2, 1U);
    __ mov(r0, 4U);
    __ str(Narrow, r4, MemOperand(sp, 40));
    __ str(Narrow, r5, MemOperand(sp, 44));
    __ str(r9, MemOperand(sp, 12));
    __ str(Narrow, r0, MemOperand(sp, 52));
    __ str(Narrow, r2, MemOperand(sp, 56));
    __ str(Narrow, r2, MemOperand(sp, 60));
    __ strb(r2, MemOperand(sp, 88));
    __ str(Narrow, r2, MemOperand(sp, 104));
    __ str(Narrow, r2, MemOperand(sp, 128));
    __ str(Narrow, r2, MemOperand(sp, 152));
    __ str(Narrow, r2, MemOperand(sp, 176));
    __ strb(r3, MemOperand(sp, 64));
    __ str(Narrow, r3, MemOperand(sp, 68));
    __ strb(r3, MemOperand(sp, 72));
    __ strb(r3, MemOperand(sp, 76));
    __ str(Narrow, r3, MemOperand(sp, 92));
    __ str(Narrow, r3, MemOperand(sp, 96));
    __ str(Narrow, r3, MemOperand(sp, 100));
    __ str(Narrow, r3, MemOperand(sp, 108));
    __ str(Narrow, r3, MemOperand(sp, 112));
    __ str(Narrow, r3, MemOperand(sp, 116));
    __ str(Narrow, r3, MemOperand(sp, 120));
    __ str(Narrow, r3, MemOperand(sp, 124));
    __ str(Narrow, r3, MemOperand(sp, 132));
    __ str(Narrow, r3, MemOperand(sp, 136));
  }
  void Generate_54(TestAssembler* assembler) {
    __ str(Narrow, r3, MemOperand(sp, 140));
    __ str(Narrow, r3, MemOperand(sp, 144));
    __ str(Narrow, r3, MemOperand(sp, 148));
    __ str(Narrow, r3, MemOperand(sp, 156));
    __ str(Narrow, r3, MemOperand(sp, 160));
    __ str(Narrow, r3, MemOperand(sp, 164));
    __ str(Narrow, r3, MemOperand(sp, 168));
    __ str(Narrow, r3, MemOperand(sp, 172));
    __ str(Narrow, r3, MemOperand(sp, 180));
    __ str(Narrow, r3, MemOperand(sp, 184));
    __ str(Narrow, r3, MemOperand(sp, 188));
    __ str(Narrow, r3, MemOperand(sp, 192));
    __ strb(r3, MemOperand(sp, 200));
    __ str(Narrow, r1, MemOperand(sp, 48));
    __ b(eq, Narrow, &l_383a);
    __ ldr(Narrow, r2, MemOperand(r4, 28));
    __ cmp(Narrow, r2, 1U);
    __ b(eq, Narrow, &l_38ce);
    // --------------
    __ bind(&l_383a);
    __ ldr(Narrow, r3, MemOperand(sp, 248));
    __ mov(Narrow, r0, r4);
    __ mov(Narrow, r1, r8);
    __ mov(Narrow, r2, sl);
    __ str(Narrow, r3, MemOperand(sp));
    __ ldr(Narrow, r3, MemOperand(sp, 252));
    __ str(Narrow, r3, MemOperand(sp, 4));
    __ mov(Narrow, r3, fp);
    // --------------
    __ bind(&l_384a);
    __ bl(&l_384a);
    __ ldrb(r3, MemOperand(sp, 72));
    __ cbz(r3, &l_3890);
    __ ldr(Narrow, r4, MemOperand(sp, 40));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r4));
    __ ldr(Narrow, r2, MemOperand(r2, 8));
    __ cmp(Narrow, r2, r3);
    __ b(ne, Narrow, &l_38e0);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ add(Narrow, r0, sp, 48U);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ subs(Narrow, r5, r5, r3);
    __ mov(Narrow, r1, r5);
    // --------------
    __ bind(&l_3870);
    __ bl(&l_3870);
    __ add(r8, r4, 96U);
    __ add(r7, r4, 44U);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_38e8);
    // --------------
    __ bind(&l_3880);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ mov(Narrow, r0, r8);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ mov(Narrow, r1, r7);
    __ add(Narrow, r2, sp, 48U);
    __ subs(Narrow, r3, r5, r3);
    // --------------
    __ bind(&l_388c);
    __ bl(&l_388c);
    // --------------
    __ bind(&l_3890);
    __ ldr(Narrow, r4, MemOperand(sp, 192));
    __ ldr(Narrow, r3, &l_3940);
    __ str(Narrow, r3, MemOperand(sp, 48));
    __ cbz(r4, &l_38a6);
    __ ldr(Narrow, r0, MemOperand(r4));
    __ cbz(r0, &l_38a0);
    // --------------
    __ bind(&l_389c);
    __ bl(&l_389c);
    // --------------
    __ bind(&l_38a0);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_38a2);
    __ bl(&l_38a2);
    // --------------
    __ bind(&l_38a6);
    __ ldrb(r3, MemOperand(sp, 29));
    __ str(Narrow, r6, MemOperand(sp, 12));
    __ cbnz(r3, &l_38b4);
    // --------------
    __ bind(&l_38ae);
    __ add(Narrow, sp, sp, 212U);
    __ pop(RegisterList(0x8ff0));
    // --------------
    __ bind(&l_38b4);
    __ ldr(Narrow, r0, MemOperand(sp, 32));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_38ae);
    __ ldr(Narrow, r3, MemOperand(sp, 36));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 29));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_38ae);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_38ae);
    // --------------
    __ bind(&l_38ce);
    __ eor(r1, r5, 0x1);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, r3, sp, 48U);
    // --------------
    __ bind(&l_38d6);
    __ bl(&l_38d6);
    __ mov(r8, 14U);
    __ b(Narrow, &l_383a);
    // --------------
    __ bind(&l_38e0);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, r1, sp, 48U);
    __ blx(r2);
    __ b(Narrow, &l_3890);
    // --------------
    __ bind(&l_38e8);
    __ ldr(r3, MemOperand(r4, 132));
    __ add(r8, r4, 96U);
    __ add(r7, r4, 44U);
    __ cmp(Narrow, r5, r3);
    __ b(eq, Narrow, &l_38fe);
    __ adds(Narrow, r2, r5, 2U);
    __ cmp(Narrow, r3, r2);
    __ b(ge, Narrow, &l_3880);
    // --------------
    __ bind(&l_38fe);
    __ movs(Narrow, r3, 0U);
    __ mov(Narrow, r2, r5);
    __ str(Narrow, r3, MemOperand(sp));
  }
  void Generate_55(TestAssembler* assembler) {
    __ mov(Narrow, r1, r7);
    __ mov(Narrow, r0, r8);
    __ movs(Narrow, r3, 2U);
    // --------------
    __ bind(&l_390a);
    __ bl(&l_390a);
    __ b(Narrow, &l_3880);
    // --------------
    __ bind(&l_3910);
    __ blx(r2);
    __ b(Narrow, &l_37a4);
    __ add(Narrow, r0, sp, 40U);
    // --------------
    __ bind(&l_3916);
    __ bl(&l_3916);
    // --------------
    __ bind(&l_391a);
    __ add(Narrow, r0, sp, 208U);
    __ str(r9, MemOperand(r0, -196, PreIndex));
    // --------------
    __ bind(&l_3920);
    __ bl(&l_3920);
    // --------------
    __ bind(&l_3924);
    __ bl(&l_3924);
    // --------------
    __ bind(&l_3928);
    __ ldr(Narrow, r3, &l_3940);
    __ add(Narrow, r0, sp, 76U);
    __ str(Narrow, r3, MemOperand(sp, 48));
    // --------------
    __ bind(&l_392e);
    __ bl(&l_392e);
    __ b(Narrow, &l_391a);
    __ b(Narrow, &l_3928);
    __ nop(Narrow);
    // --------------
    __ bind(&l_3938);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_393c);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3940);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3944);
    __ Emit32(0x00000008);
    __ push(RegisterList(0x4ff0));
    __ adds(r7, r0, 44U);
    __ ldr(Narrow, r6, &l_3b30);
    __ sub(Narrow, sp, sp, 212U);
    __ mov(Narrow, r5, r1);
    __ mov(r1, 0U);
    __ mov(Narrow, r4, r0);
    __ mov(Narrow, sl, r2);
    __ mov(Narrow, fp, r3);
    __ mov(Narrow, r8, r5);
    __ str(Narrow, r6, MemOperand(sp, 12));
    __ str(Narrow, r1, MemOperand(sp, 16));
    __ strb(r1, MemOperand(sp, 29));
    __ b(eq, Narrow, &l_39bc);
    __ ldr(Narrow, r3, MemOperand(r0, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r2, MemOperand(sp, 36));
    __ mov(Narrow, r0, r7);
    __ movs(Narrow, r1, 32U);
    __ str(Narrow, r7, MemOperand(sp, 32));
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r7);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ mov(Narrow, r0, r7);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_3b08);
    // --------------
    __ bind(&l_3998);
    __ bl(&l_3998);
    // --------------
    __ bind(&l_399c);
    __ str(Narrow, r0, MemOperand(sp, 16));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_39b6);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_39b2);
    __ bl(&l_39b2);
    // --------------
    __ bind(&l_39b6);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 29));
    // --------------
    __ bind(&l_39bc);
    __ ldr(r9, &l_3b3c);
    __ cmp(Narrow, r5, 14U);
    __ ldr(Narrow, r1, &l_3b34);
    __ mov(r3, 0U);
    __ mov(r2, 1U);
    __ mov(r0, 4U);
    __ str(Narrow, r4, MemOperand(sp, 40));
    __ str(Narrow, r5, MemOperand(sp, 44));
    __ str(r9, MemOperand(sp, 12));
    __ str(Narrow, r0, MemOperand(sp, 52));
    __ str(Narrow, r2, MemOperand(sp, 56));
    __ str(Narrow, r2, MemOperand(sp, 60));
    __ strb(r2, MemOperand(sp, 88));
    __ str(Narrow, r2, MemOperand(sp, 104));
    __ str(Narrow, r2, MemOperand(sp, 128));
    __ str(Narrow, r2, MemOperand(sp, 152));
    __ str(Narrow, r2, MemOperand(sp, 176));
    __ strb(r3, MemOperand(sp, 64));
    __ str(Narrow, r3, MemOperand(sp, 68));
    __ strb(r3, MemOperand(sp, 72));
    __ strb(r3, MemOperand(sp, 76));
    __ str(Narrow, r3, MemOperand(sp, 92));
    __ str(Narrow, r3, MemOperand(sp, 96));
    __ str(Narrow, r3, MemOperand(sp, 100));
    __ str(Narrow, r3, MemOperand(sp, 108));
    __ str(Narrow, r3, MemOperand(sp, 112));
    __ str(Narrow, r3, MemOperand(sp, 116));
    __ str(Narrow, r3, MemOperand(sp, 120));
  }
  void Generate_56(TestAssembler* assembler) {
    __ str(Narrow, r3, MemOperand(sp, 124));
    __ str(Narrow, r3, MemOperand(sp, 132));
    __ str(Narrow, r3, MemOperand(sp, 136));
    __ str(Narrow, r3, MemOperand(sp, 140));
    __ str(Narrow, r3, MemOperand(sp, 144));
    __ str(Narrow, r3, MemOperand(sp, 148));
    __ str(Narrow, r3, MemOperand(sp, 156));
    __ str(Narrow, r3, MemOperand(sp, 160));
    __ str(Narrow, r3, MemOperand(sp, 164));
    __ str(Narrow, r3, MemOperand(sp, 168));
    __ str(Narrow, r3, MemOperand(sp, 172));
    __ str(Narrow, r3, MemOperand(sp, 180));
    __ str(Narrow, r3, MemOperand(sp, 184));
    __ str(Narrow, r3, MemOperand(sp, 188));
    __ str(Narrow, r3, MemOperand(sp, 192));
    __ strb(r3, MemOperand(sp, 200));
    __ str(Narrow, r1, MemOperand(sp, 48));
    __ b(eq, Narrow, &l_3a32);
    __ ldr(Narrow, r2, MemOperand(r4, 28));
    __ cmp(Narrow, r2, 1U);
    __ b(eq, Narrow, &l_3ac6);
    // --------------
    __ bind(&l_3a32);
    __ ldr(Narrow, r3, MemOperand(sp, 248));
    __ mov(Narrow, r0, r4);
    __ mov(Narrow, r1, r8);
    __ mov(Narrow, r2, sl);
    __ str(Narrow, r3, MemOperand(sp));
    __ ldr(Narrow, r3, MemOperand(sp, 252));
    __ str(Narrow, r3, MemOperand(sp, 4));
    __ mov(Narrow, r3, fp);
    // --------------
    __ bind(&l_3a42);
    __ bl(&l_3a42);
    __ ldrb(r3, MemOperand(sp, 72));
    __ cbz(r3, &l_3a88);
    __ ldr(Narrow, r4, MemOperand(sp, 40));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r4));
    __ ldr(Narrow, r2, MemOperand(r2, 8));
    __ cmp(Narrow, r2, r3);
    __ b(ne, Narrow, &l_3ad8);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ add(Narrow, r0, sp, 48U);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ subs(Narrow, r5, r5, r3);
    __ mov(Narrow, r1, r5);
    // --------------
    __ bind(&l_3a68);
    __ bl(&l_3a68);
    __ add(r8, r4, 96U);
    __ add(r7, r4, 44U);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_3ae0);
    // --------------
    __ bind(&l_3a78);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ mov(Narrow, r0, r8);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ mov(Narrow, r1, r7);
    __ add(Narrow, r2, sp, 48U);
    __ subs(Narrow, r3, r5, r3);
    // --------------
    __ bind(&l_3a84);
    __ bl(&l_3a84);
    // --------------
    __ bind(&l_3a88);
    __ ldr(Narrow, r4, MemOperand(sp, 192));
    __ ldr(Narrow, r3, &l_3b38);
    __ str(Narrow, r3, MemOperand(sp, 48));
    __ cbz(r4, &l_3a9e);
    __ ldr(Narrow, r0, MemOperand(r4));
    __ cbz(r0, &l_3a98);
    // --------------
    __ bind(&l_3a94);
    __ bl(&l_3a94);
    // --------------
    __ bind(&l_3a98);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_3a9a);
    __ bl(&l_3a9a);
    // --------------
    __ bind(&l_3a9e);
    __ ldrb(r3, MemOperand(sp, 29));
    __ str(Narrow, r6, MemOperand(sp, 12));
    __ cbnz(r3, &l_3aac);
    // --------------
    __ bind(&l_3aa6);
    __ add(Narrow, sp, sp, 212U);
    __ pop(RegisterList(0x8ff0));
    // --------------
    __ bind(&l_3aac);
    __ ldr(Narrow, r0, MemOperand(sp, 32));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_3aa6);
    __ ldr(Narrow, r3, MemOperand(sp, 36));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 29));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_3aa6);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_3aa6);
    // --------------
    __ bind(&l_3ac6);
    __ eor(r1, r5, 0x1);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, r3, sp, 48U);
    // --------------
    __ bind(&l_3ace);
    __ bl(&l_3ace);
    __ mov(r8, 14U);
    __ b(Narrow, &l_3a32);
    // --------------
    __ bind(&l_3ad8);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, r1, sp, 48U);
    __ blx(r2);
    __ b(Narrow, &l_3a88);
    // --------------
    __ bind(&l_3ae0);
    __ ldr(r3, MemOperand(r4, 132));
    __ add(r8, r4, 96U);
    __ add(r7, r4, 44U);
    __ cmp(Narrow, r5, r3);
    __ b(eq, Narrow, &l_3af6);
    __ adds(Narrow, r2, r5, 2U);
    __ cmp(Narrow, r3, r2);
    __ b(ge, Narrow, &l_3a78);
  }
  void Generate_57(TestAssembler* assembler) {
    // --------------
    __ bind(&l_3af6);
    __ movs(Narrow, r3, 0U);
    __ mov(Narrow, r2, r5);
    __ str(Narrow, r3, MemOperand(sp));
    __ mov(Narrow, r1, r7);
    __ mov(Narrow, r0, r8);
    __ movs(Narrow, r3, 2U);
    // --------------
    __ bind(&l_3b02);
    __ bl(&l_3b02);
    __ b(Narrow, &l_3a78);
    // --------------
    __ bind(&l_3b08);
    __ blx(r2);
    __ b(Narrow, &l_399c);
    __ add(Narrow, r0, sp, 40U);
    // --------------
    __ bind(&l_3b0e);
    __ bl(&l_3b0e);
    // --------------
    __ bind(&l_3b12);
    __ add(Narrow, r0, sp, 208U);
    __ str(r9, MemOperand(r0, -196, PreIndex));
    // --------------
    __ bind(&l_3b18);
    __ bl(&l_3b18);
    // --------------
    __ bind(&l_3b1c);
    __ bl(&l_3b1c);
    // --------------
    __ bind(&l_3b20);
    __ ldr(Narrow, r3, &l_3b38);
    __ add(Narrow, r0, sp, 76U);
    __ str(Narrow, r3, MemOperand(sp, 48));
    // --------------
    __ bind(&l_3b26);
    __ bl(&l_3b26);
    __ b(Narrow, &l_3b12);
    __ b(Narrow, &l_3b20);
    __ nop(Narrow);
    // --------------
    __ bind(&l_3b30);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3b34);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3b38);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3b3c);
    __ Emit32(0x00000008);
    __ movw(r3, 0U);
    __ push(RegisterList(0x47f0));
    __ movt(r3, 0U);
    __ mov(Narrow, r6, r0);
    __ adds(r4, r6, 44U);
    __ sub(Narrow, sp, sp, 232U);
    __ ldr(Narrow, r0, MemOperand(r3));
    __ mov(r2, 800U);
    __ ldr(Narrow, r7, &l_3d1c);
    __ mov(r3, 0U);
    __ mov(Narrow, r8, r1);
    __ vstr(s0, MemOperand(sp, 8));
    __ str(Narrow, r0, MemOperand(sp, 24));
    __ str(Narrow, r7, MemOperand(sp, 36));
    __ str(Narrow, r2, MemOperand(sp, 16));
    __ str(Narrow, r3, MemOperand(sp, 40));
    __ strb(r3, MemOperand(sp, 53));
    __ b(eq, Narrow, &l_3bc4);
    __ ldr(Narrow, r3, MemOperand(r6, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r2, MemOperand(sp, 60));
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 32U);
    __ str(Narrow, r4, MemOperand(sp, 56));
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r6, 44));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r6, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_3cf8);
    // --------------
    __ bind(&l_3ba0);
    __ bl(&l_3ba0);
    // --------------
    __ bind(&l_3ba4);
    __ str(Narrow, r0, MemOperand(sp, 40));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r2, MemOperand(r0, 4));
    __ ldr(Narrow, r3, MemOperand(r0, 12));
    __ add(Narrow, r2, r2, r1);
    __ subs(Narrow, r2, r2, r3);
    __ cmp(Narrow, r2, 31U);
    __ b(hi, Narrow, &l_3bbe);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_3bba);
    __ bl(&l_3bba);
    // --------------
    __ bind(&l_3bbe);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 53));
    // --------------
    __ bind(&l_3bc4);
    __ ldr(r9, &l_3d24);
    __ movs(Narrow, r4, 0U);
    __ ldr(lr, &l_3d28);
    __ movs(Narrow, r5, 1U);
    __ movs(Narrow, r2, 14U);
    __ mov(sl, 4U);
    __ add(ip, sp, 8U);
    __ str(Narrow, r2, MemOperand(sp, 68));
    __ str(Narrow, r6, MemOperand(sp, 64));
    __ mov(Narrow, r1, r2);
    __ str(r9, MemOperand(sp, 36));
    __ mov(Narrow, r0, r6);
    __ str(sl, MemOperand(sp, 76));
    __ mov(Narrow, r3, r8);
    __ str(Narrow, r5, MemOperand(sp, 80));
    __ mov(r2, 800U);
    __ str(Narrow, r5, MemOperand(sp, 84));
    __ strb(r5, MemOperand(sp, 112));
    __ str(Narrow, r5, MemOperand(sp, 128));
    __ str(Narrow, r5, MemOperand(sp, 152));
    __ str(Narrow, r5, MemOperand(sp, 176));
  }
  void Generate_58(TestAssembler* assembler) {
    __ str(Narrow, r5, MemOperand(sp, 200));
    __ strb(r4, MemOperand(sp, 88));
    __ str(Narrow, r4, MemOperand(sp, 92));
    __ strb(r4, MemOperand(sp, 96));
    __ strb(r4, MemOperand(sp, 100));
    __ str(Narrow, r4, MemOperand(sp, 116));
    __ str(Narrow, r4, MemOperand(sp, 120));
    __ str(Narrow, r4, MemOperand(sp, 124));
    __ str(Narrow, r4, MemOperand(sp, 132));
    __ str(Narrow, r4, MemOperand(sp, 136));
    __ str(Narrow, r4, MemOperand(sp, 140));
    __ str(Narrow, r4, MemOperand(sp, 144));
    __ str(Narrow, r4, MemOperand(sp, 148));
    __ str(Narrow, r4, MemOperand(sp, 156));
    __ str(Narrow, r4, MemOperand(sp, 160));
    __ str(Narrow, r4, MemOperand(sp, 164));
    __ str(Narrow, r4, MemOperand(sp, 168));
    __ str(Narrow, r4, MemOperand(sp, 172));
    __ str(Narrow, r4, MemOperand(sp, 180));
    __ str(Narrow, r4, MemOperand(sp, 184));
    __ str(Narrow, r4, MemOperand(sp, 188));
    __ str(Narrow, r4, MemOperand(sp, 192));
    __ str(Narrow, r4, MemOperand(sp, 196));
    __ str(Narrow, r4, MemOperand(sp, 204));
    __ str(ip, MemOperand(sp));
    __ str(Narrow, r4, MemOperand(sp, 208));
    __ str(Narrow, r4, MemOperand(sp, 212));
    __ str(Narrow, r4, MemOperand(sp, 216));
    __ strb(r4, MemOperand(sp, 224));
    __ str(lr, MemOperand(sp, 72));
    // --------------
    __ bind(&l_3c46);
    __ bl(&l_3c46);
    __ ldrb(r3, MemOperand(sp, 96));
    __ cbz(r3, &l_3c8a);
    __ ldr(Narrow, r4, MemOperand(sp, 64));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r4));
    __ ldr(Narrow, r2, MemOperand(r2, 8));
    __ cmp(Narrow, r2, r3);
    __ b(ne, Narrow, &l_3cc8);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ add(Narrow, r0, sp, 72U);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ subs(Narrow, r5, r5, r3);
    __ mov(Narrow, r1, r5);
    // --------------
    __ bind(&l_3c6c);
    __ bl(&l_3c6c);
    __ add(r8, r4, 96U);
    __ add(r6, r4, 44U);
    __ cbnz(r0, &l_3cd0);
    // --------------
    __ bind(&l_3c7a);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ mov(Narrow, r0, r8);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ mov(Narrow, r1, r6);
    __ add(Narrow, r2, sp, 72U);
    __ subs(Narrow, r3, r5, r3);
    // --------------
    __ bind(&l_3c86);
    __ bl(&l_3c86);
    // --------------
    __ bind(&l_3c8a);
    __ ldr(Narrow, r4, MemOperand(sp, 216));
    __ ldr(Narrow, r3, &l_3d20);
    __ str(Narrow, r3, MemOperand(sp, 72));
    __ cbz(r4, &l_3ca0);
    __ ldr(Narrow, r0, MemOperand(r4));
    __ cbz(r0, &l_3c9a);
    // --------------
    __ bind(&l_3c96);
    __ bl(&l_3c96);
    // --------------
    __ bind(&l_3c9a);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_3c9c);
    __ bl(&l_3c9c);
    // --------------
    __ bind(&l_3ca0);
    __ ldrb(r3, MemOperand(sp, 53));
    __ str(Narrow, r7, MemOperand(sp, 36));
    __ cbnz(r3, &l_3cae);
    // --------------
    __ bind(&l_3ca8);
    __ add(Narrow, sp, sp, 232U);
    __ pop(RegisterList(0x87f0));
    // --------------
    __ bind(&l_3cae);
    __ ldr(Narrow, r0, MemOperand(sp, 56));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_3ca8);
    __ ldr(Narrow, r3, MemOperand(sp, 60));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 53));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_3ca8);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_3ca8);
    // --------------
    __ bind(&l_3cc8);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, r1, sp, 72U);
    __ blx(r2);
    __ b(Narrow, &l_3c8a);
    // --------------
    __ bind(&l_3cd0);
    __ ldr(r3, MemOperand(r4, 132));
    __ add(r8, r4, 96U);
    __ add(r6, r4, 44U);
    __ cmp(Narrow, r5, r3);
    __ b(eq, Narrow, &l_3ce6);
    __ adds(Narrow, r2, r5, 2U);
    __ cmp(Narrow, r3, r2);
    __ b(ge, Narrow, &l_3c7a);
    // --------------
    __ bind(&l_3ce6);
    __ movs(Narrow, r3, 0U);
    __ mov(Narrow, r2, r5);
    __ str(Narrow, r3, MemOperand(sp));
    __ mov(Narrow, r1, r6);
    __ mov(Narrow, r0, r8);
    __ movs(Narrow, r3, 2U);
  }
  void Generate_59(TestAssembler* assembler) {
    // --------------
    __ bind(&l_3cf2);
    __ bl(&l_3cf2);
    __ b(Narrow, &l_3c7a);
    // --------------
    __ bind(&l_3cf8);
    __ blx(r2);
    __ b(Narrow, &l_3ba4);
    __ ldr(Narrow, r3, &l_3d20);
    __ add(Narrow, r0, sp, 100U);
    __ str(Narrow, r3, MemOperand(sp, 72));
    // --------------
    __ bind(&l_3d02);
    __ bl(&l_3d02);
    // --------------
    __ bind(&l_3d06);
    __ add(Narrow, r0, sp, 232U);
    __ str(r9, MemOperand(r0, -196, PreIndex));
    // --------------
    __ bind(&l_3d0c);
    __ bl(&l_3d0c);
    // --------------
    __ bind(&l_3d10);
    __ bl(&l_3d10);
    __ add(Narrow, r0, sp, 64U);
    // --------------
    __ bind(&l_3d16);
    __ bl(&l_3d16);
    __ b(Narrow, &l_3d06);
    // --------------
    __ bind(&l_3d1c);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3d20);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3d24);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3d28);
    __ Emit32(0x00000008);
    __ push(RegisterList(0x47f0));
    __ adds(r9, r0, 44U);
    __ ldr(Narrow, r6, &l_3f14);
    __ sub(Narrow, sp, sp, 208U);
    __ mov(Narrow, r5, r1);
    __ mov(r1, 0U);
    __ mov(Narrow, r4, r0);
    __ mov(Narrow, sl, r2);
    __ mov(Narrow, r7, r3);
    __ mov(Narrow, r8, r5);
    __ str(Narrow, r6, MemOperand(sp, 12));
    __ str(Narrow, r1, MemOperand(sp, 16));
    __ strb(r1, MemOperand(sp, 29));
    __ b(eq, Narrow, &l_3da2);
    __ ldr(Narrow, r3, MemOperand(r0, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r2, MemOperand(sp, 36));
    __ mov(Narrow, r0, r9);
    __ movs(Narrow, r1, 32U);
    __ str(r9, MemOperand(sp, 32));
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r4, 44));
    __ mov(Narrow, r0, r9);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r4, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ mov(Narrow, r0, r9);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_3eec);
    // --------------
    __ bind(&l_3d7e);
    __ bl(&l_3d7e);
    // --------------
    __ bind(&l_3d82);
    __ str(Narrow, r0, MemOperand(sp, 16));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_3d9c);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_3d98);
    __ bl(&l_3d98);
    // --------------
    __ bind(&l_3d9c);
    __ movs(Narrow, r3, 1U);
    __ strb(r3, MemOperand(sp, 29));
    // --------------
    __ bind(&l_3da2);
    __ ldr(r9, &l_3f20);
    __ cmp(Narrow, r5, 14U);
    __ ldr(Narrow, r1, &l_3f18);
    __ mov(r3, 0U);
    __ mov(r2, 1U);
    __ mov(r0, 4U);
    __ str(Narrow, r4, MemOperand(sp, 40));
    __ str(Narrow, r5, MemOperand(sp, 44));
    __ str(r9, MemOperand(sp, 12));
    __ str(Narrow, r0, MemOperand(sp, 52));
    __ str(Narrow, r2, MemOperand(sp, 56));
    __ str(Narrow, r2, MemOperand(sp, 60));
    __ strb(r2, MemOperand(sp, 88));
    __ str(Narrow, r2, MemOperand(sp, 104));
    __ str(Narrow, r2, MemOperand(sp, 128));
    __ str(Narrow, r2, MemOperand(sp, 152));
    __ str(Narrow, r2, MemOperand(sp, 176));
    __ strb(r3, MemOperand(sp, 64));
    __ str(Narrow, r3, MemOperand(sp, 68));
    __ strb(r3, MemOperand(sp, 72));
    __ strb(r3, MemOperand(sp, 76));
    __ str(Narrow, r3, MemOperand(sp, 92));
    __ str(Narrow, r3, MemOperand(sp, 96));
    __ str(Narrow, r3, MemOperand(sp, 100));
    __ str(Narrow, r3, MemOperand(sp, 108));
    __ str(Narrow, r3, MemOperand(sp, 112));
    __ str(Narrow, r3, MemOperand(sp, 116));
    __ str(Narrow, r3, MemOperand(sp, 120));
    __ str(Narrow, r3, MemOperand(sp, 124));
    __ str(Narrow, r3, MemOperand(sp, 132));
    __ str(Narrow, r3, MemOperand(sp, 136));
    __ str(Narrow, r3, MemOperand(sp, 140));
    __ str(Narrow, r3, MemOperand(sp, 144));
  }
  void Generate_60(TestAssembler* assembler) {
    __ str(Narrow, r3, MemOperand(sp, 148));
    __ str(Narrow, r3, MemOperand(sp, 156));
    __ str(Narrow, r3, MemOperand(sp, 160));
    __ str(Narrow, r3, MemOperand(sp, 164));
    __ str(Narrow, r3, MemOperand(sp, 168));
    __ str(Narrow, r3, MemOperand(sp, 172));
    __ str(Narrow, r3, MemOperand(sp, 180));
    __ str(Narrow, r3, MemOperand(sp, 184));
    __ str(Narrow, r3, MemOperand(sp, 188));
    __ str(Narrow, r3, MemOperand(sp, 192));
    __ strb(r3, MemOperand(sp, 200));
    __ str(Narrow, r1, MemOperand(sp, 48));
    __ b(eq, Narrow, &l_3e18);
    __ ldr(Narrow, r2, MemOperand(r4, 28));
    __ cmp(Narrow, r2, 1U);
    __ b(eq, Narrow, &l_3eaa);
    // --------------
    __ bind(&l_3e18);
    __ ldr(Narrow, r3, MemOperand(sp, 240));
    __ mov(Narrow, r0, r4);
    __ str(Narrow, r7, MemOperand(sp));
    __ mov(Narrow, r1, r8);
    __ movs(Narrow, r2, 0U);
    __ str(Narrow, r3, MemOperand(sp, 4));
    __ mov(Narrow, r3, sl);
    // --------------
    __ bind(&l_3e26);
    __ bl(&l_3e26);
    __ ldrb(r3, MemOperand(sp, 72));
    __ cbz(r3, &l_3e6c);
    __ ldr(Narrow, r4, MemOperand(sp, 40));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r4));
    __ ldr(Narrow, r2, MemOperand(r2, 8));
    __ cmp(Narrow, r2, r3);
    __ b(ne, Narrow, &l_3ebc);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ add(Narrow, r0, sp, 48U);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ subs(Narrow, r5, r5, r3);
    __ mov(Narrow, r1, r5);
    // --------------
    __ bind(&l_3e4c);
    __ bl(&l_3e4c);
    __ add(r8, r4, 96U);
    __ add(r7, r4, 44U);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_3ec4);
    // --------------
    __ bind(&l_3e5c);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ mov(Narrow, r0, r8);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ mov(Narrow, r1, r7);
    __ add(Narrow, r2, sp, 48U);
    __ subs(Narrow, r3, r5, r3);
    // --------------
    __ bind(&l_3e68);
    __ bl(&l_3e68);
    // --------------
    __ bind(&l_3e6c);
    __ ldr(Narrow, r4, MemOperand(sp, 192));
    __ ldr(Narrow, r3, &l_3f1c);
    __ str(Narrow, r3, MemOperand(sp, 48));
    __ cbz(r4, &l_3e82);
    __ ldr(Narrow, r0, MemOperand(r4));
    __ cbz(r0, &l_3e7c);
    // --------------
    __ bind(&l_3e78);
    __ bl(&l_3e78);
    // --------------
    __ bind(&l_3e7c);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_3e7e);
    __ bl(&l_3e7e);
    // --------------
    __ bind(&l_3e82);
    __ ldrb(r3, MemOperand(sp, 29));
    __ str(Narrow, r6, MemOperand(sp, 12));
    __ cbnz(r3, &l_3e90);
    // --------------
    __ bind(&l_3e8a);
    __ add(Narrow, sp, sp, 208U);
    __ pop(RegisterList(0x87f0));
    // --------------
    __ bind(&l_3e90);
    __ ldr(Narrow, r0, MemOperand(sp, 32));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_3e8a);
    __ ldr(Narrow, r3, MemOperand(sp, 36));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 29));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_3e8a);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_3e8a);
    // --------------
    __ bind(&l_3eaa);
    __ eor(r1, r5, 0x1);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, r3, sp, 48U);
    // --------------
    __ bind(&l_3eb2);
    __ bl(&l_3eb2);
    __ mov(r8, 14U);
    __ b(Narrow, &l_3e18);
    // --------------
    __ bind(&l_3ebc);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, r1, sp, 48U);
    __ blx(r2);
    __ b(Narrow, &l_3e6c);
    // --------------
    __ bind(&l_3ec4);
    __ ldr(r3, MemOperand(r4, 132));
    __ add(r8, r4, 96U);
    __ add(r7, r4, 44U);
    __ cmp(Narrow, r5, r3);
    __ b(eq, Narrow, &l_3eda);
    __ adds(Narrow, r2, r5, 2U);
    __ cmp(Narrow, r3, r2);
    __ b(ge, Narrow, &l_3e5c);
    // --------------
    __ bind(&l_3eda);
    __ movs(Narrow, r3, 0U);
    __ mov(Narrow, r2, r5);
    __ str(Narrow, r3, MemOperand(sp));
    __ mov(Narrow, r1, r7);
    __ mov(Narrow, r0, r8);
    __ movs(Narrow, r3, 2U);
  }
  void Generate_61(TestAssembler* assembler) {
    // --------------
    __ bind(&l_3ee6);
    __ bl(&l_3ee6);
    __ b(Narrow, &l_3e5c);
    // --------------
    __ bind(&l_3eec);
    __ blx(r2);
    __ b(Narrow, &l_3d82);
    __ add(Narrow, r0, sp, 40U);
    // --------------
    __ bind(&l_3ef2);
    __ bl(&l_3ef2);
    // --------------
    __ bind(&l_3ef6);
    __ add(Narrow, r0, sp, 208U);
    __ str(r9, MemOperand(r0, -196, PreIndex));
    // --------------
    __ bind(&l_3efc);
    __ bl(&l_3efc);
    // --------------
    __ bind(&l_3f00);
    __ bl(&l_3f00);
    // --------------
    __ bind(&l_3f04);
    __ ldr(Narrow, r3, &l_3f1c);
    __ add(Narrow, r0, sp, 76U);
    __ str(Narrow, r3, MemOperand(sp, 48));
    // --------------
    __ bind(&l_3f0a);
    __ bl(&l_3f0a);
    __ b(Narrow, &l_3ef6);
    __ b(Narrow, &l_3f04);
    __ nop(Narrow);
    // --------------
    __ bind(&l_3f14);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3f18);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3f1c);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_3f20);
    __ Emit32(0x00000008);
    __ push(RegisterList(0x4ff0));
    __ adds(r4, r0, 44U);
    __ ldr(r8, &l_41ec);
    __ sub(Narrow, sp, sp, 212U);
    __ mov(Narrow, r7, r1);
    __ mov(r1, 0U);
    __ mov(Narrow, r5, r0);
    __ mov(Narrow, r9, r2);
    __ mov(Narrow, r6, r3);
    __ mov(Narrow, sl, r7);
    __ str(r8, MemOperand(sp, 12));
    __ str(Narrow, r1, MemOperand(sp, 16));
    __ strb(r1, MemOperand(sp, 29));
    __ b(eq, Narrow, &l_3f9c);
    __ ldr(Narrow, r3, MemOperand(r0, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r2, MemOperand(sp, 36));
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 32U);
    __ str(Narrow, r4, MemOperand(sp, 32));
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r5, 44));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r5, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_4160);
    // --------------
    __ bind(&l_3f78);
    __ bl(&l_3f78);
    // --------------
    __ bind(&l_3f7c);
    __ str(Narrow, r0, MemOperand(sp, 16));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_3f96);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_3f92);
    __ bl(&l_3f92);
    // --------------
    __ bind(&l_3f96);
    __ movs(Narrow, r4, 1U);
    __ strb(r4, MemOperand(sp, 29));
    // --------------
    __ bind(&l_3f9c);
    __ ldr(Narrow, r2, MemOperand(r6, 4));
    __ ldr(fp, &l_41f0);
    __ ubfx(r3, r2, 5U, 4U);
    __ str(fp, MemOperand(sp, 12));
    __ cbz(r3, &l_4000);
    __ ldr(Narrow, r1, MemOperand(r6, 8));
    __ ldr(Narrow, r0, MemOperand(r6, 16));
    __ cmp(Narrow, r1, 4U);
    __ b(eq, &l_4118);
    __ tst(r0, 0x1e0);
    __ b(ne, &l_4126);
    __ ldr(Narrow, r3, MemOperand(r6, 12));
    __ and_(r0, r9, 0x1f);
    __ cbnz(r3, &l_3fd0);
    __ and_(lr, r2, 0x1f);
    __ cmp(Narrow, r0, lr);
    __ b(eq, &l_4146);
    // --------------
    __ bind(&l_3fd0);
    __ ldr(Narrow, r3, MemOperand(r6, 12));
    __ cbnz(r3, &l_3fe6);
    __ cmp(Narrow, r0, 15U);
    __ b(eq, &l_4142);
    __ and_(r3, r2, 0x1f);
    __ cmp(Narrow, r3, 15U);
    __ it(ne);
    __ mov(ne, Narrow, r1, 1U);
    __ b(ne, Narrow, &l_400c);
    // --------------
    __ bind(&l_3fe6);
    __ cmp(Narrow, r0, 7U);
    __ b(hi, &l_4142);
    __ and_(r2, r2, 0x1f);
    __ cmp(Narrow, r2, 7U);
    __ b(hi, &l_4142);
  }
  void Generate_62(TestAssembler* assembler) {
    __ cmp(Narrow, r1, 2U);
    __ ite(hi);
    __ mov(hi, Narrow, r1, 0U);
    __ mov(ls, Narrow, r1, 1U);
    __ b(Narrow, &l_400c);
    // --------------
    __ bind(&l_4000);
    __ and_(r2, r9, 0x1f);
    __ cmp(Narrow, r2, 7U);
    __ b(ls, &l_410c);
    // --------------
    __ bind(&l_400a);
    __ mov(Narrow, r1, r3);
    // --------------
    __ bind(&l_400c);
    __ ldr(Narrow, r2, &l_41e4);
    __ cmp(Narrow, r7, 14U);
    __ mov(r4, 0U);
    __ mov(r3, 1U);
    __ mov(r0, 4U);
    __ str(Narrow, r5, MemOperand(sp, 40));
    __ str(Narrow, r7, MemOperand(sp, 44));
    __ str(Narrow, r0, MemOperand(sp, 52));
    __ str(Narrow, r3, MemOperand(sp, 56));
    __ str(Narrow, r3, MemOperand(sp, 60));
    __ strb(r3, MemOperand(sp, 88));
    __ str(Narrow, r3, MemOperand(sp, 104));
    __ str(Narrow, r3, MemOperand(sp, 128));
    __ str(Narrow, r3, MemOperand(sp, 152));
    __ str(Narrow, r3, MemOperand(sp, 176));
    __ strb(r4, MemOperand(sp, 64));
    __ str(Narrow, r4, MemOperand(sp, 68));
    __ strb(r4, MemOperand(sp, 72));
    __ strb(r4, MemOperand(sp, 76));
    __ str(Narrow, r4, MemOperand(sp, 92));
    __ str(Narrow, r4, MemOperand(sp, 96));
    __ str(Narrow, r4, MemOperand(sp, 100));
    __ str(Narrow, r4, MemOperand(sp, 108));
    __ str(Narrow, r4, MemOperand(sp, 112));
    __ str(Narrow, r4, MemOperand(sp, 116));
    __ str(Narrow, r4, MemOperand(sp, 120));
    __ str(Narrow, r4, MemOperand(sp, 124));
    __ str(Narrow, r4, MemOperand(sp, 132));
    __ str(Narrow, r4, MemOperand(sp, 136));
    __ str(Narrow, r4, MemOperand(sp, 140));
    __ str(Narrow, r4, MemOperand(sp, 144));
    __ str(Narrow, r4, MemOperand(sp, 148));
    __ str(Narrow, r4, MemOperand(sp, 156));
    __ str(Narrow, r4, MemOperand(sp, 160));
    __ str(Narrow, r4, MemOperand(sp, 164));
    __ str(Narrow, r4, MemOperand(sp, 168));
    __ str(Narrow, r4, MemOperand(sp, 172));
    __ str(Narrow, r4, MemOperand(sp, 180));
    __ str(Narrow, r4, MemOperand(sp, 184));
    __ str(Narrow, r4, MemOperand(sp, 188));
    __ str(Narrow, r4, MemOperand(sp, 192));
    __ strb(r1, MemOperand(sp, 200));
    __ str(Narrow, r2, MemOperand(sp, 48));
    __ b(eq, Narrow, &l_407a);
    __ ldr(Narrow, r2, MemOperand(r5, 28));
    __ cmp(Narrow, r2, r3);
    __ b(eq, Narrow, &l_4164);
    // --------------
    __ bind(&l_407a);
    __ str(Narrow, r6, MemOperand(sp));
    __ mov(Narrow, r0, r5);
    __ mov(Narrow, r1, sl);
    __ mov(Narrow, r3, r9);
    __ movs(Narrow, r2, 0U);
    // --------------
    __ bind(&l_4084);
    __ bl(&l_4084);
    __ ldrb(r3, MemOperand(sp, 72));
    __ cbz(r3, &l_40cc);
    __ ldr(Narrow, r4, MemOperand(sp, 40));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r4));
    __ ldr(Narrow, r2, MemOperand(r2, 8));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_41ac);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ add(Narrow, r0, sp, 48U);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ subs(Narrow, r5, r5, r3);
    __ mov(Narrow, r1, r5);
    // --------------
    __ bind(&l_40ac);
    __ bl(&l_40ac);
    __ add(r7, r4, 96U);
    __ add(r6, r4, 44U);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_4184);
    // --------------
    __ bind(&l_40bc);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ mov(Narrow, r0, r7);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ mov(Narrow, r1, r6);
    __ add(Narrow, r2, sp, 48U);
    __ subs(Narrow, r3, r5, r3);
    // --------------
    __ bind(&l_40c8);
    __ bl(&l_40c8);
    // --------------
    __ bind(&l_40cc);
    __ ldr(Narrow, r4, MemOperand(sp, 192));
    __ ldr(Narrow, r3, &l_41e8);
    __ str(Narrow, r3, MemOperand(sp, 48));
    __ cbz(r4, &l_40e2);
    __ ldr(Narrow, r0, MemOperand(r4));
    __ cbz(r0, &l_40dc);
    // --------------
    __ bind(&l_40d8);
    __ bl(&l_40d8);
    // --------------
    __ bind(&l_40dc);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_40de);
    __ bl(&l_40de);
    // --------------
    __ bind(&l_40e2);
    __ ldrb(r3, MemOperand(sp, 29));
    __ str(r8, MemOperand(sp, 12));
    __ cbnz(r3, &l_40f2);
  }
  void Generate_63(TestAssembler* assembler) {
    // --------------
    __ bind(&l_40ec);
    __ add(Narrow, sp, sp, 212U);
    __ pop(RegisterList(0x8ff0));
    // --------------
    __ bind(&l_40f2);
    __ ldr(Narrow, r0, MemOperand(sp, 32));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_40ec);
    __ ldr(Narrow, r3, MemOperand(sp, 36));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 29));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_40ec);
    // --------------
    __ bind(&l_4104);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_40ec);
    // --------------
    __ bind(&l_410c);
    __ ldr(Narrow, r3, MemOperand(r6));
    __ cmp(Narrow, r3, 255U);
    __ ite(hi);
    __ mov(hi, Narrow, r1, 0U);
    __ mov(ls, Narrow, r1, 1U);
    __ b(Narrow, &l_400c);
    // --------------
    __ bind(&l_4118);
    __ tst(r0, 0x1e0);
    __ it(eq);
    __ and_(eq, r0, r9, 0x1f);
    __ b(eq, &l_3fe6);
    // --------------
    __ bind(&l_4126);
    __ and_(r3, r2, 0x1f);
    __ and_(r2, r9, 0x1f);
    __ cmp(Narrow, r2, 7U);
    __ it(ls);
    __ cmp(ls, Narrow, r2, r3);
    __ ite(eq);
    __ mov(eq, Narrow, r3, 1U);
    __ mov(ne, Narrow, r3, 0U);
    __ b(ne, &l_400a);
    __ cmp(Narrow, r1, 3U);
    __ b(ls, Narrow, &l_41b4);
    // --------------
    __ bind(&l_4142);
    __ movs(Narrow, r1, 0U);
    __ b(Narrow, &l_400c);
    // --------------
    __ bind(&l_4146);
    __ str(r8, MemOperand(sp, 12));
    __ cmp(Narrow, r4, 0U);
    __ b(eq, Narrow, &l_40ec);
    __ ldr(Narrow, r0, MemOperand(sp, 32));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_40ec);
    __ ldr(Narrow, r2, MemOperand(sp, 36));
    __ strb(r3, MemOperand(sp, 29));
    __ cmp(Narrow, r2, 1U);
    __ b(ne, Narrow, &l_40ec);
    __ b(Narrow, &l_4104);
    // --------------
    __ bind(&l_4160);
    __ blx(r2);
    __ b(Narrow, &l_3f7c);
    // --------------
    __ bind(&l_4164);
    __ cbz(r1, &l_4172);
    __ mov(Narrow, r1, r7);
    __ mov(Narrow, r0, r5);
    __ movs(Narrow, r2, 8U);
    // --------------
    __ bind(&l_416c);
    __ bl(&l_416c);
    __ b(Narrow, &l_407a);
    // --------------
    __ bind(&l_4172);
    __ eor(r1, r7, 0x1);
    __ mov(Narrow, r0, r5);
    __ add(Narrow, r3, sp, 48U);
    // --------------
    __ bind(&l_417a);
    __ bl(&l_417a);
    __ mov(sl, 14U);
    __ b(Narrow, &l_407a);
    // --------------
    __ bind(&l_4184);
    __ ldr(r3, MemOperand(r4, 132));
    __ add(r7, r4, 96U);
    __ add(r6, r4, 44U);
    __ cmp(Narrow, r5, r3);
    __ b(eq, Narrow, &l_419a);
    __ adds(Narrow, r2, r5, 2U);
    __ cmp(Narrow, r3, r2);
    __ b(ge, Narrow, &l_40bc);
    // --------------
    __ bind(&l_419a);
    __ movs(Narrow, r3, 0U);
    __ mov(Narrow, r2, r5);
    __ str(Narrow, r3, MemOperand(sp));
    __ mov(Narrow, r1, r6);
    __ mov(Narrow, r0, r7);
    __ movs(Narrow, r3, 2U);
    // --------------
    __ bind(&l_41a6);
    __ bl(&l_41a6);
    __ b(Narrow, &l_40bc);
    // --------------
    __ bind(&l_41ac);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, r1, sp, 48U);
    __ blx(r2);
    __ b(Narrow, &l_40cc);
    // --------------
    __ bind(&l_41b4);
    __ and_(r3, r0, 0x1f);
    __ cmp(Narrow, r3, 7U);
    __ ite(hi);
    __ mov(hi, Narrow, r1, 0U);
    __ mov(ls, Narrow, r1, 1U);
    __ b(Narrow, &l_400c);
    __ add(Narrow, r0, sp, 40U);
    // --------------
    __ bind(&l_41c4);
    __ bl(&l_41c4);
    // --------------
    __ bind(&l_41c8);
    __ add(Narrow, r0, sp, 208U);
    __ str(fp, MemOperand(r0, -196, PreIndex));
    // --------------
    __ bind(&l_41ce);
    __ bl(&l_41ce);
    // --------------
    __ bind(&l_41d2);
    __ bl(&l_41d2);
    // --------------
    __ bind(&l_41d6);
    __ ldr(Narrow, r3, &l_41e8);
    __ add(Narrow, r0, sp, 76U);
    __ str(Narrow, r3, MemOperand(sp, 48));
    // --------------
    __ bind(&l_41dc);
    __ bl(&l_41dc);
    __ b(Narrow, &l_41c8);
    __ b(Narrow, &l_41d6);
  }
  void Generate_64(TestAssembler* assembler) {
    // --------------
    __ bind(&l_41e4);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_41e8);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_41ec);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_41f0);
    __ Emit32(0x00000008);
    __ push(RegisterList(0x4ff0));
    __ adds(r4, r0, 44U);
    __ ldr(r8, &l_4528);
    __ sub(Narrow, sp, sp, 212U);
    __ mov(Narrow, fp, r1);
    __ mov(r1, 0U);
    __ mov(Narrow, r5, r0);
    __ mov(Narrow, r9, r2);
    __ mov(Narrow, r7, r3);
    __ mov(Narrow, sl, fp);
    __ str(r8, MemOperand(sp, 12));
    __ str(Narrow, r1, MemOperand(sp, 16));
    __ strb(r1, MemOperand(sp, 29));
    __ ldr(Narrow, r6, MemOperand(sp, 248));
    __ b(eq, Narrow, &l_426e);
    __ ldr(Narrow, r3, MemOperand(r0, 44));
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r2, MemOperand(sp, 36));
    __ mov(Narrow, r0, r4);
    __ movs(Narrow, r1, 32U);
    __ str(Narrow, r4, MemOperand(sp, 32));
    __ ldr(Narrow, r3, MemOperand(r3, 32));
    __ blx(r3);
    __ ldr(Narrow, r3, MemOperand(r5, 44));
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r3, MemOperand(r3, 24));
    __ blx(r3);
    __ ldr(Narrow, r2, MemOperand(r5, 44));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ mov(Narrow, r0, r4);
    __ ldr(Narrow, r2, MemOperand(r2));
    __ cmp(Narrow, r2, r3);
    __ b(ne, &l_445e);
    // --------------
    __ bind(&l_424a);
    __ bl(&l_424a);
    // --------------
    __ bind(&l_424e);
    __ str(Narrow, r0, MemOperand(sp, 16));
    __ ldr(Narrow, r1, MemOperand(r0, 20));
    __ ldr(Narrow, r3, MemOperand(r0, 4));
    __ ldr(Narrow, r2, MemOperand(r0, 12));
    __ add(Narrow, r3, r3, r1);
    __ subs(Narrow, r3, r3, r2);
    __ cmp(Narrow, r3, 31U);
    __ b(hi, Narrow, &l_4268);
    __ lsls(Narrow, r1, r1, 1U);
    __ adds(r0, 4U);
    __ adds(Narrow, r1, r1, 32U);
    // --------------
    __ bind(&l_4264);
    __ bl(&l_4264);
    // --------------
    __ bind(&l_4268);
    __ movs(Narrow, r4, 1U);
    __ strb(r4, MemOperand(sp, 29));
    // --------------
    __ bind(&l_426e);
    __ ldr(Narrow, r3, &l_451c);
    __ cmp(fp, 14U);
    __ str(Narrow, r3, MemOperand(sp, 12));
    __ b(eq, Narrow, &l_42a0);
    // --------------
    __ bind(&l_4278);
    __ ldr(Narrow, r3, MemOperand(r6, 4));
    __ tst(r3, 0x1e0);
    __ b(ne, Narrow, &l_42b6);
    __ ldr(Narrow, r1, MemOperand(r6));
    // --------------
    __ bind(&l_4282);
    __ cmp(Narrow, r1, 7U);
    __ b(hi, &l_443e);
    __ and_(r3, r7, 0x1f);
    __ cmp(Narrow, r3, 7U);
    __ b(hi, &l_43fc);
    __ and_(r3, r9, 0x1f);
    __ cmp(Narrow, r3, 7U);
    __ b(hi, &l_4404);
    // --------------
    __ bind(&l_429c);
    __ movs(Narrow, r1, 1U);
    __ b(Narrow, &l_4312);
    // --------------
    __ bind(&l_42a0);
    __ and_(r3, r9, 0x1f);
    __ and_(r2, r7, 0x1f);
    __ cmp(Narrow, r3, r2);
    __ b(ne, Narrow, &l_4278);
    __ ldr(Narrow, r3, MemOperand(r6, 4));
    __ tst(r3, 0x1e0);
    __ b(eq, &l_44d4);
    // --------------
    __ bind(&l_42b6);
    __ ldr(Narrow, r2, MemOperand(r6, 8));
    __ cmp(Narrow, r2, 4U);
    __ b(eq, Narrow, &l_4310);
    __ ldr(Narrow, r2, MemOperand(r6, 16));
    __ tst(r2, 0x1e0);
    __ b(ne, Narrow, &l_42e0);
    __ ldr(Narrow, r2, MemOperand(r6, 12));
    __ cbnz(r2, &l_42e0);
    __ and_(r2, r9, 0x1f);
    __ cmp(Narrow, r2, 7U);
    __ b(hi, Narrow, &l_42e0);
    __ and_(r2, r7, 0x1f);
    __ cmp(Narrow, r2, 7U);
    __ b(hi, Narrow, &l_42e0);
    __ and_(r2, r3, 0x1f);
    __ cmp(Narrow, r2, 7U);
    __ b(ls, Narrow, &l_429c);
    // --------------
    __ bind(&l_42e0);
    __ ldr(Narrow, r2, MemOperand(r6, 16));
    __ tst(r2, 0x1e0);
    __ b(ne, Narrow, &l_4304);
    __ ldr(Narrow, r2, MemOperand(r6, 12));
    __ cbnz(r2, &l_4304);
  }
  void Generate_65(TestAssembler* assembler) {
    __ and_(r1, r9, 0x1f);
    __ cmp(Narrow, r1, 15U);
    __ b(eq, Narrow, &l_4304);
    __ and_(r2, r7, 0x1f);
    __ cmp(Narrow, r1, r2);
    __ b(ne, Narrow, &l_4304);
    __ and_(r2, r3, 0x1d);
    __ cmp(Narrow, r2, 13U);
    __ b(ne, Narrow, &l_429c);
    // --------------
    __ bind(&l_4304);
    __ ldr(Narrow, r1, MemOperand(r6, 16));
    __ ubfx(r2, r1, 5U, 4U);
    __ cmp(Narrow, r2, 0U);
    __ b(eq, &l_444c);
    // --------------
    __ bind(&l_4310);
    __ movs(Narrow, r1, 0U);
    // --------------
    __ bind(&l_4312);
    __ ldr(Narrow, r2, &l_4520);
    __ cmp(fp, 14U);
    __ mov(r4, 0U);
    __ mov(r3, 1U);
    __ mov(r0, 4U);
    __ str(Narrow, r5, MemOperand(sp, 40));
    __ str(fp, MemOperand(sp, 44));
    __ str(Narrow, r0, MemOperand(sp, 52));
    __ str(Narrow, r3, MemOperand(sp, 56));
    __ str(Narrow, r3, MemOperand(sp, 60));
    __ strb(r3, MemOperand(sp, 88));
    __ str(Narrow, r3, MemOperand(sp, 104));
    __ str(Narrow, r3, MemOperand(sp, 128));
    __ str(Narrow, r3, MemOperand(sp, 152));
    __ str(Narrow, r3, MemOperand(sp, 176));
    __ strb(r4, MemOperand(sp, 64));
    __ str(Narrow, r4, MemOperand(sp, 68));
    __ strb(r4, MemOperand(sp, 72));
    __ strb(r4, MemOperand(sp, 76));
    __ str(Narrow, r4, MemOperand(sp, 92));
    __ str(Narrow, r4, MemOperand(sp, 96));
    __ str(Narrow, r4, MemOperand(sp, 100));
    __ str(Narrow, r4, MemOperand(sp, 108));
    __ str(Narrow, r4, MemOperand(sp, 112));
    __ str(Narrow, r4, MemOperand(sp, 116));
    __ str(Narrow, r4, MemOperand(sp, 120));
    __ str(Narrow, r4, MemOperand(sp, 124));
    __ str(Narrow, r4, MemOperand(sp, 132));
    __ str(Narrow, r4, MemOperand(sp, 136));
    __ str(Narrow, r4, MemOperand(sp, 140));
    __ str(Narrow, r4, MemOperand(sp, 144));
    __ str(Narrow, r4, MemOperand(sp, 148));
    __ str(Narrow, r4, MemOperand(sp, 156));
    __ str(Narrow, r4, MemOperand(sp, 160));
    __ str(Narrow, r4, MemOperand(sp, 164));
    __ str(Narrow, r4, MemOperand(sp, 168));
    __ str(Narrow, r4, MemOperand(sp, 172));
    __ str(Narrow, r4, MemOperand(sp, 180));
    __ str(Narrow, r4, MemOperand(sp, 184));
    __ str(Narrow, r4, MemOperand(sp, 188));
    __ str(Narrow, r4, MemOperand(sp, 192));
    __ strb(r1, MemOperand(sp, 200));
    __ str(Narrow, r2, MemOperand(sp, 48));
    __ b(eq, Narrow, &l_4384);
    __ ldr(Narrow, r2, MemOperand(r5, 28));
    __ cmp(Narrow, r2, r3);
    __ b(eq, Narrow, &l_4462);
    // --------------
    __ bind(&l_4384);
    __ str(Narrow, r7, MemOperand(sp));
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r6, MemOperand(sp, 4));
    __ mov(Narrow, r1, sl);
    __ mov(Narrow, r3, r9);
    __ movs(Narrow, r2, 0U);
    // --------------
    __ bind(&l_4390);
    __ bl(&l_4390);
    __ ldrb(r3, MemOperand(sp, 72));
    __ cbz(r3, &l_43d6);
    __ ldr(Narrow, r4, MemOperand(sp, 40));
    __ movw(r3, 0U);
    __ movt(r3, 0U);
    __ ldr(Narrow, r2, MemOperand(r4));
    __ ldr(Narrow, r2, MemOperand(r2, 8));
    __ cmp(Narrow, r2, r3);
    __ b(ne, Narrow, &l_44aa);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ add(Narrow, r0, sp, 48U);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ subs(Narrow, r5, r5, r3);
    __ mov(Narrow, r1, r5);
    // --------------
    __ bind(&l_43b6);
    __ bl(&l_43b6);
    __ add(r7, r4, 96U);
    __ add(r6, r4, 44U);
    __ cmp(Narrow, r0, 0U);
    __ b(ne, Narrow, &l_4482);
    // --------------
    __ bind(&l_43c6);
    __ ldr(Narrow, r5, MemOperand(r4, 12));
    __ mov(Narrow, r0, r7);
    __ ldr(Narrow, r3, MemOperand(r4, 4));
    __ mov(Narrow, r1, r6);
    __ add(Narrow, r2, sp, 48U);
    __ subs(Narrow, r3, r5, r3);
    // --------------
    __ bind(&l_43d2);
    __ bl(&l_43d2);
    // --------------
    __ bind(&l_43d6);
    __ ldr(Narrow, r4, MemOperand(sp, 192));
    __ ldr(Narrow, r3, &l_4524);
    __ str(Narrow, r3, MemOperand(sp, 48));
    __ cbz(r4, &l_43ec);
    __ ldr(Narrow, r0, MemOperand(r4));
    __ cbz(r0, &l_43e6);
  }
  void Generate_66(TestAssembler* assembler) {
    // --------------
    __ bind(&l_43e2);
    __ bl(&l_43e2);
    // --------------
    __ bind(&l_43e6);
    __ mov(Narrow, r0, r4);
    // --------------
    __ bind(&l_43e8);
    __ bl(&l_43e8);
    // --------------
    __ bind(&l_43ec);
    __ ldrb(r3, MemOperand(sp, 29));
    __ str(r8, MemOperand(sp, 12));
    __ cbnz(r3, &l_4424);
    // --------------
    __ bind(&l_43f6);
    __ add(Narrow, sp, sp, 212U);
    __ pop(RegisterList(0x8ff0));
    // --------------
    __ bind(&l_43fc);
    __ and_(r3, r9, 0x1f);
    __ cmp(Narrow, r3, 7U);
    __ b(ls, Narrow, &l_4454);
    // --------------
    __ bind(&l_4404);
    __ ands(r1, r1, 0x3);
    __ b(ne, Narrow, &l_4310);
    __ and_(r3, r9, 0x1f);
    __ cmp(Narrow, r3, 7U);
    __ b(hi, &l_4312);
    __ and_(r1, r7, 0x1f);
    __ sub(r1, r1, 13U);
    __ clz(r1, r1);
    __ lsrs(Narrow, r1, r1, 5U);
    __ b(Narrow, &l_4312);
    // --------------
    __ bind(&l_4424);
    __ ldr(Narrow, r0, MemOperand(sp, 32));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_43f6);
    __ ldr(Narrow, r3, MemOperand(sp, 36));
    __ movs(Narrow, r2, 0U);
    __ strb(r2, MemOperand(sp, 29));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_43f6);
    // --------------
    __ bind(&l_4436);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 28));
    __ blx(r3);
    __ b(Narrow, &l_43f6);
    // --------------
    __ bind(&l_443e);
    __ cmp(Narrow, r1, 255U);
    __ b(ls, Narrow, &l_43fc);
    __ cmp(r1, 1020U);
    __ b(hi, &l_4310);
    __ b(Narrow, &l_4404);
    // --------------
    __ bind(&l_444c);
    __ ldr(Narrow, r1, MemOperand(r6, 12));
    __ cbz(r1, &l_44b2);
    __ mov(Narrow, r1, r2);
    __ b(Narrow, &l_4312);
    // --------------
    __ bind(&l_4454);
    __ and_(r2, r7, 0x1f);
    __ cmp(Narrow, r3, r2);
    __ b(ne, Narrow, &l_4404);
    __ b(Narrow, &l_429c);
    // --------------
    __ bind(&l_445e);
    __ blx(r2);
    __ b(Narrow, &l_424e);
    // --------------
    __ bind(&l_4462);
    __ cbz(r1, &l_4470);
    __ mov(Narrow, r1, fp);
    __ mov(Narrow, r0, r5);
    __ movs(Narrow, r2, 8U);
    // --------------
    __ bind(&l_446a);
    __ bl(&l_446a);
    __ b(Narrow, &l_4384);
    // --------------
    __ bind(&l_4470);
    __ eor(r1, fp, 0x1);
    __ mov(Narrow, r0, r5);
    __ add(Narrow, r3, sp, 48U);
    // --------------
    __ bind(&l_4478);
    __ bl(&l_4478);
    __ mov(sl, 14U);
    __ b(Narrow, &l_4384);
    // --------------
    __ bind(&l_4482);
    __ ldr(r3, MemOperand(r4, 132));
    __ add(r7, r4, 96U);
    __ add(r6, r4, 44U);
    __ cmp(Narrow, r5, r3);
    __ b(eq, Narrow, &l_4498);
    __ adds(Narrow, r2, r5, 2U);
    __ cmp(Narrow, r3, r2);
    __ b(ge, Narrow, &l_43c6);
    // --------------
    __ bind(&l_4498);
    __ movs(Narrow, r3, 0U);
    __ mov(Narrow, r2, r5);
    __ str(Narrow, r3, MemOperand(sp));
    __ mov(Narrow, r1, r6);
    __ mov(Narrow, r0, r7);
    __ movs(Narrow, r3, 2U);
    // --------------
    __ bind(&l_44a4);
    __ bl(&l_44a4);
    __ b(Narrow, &l_43c6);
    // --------------
    __ bind(&l_44aa);
    __ mov(Narrow, r0, r4);
    __ add(Narrow, r1, sp, 48U);
    __ blx(r2);
    __ b(Narrow, &l_43d6);
    // --------------
    __ bind(&l_44b2);
    __ and_(r0, r9, 0x1f);
    __ cmp(Narrow, r0, 15U);
    __ b(eq, &l_4312);
    __ and_(r2, r7, 0x1f);
    __ cmp(Narrow, r2, 13U);
    __ b(ne, &l_4312);
    __ and_(r3, r3, 0x1f);
    __ subs(Narrow, r1, r0, r3);
    __ clz(r1, r1);
    __ lsrs(Narrow, r1, r1, 5U);
    __ b(Narrow, &l_4312);
    // --------------
    __ bind(&l_44d4);
    __ ldr(Narrow, r1, MemOperand(r6));
    __ cmp(Narrow, r1, 0U);
    __ b(ne, &l_4282);
    __ str(r8, MemOperand(sp, 12));
    __ cmp(Narrow, r4, 0U);
    __ b(eq, Narrow, &l_43f6);
    __ ldr(Narrow, r0, MemOperand(sp, 32));
    __ cmp(Narrow, r0, 0U);
    __ b(eq, Narrow, &l_43f6);
  }
  void Generate_67(TestAssembler* assembler) {
    __ ldr(Narrow, r3, MemOperand(sp, 36));
    __ strb(r1, MemOperand(sp, 29));
    __ cmp(Narrow, r3, 1U);
    __ b(ne, Narrow, &l_43f6);
    __ b(Narrow, &l_4436);
    // --------------
    __ bind(&l_44f6);
    __ ldr(Narrow, r3, &l_4524);
    __ add(Narrow, r0, sp, 76U);
    __ str(Narrow, r3, MemOperand(sp, 48));
    // --------------
    __ bind(&l_44fc);
    __ bl(&l_44fc);
    // --------------
    __ bind(&l_4500);
    __ ldr(Narrow, r3, &l_451c);
    __ add(Narrow, r0, sp, 208U);
    __ str(r3, MemOperand(r0, -196, PreIndex));
    // --------------
    __ bind(&l_4508);
    __ bl(&l_4508);
    // --------------
    __ bind(&l_450c);
    __ bl(&l_450c);
    __ add(Narrow, r0, sp, 40U);
    // --------------
    __ bind(&l_4512);
    __ bl(&l_4512);
    __ b(Narrow, &l_4500);
    __ b(Narrow, &l_44f6);
    __ nop(Narrow);
    // --------------
    __ bind(&l_451c);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_4520);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_4524);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_4528);
    __ Emit32(0x00000008);
    __ push(Narrow, RegisterList(r4, r5, r6, lr));
    __ mov(Narrow, r6, r0);
    __ ldr(Narrow, r4, MemOperand(r0, 108));
    __ ldr(Narrow, r5, MemOperand(r0, 112));
    __ ldr(Narrow, r3, &l_4594);
    __ cmp(Narrow, r4, r5);
    __ add(r2, r3, 504U);
    __ str(Narrow, r3, MemOperand(r0));
    __ str(Narrow, r2, MemOperand(r0, 44));
    __ b(eq, Narrow, &l_4554);
    // --------------
    __ bind(&l_4542);
    __ ldr(r0, MemOperand(r4, 4, PostIndex));
    __ cbz(r0, &l_454e);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 4));
    __ blx(r3);
    // --------------
    __ bind(&l_454e);
    __ cmp(Narrow, r5, r4);
    __ b(ne, Narrow, &l_4542);
    __ ldr(Narrow, r5, MemOperand(r6, 108));
    // --------------
    __ bind(&l_4554);
    __ cbz(r5, &l_455c);
    __ mov(Narrow, r0, r5);
    // --------------
    __ bind(&l_4558);
    __ bl(&l_4558);
    // --------------
    __ bind(&l_455c);
    __ ldr(Narrow, r0, MemOperand(r6, 96));
    __ cbz(r0, &l_4564);
    // --------------
    __ bind(&l_4560);
    __ bl(&l_4560);
    // --------------
    __ bind(&l_4564);
    __ ldr(Narrow, r3, &l_4598);
    __ mov(Narrow, r0, r6);
    __ str(r3, MemOperand(r0, 4, PostIndex));
    // --------------
    __ bind(&l_456c);
    __ bl(&l_456c);
    __ mov(Narrow, r0, r6);
    __ pop(Narrow, RegisterList(r4, r5, r6, pc));
    __ ldr(Narrow, r0, MemOperand(r6, 108));
    __ cbz(r0, &l_457c);
    // --------------
    __ bind(&l_4578);
    __ bl(&l_4578);
    // --------------
    __ bind(&l_457c);
    __ ldr(Narrow, r0, MemOperand(r6, 96));
    __ cbz(r0, &l_4584);
    // --------------
    __ bind(&l_4580);
    __ bl(&l_4580);
    // --------------
    __ bind(&l_4584);
    __ ldr(Narrow, r3, &l_4598);
    __ mov(Narrow, r0, r6);
    __ str(r3, MemOperand(r0, 4, PostIndex));
    // --------------
    __ bind(&l_458c);
    __ bl(&l_458c);
    // --------------
    __ bind(&l_4590);
    __ bl(&l_4590);
    // --------------
    __ bind(&l_4594);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_4598);
    __ Emit32(0x00000008);
    __ sub(r0, r0, 44U);
    // --------------
    __ bind(&l_45a0);
    __ b(Wide, &l_45a0);
    __ ldr(Narrow, r3, &l_4724);
    __ mov(r1, 4096U);
    __ push(Narrow, RegisterList(0x40f0));
    __ sub(Narrow, sp, sp, 316U);
    __ add(Narrow, r0, sp, 12U);
    __ ldr(Narrow, r4, &l_4728);
    __ str(Narrow, r3, MemOperand(sp, 8));
    // --------------
    __ bind(&l_45b4);
    __ bl(&l_45b4);
    __ movw(r3, 0U);
    __ movs(Narrow, r0, 14U);
    __ movt(r3, 0U);
    __ str(Narrow, r0, MemOperand(sp, 40));
    __ ldr(Narrow, r1, MemOperand(r3, 112));
    __ add(r5, r4, 504U);
    __ movs(Narrow, r3, 0U);
    __ movs(Narrow, r2, 1U);
    __ str(Narrow, r4, MemOperand(sp, 8));
    __ ubfx(r0, r1, 5U, 4U);
    __ str(Narrow, r5, MemOperand(sp, 52));
    __ strb(r3, MemOperand(sp, 32));
    __ str(Narrow, r3, MemOperand(sp, 36));
    __ strh(r3, MemOperand(sp, 44));
    __ strb(r3, MemOperand(sp, 47));
    __ strb(r3, MemOperand(sp, 48));
    __ strb(r2, MemOperand(sp, 46));
    __ cbz(r0, &l_45f6);
    __ and_(r1, r1, 0x1f);
    __ lsl(r0, r2, r1);
    // --------------
    __ bind(&l_45f6);
    __ ldr(Narrow, r5, &l_472c);
    __ movs(Narrow, r3, 0U);
    __ movs(Narrow, r2, 1U);
    __ movs(Narrow, r4, 4U);
  }
  void Generate_68(TestAssembler* assembler) {
    __ movs(Narrow, r6, 0U);
    __ movs(Narrow, r7, 0U);
    __ mvn(lr, 2147483648U);
    __ add(Narrow, r1, sp, 312U);
    __ str(Narrow, r0, MemOperand(sp, 56));
    __ add(Narrow, r0, sp, 8U);
    __ strd(r6, r7, MemOperand(sp, 64));
    __ str(Narrow, r3, MemOperand(sp, 72));
    __ str(Narrow, r3, MemOperand(sp, 76));
    __ str(Narrow, r3, MemOperand(sp, 104));
    __ str(Narrow, r3, MemOperand(sp, 108));
    __ str(Narrow, r3, MemOperand(sp, 112));
    __ str(Narrow, r3, MemOperand(sp, 116));
    __ str(Narrow, r3, MemOperand(sp, 120));
    __ str(Narrow, r3, MemOperand(sp, 124));
    __ str(Narrow, r3, MemOperand(sp, 144));
    __ str(Narrow, r3, MemOperand(sp, 148));
    __ strb(r3, MemOperand(sp, 152));
    __ str(Narrow, r3, MemOperand(sp, 156));
    __ strb(r3, MemOperand(sp, 176));
    __ str(Narrow, r3, MemOperand(sp, 180));
    __ strb(r3, MemOperand(sp, 184));
    __ strb(r3, MemOperand(sp, 188));
    __ str(Narrow, r3, MemOperand(sp, 204));
    __ str(Narrow, r3, MemOperand(sp, 208));
    __ str(Narrow, r3, MemOperand(sp, 212));
    __ str(Narrow, r3, MemOperand(sp, 220));
    __ str(Narrow, r3, MemOperand(sp, 224));
    __ str(Narrow, r3, MemOperand(sp, 228));
    __ str(Narrow, r4, MemOperand(sp, 128));
    __ str(Narrow, r4, MemOperand(sp, 132));
    __ str(Narrow, r4, MemOperand(sp, 136));
    __ str(Narrow, r4, MemOperand(sp, 164));
    __ str(lr, MemOperand(sp, 140));
    __ str(Narrow, r2, MemOperand(sp, 168));
    __ str(Narrow, r2, MemOperand(sp, 172));
    __ strb(r2, MemOperand(sp, 200));
    __ str(Narrow, r2, MemOperand(sp, 216));
    __ str(Narrow, r3, MemOperand(sp, 232));
    __ str(r5, MemOperand(r1, -152, PreIndex));
    __ str(Narrow, r3, MemOperand(sp, 236));
    __ str(Narrow, r2, MemOperand(sp, 240));
    __ str(Narrow, r3, MemOperand(sp, 244));
    __ str(Narrow, r3, MemOperand(sp, 248));
    __ str(Narrow, r3, MemOperand(sp, 252));
    __ str(Narrow, r3, MemOperand(sp, 256));
    __ str(Narrow, r3, MemOperand(sp, 260));
    __ str(Narrow, r2, MemOperand(sp, 264));
    __ str(Narrow, r3, MemOperand(sp, 268));
    __ str(Narrow, r3, MemOperand(sp, 272));
    __ str(Narrow, r3, MemOperand(sp, 276));
    __ str(Narrow, r3, MemOperand(sp, 280));
    __ str(Narrow, r3, MemOperand(sp, 284));
    __ str(Narrow, r2, MemOperand(sp, 288));
    __ str(Narrow, r3, MemOperand(sp, 292));
    __ str(Narrow, r3, MemOperand(sp, 296));
    __ str(Narrow, r3, MemOperand(sp, 300));
    __ str(Narrow, r3, MemOperand(sp, 304));
    // --------------
    __ bind(&l_4684);
    __ bl(&l_4684);
    __ add(Narrow, r0, sp, 8U);
    // --------------
    __ bind(&l_468a);
    __ bl(&l_468a);
    __ ldr(Narrow, r4, MemOperand(sp, 20));
    __ movs(Narrow, r3, 1U);
    __ ldr(Narrow, r2, MemOperand(sp, 12));
    __ add(Narrow, r0, sp, 104U);
    __ str(Narrow, r3, MemOperand(sp));
    __ add(Narrow, r1, sp, 52U);
    __ movs(Narrow, r3, 0U);
    __ subs(Narrow, r2, r4, r2);
    // --------------
    __ bind(&l_469e);
    __ bl(&l_469e);
    __ ldr(Narrow, r6, MemOperand(sp, 12));
    __ movs(Narrow, r5, 0U);
    __ ldr(Narrow, r4, MemOperand(sp, 20));
    __ mov(r3, 4294967295U);
    __ str(Narrow, r5, MemOperand(sp, 4));
    __ mov(Narrow, r0, r5);
    __ str(Narrow, r3, MemOperand(sp));
    __ movs(Narrow, r2, 7U);
    __ subs(Narrow, r4, r4, r6);
    __ movs(Narrow, r3, 33U);
    __ mov(Narrow, r1, r4);
    __ strb(r5, MemOperand(sp, 24));
    // --------------
    __ bind(&l_46be);
    __ bl(&l_46be);
    __ mov(Narrow, r2, r4);
    __ mov(Narrow, r5, r0);
    __ mov(Narrow, r1, r6);
    // --------------
    __ bind(&l_46c8);
    __ bl(&l_46c8);
    __ mov(Narrow, r0, r5);
    __ adds(Narrow, r1, r5, r4);
    // --------------
    __ bind(&l_46d0);
    __ bl(&l_46d0);
    __ ldr(Narrow, r2, MemOperand(sp, 36));
    __ mov(r0, 1000U);
    __ ldr(Narrow, r3, MemOperand(sp, 180));
    __ cmp(Narrow, r2, 1U);
    __ it(eq);
    __ add(eq, r3, 1U);
    __ add(Narrow, r3, r3, r5);
    __ blx(r3);
    __ mov(Narrow, r1, r4);
    __ mov(Narrow, r0, r5);
  }
  void Generate_69(TestAssembler* assembler) {
    // --------------
    __ bind(&l_46ea);
    __ bl(&l_46ea);
    __ ldr(Narrow, r3, &l_4730);
    __ add(Narrow, r0, sp, 188U);
    __ str(Narrow, r3, MemOperand(sp, 160));
    // --------------
    __ bind(&l_46f4);
    __ bl(&l_46f4);
    __ add(Narrow, r0, sp, 8U);
    // --------------
    __ bind(&l_46fa);
    __ bl(&l_46fa);
    __ movs(Narrow, r0, 0U);
    __ add(Narrow, sp, sp, 316U);
    __ pop(Narrow, RegisterList(0x80f0));
    // --------------
    __ bind(&l_4704);
    __ ldr(Narrow, r3, &l_4730);
    __ add(Narrow, r0, sp, 188U);
    __ str(Narrow, r3, MemOperand(sp, 160));
    // --------------
    __ bind(&l_470a);
    __ bl(&l_470a);
    __ add(Narrow, r0, sp, 8U);
    // --------------
    __ bind(&l_4710);
    __ bl(&l_4710);
    // --------------
    __ bind(&l_4714);
    __ bl(&l_4714);
    __ mov(Narrow, r0, r5);
    __ mov(Narrow, r1, r4);
    // --------------
    __ bind(&l_471c);
    __ bl(&l_471c);
    __ b(Narrow, &l_4704);
    __ nop(Narrow);
    // --------------
    __ bind(&l_4724);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_4728);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_472c);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_4730);
    __ Emit32(0x00000008);
    __ movw(r3, 0U);
    __ push(RegisterList(0x4ff0));
    __ movt(r3, 0U);
    __ mov(r9, 16416U);
    __ mov(sl, 16417U);
    __ mov(r2, 16419U);
    __ str(r9, MemOperand(r3, 92));
    __ mov(r9, 16420U);
    __ str(sl, MemOperand(r3, 88));
    __ mov(sl, 16421U);
    __ str(r9, MemOperand(r3, 24));
    __ mov(r9, 16424U);
    __ str(Narrow, r2, MemOperand(r3, 76));
    __ mov(fp, 16418U);
    __ str(sl, MemOperand(r3, 28));
    __ mov(r2, 16423U);
    __ str(r9, MemOperand(r3, 124));
    __ mov(sl, 16425U);
    __ mov(r9, 16448U);
    __ str(fp, MemOperand(r3, 36));
    __ str(Narrow, r2, MemOperand(r3, 120));
    __ mov(fp, 16422U);
    __ str(sl, MemOperand(r3, 128));
    __ mov(r2, 16427U);
    __ str(r9, MemOperand(r3, 60));
    __ mov(sl, 16449U);
    __ mov(r9, 16452U);
    __ movs(Narrow, r1, 136U);
    __ sub(Narrow, sp, sp, 12U);
    __ str(fp, MemOperand(r3, 32));
    __ str(r2, MemOperand(r3, 136));
    __ movt(r1, 1U);
    __ str(sl, MemOperand(r3, 64));
    __ mov(fp, 16426U);
    __ str(r9, MemOperand(r3, 164));
    __ mov(r2, 16451U);
    __ mov(sl, 16453U);
    __ mov(r9, 16456U);
    __ str(Narrow, r1, MemOperand(sp, 4));
    __ movs(Narrow, r1, 15U);
    __ str(fp, MemOperand(r3, 132));
    __ mov(fp, 16450U);
    __ str(Narrow, r2, MemOperand(r3, 72));
    __ mov(r2, 16455U);
    __ str(sl, MemOperand(r3, 168));
    __ mov(sl, 16457U);
    __ str(r9, MemOperand(r3, 180));
    __ mov(r9, 16429U);
    __ str(fp, MemOperand(r3, 68));
    __ mov(fp, 16454U);
    __ str(r2, MemOperand(r3, 176));
    __ mov(r2, 16428U);
    __ str(sl, MemOperand(r3, 184));
    __ mov(sl, 16431U);
    __ str(Narrow, r1, MemOperand(r3, 116));
    __ mov(r1, 16430U);
    __ str(r9, MemOperand(r3, 140));
    __ mov(r8, 128U);
    __ str(r9, MemOperand(r3, 56));
    __ mov(r9, 16384U);
    __ str(fp, MemOperand(r3, 172));
    __ mov(ip, 129U);
    __ str(Narrow, r2, MemOperand(r3, 112));
    __ mov(lr, 130U);
    __ str(r2, MemOperand(r3, 152));
    __ movs(Narrow, r7, 131U);
    __ str(r1, MemOperand(r3, 144));
    __ movs(Narrow, r6, 132U);
    __ str(Narrow, r1, MemOperand(r3, 108));
    __ movs(Narrow, r1, 0U);
    __ str(sl, MemOperand(r3, 148));
    __ movs(Narrow, r5, 133U);
    __ str(sl, MemOperand(r3, 156));
    __ mov(sl, 16458U);
  }
  void Generate_70(TestAssembler* assembler) {
    __ str(r9, MemOperand(r3, 40));
    __ movs(Narrow, r4, 134U);
    __ str(r9, MemOperand(r3, 272));
    __ mov(r9, 16459U);
    __ str(r9, MemOperand(r3, 192));
    __ mov(r9, 16460U);
    __ str(r9, MemOperand(r3, 196));
    __ mov(r9, 16461U);
    __ str(r9, MemOperand(r3, 200));
    __ mov(r9, 16462U);
    __ str(r9, MemOperand(r3, 204));
    __ mov(r9, 16463U);
    __ str(r9, MemOperand(r3, 52));
    __ mov(r9, 16464U);
    __ str(r9, MemOperand(r3, 208));
    __ mov(r9, 16465U);
    __ str(r9, MemOperand(r3, 212));
    __ mov(r9, 16466U);
    __ str(r9, MemOperand(r3, 216));
    __ mov(r9, 16467U);
    __ str(r9, MemOperand(r3, 220));
    __ mov(r9, 16468U);
    __ str(r9, MemOperand(r3, 224));
    __ mov(r9, 16469U);
    __ str(r9, MemOperand(r3, 228));
    __ mov(r9, 16470U);
    __ str(r9, MemOperand(r3, 232));
    __ mov(r9, 16471U);
    __ str(r9, MemOperand(r3, 236));
    __ mov(r9, 16472U);
    __ str(r9, MemOperand(r3, 240));
    __ mov(r9, 16473U);
    __ str(r9, MemOperand(r3, 244));
    __ mov(r9, 16474U);
    __ str(r9, MemOperand(r3, 248));
    __ mov(r9, 16475U);
    __ str(r9, MemOperand(r3, 252));
    __ mov(r9, 16476U);
    __ str(r9, MemOperand(r3, 256));
    __ mov(r9, 16477U);
    __ str(r9, MemOperand(r3, 260));
    __ mov(r9, 16478U);
    __ str(r9, MemOperand(r3, 264));
    __ mov(r9, 16479U);
    __ str(r9, MemOperand(r3, 268));
    __ mov(r9, 32864U);
    __ str(r9, MemOperand(r3, 276));
    __ mov(r9, 32865U);
    __ str(r9, MemOperand(r3, 280));
    __ mov(r9, 32866U);
    __ str(r9, MemOperand(r3, 284));
    __ mov(r9, 32867U);
    __ str(r9, MemOperand(r3, 288));
    __ mov(r9, 32868U);
    __ str(r9, MemOperand(r3, 84));
    __ mov(r9, 32869U);
    __ str(r9, MemOperand(r3, 292));
    __ mov(r9, 32870U);
    __ str(r9, MemOperand(r3, 44));
    __ mov(r9, 32871U);
    __ str(r9, MemOperand(r3, 48));
    __ mov(r9, 32872U);
    __ str(r9, MemOperand(r3, 296));
    __ mov(r9, 32873U);
    __ str(r1, MemOperand(r3, 160));
    __ movs(Narrow, r0, 135U);
    __ str(sl, MemOperand(r3, 188));
    __ mov(fp, 137U);
    __ str(r9, MemOperand(r3, 300));
    __ mov(r9, 32874U);
    __ str(r9, MemOperand(r3, 304));
    __ mov(r9, 32875U);
    __ str(r9, MemOperand(r3, 308));
    __ mov(r9, 32876U);
    __ str(r9, MemOperand(r3, 312));
    __ mov(r9, 32877U);
    __ str(r9, MemOperand(r3, 316));
    __ mov(r9, 32878U);
    __ str(r9, MemOperand(r3, 320));
    __ mov(r9, 32879U);
    __ str(r9, MemOperand(r3, 324));
    __ mov(r9, 32880U);
    __ str(r9, MemOperand(r3, 328));
    __ mov(r9, 32881U);
    __ str(r9, MemOperand(r3, 332));
    __ mov(r9, 32882U);
    __ str(r9, MemOperand(r3, 336));
    __ mov(r9, 32883U);
    __ str(r9, MemOperand(r3, 340));
    __ mov(r9, 32884U);
    __ str(r9, MemOperand(r3, 344));
    __ mov(r9, 32885U);
    __ str(r9, MemOperand(r3, 348));
    __ mov(r9, 32886U);
    __ str(r9, MemOperand(r3, 352));
    __ mov(r9, 32887U);
    __ str(r9, MemOperand(r3, 356));
    __ mov(r9, 32888U);
    __ str(r9, MemOperand(r3, 360));
    __ mov(r9, 32889U);
  }
  void Generate_71(TestAssembler* assembler) {
    __ str(r9, MemOperand(r3, 364));
    __ mov(r9, 32890U);
    __ str(r9, MemOperand(r3, 368));
    __ mov(r9, 32891U);
    __ str(r9, MemOperand(r3, 372));
    __ mov(r9, 32892U);
    __ str(r9, MemOperand(r3, 376));
    __ mov(r9, 32893U);
    __ ldr(Narrow, r1, MemOperand(sp, 4));
    __ movs(Narrow, r2, 138U);
    __ str(r9, MemOperand(r3, 380));
    __ mov(r9, 32894U);
    __ str(r9, MemOperand(r3, 384));
    __ mov(r9, 32895U);
    __ str(r9, MemOperand(r3, 388));
    __ movt(r8, 1U);
    __ movt(ip, 1U);
    __ movt(lr, 1U);
    __ movt(r7, 1U);
    __ movt(r6, 1U);
    __ movt(r5, 1U);
    __ movt(r4, 1U);
    __ movt(r0, 1U);
    __ movt(fp, 1U);
    __ movt(r2, 1U);
    __ mov(r9, 32768U);
    __ str(r9, MemOperand(r3));
    __ mov(sl, 139U);
    __ str(r8, MemOperand(r3, 4));
    __ mov(r9, 140U);
    __ str(ip, MemOperand(r3, 8));
    __ mov(ip, 141U);
    __ str(lr, MemOperand(r3, 80));
    __ movt(sl, 1U);
    __ str(r7, MemOperand(r3, 392));
    __ movs(Narrow, r7, 142U);
    __ str(r6, MemOperand(r3, 396));
    __ movs(Narrow, r6, 143U);
    __ str(r5, MemOperand(r3, 400));
    __ movt(r9, 1U);
    __ str(r4, MemOperand(r3, 404));
    __ movt(ip, 1U);
    __ str(r0, MemOperand(r3, 408));
    __ movt(r7, 1U);
    __ str(Narrow, r1, MemOperand(r3, 96));
    __ movt(r6, 1U);
    __ str(fp, MemOperand(r3, 100));
    __ mov(r5, 65536U);
    __ str(Narrow, r2, MemOperand(r3, 104));
    __ movs(Narrow, r2, 0U);
    __ movs(Narrow, r4, 1U);
    __ str(r2, MemOperand(r3, 424));
    __ movs(Narrow, r2, 15U);
    __ movs(Narrow, r0, 2U);
    __ movs(Narrow, r1, 3U);
    __ mov(lr, 4U);
    __ str(sl, MemOperand(r3, 412));
    __ mov(r8, 6U);
    __ str(r9, MemOperand(r3, 416));
    __ mov(sl, 8U);
    __ mov(r9, 7U);
    __ str(r2, MemOperand(r3, 484));
    __ mov(fp, 9U);
    __ str(ip, MemOperand(r3, 12));
    __ str(Narrow, r7, MemOperand(r3, 16));
    __ mov(ip, 10U);
    __ movs(Narrow, r7, 11U);
    __ str(Narrow, r6, MemOperand(r3, 20));
    __ str(r5, MemOperand(r3, 420));
    __ movs(Narrow, r6, 12U);
    __ movs(Narrow, r5, 13U);
    __ str(r4, MemOperand(r3, 428));
    __ movs(Narrow, r2, 5U);
    __ movs(Narrow, r4, 14U);
    __ str(r0, MemOperand(r3, 432));
    __ str(r1, MemOperand(r3, 436));
    __ str(lr, MemOperand(r3, 440));
    __ str(r2, MemOperand(r3, 444));
    __ str(r8, MemOperand(r3, 448));
    __ str(r9, MemOperand(r3, 452));
    __ str(sl, MemOperand(r3, 456));
    __ str(fp, MemOperand(r3, 460));
    __ str(ip, MemOperand(r3, 464));
    __ str(r7, MemOperand(r3, 468));
    __ str(r6, MemOperand(r3, 472));
    __ str(r5, MemOperand(r3, 476));
    __ str(r4, MemOperand(r3, 480));
    __ add(Narrow, sp, sp, 12U);
    __ pop(RegisterList(0x8ff0));
    __ nop(Narrow);
    // --------------
    __ bind(&l_4b0c);
    __ push(Narrow, RegisterList(r4, r5, r6, lr));
    __ mov(Narrow, r6, r0);
    __ ldr(Narrow, r4, MemOperand(r0, 108));
    __ ldr(Narrow, r5, MemOperand(r0, 112));
    __ ldr(Narrow, r3, &l_4b7c);
    __ cmp(Narrow, r4, r5);
    __ add(r2, r3, 504U);
    __ str(Narrow, r3, MemOperand(r0));
    __ str(Narrow, r2, MemOperand(r0, 44));
    __ b(eq, Narrow, &l_4b34);
  }
  void Generate_72(TestAssembler* assembler) {
    // --------------
    __ bind(&l_4b22);
    __ ldr(r0, MemOperand(r4, 4, PostIndex));
    __ cbz(r0, &l_4b2e);
    __ ldr(Narrow, r3, MemOperand(r0));
    __ ldr(Narrow, r3, MemOperand(r3, 4));
    __ blx(r3);
    // --------------
    __ bind(&l_4b2e);
    __ cmp(Narrow, r5, r4);
    __ b(ne, Narrow, &l_4b22);
    __ ldr(Narrow, r5, MemOperand(r6, 108));
    // --------------
    __ bind(&l_4b34);
    __ cbz(r5, &l_4b3c);
    __ mov(Narrow, r0, r5);
    // --------------
    __ bind(&l_4b38);
    __ bl(&l_4b38);
    // --------------
    __ bind(&l_4b3c);
    __ ldr(Narrow, r0, MemOperand(r6, 96));
    __ cbz(r0, &l_4b44);
    // --------------
    __ bind(&l_4b40);
    __ bl(&l_4b40);
    // --------------
    __ bind(&l_4b44);
    __ ldr(Narrow, r3, &l_4b80);
    __ mov(Narrow, r0, r6);
    __ str(r3, MemOperand(r0, 4, PostIndex));
    // --------------
    __ bind(&l_4b4c);
    __ bl(&l_4b4c);
    __ mov(Narrow, r0, r6);
    // --------------
    __ bind(&l_4b52);
    __ bl(&l_4b52);
    __ mov(Narrow, r0, r6);
    __ pop(Narrow, RegisterList(r4, r5, r6, pc));
    __ ldr(Narrow, r0, MemOperand(r6, 108));
    __ cbz(r0, &l_4b62);
    // --------------
    __ bind(&l_4b5e);
    __ bl(&l_4b5e);
    // --------------
    __ bind(&l_4b62);
    __ ldr(Narrow, r0, MemOperand(r6, 96));
    __ cbz(r0, &l_4b6a);
    // --------------
    __ bind(&l_4b66);
    __ bl(&l_4b66);
    // --------------
    __ bind(&l_4b6a);
    __ ldr(Narrow, r3, &l_4b80);
    __ mov(Narrow, r0, r6);
    __ str(r3, MemOperand(r0, 4, PostIndex));
    // --------------
    __ bind(&l_4b72);
    __ bl(&l_4b72);
    // --------------
    __ bind(&l_4b76);
    __ bl(&l_4b76);
    __ nop(Narrow);
    // --------------
    __ bind(&l_4b7c);
    __ Emit32(0x00000008);
    // --------------
    __ bind(&l_4b80);
    __ Emit32(0x00000008);
    __ sub(r0, r0, 44U);
    __ b(Narrow, &l_4b0c);
    __ nop(Narrow);
  }

 public:
  explicit Generator(TestAssembler* assembler) {
    Generate_1(assembler);
    Generate_2(assembler);
    Generate_3(assembler);
    Generate_4(assembler);
    Generate_5(assembler);
    Generate_6(assembler);
    Generate_7(assembler);
    Generate_8(assembler);
    Generate_9(assembler);
    Generate_10(assembler);
    Generate_11(assembler);
    Generate_12(assembler);
    Generate_13(assembler);
    Generate_14(assembler);
    Generate_15(assembler);
    Generate_16(assembler);
    Generate_17(assembler);
    Generate_18(assembler);
    Generate_19(assembler);
    Generate_20(assembler);
    Generate_21(assembler);
    Generate_22(assembler);
    Generate_23(assembler);
    Generate_24(assembler);
    Generate_25(assembler);
    Generate_26(assembler);
    Generate_27(assembler);
    Generate_28(assembler);
    Generate_29(assembler);
    Generate_30(assembler);
    Generate_31(assembler);
    Generate_32(assembler);
    Generate_33(assembler);
    Generate_34(assembler);
    Generate_35(assembler);
    Generate_36(assembler);
    Generate_37(assembler);
    Generate_38(assembler);
    Generate_39(assembler);
    Generate_40(assembler);
    Generate_41(assembler);
    Generate_42(assembler);
    Generate_43(assembler);
    Generate_44(assembler);
    Generate_45(assembler);
    Generate_46(assembler);
    Generate_47(assembler);
    Generate_48(assembler);
    Generate_49(assembler);
    Generate_50(assembler);
    Generate_51(assembler);
    Generate_52(assembler);
    Generate_53(assembler);
    Generate_54(assembler);
    Generate_55(assembler);
    Generate_56(assembler);
    Generate_57(assembler);
    Generate_58(assembler);
    Generate_59(assembler);
    Generate_60(assembler);
    Generate_61(assembler);
    Generate_62(assembler);
    Generate_63(assembler);
    Generate_64(assembler);
    Generate_65(assembler);
    Generate_66(assembler);
    Generate_67(assembler);
    Generate_68(assembler);
    Generate_69(assembler);
    Generate_70(assembler);
    Generate_71(assembler);
    Generate_72(assembler);
  }
};

#ifdef __arm__
const int kMaxPerfCpus = 10;
#endif

class PerfScope {
 public:
  explicit PerfScope(int64_t* counter);
  ~PerfScope();

 private:
#ifdef __arm__
  int fd_[kMaxPerfCpus];
#endif
  int64_t* counter_;
};


PerfScope::PerfScope(int64_t* counter) : counter_(counter) {
  *counter_ = -1;
#ifdef __arm__
  perf_event_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.size = sizeof(attr);
  attr.type = PERF_TYPE_HARDWARE;
  attr.config = PERF_COUNT_HW_CPU_CYCLES;
  attr.exclude_kernel = 1;
  bool ok = false;
  for (int cpu = 0; cpu < kMaxPerfCpus; cpu++) {
    fd_[cpu] =
        static_cast<int>(syscall(__NR_perf_event_open, &attr, 0, cpu, -1, 0));
    if (fd_[cpu] >= 0) {
      if (ioctl(fd_[cpu], PERF_EVENT_IOC_ENABLE, 0) == 0) ok = true;
    }
  }
  if (!ok) printf("Can't set perf counter.\n");
#endif
}


PerfScope::~PerfScope() {
  *counter_ = 0;
#ifdef __arm__
  for (int cpu = 0; cpu < 6; cpu++) {
    int64_t tmp;
    if (fd_[cpu] >= 0) {
      size_t size = read(fd_[cpu], &tmp, sizeof(tmp));
      if (size != sizeof(tmp)) {
        printf("Could not read counter for cpu %d.\n", cpu);
      } else if (tmp > *counter_) {
        *counter_ = tmp;
      }
      close(fd_[cpu]);
    }
  }
#endif
}


void Benchmark() {
  TestAssembler assembler;
  {
    int64_t cycles = 0;
    timeval start;
    timeval end;
    {
      PerfScope scope(&cycles);
      gettimeofday(&start, NULL);
      Generator generator(&assembler);
      assembler.FinalizeCode();
      gettimeofday(&end, NULL);
    }

    double delta = (end.tv_sec - start.tv_sec) +
                   static_cast<double>(end.tv_usec - start.tv_usec) / 1000000;
    printf("T32 assembler: time: %gs, cycles: %" PRId64 "\n", delta, cycles);
  }
  {
    PrintDisassembler disassembler(std::cerr);
    int64_t cycles = 0;
    timeval start;
    timeval end;
    {
      PerfScope scope(&cycles);
      gettimeofday(&start, NULL);
      disassembler.DisassembleT32Buffer(assembler.GetBuffer()
                                            ->GetStartAddress<uint16_t*>(),
                                        assembler.GetCursorOffset());
      gettimeofday(&end, NULL);
    }

    double delta = (end.tv_sec - start.tv_sec) +
                   static_cast<double>(end.tv_usec - start.tv_usec) / 1000000;
    printf("T32 disassembler: time: %gs, cycles: %" PRId64 "\n", delta, cycles);
  }
}


int main(int argc, char* argv[]) {
  if (argc > 2) {
    printf("Usage: %s [iteration count]\n", argv[0]);
    exit(1);
  }
#ifdef VIXL_INCLUDE_TARGET_T32
  int count = 1;
  if (argc == 2) count = atoi(argv[1]);
  while (count > 0) {
    Benchmark();
    count--;
  }
#else
  printf("Speed test only available for T32.\n");
  exit(0);  // Even if the test is not done, it must not fail.
#endif

  return 0;
}
