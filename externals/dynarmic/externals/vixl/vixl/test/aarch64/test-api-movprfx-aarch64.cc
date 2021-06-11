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

#include <cstdio>
#include <cstring>
#include <string>

#include "test-runner.h"
#include "test-utils.h"
#include "aarch64/test-utils-aarch64.h"

#include "aarch64/assembler-aarch64.h"
#include "aarch64/instructions-aarch64.h"

#define __ assm.
#define TEST(name) TEST_(AARCH64_API_##name)

namespace vixl {
namespace aarch64 {

static void CheckAndMaybeDisassembleMovprfxPairs(const CodeBuffer* buffer,
                                                 bool can_take_movprfx) {
  const Instruction* pair = buffer->GetStartAddress<Instruction*>();
  const Instruction* end = buffer->GetEndAddress<Instruction*>();
  bool any_failures = false;
  PrintDisassembler print_disasm(stdout);
  while (pair < end) {
    const Instruction* movprfx = pair;
    const Instruction* candidate = pair->GetNextInstruction();
    const Instruction* next_pair = candidate->GetNextInstruction();
    VIXL_ASSERT(candidate < end);

    bool failed = can_take_movprfx != candidate->CanTakeSVEMovprfx(movprfx);
    any_failures = any_failures || failed;

    if (failed || Test::disassemble()) {
      printf("----\n");
      if (failed) {
        printf("# ERROR: Expected %sCanTakeSVEMovprfx(movprfx):\n",
               can_take_movprfx ? "" : "!");
      }
      print_disasm.DisassembleBuffer(pair, next_pair);
    }

    pair = next_pair;
  }
  // Abort only at the end, so we can see the individual failures.
  VIXL_CHECK(!any_failures);
}

TEST(movprfx_negative_aliasing) {
  // Test that CanTakeSVEMovprfx() checks that the movprfx destination does not
  // alias an input to the prefixed instruction.
  Assembler assm;
  assm.GetCPUFeatures()->Combine(CPUFeatures::kSVE);
  {
    // We have to use the Assembler directly to generate movprfx, so we need
    // to manually reserve space for the code we're about to emit.
    static const size_t kPairCount = 73;
    CodeBufferCheckScope guard(&assm, kPairCount * 2 * kInstructionSize);

    __ movprfx(z0.VnB(), p0.Merging(), z9.VnB());
    __ abs(z0.VnB(), p0.Merging(), z0.VnB());

    __ movprfx(z1, z17);
    __ add(z1.VnH(), p2.Merging(), z1.VnH(), z1.VnH());

    __ movprfx(z12, z13);
    __ and_(z12.VnD(), p5.Merging(), z12.VnD(), z12.VnD());

    __ movprfx(z2, z4);
    __ asr(z2.VnS(), p2.Merging(), z2.VnS(), z2.VnS());

    __ movprfx(z10, z18);
    __ asr(z10.VnH(), p2.Merging(), z10.VnH(), z10.VnD());

    __ movprfx(z17.VnD(), p5.Zeroing(), z20.VnD());
    __ asr(z17.VnD(), p5.Merging(), z17.VnD(), z17.VnD());

    __ movprfx(z22, z9);
    __ asrr(z22.VnH(), p1.Merging(), z22.VnH(), z22.VnH());

    __ movprfx(z0.VnS(), p6.Zeroing(), z6.VnS());
    __ bic(z0.VnS(), p6.Merging(), z0.VnS(), z0.VnS());

    __ movprfx(z12, z16);
    __ clasta(z12.VnD(), p5, z12.VnD(), z12.VnD());

    __ movprfx(z7, z15);
    __ clastb(z7.VnS(), p7, z7.VnS(), z7.VnS());

    __ movprfx(z10, z29);
    __ cls(z10.VnH(), p2.Merging(), z10.VnH());

    __ movprfx(z6, z13);
    __ clz(z6.VnB(), p4.Merging(), z6.VnB());

    __ movprfx(z14.VnS(), p6.Zeroing(), z3.VnS());
    __ cnot(z14.VnS(), p6.Merging(), z14.VnS());

    __ movprfx(z5.VnD(), p6.Merging(), z4.VnD());
    __ cnt(z5.VnD(), p6.Merging(), z5.VnD());

    __ movprfx(z19.VnB(), p6.Zeroing(), z4.VnB());
    __ eor(z19.VnB(), p6.Merging(), z19.VnB(), z19.VnB());

    __ movprfx(z27, z2);
    __ ext(z27.VnB(), z27.VnB(), z27.VnB(), 42);

    __ movprfx(z4.VnS(), p1.Zeroing(), z22.VnS());
    __ lsl(z4.VnS(), p1.Merging(), z4.VnS(), z4.VnS());

    __ movprfx(z4, z5);
    __ lsl(z4.VnB(), p5.Merging(), z4.VnB(), z4.VnD());

    __ movprfx(z11.VnD(), p4.Merging(), z29.VnD());
    __ lsl(z11.VnD(), p4.Merging(), z11.VnD(), z11.VnD());

    __ movprfx(z12.VnD(), p6.Merging(), z3.VnD());
    __ lslr(z12.VnD(), p6.Merging(), z12.VnD(), z12.VnD());

    __ movprfx(z7, z2);
    __ lsr(z7.VnB(), p4.Merging(), z7.VnB(), z7.VnB());

    __ movprfx(z25.VnH(), p6.Merging(), z28.VnH());
    __ lsr(z25.VnH(), p6.Merging(), z25.VnH(), z25.VnD());

    __ movprfx(z14.VnD(), p6.Merging(), z6.VnD());
    __ lsr(z14.VnD(), p6.Merging(), z14.VnD(), z14.VnD());

    __ movprfx(z26.VnH(), p6.Zeroing(), z27.VnH());
    __ lsrr(z26.VnH(), p6.Merging(), z26.VnH(), z26.VnH());

    __ movprfx(z17.VnS(), p4.Zeroing(), z29.VnS());
    __ mad(z17.VnS(), p4.Merging(), z17.VnS(), z23.VnS());

    __ movprfx(z7, z17);
    __ mad(z7.VnD(), p5.Merging(), z4.VnD(), z7.VnD());

    __ movprfx(z11, z7);
    __ mla(z11.VnS(), p1.Merging(), z11.VnS(), z27.VnS());

    __ movprfx(z7, z5);
    __ mla(z7.VnH(), p0.Merging(), z5.VnH(), z7.VnH());

    __ movprfx(z1.VnH(), p0.Merging(), z17.VnH());
    __ mls(z1.VnH(), p0.Merging(), z1.VnH(), z31.VnH());

    __ movprfx(z22.VnB(), p3.Merging(), z18.VnB());
    __ mls(z22.VnB(), p3.Merging(), z18.VnB(), z22.VnB());

    __ movprfx(z7.VnS(), p0.Merging(), z10.VnS());
    __ msb(z7.VnS(), p0.Merging(), z7.VnS(), z10.VnS());

    __ movprfx(z12, z6);
    __ msb(z12.VnH(), p7.Merging(), z6.VnH(), z12.VnH());

    __ movprfx(z8.VnB(), p4.Merging(), z3.VnB());
    __ mul(z8.VnB(), p4.Merging(), z8.VnB(), z8.VnB());

    __ movprfx(z9, z26);
    __ neg(z9.VnS(), p7.Merging(), z9.VnS());

    __ movprfx(z16, z8);
    __ not_(z16.VnH(), p6.Merging(), z16.VnH());

    __ movprfx(z25.VnH(), p5.Zeroing(), z11.VnH());
    __ orr(z25.VnH(), p5.Merging(), z25.VnH(), z25.VnH());

    __ movprfx(z17.VnH(), p1.Merging(), z22.VnH());
    __ rbit(z17.VnH(), p1.Merging(), z17.VnH());

    __ movprfx(z11, z25);
    __ revb(z11.VnD(), p6.Merging(), z11.VnD());

    __ movprfx(z13, z27);
    __ revh(z13.VnS(), p2.Merging(), z13.VnS());

    __ movprfx(z30.VnD(), p6.Merging(), z20.VnD());
    __ revw(z30.VnD(), p6.Merging(), z30.VnD());

    __ movprfx(z2.VnD(), p2.Merging(), z21.VnD());
    __ sabd(z2.VnD(), p2.Merging(), z2.VnD(), z2.VnD());

    __ movprfx(z0, z7);
    __ sdiv(z0.VnD(), p0.Merging(), z0.VnD(), z0.VnD());

    __ movprfx(z19, z28);
    __ sdivr(z19.VnS(), p1.Merging(), z19.VnS(), z19.VnS());

    __ movprfx(z5, z18);
    __ sdot(z5.VnS(), z18.VnB(), z5.VnB(), 1);

    __ movprfx(z15, z11);
    __ sdot(z15.VnD(), z2.VnH(), z15.VnH(), 1);

    __ movprfx(z30, z13);
    __ sdot(z30.VnD(), z30.VnH(), z13.VnH(), 1);

    __ movprfx(z8, z9);
    __ sdot(z8.VnS(), z8.VnB(), z9.VnB());

    __ movprfx(z23, z14);
    __ sdot(z23.VnS(), z14.VnB(), z23.VnB());

    __ movprfx(z26, z5);
    __ sdot(z26.VnS(), z26.VnB(), z5.VnB(), 1);

    __ movprfx(z14, z15);
    __ smax(z14.VnB(), p2.Merging(), z14.VnB(), z14.VnB());

    __ movprfx(z26.VnS(), p0.Merging(), z10.VnS());
    __ smin(z26.VnS(), p0.Merging(), z26.VnS(), z26.VnS());

    __ movprfx(z22, z18);
    __ smulh(z22.VnB(), p2.Merging(), z22.VnB(), z22.VnB());

    __ movprfx(z8, z19);
    __ splice(z8.VnD(), p2, z8.VnD(), z8.VnD());

    __ movprfx(z23.VnH(), p6.Zeroing(), z2.VnH());
    __ sub(z23.VnH(), p6.Merging(), z23.VnH(), z23.VnH());

    __ movprfx(z25.VnS(), p2.Merging(), z21.VnS());
    __ subr(z25.VnS(), p2.Merging(), z25.VnS(), z25.VnS());

    __ movprfx(z28, z31);
    __ sxtb(z28.VnS(), p6.Merging(), z28.VnS());

    __ movprfx(z14.VnD(), p6.Merging(), z17.VnD());
    __ sxth(z14.VnD(), p6.Merging(), z14.VnD());

    __ movprfx(z21.VnD(), p0.Zeroing(), z28.VnD());
    __ sxtw(z21.VnD(), p0.Merging(), z21.VnD());

    __ movprfx(z25, z30);
    __ uabd(z25.VnB(), p5.Merging(), z25.VnB(), z25.VnB());

    __ movprfx(z13.VnD(), p2.Merging(), z30.VnD());
    __ udiv(z13.VnD(), p2.Merging(), z13.VnD(), z13.VnD());

    __ movprfx(z19.VnD(), p4.Zeroing(), z6.VnD());
    __ udivr(z19.VnD(), p4.Merging(), z19.VnD(), z19.VnD());

    __ movprfx(z1, z20);
    __ udot(z1.VnS(), z18.VnB(), z1.VnB(), 1);

    __ movprfx(z8, z2);
    __ udot(z8.VnD(), z2.VnH(), z8.VnH(), 1);

    __ movprfx(z28, z10);
    __ udot(z28.VnD(), z28.VnH(), z7.VnH(), 1);

    __ movprfx(z21, z11);
    __ udot(z21.VnD(), z21.VnH(), z11.VnH());

    __ movprfx(z1, z22);
    __ udot(z1.VnD(), z10.VnH(), z1.VnH());

    __ movprfx(z8, z23);
    __ udot(z8.VnS(), z8.VnB(), z0.VnB(), 1);

    __ movprfx(z10.VnB(), p5.Zeroing(), z0.VnB());
    __ umax(z10.VnB(), p5.Merging(), z10.VnB(), z10.VnB());

    __ movprfx(z0.VnS(), p2.Zeroing(), z30.VnS());
    __ umin(z0.VnS(), p2.Merging(), z0.VnS(), z0.VnS());

    __ movprfx(z26.VnD(), p6.Zeroing(), z29.VnD());
    __ umulh(z26.VnD(), p6.Merging(), z26.VnD(), z26.VnD());

    __ movprfx(z23, z25);
    __ uxtb(z23.VnS(), p7.Merging(), z23.VnS());

    __ movprfx(z14.VnS(), p3.Zeroing(), z5.VnS());
    __ uxth(z14.VnS(), p3.Merging(), z14.VnS());

    __ movprfx(z14, z5);
    __ uxtw(z14.VnD(), p3.Merging(), z14.VnD());
  }
  assm.FinalizeCode();

  CheckAndMaybeDisassembleMovprfxPairs(assm.GetBuffer(), false);
}

TEST(movprfx_negative_aliasing_fp) {
  // Test that CanTakeSVEMovprfx() checks that the movprfx destination does not
  // alias an input to the prefixed instruction.
  Assembler assm;
  assm.GetCPUFeatures()->Combine(CPUFeatures::kSVE);
  {
    // We have to use the Assembler directly to generate movprfx, so we need
    // to manually reserve space for the code we're about to emit.
    static const size_t kPairCount = 78;
    CodeBufferCheckScope guard(&assm, kPairCount * 2 * kInstructionSize);

    __ movprfx(z17.VnS(), p1.Zeroing(), z12.VnS());
    __ fabd(z17.VnS(), p1.Merging(), z17.VnS(), z17.VnS());

    __ movprfx(z13, z23);
    __ fabs(z13.VnS(), p4.Merging(), z13.VnS());

    __ movprfx(z24.VnS(), p5.Merging(), z15.VnS());
    __ fadd(z24.VnS(), p5.Merging(), z24.VnS(), z24.VnS());

    __ movprfx(z28.VnD(), p5.Zeroing(), z14.VnD());
    __ fcadd(z28.VnD(), p5.Merging(), z28.VnD(), z28.VnD(), 90);

    __ movprfx(z5, z0);
    __ fcmla(z5.VnH(), z0.VnH(), z5.VnH(), 2, 180);

    __ movprfx(z10, z4);
    __ fcmla(z10.VnS(), z8.VnS(), z10.VnS(), 1, 270);

    __ movprfx(z12, z26);
    __ fcmla(z12.VnH(), z12.VnH(), z3.VnH(), 2, 180);

    __ movprfx(z8, z1);
    __ fcmla(z8.VnS(), z8.VnS(), z1.VnS(), 1, 270);

    __ movprfx(z16.VnD(), p0.Merging(), z13.VnD());
    __ fcvt(z16.VnD(), p0.Merging(), z16.VnH());

    __ movprfx(z12.VnD(), p7.Zeroing(), z13.VnD());
    __ fcvt(z12.VnD(), p7.Merging(), z12.VnS());

    __ movprfx(z14, z26);
    __ fcvt(z14.VnS(), p5.Merging(), z14.VnD());

    __ movprfx(z26, z2);
    __ fcvt(z26.VnH(), p7.Merging(), z26.VnD());

    __ movprfx(z25.VnD(), p2.Merging(), z13.VnD());
    __ fcvtzs(z25.VnD(), p2.Merging(), z25.VnH());

    __ movprfx(z31, z2);
    __ fcvtzs(z31.VnH(), p7.Merging(), z31.VnH());

    __ movprfx(z21.VnD(), p1.Merging(), z7.VnD());
    __ fcvtzs(z21.VnD(), p1.Merging(), z21.VnS());

    __ movprfx(z5, z17);
    __ fcvtzs(z5.VnS(), p5.Merging(), z5.VnD());

    __ movprfx(z19.VnD(), p1.Zeroing(), z16.VnD());
    __ fcvtzu(z19.VnD(), p1.Merging(), z19.VnH());

    __ movprfx(z2.VnH(), p7.Zeroing(), z28.VnH());
    __ fcvtzu(z2.VnH(), p7.Merging(), z2.VnH());

    __ movprfx(z21.VnD(), p7.Zeroing(), z27.VnD());
    __ fcvtzu(z21.VnD(), p7.Merging(), z21.VnS());

    __ movprfx(z22.VnD(), p4.Zeroing(), z8.VnD());
    __ fcvtzu(z22.VnS(), p4.Merging(), z22.VnD());

    __ movprfx(z0.VnS(), p5.Merging(), z5.VnS());
    __ fdiv(z0.VnS(), p5.Merging(), z0.VnS(), z0.VnS());

    __ movprfx(z12, z24);
    __ fdivr(z12.VnD(), p7.Merging(), z12.VnD(), z12.VnD());

    __ movprfx(z14.VnD(), p6.Zeroing(), z21.VnD());
    __ fmad(z14.VnD(), p6.Merging(), z14.VnD(), z3.VnD());

    __ movprfx(z2.VnS(), p5.Zeroing(), z10.VnS());
    __ fmad(z2.VnS(), p5.Merging(), z14.VnS(), z2.VnS());

    __ movprfx(z24, z5);
    __ fmax(z24.VnS(), p1.Merging(), z24.VnS(), z24.VnS());

    __ movprfx(z15.VnD(), p2.Merging(), z26.VnD());
    __ fmaxnm(z15.VnD(), p2.Merging(), z15.VnD(), z15.VnD());

    __ movprfx(z20, z22);
    __ fmin(z20.VnH(), p0.Merging(), z20.VnH(), z20.VnH());

    __ movprfx(z24.VnS(), p6.Zeroing(), z30.VnS());
    __ fminnm(z24.VnS(), p6.Merging(), z24.VnS(), z24.VnS());

    __ movprfx(z4, z24);
    __ fmla(z4.VnH(), z24.VnH(), z4.VnH(), 7);

    __ movprfx(z4, z7);
    __ fmla(z4.VnS(), z24.VnS(), z4.VnS(), 3);

    __ movprfx(z5, z28);
    __ fmla(z5.VnD(), z28.VnD(), z5.VnD(), 1);

    __ movprfx(z24, z2);
    __ fmla(z24.VnD(), z24.VnD(), z2.VnD(), 1);

    __ movprfx(z7, z21);
    __ fmla(z7.VnH(), p2.Merging(), z7.VnH(), z31.VnH());

    __ movprfx(z25.VnH(), p5.Zeroing(), z29.VnH());
    __ fmla(z25.VnH(), p5.Merging(), z29.VnH(), z25.VnH());

    __ movprfx(z31, z25);
    __ fmla(z31.VnH(), z31.VnH(), z2.VnH(), 7);

    __ movprfx(z15, z4);
    __ fmla(z15.VnS(), z15.VnS(), z4.VnS(), 3);

    __ movprfx(z7, z11);
    __ fmls(z7.VnH(), z11.VnH(), z7.VnH(), 4);

    __ movprfx(z3, z10);
    __ fmls(z3.VnS(), z10.VnS(), z3.VnS(), 3);

    __ movprfx(z5, z16);
    __ fmls(z5.VnD(), z16.VnD(), z5.VnD(), 1);

    __ movprfx(z31, z26);
    __ fmls(z31.VnD(), z31.VnD(), z8.VnD(), 1);

    __ movprfx(z5.VnH(), p3.Merging(), z2.VnH());
    __ fmls(z5.VnH(), p3.Merging(), z5.VnH(), z2.VnH());

    __ movprfx(z22.VnS(), p3.Zeroing(), z17.VnS());
    __ fmls(z22.VnS(), p3.Merging(), z21.VnS(), z22.VnS());

    __ movprfx(z17, z2);
    __ fmls(z17.VnH(), z17.VnH(), z2.VnH(), 4);

    __ movprfx(z28, z11);
    __ fmls(z28.VnS(), z28.VnS(), z0.VnS(), 3);

    __ movprfx(z15.VnD(), p1.Merging(), z31.VnD());
    __ fmsb(z15.VnD(), p1.Merging(), z15.VnD(), z31.VnD());

    __ movprfx(z21.VnD(), p0.Zeroing(), z5.VnD());
    __ fmsb(z21.VnD(), p0.Merging(), z19.VnD(), z21.VnD());

    __ movprfx(z0.VnH(), p3.Merging(), z31.VnH());
    __ fmul(z0.VnH(), p3.Merging(), z0.VnH(), z0.VnH());

    __ movprfx(z31.VnH(), p6.Merging(), z8.VnH());
    __ fmulx(z31.VnH(), p6.Merging(), z31.VnH(), z31.VnH());

    __ movprfx(z17.VnH(), p1.Zeroing(), z10.VnH());
    __ fneg(z17.VnH(), p1.Merging(), z17.VnH());

    __ movprfx(z22, z31);
    __ fnmad(z22.VnH(), p1.Merging(), z22.VnH(), z23.VnH());

    __ movprfx(z14.VnD(), p0.Zeroing(), z26.VnD());
    __ fnmad(z14.VnD(), p0.Merging(), z2.VnD(), z14.VnD());

    __ movprfx(z13.VnH(), p6.Zeroing(), z29.VnH());
    __ fnmla(z13.VnH(), p6.Merging(), z13.VnH(), z26.VnH());

    __ movprfx(z19.VnH(), p7.Zeroing(), z25.VnH());
    __ fnmla(z19.VnH(), p7.Merging(), z25.VnH(), z19.VnH());

    __ movprfx(z27.VnH(), p5.Merging(), z24.VnH());
    __ fnmls(z27.VnH(), p5.Merging(), z27.VnH(), z24.VnH());

    __ movprfx(z6.VnH(), p6.Zeroing(), z21.VnH());
    __ fnmls(z6.VnH(), p6.Merging(), z21.VnH(), z6.VnH());

    __ movprfx(z7.VnS(), p3.Merging(), z23.VnS());
    __ fnmsb(z7.VnS(), p3.Merging(), z7.VnS(), z23.VnS());

    __ movprfx(z29.VnH(), p2.Zeroing(), z24.VnH());
    __ fnmsb(z29.VnH(), p2.Merging(), z24.VnH(), z29.VnH());

    __ movprfx(z7.VnH(), p6.Merging(), z23.VnH());
    __ frecpx(z7.VnH(), p6.Merging(), z7.VnH());

    __ movprfx(z17.VnS(), p5.Zeroing(), z2.VnS());
    __ frinta(z17.VnS(), p5.Merging(), z17.VnS());

    __ movprfx(z0.VnS(), p2.Zeroing(), z7.VnS());
    __ frinti(z0.VnS(), p2.Merging(), z0.VnS());

    __ movprfx(z8.VnH(), p3.Merging(), z20.VnH());
    __ frintm(z8.VnH(), p3.Merging(), z8.VnH());

    __ movprfx(z3.VnD(), p2.Zeroing(), z20.VnD());
    __ frintn(z3.VnD(), p2.Merging(), z3.VnD());

    __ movprfx(z11, z3);
    __ frintp(z11.VnS(), p4.Merging(), z11.VnS());

    __ movprfx(z23, z29);
    __ frintx(z23.VnD(), p4.Merging(), z23.VnD());

    __ movprfx(z4.VnH(), p4.Zeroing(), z14.VnH());
    __ frintz(z4.VnH(), p4.Merging(), z4.VnH());

    __ movprfx(z18.VnH(), p3.Zeroing(), z0.VnH());
    __ fscale(z18.VnH(), p3.Merging(), z18.VnH(), z18.VnH());

    __ movprfx(z2.VnS(), p6.Zeroing(), z4.VnS());
    __ fsqrt(z2.VnS(), p6.Merging(), z2.VnS());

    __ movprfx(z14.VnD(), p4.Zeroing(), z31.VnD());
    __ fsub(z14.VnD(), p4.Merging(), z14.VnD(), z14.VnD());

    __ movprfx(z31.VnH(), p2.Merging(), z6.VnH());
    __ fsubr(z31.VnH(), p2.Merging(), z31.VnH(), z31.VnH());

    __ movprfx(z4, z30);
    __ ftmad(z4.VnH(), z4.VnH(), z4.VnH(), 2);

    __ movprfx(z25.VnD(), p6.Zeroing(), z2.VnD());
    __ scvtf(z25.VnD(), p6.Merging(), z25.VnS());

    __ movprfx(z0.VnD(), p3.Merging(), z16.VnD());
    __ scvtf(z0.VnD(), p3.Merging(), z0.VnD());

    __ movprfx(z19, z23);
    __ scvtf(z19.VnS(), p7.Merging(), z19.VnD());

    __ movprfx(z19, z4);
    __ scvtf(z19.VnH(), p4.Merging(), z19.VnD());

    __ movprfx(z13.VnD(), p4.Zeroing(), z6.VnD());
    __ ucvtf(z13.VnD(), p4.Merging(), z13.VnS());

    __ movprfx(z6.VnH(), p0.Zeroing(), z14.VnH());
    __ ucvtf(z6.VnH(), p0.Merging(), z6.VnH());

    __ movprfx(z19.VnS(), p4.Merging(), z12.VnS());
    __ ucvtf(z19.VnH(), p4.Merging(), z19.VnS());

    __ movprfx(z0.VnD(), p5.Zeroing(), z12.VnD());
    __ ucvtf(z0.VnH(), p5.Merging(), z0.VnD());
  }
  assm.FinalizeCode();

  CheckAndMaybeDisassembleMovprfxPairs(assm.GetBuffer(), false);
}

TEST(movprfx_negative_instructions) {
  Assembler assm;
  assm.GetCPUFeatures()->Combine(CPUFeatures::kSVE);
  {
    // We have to use the Assembler directly to generate movprfx, so we need
    // to manually reserve space for the code we're about to emit.
    static const size_t kPairCount = 13;
    CodeBufferCheckScope guard(&assm, kPairCount * 2 * kInstructionSize);

    __ movprfx(z26, z11);
    __ add(z26.VnB(), z11.VnB(), z4.VnB());

    // The merging form can take movprfx, but the zeroing form cannot.
    __ movprfx(z29.VnB(), p3.Zeroing(), z7.VnB());
    __ cpy(z29.VnB(), p3.Zeroing(), -42);

    // Frecpx can take movprfx, but frecpe and frecps cannot.
    __ movprfx(z13, z15);
    __ frecpe(z13.VnD(), z26.VnD());

    __ movprfx(z19, z1);
    __ frecps(z19.VnD(), z1.VnD(), z12.VnD());

    __ movprfx(z6, z12);
    __ frsqrte(z6.VnS(), z12.VnS());

    __ movprfx(z29, z5);
    __ frsqrts(z29.VnH(), z5.VnH(), z20.VnH());

    // Ftmad can take movprfx, but ftsmul and ftssel cannot.
    __ movprfx(z1, z31);
    __ ftsmul(z1.VnD(), z31.VnD(), z16.VnD());

    __ movprfx(z8, z27);
    __ ftssel(z8.VnH(), z27.VnH(), z1.VnH());

    // This looks like a merging unary operation, but it's actually an alias of
    // sel, which isn't destructive.
    __ movprfx(z0, z18);
    __ mov(z0.VnS(), p6.Merging(), z18.VnS());

    // The merging form can take movprfx, but the zeroing form cannot.
    __ movprfx(z12.VnS(), p2.Merging(), z11.VnS());
    __ mov(z12.VnS(), p2.Zeroing(), -42);

    __ movprfx(z13, z6);
    __ movprfx(z13, z2);

    // Movprfx can never prefix itself.
    __ movprfx(z3.VnD(), p5.Zeroing(), z8.VnD());
    __ movprfx(z3.VnD(), p5.Merging(), z8.VnD());

    __ movprfx(z1.VnD(), p3.Zeroing(), z14.VnD());
    __ movprfx(z1.VnD(), p3.Zeroing(), z18.VnD());
  }
  assm.FinalizeCode();

  CheckAndMaybeDisassembleMovprfxPairs(assm.GetBuffer(), false);
}

TEST(movprfx_negative_lane_size) {
  // Test that CanTakeSVEMovprfx() checks that the (predicated) movprfx lane
  // size is compatible with the prefixed instruction.
  Assembler assm;
  assm.GetCPUFeatures()->Combine(CPUFeatures::kSVE);
  {
    // We have to use the Assembler directly to generate movprfx, so we need
    // to manually reserve space for the code we're about to emit.
    static const size_t kPairCount = 63;
    CodeBufferCheckScope guard(&assm, kPairCount * 2 * kInstructionSize);

    __ movprfx(z0.VnH(), p2.Zeroing(), z17.VnH());
    __ abs(z0.VnS(), p2.Merging(), z17.VnS());

    __ movprfx(z10.VnD(), p0.Zeroing(), z4.VnD());
    __ add(z10.VnS(), p0.Merging(), z10.VnS(), z2.VnS());

    __ movprfx(z25.VnS(), p4.Zeroing(), z26.VnS());
    __ and_(z25.VnB(), p4.Merging(), z25.VnB(), z27.VnB());

    __ movprfx(z26.VnD(), p5.Merging(), z23.VnD());
    __ asr(z26.VnB(), p5.Merging(), z26.VnB(), 3);

    __ movprfx(z25.VnS(), p7.Zeroing(), z14.VnS());
    __ asr(z25.VnH(), p7.Merging(), z25.VnH(), z14.VnH());

    __ movprfx(z12.VnS(), p7.Zeroing(), z23.VnS());
    __ asr(z12.VnH(), p7.Merging(), z12.VnH(), z23.VnD());

    __ movprfx(z3.VnH(), p4.Zeroing(), z18.VnH());
    __ asr(z3.VnD(), p4.Merging(), z3.VnD(), z15.VnD());

    __ movprfx(z29.VnH(), p4.Merging(), z31.VnH());
    __ asrd(z29.VnB(), p4.Merging(), z29.VnB(), 3);

    __ movprfx(z31.VnH(), p5.Zeroing(), z14.VnH());
    __ asrr(z31.VnB(), p5.Merging(), z31.VnB(), z5.VnB());

    __ movprfx(z0.VnS(), p6.Zeroing(), z18.VnS());
    __ bic(z0.VnB(), p6.Merging(), z0.VnB(), z23.VnB());

    __ movprfx(z19.VnH(), p2.Zeroing(), z24.VnH());
    __ cls(z19.VnB(), p2.Merging(), z24.VnB());

    __ movprfx(z14.VnS(), p5.Zeroing(), z4.VnS());
    __ clz(z14.VnD(), p5.Merging(), z10.VnD());

    __ movprfx(z0.VnD(), p5.Merging(), z2.VnD());
    __ cnot(z0.VnH(), p5.Merging(), z2.VnH());

    __ movprfx(z0.VnB(), p3.Zeroing(), z19.VnB());
    __ cnt(z0.VnH(), p3.Merging(), z8.VnH());

    __ movprfx(z29.VnS(), p0.Merging(), z7.VnS());
    __ cpy(z29.VnD(), p0.Merging(), -42);

    __ movprfx(z13.VnB(), p2.Merging(), z31.VnB());
    __ cpy(z13.VnS(), p2.Merging(), w13);

    __ movprfx(z0.VnS(), p3.Merging(), z15.VnS());
    __ cpy(z0.VnH(), p3.Merging(), h0);

    __ movprfx(z2.VnD(), p6.Zeroing(), z26.VnD());
    __ eor(z2.VnB(), p6.Merging(), z2.VnB(), z26.VnB());

    __ movprfx(z7.VnS(), p7.Zeroing(), z30.VnS());
    __ lsl(z7.VnD(), p7.Merging(), z7.VnD(), 3);

    __ movprfx(z11.VnH(), p3.Merging(), z23.VnH());
    __ lsl(z11.VnB(), p3.Merging(), z11.VnB(), z21.VnB());

    __ movprfx(z31.VnS(), p7.Zeroing(), z21.VnS());
    __ lsl(z31.VnH(), p7.Merging(), z31.VnH(), z21.VnD());

    __ movprfx(z26.VnH(), p0.Merging(), z0.VnH());
    __ lsl(z26.VnD(), p0.Merging(), z26.VnD(), z24.VnD());

    __ movprfx(z1.VnS(), p2.Zeroing(), z6.VnS());
    __ lslr(z1.VnB(), p2.Merging(), z1.VnB(), z6.VnB());

    __ movprfx(z4.VnD(), p4.Zeroing(), z6.VnD());
    __ lsr(z4.VnH(), p4.Merging(), z4.VnH(), 3);

    __ movprfx(z27.VnH(), p0.Zeroing(), z29.VnH());
    __ lsr(z27.VnS(), p0.Merging(), z27.VnS(), z29.VnS());

    __ movprfx(z5.VnD(), p2.Zeroing(), z16.VnD());
    __ lsr(z5.VnH(), p2.Merging(), z5.VnH(), z2.VnD());

    __ movprfx(z27.VnB(), p4.Zeroing(), z5.VnB());
    __ lsr(z27.VnD(), p4.Merging(), z27.VnD(), z5.VnD());

    __ movprfx(z27.VnS(), p3.Merging(), z13.VnS());
    __ lsrr(z27.VnD(), p3.Merging(), z27.VnD(), z13.VnD());

    __ movprfx(z30.VnS(), p2.Zeroing(), z14.VnS());
    __ mad(z30.VnB(), p2.Merging(), z20.VnB(), z14.VnB());

    __ movprfx(z14.VnB(), p6.Merging(), z11.VnB());
    __ mla(z14.VnD(), p6.Merging(), z28.VnD(), z11.VnD());

    __ movprfx(z28.VnH(), p2.Zeroing(), z22.VnH());
    __ mls(z28.VnS(), p2.Merging(), z3.VnS(), z22.VnS());

    // Aliases of cpy.
    __ movprfx(z18.VnH(), p6.Zeroing(), z25.VnH());
    __ mov(z18.VnD(), p6.Merging(), -42);

    __ movprfx(z22.VnD(), p2.Zeroing(), z6.VnD());
    __ mov(z22.VnS(), p2.Merging(), w22);

    __ movprfx(z3.VnH(), p0.Zeroing(), z13.VnH());
    __ mov(z3.VnB(), p0.Merging(), b0);

    __ movprfx(z31.VnS(), p7.Zeroing(), z12.VnS());
    __ msb(z31.VnH(), p7.Merging(), z14.VnH(), z12.VnH());

    __ movprfx(z16.VnS(), p7.Zeroing(), z6.VnS());
    __ mul(z16.VnB(), p7.Merging(), z16.VnB(), z30.VnB());

    __ movprfx(z17.VnD(), p7.Merging(), z1.VnD());
    __ neg(z17.VnB(), p7.Merging(), z1.VnB());

    __ movprfx(z31.VnH(), p4.Zeroing(), z12.VnH());
    __ not_(z31.VnB(), p4.Merging(), z12.VnB());

    __ movprfx(z9.VnH(), p3.Zeroing(), z23.VnH());
    __ orr(z9.VnS(), p3.Merging(), z9.VnS(), z13.VnS());

    __ movprfx(z25.VnD(), p2.Zeroing(), z21.VnD());
    __ rbit(z25.VnS(), p2.Merging(), z21.VnS());

    __ movprfx(z26.VnH(), p3.Merging(), z13.VnH());
    __ revb(z26.VnD(), p3.Merging(), z13.VnD());

    __ movprfx(z8.VnH(), p5.Merging(), z20.VnH());
    __ revh(z8.VnS(), p5.Merging(), z0.VnS());

    __ movprfx(z22.VnH(), p6.Merging(), z15.VnH());
    __ revw(z22.VnD(), p6.Merging(), z10.VnD());

    __ movprfx(z1.VnD(), p3.Merging(), z15.VnD());
    __ sabd(z1.VnB(), p3.Merging(), z1.VnB(), z15.VnB());

    __ movprfx(z25.VnD(), p1.Zeroing(), z30.VnD());
    __ sdiv(z25.VnS(), p1.Merging(), z25.VnS(), z30.VnS());

    __ movprfx(z19.VnS(), p3.Zeroing(), z11.VnS());
    __ sdivr(z19.VnD(), p3.Merging(), z19.VnD(), z24.VnD());

    __ movprfx(z12.VnH(), p2.Merging(), z2.VnH());
    __ smax(z12.VnS(), p2.Merging(), z12.VnS(), z24.VnS());

    __ movprfx(z3.VnD(), p1.Merging(), z15.VnD());
    __ smin(z3.VnS(), p1.Merging(), z3.VnS(), z20.VnS());

    __ movprfx(z13.VnS(), p5.Merging(), z22.VnS());
    __ smulh(z13.VnB(), p5.Merging(), z13.VnB(), z27.VnB());

    __ movprfx(z11.VnH(), p5.Zeroing(), z25.VnH());
    __ sub(z11.VnB(), p5.Merging(), z11.VnB(), z7.VnB());

    __ movprfx(z3.VnB(), p6.Merging(), z13.VnB());
    __ subr(z3.VnS(), p6.Merging(), z3.VnS(), z13.VnS());

    __ movprfx(z26.VnH(), p5.Merging(), z1.VnH());
    __ sxtb(z26.VnS(), p5.Merging(), z17.VnS());

    __ movprfx(z11.VnB(), p7.Zeroing(), z26.VnB());
    __ sxth(z11.VnS(), p7.Merging(), z26.VnS());

    __ movprfx(z1.VnS(), p2.Merging(), z21.VnS());
    __ sxtw(z1.VnD(), p2.Merging(), z21.VnD());

    __ movprfx(z4.VnS(), p6.Zeroing(), z6.VnS());
    __ uabd(z4.VnH(), p6.Merging(), z4.VnH(), z6.VnH());

    __ movprfx(z26.VnB(), p2.Zeroing(), z11.VnB());
    __ udiv(z26.VnD(), p2.Merging(), z26.VnD(), z11.VnD());

    __ movprfx(z19.VnB(), p5.Merging(), z6.VnB());
    __ udivr(z19.VnS(), p5.Merging(), z19.VnS(), z9.VnS());

    __ movprfx(z16.VnB(), p4.Merging(), z6.VnB());
    __ umax(z16.VnH(), p4.Merging(), z16.VnH(), z6.VnH());

    __ movprfx(z1.VnD(), p0.Zeroing(), z4.VnD());
    __ umin(z1.VnS(), p0.Merging(), z1.VnS(), z28.VnS());

    __ movprfx(z25.VnD(), p7.Merging(), z4.VnD());
    __ umulh(z25.VnB(), p7.Merging(), z25.VnB(), z16.VnB());

    __ movprfx(z29.VnB(), p4.Merging(), z2.VnB());
    __ uxtb(z29.VnS(), p4.Merging(), z31.VnS());

    __ movprfx(z27.VnH(), p5.Merging(), z21.VnH());
    __ uxth(z27.VnD(), p5.Merging(), z1.VnD());

    __ movprfx(z29.VnB(), p2.Merging(), z7.VnB());
    __ uxtw(z29.VnD(), p2.Merging(), z7.VnD());
  }
  assm.FinalizeCode();

  CheckAndMaybeDisassembleMovprfxPairs(assm.GetBuffer(), false);
}

TEST(movprfx_negative_lane_size_fp) {
  // Test that CanTakeSVEMovprfx() checks that the (predicated) movprfx lane
  // size is compatible with the prefixed instruction.
  Assembler assm;
  assm.GetCPUFeatures()->Combine(CPUFeatures::kSVE);
  {
    // We have to use the Assembler directly to generate movprfx, so we need
    // to manually reserve space for the code we're about to emit.
    static const size_t kPairCount = 64;
    CodeBufferCheckScope guard(&assm, kPairCount * 2 * kInstructionSize);

    __ movprfx(z29.VnD(), p5.Zeroing(), z8.VnD());
    __ fabd(z29.VnS(), p5.Merging(), z29.VnS(), z26.VnS());

    __ movprfx(z9.VnB(), p0.Zeroing(), z1.VnB());
    __ fabs(z9.VnS(), p0.Merging(), z15.VnS());

    __ movprfx(z24.VnD(), p0.Zeroing(), z8.VnD());
    __ fadd(z24.VnH(), p0.Merging(), z24.VnH(), 0.5);

    __ movprfx(z24.VnB(), p1.Zeroing(), z27.VnB());
    __ fadd(z24.VnH(), p1.Merging(), z24.VnH(), z27.VnH());

    __ movprfx(z14.VnH(), p7.Merging(), z12.VnH());
    __ fcadd(z14.VnD(), p7.Merging(), z14.VnD(), z12.VnD(), 90);

    __ movprfx(z10.VnB(), p6.Merging(), z11.VnB());
    __ fcpy(z10.VnH(), p6.Merging(), 1.25);

    __ movprfx(z12.VnB(), p6.Merging(), z18.VnB());
    __ fcvt(z12.VnD(), p6.Merging(), z18.VnH());

    __ movprfx(z18.VnH(), p7.Zeroing(), z2.VnH());
    __ fcvt(z18.VnD(), p7.Merging(), z0.VnS());

    __ movprfx(z3.VnH(), p5.Merging(), z14.VnH());
    __ fcvt(z3.VnS(), p5.Merging(), z21.VnD());

    __ movprfx(z15.VnH(), p1.Zeroing(), z12.VnH());
    __ fcvt(z15.VnH(), p1.Merging(), z12.VnD());

    __ movprfx(z3.VnH(), p2.Merging(), z22.VnH());
    __ fcvtzs(z3.VnD(), p2.Merging(), z7.VnH());

    __ movprfx(z17.VnS(), p3.Merging(), z14.VnS());
    __ fcvtzs(z17.VnD(), p3.Merging(), z14.VnD());

    __ movprfx(z2.VnH(), p1.Zeroing(), z16.VnH());
    __ fcvtzs(z2.VnS(), p1.Merging(), z31.VnH());

    __ movprfx(z13.VnB(), p2.Merging(), z9.VnB());
    __ fcvtzs(z13.VnS(), p2.Merging(), z23.VnD());

    __ movprfx(z19.VnB(), p1.Merging(), z4.VnB());
    __ fcvtzu(z19.VnD(), p1.Merging(), z14.VnH());

    __ movprfx(z29.VnS(), p2.Merging(), z19.VnS());
    __ fcvtzu(z29.VnD(), p2.Merging(), z19.VnD());

    __ movprfx(z21.VnS(), p4.Zeroing(), z17.VnS());
    __ fcvtzu(z21.VnD(), p4.Merging(), z17.VnS());

    __ movprfx(z19.VnH(), p4.Zeroing(), z30.VnH());
    __ fcvtzu(z19.VnS(), p4.Merging(), z16.VnD());

    __ movprfx(z10.VnS(), p7.Zeroing(), z27.VnS());
    __ fdiv(z10.VnH(), p7.Merging(), z10.VnH(), z27.VnH());

    __ movprfx(z7.VnD(), p7.Zeroing(), z17.VnD());
    __ fdivr(z7.VnH(), p7.Merging(), z7.VnH(), z28.VnH());

    __ movprfx(z22.VnB(), p0.Merging(), z27.VnB());
    __ fmad(z22.VnH(), p0.Merging(), z27.VnH(), z15.VnH());

    __ movprfx(z14.VnD(), p1.Zeroing(), z11.VnD());
    __ fmax(z14.VnS(), p1.Merging(), z14.VnS(), 0.0);

    __ movprfx(z27.VnB(), p5.Merging(), z14.VnB());
    __ fmax(z27.VnD(), p5.Merging(), z27.VnD(), z14.VnD());

    __ movprfx(z31.VnH(), p7.Merging(), z24.VnH());
    __ fmaxnm(z31.VnD(), p7.Merging(), z31.VnD(), 0.0);

    __ movprfx(z11.VnD(), p7.Zeroing(), z25.VnD());
    __ fmaxnm(z11.VnS(), p7.Merging(), z11.VnS(), z28.VnS());

    __ movprfx(z31.VnD(), p6.Merging(), z19.VnD());
    __ fmin(z31.VnH(), p6.Merging(), z31.VnH(), 0.0);

    __ movprfx(z20.VnS(), p3.Zeroing(), z15.VnS());
    __ fmin(z20.VnH(), p3.Merging(), z20.VnH(), z8.VnH());

    __ movprfx(z6.VnS(), p0.Merging(), z30.VnS());
    __ fminnm(z6.VnH(), p0.Merging(), z6.VnH(), 0.0);

    __ movprfx(z1.VnH(), p1.Zeroing(), z14.VnH());
    __ fminnm(z1.VnS(), p1.Merging(), z1.VnS(), z14.VnS());

    __ movprfx(z13.VnB(), p3.Zeroing(), z21.VnB());
    __ fmla(z13.VnD(), p3.Merging(), z12.VnD(), z21.VnD());

    __ movprfx(z15.VnS(), p1.Zeroing(), z20.VnS());
    __ fmls(z15.VnH(), p1.Merging(), z28.VnH(), z20.VnH());

    __ movprfx(z19.VnD(), p3.Zeroing(), z31.VnD());
    __ fmov(z19.VnH(), p3.Merging(), 0.0);

    __ movprfx(z16.VnS(), p7.Merging(), z30.VnS());
    __ fmov(z16.VnH(), p7.Merging(), 2.5);

    __ movprfx(z21.VnB(), p1.Merging(), z28.VnB());
    __ fmsb(z21.VnH(), p1.Merging(), z30.VnH(), z28.VnH());

    __ movprfx(z21.VnS(), p1.Zeroing(), z19.VnS());
    __ fmul(z21.VnH(), p1.Merging(), z21.VnH(), 2.0);

    __ movprfx(z28.VnB(), p7.Zeroing(), z8.VnB());
    __ fmul(z28.VnS(), p7.Merging(), z28.VnS(), z26.VnS());

    __ movprfx(z2.VnB(), p4.Merging(), z31.VnB());
    __ fmulx(z2.VnH(), p4.Merging(), z2.VnH(), z31.VnH());

    __ movprfx(z6.VnB(), p2.Zeroing(), z0.VnB());
    __ fneg(z6.VnS(), p2.Merging(), z28.VnS());

    __ movprfx(z26.VnB(), p0.Zeroing(), z21.VnB());
    __ fnmad(z26.VnH(), p0.Merging(), z21.VnH(), z18.VnH());

    __ movprfx(z15.VnB(), p1.Zeroing(), z26.VnB());
    __ fnmla(z15.VnH(), p1.Merging(), z26.VnH(), z18.VnH());

    __ movprfx(z16.VnS(), p0.Merging(), z1.VnS());
    __ fnmls(z16.VnD(), p0.Merging(), z1.VnD(), z13.VnD());

    __ movprfx(z4.VnH(), p0.Zeroing(), z16.VnH());
    __ fnmsb(z4.VnS(), p0.Merging(), z30.VnS(), z3.VnS());

    // Note that frecpe and frecps _cannot_ take movprfx.
    __ movprfx(z9.VnH(), p0.Zeroing(), z21.VnH());
    __ frecpx(z9.VnS(), p0.Merging(), z14.VnS());

    __ movprfx(z6.VnH(), p2.Zeroing(), z28.VnH());
    __ frinta(z6.VnD(), p2.Merging(), z28.VnD());

    __ movprfx(z12.VnS(), p4.Zeroing(), z7.VnS());
    __ frinti(z12.VnH(), p4.Merging(), z7.VnH());

    __ movprfx(z6.VnB(), p5.Merging(), z20.VnB());
    __ frintm(z6.VnD(), p5.Merging(), z20.VnD());

    __ movprfx(z7.VnB(), p6.Merging(), z19.VnB());
    __ frintn(z7.VnH(), p6.Merging(), z11.VnH());

    __ movprfx(z12.VnD(), p2.Merging(), z31.VnD());
    __ frintp(z12.VnS(), p2.Merging(), z31.VnS());

    __ movprfx(z1.VnS(), p5.Merging(), z10.VnS());
    __ frintx(z1.VnD(), p5.Merging(), z0.VnD());

    __ movprfx(z6.VnH(), p0.Merging(), z12.VnH());
    __ frintz(z6.VnS(), p0.Merging(), z7.VnS());

    __ movprfx(z8.VnH(), p2.Merging(), z6.VnH());
    __ fscale(z8.VnD(), p2.Merging(), z8.VnD(), z6.VnD());

    __ movprfx(z20.VnH(), p2.Zeroing(), z2.VnH());
    __ fsqrt(z20.VnD(), p2.Merging(), z15.VnD());

    __ movprfx(z28.VnS(), p6.Zeroing(), z19.VnS());
    __ fsub(z28.VnD(), p6.Merging(), z28.VnD(), 1.0);

    __ movprfx(z6.VnB(), p0.Zeroing(), z12.VnB());
    __ fsub(z6.VnD(), p0.Merging(), z6.VnD(), z20.VnD());

    __ movprfx(z6.VnS(), p7.Zeroing(), z11.VnS());
    __ fsubr(z6.VnH(), p7.Merging(), z6.VnH(), 1.0);

    __ movprfx(z28.VnB(), p3.Merging(), z10.VnB());
    __ fsubr(z28.VnS(), p3.Merging(), z28.VnS(), z9.VnS());

    __ movprfx(z22.VnB(), p3.Zeroing(), z14.VnB());
    __ scvtf(z22.VnD(), p3.Merging(), z24.VnS());

    __ movprfx(z20.VnS(), p2.Merging(), z9.VnS());
    __ scvtf(z20.VnH(), p2.Merging(), z9.VnH());

    __ movprfx(z19.VnH(), p1.Merging(), z21.VnH());
    __ scvtf(z19.VnS(), p1.Merging(), z6.VnD());

    __ movprfx(z31.VnS(), p3.Merging(), z22.VnS());
    __ scvtf(z31.VnH(), p3.Merging(), z22.VnD());

    __ movprfx(z8.VnS(), p3.Merging(), z3.VnS());
    __ ucvtf(z8.VnD(), p3.Merging(), z1.VnS());

    __ movprfx(z0.VnB(), p0.Merging(), z23.VnB());
    __ ucvtf(z0.VnH(), p0.Merging(), z12.VnH());

    __ movprfx(z8.VnH(), p3.Zeroing(), z4.VnH());
    __ ucvtf(z8.VnH(), p3.Merging(), z4.VnS());

    __ movprfx(z20.VnH(), p2.Zeroing(), z10.VnH());
    __ ucvtf(z20.VnH(), p2.Merging(), z11.VnD());
  }
  assm.FinalizeCode();

  CheckAndMaybeDisassembleMovprfxPairs(assm.GetBuffer(), false);
}

TEST(movprfx_negative_predication) {
  // Test that CanTakeSVEMovprfx() is false when a predicated movprfx appears
  // before an unpredicated instruction.
  Assembler assm;
  assm.GetCPUFeatures()->Combine(CPUFeatures::kSVE);
  {
    // We have to use the Assembler directly to generate movprfx, so we need
    // to manually reserve space for the code we're about to emit.
    static const size_t kPairCount = 54;
    CodeBufferCheckScope guard(&assm, kPairCount * 2 * kInstructionSize);

    __ movprfx(z27.VnS(), p1.Zeroing(), z12.VnS());
    __ add(z27.VnS(), z27.VnS(), 42);

    __ movprfx(z31.VnS(), p6.Zeroing(), z1.VnS());
    __ and_(z31.VnS(), z31.VnS(), 4);

    __ movprfx(z27.VnS(), p5.Merging(), z24.VnS());
    __ bic(z27.VnS(), z27.VnS(), 4);

    __ movprfx(z6.VnH(), p7.Merging(), z30.VnH());
    __ clasta(z6.VnH(), p7, z6.VnH(), z14.VnH());

    __ movprfx(z11.VnB(), p6.Merging(), z5.VnB());
    __ clastb(z11.VnB(), p6, z11.VnB(), z29.VnB());

    __ movprfx(z5.VnD(), p0.Merging(), z1.VnD());
    __ decd(z5.VnD(), SVE_MUL3);

    __ movprfx(z11.VnH(), p7.Zeroing(), z28.VnH());
    __ dech(z11.VnH(), SVE_VL2);

    __ movprfx(z14.VnS(), p5.Zeroing(), z6.VnS());
    __ decp(z14.VnS(), p5);

    __ movprfx(z6.VnS(), p5.Merging(), z10.VnS());
    __ decw(z6.VnS(), SVE_ALL);

    __ movprfx(z27.VnH(), p7.Zeroing(), z9.VnH());
    __ eon(z27.VnH(), z27.VnH(), 4);

    __ movprfx(z3.VnS(), p3.Zeroing(), z2.VnS());
    __ eor(z3.VnS(), z3.VnS(), 4);

    __ movprfx(z30.VnB(), p2.Zeroing(), z25.VnB());
    __ ext(z30.VnB(), z30.VnB(), z25.VnB(), 42);

    __ movprfx(z22.VnD(), p0.Merging(), z0.VnD());
    __ incd(z22.VnD(), SVE_MUL3);

    __ movprfx(z7.VnH(), p3.Merging(), z3.VnH());
    __ inch(z7.VnH(), SVE_VL2);

    __ movprfx(z9.VnD(), p1.Zeroing(), z28.VnD());
    __ incp(z9.VnD(), p1);

    __ movprfx(z30.VnS(), p3.Merging(), z4.VnS());
    __ incw(z30.VnS(), SVE_ALL);

    __ movprfx(z30.VnB(), p7.Zeroing(), z21.VnB());
    __ insr(z30.VnB(), w30);

    __ movprfx(z2.VnB(), p4.Zeroing(), z26.VnB());
    __ insr(z2.VnB(), b0);

    __ movprfx(z27.VnS(), p5.Zeroing(), z5.VnS());
    __ mul(z27.VnS(), z27.VnS(), 42);

    __ movprfx(z5.VnS(), p0.Merging(), z26.VnS());
    __ orn(z5.VnS(), z5.VnS(), 4);

    __ movprfx(z5.VnS(), p0.Merging(), z26.VnS());
    __ orn(z5.VnS(), z5.VnS(), 4);

    __ movprfx(z16.VnD(), p1.Merging(), z13.VnD());
    __ sdot(z16.VnD(), z11.VnH(), z7.VnH(), 1);

    __ movprfx(z27.VnD(), p5.Merging(), z18.VnD());
    __ sdot(z27.VnD(), z18.VnH(), z0.VnH());

    __ movprfx(z20.VnS(), p6.Merging(), z1.VnS());
    __ sdot(z20.VnS(), z10.VnB(), z1.VnB(), 1);

    __ movprfx(z19.VnD(), p0.Zeroing(), z7.VnD());
    __ smax(z19.VnD(), z19.VnD(), 42);

    __ movprfx(z15.VnD(), p1.Zeroing(), z7.VnD());
    __ smin(z15.VnD(), z15.VnD(), 42);

    __ movprfx(z15.VnB(), p5.Merging(), z3.VnB());
    __ splice(z15.VnB(), p5, z15.VnB(), z3.VnB());

    __ movprfx(z5.VnB(), p6.Zeroing(), z4.VnB());
    __ sqadd(z5.VnB(), z5.VnB(), 42);

    __ movprfx(z16.VnD(), p0.Zeroing(), z18.VnD());
    __ sqdecd(z16.VnD(), SVE_MUL3);

    __ movprfx(z7.VnH(), p3.Merging(), z28.VnH());
    __ sqdech(z7.VnH(), SVE_VL2);

    __ movprfx(z7.VnS(), p2.Merging(), z13.VnS());
    __ sqdecp(z7.VnS(), p2);

    __ movprfx(z22.VnS(), p7.Zeroing(), z20.VnS());
    __ sqdecw(z22.VnS(), SVE_ALL);

    __ movprfx(z26.VnD(), p1.Zeroing(), z0.VnD());
    __ sqincd(z26.VnD(), SVE_MUL3);

    __ movprfx(z15.VnH(), p7.Zeroing(), z27.VnH());
    __ sqinch(z15.VnH(), SVE_VL2);

    __ movprfx(z4.VnD(), p7.Merging(), z13.VnD());
    __ sqincp(z4.VnD(), p7);

    __ movprfx(z29.VnS(), p6.Merging(), z14.VnS());
    __ sqincw(z29.VnS(), SVE_ALL);

    __ movprfx(z17.VnB(), p1.Merging(), z24.VnB());
    __ sqsub(z17.VnB(), z17.VnB(), 42);

    __ movprfx(z26.VnS(), p5.Zeroing(), z19.VnS());
    __ sub(z26.VnS(), z26.VnS(), 42);

    __ movprfx(z15.VnD(), p1.Merging(), z3.VnD());
    __ subr(z15.VnD(), z15.VnD(), 42);

    __ movprfx(z4.VnD(), p2.Zeroing(), z14.VnD());
    __ udot(z4.VnD(), z15.VnH(), z7.VnH(), 1);

    __ movprfx(z29.VnD(), p4.Zeroing(), z28.VnD());
    __ udot(z29.VnD(), z2.VnH(), z17.VnH());

    __ movprfx(z7.VnS(), p6.Merging(), z3.VnS());
    __ udot(z7.VnS(), z14.VnB(), z1.VnB(), 1);

    __ movprfx(z14.VnB(), p3.Merging(), z5.VnB());
    __ umax(z14.VnB(), z14.VnB(), 42);

    __ movprfx(z4.VnD(), p1.Zeroing(), z2.VnD());
    __ umin(z4.VnD(), z4.VnD(), 42);

    __ movprfx(z19.VnB(), p0.Zeroing(), z27.VnB());
    __ uqadd(z19.VnB(), z19.VnB(), 42);

    __ movprfx(z24.VnD(), p7.Zeroing(), z11.VnD());
    __ uqdecd(z24.VnD(), SVE_MUL3);

    __ movprfx(z24.VnH(), p4.Zeroing(), z18.VnH());
    __ uqdech(z24.VnH(), SVE_VL2);

    __ movprfx(z31.VnS(), p5.Zeroing(), z2.VnS());
    __ uqdecp(z31.VnS(), p5);

    __ movprfx(z19.VnS(), p6.Merging(), z21.VnS());
    __ uqdecw(z19.VnS(), SVE_ALL);

    __ movprfx(z27.VnD(), p0.Merging(), z21.VnD());
    __ uqincd(z27.VnD(), SVE_MUL3);

    __ movprfx(z13.VnH(), p4.Zeroing(), z12.VnH());
    __ uqinch(z13.VnH(), SVE_VL2);

    __ movprfx(z0.VnD(), p4.Zeroing(), z1.VnD());
    __ uqincp(z0.VnD(), p4);

    __ movprfx(z12.VnS(), p4.Merging(), z21.VnS());
    __ uqincw(z12.VnS(), SVE_ALL);

    __ movprfx(z9.VnD(), p0.Zeroing(), z16.VnD());
    __ uqsub(z9.VnD(), z9.VnD(), 42);
  }
  assm.FinalizeCode();

  CheckAndMaybeDisassembleMovprfxPairs(assm.GetBuffer(), false);
}

TEST(movprfx_negative_predication_fp) {
  // Test that CanTakeSVEMovprfx() is false when a predicated movprfx appears
  // before an unpredicated instruction.
  Assembler assm;
  assm.GetCPUFeatures()->Combine(CPUFeatures::kSVE);
  {
    // We have to use the Assembler directly to generate movprfx, so we need
    // to manually reserve space for the code we're about to emit.
    static const size_t kPairCount = 9;
    CodeBufferCheckScope guard(&assm, kPairCount * 2 * kInstructionSize);

    __ movprfx(z10.VnH(), p3.Zeroing(), z3.VnH());
    __ fcmla(z10.VnH(), z22.VnH(), z3.VnH(), 2, 180);

    __ movprfx(z12.VnS(), p4.Merging(), z14.VnS());
    __ fcmla(z12.VnS(), z3.VnS(), z10.VnS(), 1, 270);

    __ movprfx(z16.VnD(), p3.Zeroing(), z24.VnD());
    __ fmla(z16.VnD(), z24.VnD(), z8.VnD(), 1);

    __ movprfx(z9.VnH(), p7.Zeroing(), z0.VnH());
    __ fmla(z9.VnH(), z8.VnH(), z0.VnH(), 7);

    __ movprfx(z23.VnS(), p5.Merging(), z5.VnS());
    __ fmla(z23.VnS(), z7.VnS(), z5.VnS(), 3);

    __ movprfx(z19.VnD(), p6.Zeroing(), z8.VnD());
    __ fmls(z19.VnD(), z27.VnD(), z13.VnD(), 1);

    __ movprfx(z25.VnH(), p7.Merging(), z24.VnH());
    __ fmls(z25.VnH(), z24.VnH(), z4.VnH(), 4);

    __ movprfx(z2.VnS(), p1.Zeroing(), z0.VnS());
    __ fmls(z2.VnS(), z9.VnS(), z0.VnS(), 3);

    // Note that ftsmul and ftssel _cannot_ take movprfx.
    __ movprfx(z22.VnD(), p6.Merging(), z16.VnD());
    __ ftmad(z22.VnD(), z22.VnD(), z20.VnD(), 2);
  }
  assm.FinalizeCode();

  CheckAndMaybeDisassembleMovprfxPairs(assm.GetBuffer(), false);
}

TEST(movprfx_positive) {
  Assembler assm;
  assm.GetCPUFeatures()->Combine(CPUFeatures::kSVE);
  {
    // We have to use the Assembler directly to generate movprfx, so we need
    // to manually reserve space for the code we're about to emit.
    static const size_t kPairCount = 117;
    CodeBufferCheckScope guard(&assm, kPairCount * 2 * kInstructionSize);

    __ movprfx(z17, z28);
    __ abs(z17.VnB(), p6.Merging(), z28.VnB());

    __ movprfx(z9, z7);
    __ add(z9.VnB(), p5.Merging(), z9.VnB(), z29.VnB());

    __ movprfx(z11, z0);
    __ add(z11.VnD(), z11.VnD(), 42);

    __ movprfx(z8.VnS(), p3.Zeroing(), z28.VnS());
    __ and_(z8.VnS(), p3.Merging(), z8.VnS(), z31.VnS());

    __ movprfx(z20, z23);
    __ and_(z20.VnS(), z20.VnS(), 4);

    __ movprfx(z24.VnD(), p5.Merging(), z11.VnD());
    __ asr(z24.VnD(), p5.Merging(), z24.VnD(), 3);

    __ movprfx(z1, z13);
    __ asr(z1.VnH(), p3.Merging(), z1.VnH(), z4.VnH());

    __ movprfx(z0.VnB(), p7.Zeroing(), z28.VnB());
    __ asr(z0.VnB(), p7.Merging(), z0.VnB(), z28.VnD());

    __ movprfx(z15, z5);
    __ asr(z15.VnD(), p3.Merging(), z15.VnD(), z5.VnD());

    __ movprfx(z24.VnH(), p3.Merging(), z22.VnH());
    __ asrd(z24.VnH(), p3.Merging(), z24.VnH(), 3);

    __ movprfx(z2.VnS(), p3.Zeroing(), z20.VnS());
    __ asrr(z2.VnS(), p3.Merging(), z2.VnS(), z15.VnS());

    __ movprfx(z17.VnB(), p7.Merging(), z6.VnB());
    __ bic(z17.VnB(), p7.Merging(), z17.VnB(), z25.VnB());

    __ movprfx(z31, z6);
    __ bic(z31.VnD(), z31.VnD(), 4);

    __ movprfx(z20, z2);
    __ clasta(z20.VnB(), p4, z20.VnB(), z15.VnB());

    __ movprfx(z27, z11);
    __ clastb(z27.VnB(), p5, z27.VnB(), z6.VnB());

    __ movprfx(z3.VnS(), p7.Zeroing(), z17.VnS());
    __ cls(z3.VnS(), p7.Merging(), z0.VnS());

    __ movprfx(z29.VnB(), p0.Zeroing(), z24.VnB());
    __ clz(z29.VnB(), p0.Merging(), z7.VnB());

    __ movprfx(z2.VnH(), p7.Zeroing(), z29.VnH());
    __ cnot(z2.VnH(), p7.Merging(), z28.VnH());

    __ movprfx(z23, z5);
    __ cnt(z23.VnH(), p0.Merging(), z12.VnH());

    __ movprfx(z5, z3);
    __ cpy(z5.VnD(), p1.Merging(), -42);

    __ movprfx(z0, z12);
    __ cpy(z0.VnB(), p1.Merging(), w0);

    __ movprfx(z27, z8);
    __ cpy(z27.VnB(), p0.Merging(), b0);

    __ movprfx(z20, z24);
    __ decd(z20.VnD(), SVE_MUL3);

    __ movprfx(z5, z28);
    __ dech(z5.VnH(), SVE_VL2);

    __ movprfx(z7, z3);
    __ decp(z7.VnD(), p2);

    __ movprfx(z4, z7);
    __ decw(z4.VnS(), SVE_ALL);

    __ movprfx(z3, z18);
    __ eon(z3.VnS(), z3.VnS(), 4);

    __ movprfx(z4.VnD(), p0.Merging(), z10.VnD());
    __ eor(z4.VnD(), p0.Merging(), z4.VnD(), z10.VnD());

    __ movprfx(z15, z18);
    __ eor(z15.VnH(), z15.VnH(), 4);

    __ movprfx(z30, z11);
    __ ext(z30.VnB(), z30.VnB(), z11.VnB(), 42);

    __ movprfx(z19, z28);
    __ incd(z19.VnD(), SVE_MUL3);

    __ movprfx(z13, z7);
    __ inch(z13.VnH(), SVE_VL2);

    __ movprfx(z14, z21);
    __ incp(z14.VnD(), p1);

    __ movprfx(z26, z12);
    __ incw(z26.VnS(), SVE_ALL);

    __ movprfx(z16, z2);
    __ insr(z16.VnB(), w16);

    __ movprfx(z20, z26);
    __ insr(z20.VnB(), b0);

    __ movprfx(z30.VnD(), p0.Merging(), z23.VnD());
    __ lsl(z30.VnD(), p0.Merging(), z30.VnD(), 3);

    __ movprfx(z28.VnS(), p2.Zeroing(), z6.VnS());
    __ lsl(z28.VnS(), p2.Merging(), z28.VnS(), z6.VnS());

    __ movprfx(z15.VnH(), p6.Zeroing(), z3.VnH());
    __ lsl(z15.VnH(), p6.Merging(), z15.VnH(), z3.VnD());

    __ movprfx(z13.VnD(), p4.Zeroing(), z14.VnD());
    __ lsl(z13.VnD(), p4.Merging(), z13.VnD(), z25.VnD());

    __ movprfx(z14, z5);
    __ lslr(z14.VnS(), p0.Merging(), z14.VnS(), z17.VnS());

    __ movprfx(z21, z1);
    __ lsr(z21.VnH(), p5.Merging(), z21.VnH(), 3);

    __ movprfx(z11.VnH(), p0.Zeroing(), z13.VnH());
    __ lsr(z11.VnH(), p0.Merging(), z11.VnH(), z9.VnH());

    __ movprfx(z24, z29);
    __ lsr(z24.VnS(), p4.Merging(), z24.VnS(), z1.VnD());

    __ movprfx(z1.VnD(), p6.Merging(), z9.VnD());
    __ lsr(z1.VnD(), p6.Merging(), z1.VnD(), z9.VnD());

    __ movprfx(z22, z3);
    __ lsrr(z22.VnB(), p3.Merging(), z22.VnB(), z3.VnB());

    __ movprfx(z24.VnB(), p2.Zeroing(), z5.VnB());
    __ mad(z24.VnB(), p2.Merging(), z5.VnB(), z10.VnB());

    __ movprfx(z8, z4);
    __ mla(z8.VnS(), p6.Merging(), z4.VnS(), z26.VnS());

    __ movprfx(z10, z8);
    __ mls(z10.VnS(), p4.Merging(), z23.VnS(), z16.VnS());

    // Aliases of cpy.
    __ movprfx(z4.VnH(), p5.Zeroing(), z2.VnH());
    __ mov(z4.VnH(), p5.Merging(), -42);

    __ movprfx(z2.VnB(), p3.Zeroing(), z24.VnB());
    __ mov(z2.VnB(), p3.Merging(), w2);

    __ movprfx(z27, z13);
    __ mov(z27.VnD(), p3.Merging(), d0);

    __ movprfx(z18.VnB(), p5.Zeroing(), z11.VnB());
    __ msb(z18.VnB(), p5.Merging(), z3.VnB(), z11.VnB());

    __ movprfx(z29, z16);
    __ mul(z29.VnS(), p6.Merging(), z29.VnS(), z9.VnS());

    __ movprfx(z21, z23);
    __ mul(z21.VnH(), z21.VnH(), 42);

    __ movprfx(z7.VnS(), p4.Merging(), z14.VnS());
    __ neg(z7.VnS(), p4.Merging(), z14.VnS());

    __ movprfx(z8.VnD(), p4.Zeroing(), z5.VnD());
    __ not_(z8.VnD(), p4.Merging(), z5.VnD());

    __ movprfx(z14, z13);
    __ orn(z14.VnS(), z14.VnS(), 4);

    __ movprfx(z14, z13);
    __ orn(z14.VnS(), z14.VnS(), 4);

    __ movprfx(z27, z17);
    __ orr(z27.VnD(), p2.Merging(), z27.VnD(), z17.VnD());

    __ movprfx(z13.VnH(), p2.Zeroing(), z27.VnH());
    __ rbit(z13.VnH(), p2.Merging(), z1.VnH());

    __ movprfx(z1, z29);
    __ revb(z1.VnS(), p4.Merging(), z6.VnS());

    __ movprfx(z18.VnD(), p2.Zeroing(), z10.VnD());
    __ revh(z18.VnD(), p2.Merging(), z16.VnD());

    __ movprfx(z2.VnD(), p1.Merging(), z10.VnD());
    __ revw(z2.VnD(), p1.Merging(), z1.VnD());

    __ movprfx(z28.VnS(), p7.Merging(), z11.VnS());
    __ sabd(z28.VnS(), p7.Merging(), z28.VnS(), z11.VnS());

    __ movprfx(z22.VnS(), p0.Merging(), z20.VnS());
    __ sdiv(z22.VnS(), p0.Merging(), z22.VnS(), z6.VnS());

    __ movprfx(z13.VnS(), p7.Merging(), z0.VnS());
    __ sdivr(z13.VnS(), p7.Merging(), z13.VnS(), z2.VnS());

    __ movprfx(z0, z12);
    __ sdot(z0.VnD(), z10.VnH(), z12.VnH(), 1);

    __ movprfx(z8, z15);
    __ sdot(z8.VnS(), z15.VnB(), z12.VnB());

    __ movprfx(z13, z0);
    __ sdot(z13.VnS(), z10.VnB(), z0.VnB(), 1);

    __ movprfx(z11, z13);
    __ smax(z11.VnB(), p5.Merging(), z11.VnB(), z24.VnB());

    __ movprfx(z3, z17);
    __ smax(z3.VnD(), z3.VnD(), 42);

    __ movprfx(z10, z29);
    __ smin(z10.VnD(), p4.Merging(), z10.VnD(), z29.VnD());

    __ movprfx(z13, z29);
    __ smin(z13.VnD(), z13.VnD(), 42);

    __ movprfx(z6, z17);
    __ smulh(z6.VnS(), p7.Merging(), z6.VnS(), z31.VnS());

    __ movprfx(z19, z20);
    __ splice(z19.VnB(), p3, z19.VnB(), z20.VnB());

    __ movprfx(z0, z3);
    __ sqadd(z0.VnD(), z0.VnD(), 42);

    __ movprfx(z29, z5);
    __ sqdecd(z29.VnD(), SVE_MUL3);

    __ movprfx(z25, z11);
    __ sqdech(z25.VnH(), SVE_VL2);

    __ movprfx(z16, z9);
    __ sqdecp(z16.VnS(), p1);

    __ movprfx(z8, z17);
    __ sqdecw(z8.VnS(), SVE_ALL);

    __ movprfx(z4, z5);
    __ sqincd(z4.VnD(), SVE_MUL3);

    __ movprfx(z0, z17);
    __ sqinch(z0.VnH(), SVE_VL2);

    __ movprfx(z7, z27);
    __ sqincp(z7.VnS(), p6);

    __ movprfx(z10, z9);
    __ sqincw(z10.VnS(), SVE_ALL);

    __ movprfx(z31, z22);
    __ sqsub(z31.VnB(), z31.VnB(), 42);

    __ movprfx(z12.VnH(), p7.Zeroing(), z23.VnH());
    __ sub(z12.VnH(), p7.Merging(), z12.VnH(), z23.VnH());

    __ movprfx(z10, z1);
    __ sub(z10.VnH(), z10.VnH(), 42);

    __ movprfx(z15.VnB(), p0.Merging(), z0.VnB());
    __ subr(z15.VnB(), p0.Merging(), z15.VnB(), z0.VnB());

    __ movprfx(z17, z2);
    __ subr(z17.VnH(), z17.VnH(), 42);

    __ movprfx(z5, z3);
    __ sxtb(z5.VnD(), p6.Merging(), z20.VnD());

    __ movprfx(z11, z17);
    __ sxth(z11.VnD(), p6.Merging(), z25.VnD());

    __ movprfx(z26, z4);
    __ sxtw(z26.VnD(), p5.Merging(), z4.VnD());

    __ movprfx(z15.VnD(), p0.Zeroing(), z8.VnD());
    __ uabd(z15.VnD(), p0.Merging(), z15.VnD(), z20.VnD());

    __ movprfx(z21, z24);
    __ udiv(z21.VnD(), p3.Merging(), z21.VnD(), z24.VnD());

    __ movprfx(z22, z10);
    __ udivr(z22.VnD(), p7.Merging(), z22.VnD(), z27.VnD());

    __ movprfx(z27, z25);
    __ udot(z27.VnD(), z29.VnH(), z3.VnH(), 1);

    __ movprfx(z29, z10);
    __ udot(z29.VnS(), z10.VnB(), z21.VnB());

    __ movprfx(z18, z0);
    __ udot(z18.VnS(), z14.VnB(), z0.VnB(), 1);

    __ movprfx(z6, z30);
    __ umax(z6.VnS(), p2.Merging(), z6.VnS(), z27.VnS());

    __ movprfx(z31, z17);
    __ umax(z31.VnD(), z31.VnD(), 42);

    __ movprfx(z27.VnS(), p0.Merging(), z20.VnS());
    __ umin(z27.VnS(), p0.Merging(), z27.VnS(), z8.VnS());

    __ movprfx(z0, z11);
    __ umin(z0.VnH(), z0.VnH(), 42);

    __ movprfx(z21, z17);
    __ umulh(z21.VnB(), p0.Merging(), z21.VnB(), z30.VnB());

    __ movprfx(z9, z24);
    __ uqadd(z9.VnD(), z9.VnD(), 42);

    __ movprfx(z18, z13);
    __ uqdecd(z18.VnD(), SVE_MUL3);

    __ movprfx(z20, z23);
    __ uqdech(z20.VnH(), SVE_VL2);

    __ movprfx(z12, z29);
    __ uqdecp(z12.VnS(), p7);

    __ movprfx(z24, z25);
    __ uqdecw(z24.VnS(), SVE_ALL);

    __ movprfx(z13, z1);
    __ uqincd(z13.VnD(), SVE_MUL3);

    __ movprfx(z5, z19);
    __ uqinch(z5.VnH(), SVE_VL2);

    __ movprfx(z6, z25);
    __ uqincp(z6.VnS(), p5);

    __ movprfx(z12, z14);
    __ uqincw(z12.VnS(), SVE_ALL);

    __ movprfx(z13, z6);
    __ uqsub(z13.VnH(), z13.VnH(), 42);

    __ movprfx(z31, z3);
    __ uxtb(z31.VnS(), p0.Merging(), z3.VnS());

    __ movprfx(z18.VnD(), p4.Merging(), z25.VnD());
    __ uxth(z18.VnD(), p4.Merging(), z25.VnD());

    __ movprfx(z18.VnD(), p7.Merging(), z25.VnD());
    __ uxtw(z18.VnD(), p7.Merging(), z25.VnD());
  }
  assm.FinalizeCode();

  CheckAndMaybeDisassembleMovprfxPairs(assm.GetBuffer(), true);
}

TEST(movprfx_positive_fp) {
  Assembler assm;
  assm.GetCPUFeatures()->Combine(CPUFeatures::kSVE);
  {
    // We have to use the Assembler directly to generate movprfx, so we need
    // to manually reserve space for the code we're about to emit.
    static const size_t kPairCount = 73;
    CodeBufferCheckScope guard(&assm, kPairCount * 2 * kInstructionSize);

    __ movprfx(z18.VnS(), p6.Zeroing(), z20.VnS());
    __ fabd(z18.VnS(), p6.Merging(), z18.VnS(), z19.VnS());

    __ movprfx(z28.VnD(), p4.Zeroing(), z24.VnD());
    __ fabs(z28.VnD(), p4.Merging(), z24.VnD());

    __ movprfx(z12, z8);
    __ fadd(z12.VnS(), p2.Merging(), z12.VnS(), 0.5);

    __ movprfx(z0.VnS(), p1.Merging(), z9.VnS());
    __ fadd(z0.VnS(), p1.Merging(), z0.VnS(), z9.VnS());

    __ movprfx(z10.VnH(), p2.Merging(), z2.VnH());
    __ fcadd(z10.VnH(), p2.Merging(), z10.VnH(), z20.VnH(), 90);

    __ movprfx(z21, z6);
    __ fcmla(z21.VnH(), z31.VnH(), z6.VnH(), 2, 180);

    __ movprfx(z16, z6);
    __ fcmla(z16.VnS(), z11.VnS(), z6.VnS(), 1, 270);

    __ movprfx(z15.VnH(), p6.Merging(), z16.VnH());
    __ fcpy(z15.VnH(), p6.Merging(), 1.25);

    __ movprfx(z1, z14);
    __ fcvt(z1.VnD(), p2.Merging(), z4.VnH());

    __ movprfx(z25.VnD(), p6.Merging(), z1.VnD());
    __ fcvt(z25.VnD(), p6.Merging(), z1.VnS());

    __ movprfx(z18.VnS(), p2.Merging(), z2.VnS());
    __ fcvt(z18.VnH(), p2.Merging(), z7.VnS());

    __ movprfx(z21.VnD(), p5.Zeroing(), z26.VnD());
    __ fcvt(z21.VnH(), p5.Merging(), z26.VnD());

    __ movprfx(z12.VnD(), p1.Merging(), z18.VnD());
    __ fcvtzs(z12.VnD(), p1.Merging(), z18.VnH());

    __ movprfx(z3.VnS(), p2.Merging(), z0.VnS());
    __ fcvtzs(z3.VnS(), p2.Merging(), z26.VnS());

    __ movprfx(z21.VnS(), p4.Merging(), z7.VnS());
    __ fcvtzs(z21.VnS(), p4.Merging(), z7.VnH());

    __ movprfx(z16.VnD(), p3.Zeroing(), z4.VnD());
    __ fcvtzs(z16.VnS(), p3.Merging(), z28.VnD());

    __ movprfx(z31.VnD(), p4.Merging(), z1.VnD());
    __ fcvtzu(z31.VnD(), p4.Merging(), z1.VnH());

    __ movprfx(z23.VnH(), p0.Zeroing(), z28.VnH());
    __ fcvtzu(z23.VnH(), p0.Merging(), z28.VnH());

    __ movprfx(z2, z12);
    __ fcvtzu(z2.VnD(), p3.Merging(), z28.VnS());

    __ movprfx(z4, z7);
    __ fcvtzu(z4.VnS(), p7.Merging(), z16.VnD());

    __ movprfx(z13.VnS(), p3.Zeroing(), z23.VnS());
    __ fdiv(z13.VnS(), p3.Merging(), z13.VnS(), z23.VnS());

    __ movprfx(z6.VnD(), p1.Zeroing(), z16.VnD());
    __ fdivr(z6.VnD(), p1.Merging(), z6.VnD(), z5.VnD());

    __ movprfx(z31, z23);
    __ fmad(z31.VnS(), p5.Merging(), z23.VnS(), z11.VnS());

    __ movprfx(z14.VnH(), p7.Merging(), z21.VnH());
    __ fmax(z14.VnH(), p7.Merging(), z14.VnH(), 0.0);

    __ movprfx(z17.VnS(), p4.Merging(), z9.VnS());
    __ fmax(z17.VnS(), p4.Merging(), z17.VnS(), z9.VnS());

    __ movprfx(z1.VnS(), p3.Zeroing(), z30.VnS());
    __ fmaxnm(z1.VnS(), p3.Merging(), z1.VnS(), 0.0);

    __ movprfx(z10.VnD(), p1.Zeroing(), z17.VnD());
    __ fmaxnm(z10.VnD(), p1.Merging(), z10.VnD(), z17.VnD());

    __ movprfx(z3, z13);
    __ fmin(z3.VnS(), p0.Merging(), z3.VnS(), 0.0);

    __ movprfx(z15, z21);
    __ fmin(z15.VnS(), p4.Merging(), z15.VnS(), z21.VnS());

    __ movprfx(z30.VnH(), p7.Zeroing(), z25.VnH());
    __ fminnm(z30.VnH(), p7.Merging(), z30.VnH(), 0.0);

    __ movprfx(z31, z15);
    __ fminnm(z31.VnD(), p5.Merging(), z31.VnD(), z25.VnD());

    __ movprfx(z27, z28);
    __ fmla(z27.VnD(), z28.VnD(), z12.VnD(), 1);

    __ movprfx(z26.VnH(), p6.Zeroing(), z13.VnH());
    __ fmla(z26.VnH(), p6.Merging(), z13.VnH(), z7.VnH());

    __ movprfx(z26, z10);
    __ fmla(z26.VnH(), z10.VnH(), z1.VnH(), 7);

    __ movprfx(z0, z1);
    __ fmla(z0.VnS(), z25.VnS(), z1.VnS(), 3);

    __ movprfx(z7, z3);
    __ fmls(z7.VnD(), z30.VnD(), z3.VnD(), 1);

    __ movprfx(z1, z24);
    __ fmls(z1.VnD(), p5.Merging(), z20.VnD(), z24.VnD());

    __ movprfx(z19, z18);
    __ fmls(z19.VnH(), z18.VnH(), z7.VnH(), 4);

    __ movprfx(z0, z26);
    __ fmls(z0.VnS(), z17.VnS(), z4.VnS(), 3);

    __ movprfx(z19.VnS(), p7.Zeroing(), z6.VnS());
    __ fmov(z19.VnS(), p7.Merging(), 0.0);

    __ movprfx(z21, z15);
    __ fmov(z21.VnH(), p7.Merging(), 2.5);

    __ movprfx(z23, z18);
    __ fmsb(z23.VnS(), p4.Merging(), z1.VnS(), z7.VnS());

    __ movprfx(z8, z28);
    __ fmul(z8.VnS(), p4.Merging(), z8.VnS(), 2.0);

    __ movprfx(z6.VnD(), p6.Merging(), z27.VnD());
    __ fmul(z6.VnD(), p6.Merging(), z6.VnD(), z27.VnD());

    __ movprfx(z6.VnH(), p0.Merging(), z19.VnH());
    __ fmulx(z6.VnH(), p0.Merging(), z6.VnH(), z19.VnH());

    __ movprfx(z5.VnH(), p0.Merging(), z1.VnH());
    __ fneg(z5.VnH(), p0.Merging(), z1.VnH());

    __ movprfx(z22.VnD(), p4.Zeroing(), z24.VnD());
    __ fnmad(z22.VnD(), p4.Merging(), z24.VnD(), z12.VnD());

    __ movprfx(z5.VnS(), p0.Merging(), z29.VnS());
    __ fnmla(z5.VnS(), p0.Merging(), z17.VnS(), z29.VnS());

    __ movprfx(z5, z3);
    __ fnmls(z5.VnD(), p5.Merging(), z3.VnD(), z2.VnD());

    __ movprfx(z9.VnD(), p2.Zeroing(), z7.VnD());
    __ fnmsb(z9.VnD(), p2.Merging(), z7.VnD(), z23.VnD());

    // Note that frecpe and frecps _cannot_ take movprfx.
    __ movprfx(z12.VnH(), p1.Zeroing(), z17.VnH());
    __ frecpx(z12.VnH(), p1.Merging(), z4.VnH());

    __ movprfx(z28.VnS(), p4.Zeroing(), z27.VnS());
    __ frinta(z28.VnS(), p4.Merging(), z24.VnS());

    __ movprfx(z7.VnD(), p7.Merging(), z25.VnD());
    __ frinti(z7.VnD(), p7.Merging(), z25.VnD());

    __ movprfx(z10, z21);
    __ frintm(z10.VnD(), p5.Merging(), z26.VnD());

    __ movprfx(z25, z21);
    __ frintn(z25.VnH(), p4.Merging(), z1.VnH());

    __ movprfx(z25, z9);
    __ frintp(z25.VnH(), p1.Merging(), z9.VnH());

    __ movprfx(z30, z16);
    __ frintx(z30.VnS(), p1.Merging(), z16.VnS());

    __ movprfx(z0.VnD(), p5.Merging(), z9.VnD());
    __ frintz(z0.VnD(), p5.Merging(), z23.VnD());

    __ movprfx(z11.VnD(), p7.Merging(), z2.VnD());
    __ fscale(z11.VnD(), p7.Merging(), z11.VnD(), z2.VnD());

    __ movprfx(z23.VnS(), p4.Merging(), z17.VnS());
    __ fsqrt(z23.VnS(), p4.Merging(), z10.VnS());

    __ movprfx(z0.VnD(), p2.Merging(), z26.VnD());
    __ fsub(z0.VnD(), p2.Merging(), z0.VnD(), 1.0);

    __ movprfx(z28.VnD(), p1.Zeroing(), z16.VnD());
    __ fsub(z28.VnD(), p1.Merging(), z28.VnD(), z16.VnD());

    __ movprfx(z22, z27);
    __ fsubr(z22.VnD(), p4.Merging(), z22.VnD(), 1.0);

    __ movprfx(z4.VnS(), p2.Merging(), z26.VnS());
    __ fsubr(z4.VnS(), p2.Merging(), z4.VnS(), z26.VnS());

    // Note that ftsmul and ftssel _cannot_ take movprfx.
    __ movprfx(z10, z4);
    __ ftmad(z10.VnS(), z10.VnS(), z4.VnS(), 2);

    __ movprfx(z2, z16);
    __ scvtf(z2.VnD(), p1.Merging(), z16.VnS());

    __ movprfx(z10, z20);
    __ scvtf(z10.VnD(), p5.Merging(), z20.VnD());

    __ movprfx(z29, z28);
    __ scvtf(z29.VnS(), p0.Merging(), z31.VnD());

    __ movprfx(z26.VnD(), p3.Merging(), z13.VnD());
    __ scvtf(z26.VnH(), p3.Merging(), z5.VnD());

    __ movprfx(z7.VnD(), p3.Zeroing(), z26.VnD());
    __ ucvtf(z7.VnD(), p3.Merging(), z26.VnS());

    __ movprfx(z13, z17);
    __ ucvtf(z13.VnD(), p7.Merging(), z17.VnD());

    __ movprfx(z24.VnD(), p1.Merging(), z31.VnD());
    __ ucvtf(z24.VnS(), p1.Merging(), z18.VnD());

    __ movprfx(z17.VnD(), p4.Merging(), z22.VnD());
    __ ucvtf(z17.VnH(), p4.Merging(), z4.VnD());
  }
  assm.FinalizeCode();

  CheckAndMaybeDisassembleMovprfxPairs(assm.GetBuffer(), true);
}


}  // namespace aarch64
}  // namespace vixl
