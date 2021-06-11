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

void GenerateAdd3Double(MacroAssembler* masm) {
  // double add3_double(double x, double y, double z)
  //  Argument locations:
  //    x -> d0
  //    y -> d1
  //    z -> d2
  __ Fadd(d0, d0, d1);  // d0 <- x + y
  __ Fadd(d0, d0, d2);  // d0 <- d0 + z

  // The return value is already in d0.
  __ Ret();
}


#ifndef TEST_EXAMPLES
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
int main(void) {
  MacroAssembler masm;
  Decoder decoder;
  Simulator simulator(&decoder);

  // Generate the code for the example function.
  Label add3_double;
  masm.Bind(&add3_double);
  GenerateAdd3Double(&masm);
  masm.FinalizeCode();

  // Run the example function.
  double a = 498.36547;
  double b = 23.369;
  double c = 7964.697954;
  simulator.WriteDRegister(0, a);
  simulator.WriteDRegister(1, b);
  simulator.WriteDRegister(2, c);
  simulator.RunFrom(masm.GetLabelAddress<Instruction*>(&add3_double));
  printf("%f + %f + %f = %f\n", a, b, c, simulator.ReadDRegister(0));

  return 0;
}
#else
// Without the simulator there is nothing to test.
int main(void) { return 0; }
#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
#endif  // TEST_EXAMPLES
