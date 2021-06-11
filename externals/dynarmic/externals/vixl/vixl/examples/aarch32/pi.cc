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

#include "examples.h"

using namespace vixl;
using namespace vixl::aarch32;

#define __ masm->

void GenerateApproximatePi(MacroAssembler* masm) {
  // double ApproximatePi(uint32_t iterations)
  // Very rough approximation of pi
  // pi/4 = 1 - 1/3 + 1/5 - 1/7 + ... + (-1)^n / (2n + 1)
  __ Cmp(r0, 0);
  __ Bx(eq, lr);
  __ Vpush(Untyped64, DRegisterList(d8, 8));
  __ Vldr(d0, 1.0);
  __ Vldr(d1, 3.0);
  __ Vldr(d2, 5.0);
  __ Vldr(d3, 7.0);


  __ Vmov(d4, 8.0);
  __ Vmov(d5, 1.0);

  __ Vmov(I64, d10, 0);  // d10 = 0.0;
  __ Vmov(I64, d11, 0);  // d11 = 0.0;
  __ Vmov(I64, d12, 0);  // d12 = 0.0;
  __ Vmov(I64, d13, 0);  // d13 = 0.0

  Label loop;
  __ Bind(&loop);

  __ Vdiv(F64, d6, d5, d0);
  __ Vdiv(F64, d7, d5, d1);
  __ Vdiv(F64, d8, d5, d2);
  __ Vdiv(F64, d9, d5, d3);

  __ Vadd(F64, d10, d10, d6);
  __ Vadd(F64, d11, d11, d7);
  __ Vadd(F64, d12, d12, d8);
  __ Vadd(F64, d13, d13, d9);

  __ Vadd(F64, d0, d0, d4);
  __ Vadd(F64, d1, d1, d4);
  __ Vadd(F64, d2, d2, d4);
  __ Vadd(F64, d3, d3, d4);

  __ Subs(r0, r0, 1);
  __ B(ne, &loop);

  __ Vmov(F64, d4, 4.0);
  __ Vadd(F64, d10, d10, d12);
  __ Vadd(F64, d11, d11, d13);
  __ Vsub(F64, d10, d10, d11);
  __ Vmul(F64, d0, d10, d4);
  __ Vpop(Untyped64, DRegisterList(d8, 8));
  __ Bx(lr);
}

#ifndef TEST_EXAMPLES
int main() {
  MacroAssembler masm;
  // Generate the code for the example function.
  Label pi_approx;
  masm.Bind(&pi_approx);
  GenerateApproximatePi(&masm);
  masm.FinalizeCode();
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH32
  // There is no simulator defined for VIXL AArch32.
  printf("This example cannot be simulated\n");
#else
  byte* code = masm.GetBuffer()->GetStartAddress<byte*>();
  uint32_t code_size = masm.GetSizeOfCodeGenerated();
  ExecutableMemory memory(code, code_size);
  // Run the example function.
  double (*pi_function)(uint32_t) =
      memory.GetEntryPoint<double (*)(uint32_t)>(pi_approx,
                                                 masm.GetInstructionSetInUse());
  uint32_t repeat = 10000000;
  double output_value = (*pi_function)(repeat);
  printf("native: pi_approx(%u) = %3.10f\n", repeat, output_value);
#endif
  return 0;
}
#endif  // TEST_EXAMPLES
