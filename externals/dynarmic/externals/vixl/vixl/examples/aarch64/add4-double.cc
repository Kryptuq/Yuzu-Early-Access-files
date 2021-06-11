// Copyright 2014, VIXL authors
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
using namespace vixl::aarch64;

#define __ masm->

void GenerateAdd4Double(MacroAssembler* masm) {
  // double Add4Double(uint64_t a, double b, uint64_t c, double d)
  //  Argument locations:
  //    a -> x0
  //    b -> d0
  //    c -> x1
  //    d -> d1

  // Turn 'a' and 'c' into double values.
  __ Ucvtf(d2, x0);
  __ Ucvtf(d3, x1);

  // Add everything together.
  __ Fadd(d0, d0, d1);
  __ Fadd(d2, d2, d3);
  __ Fadd(d0, d0, d2);

  // The return value is in d0.
  __ Ret();
}


#ifndef TEST_EXAMPLES
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
int main(void) {
  MacroAssembler masm;
  Decoder decoder;
  Simulator simulator(&decoder);

  // Generate the code for the example function.
  Label add4_double;
  masm.Bind(&add4_double);
  GenerateAdd4Double(&masm);
  masm.FinalizeCode();

  // Run the example function.
  uint64_t a = 21;
  double b = 987.3654;
  uint64_t c = 4387;
  double d = 36.698754;
  simulator.WriteXRegister(0, a);
  simulator.WriteDRegister(0, b);
  simulator.WriteXRegister(1, c);
  simulator.WriteDRegister(1, d);
  simulator.RunFrom(masm.GetLabelAddress<Instruction*>(&add4_double));
  // clang-format off
  printf("%" PRIu64 " + %f + %" PRIu64 " + %f = %f\n",
         a, b, c, d, simulator.ReadDRegister(0));
  // clang-format on

  return 0;
}
#else
// Without the simulator there is nothing to test.
int main(void) { return 0; }
#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
#endif  // TEST_EXAMPLES
