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

void GenerateFactorial(MacroAssembler* masm) {
  // uint64_t factorial(uint64_t n)
  // Argument location:
  //   n -> x0

  Label loop, end;

  __ Mov(x1, x0);
  __ Mov(x0, 1);  // Use x0 as the accumulator.

  __ Cbz(x1, &end);  // Nothing to do if the input is null.

  __ Bind(&loop);
  __ Mul(x0, x0, x1);
  __ Sub(x1, x1, 1);
  __ Cbnz(x1, &loop);

  __ Bind(&end);
  // The return value is in x0.
  __ Ret();
}


#ifndef TEST_EXAMPLES
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
int main(void) {
  MacroAssembler masm;
  Decoder decoder;
  Simulator simulator(&decoder);

  // Generate the code for the example function.
  Label factorial;
  masm.Bind(&factorial);
  GenerateFactorial(&masm);
  masm.FinalizeCode();

  // Run the example function.
  uint64_t input_val = 16;
  simulator.WriteXRegister(0, input_val);
  simulator.RunFrom(masm.GetLabelAddress<Instruction*>(&factorial));
  printf("factorial(%" PRIu64 ") = %" PRId64 "\n",
         input_val,
         simulator.ReadXRegister(0));

  return 0;
}
#else
// Without the simulator there is nothing to test.
int main(void) { return 0; }
#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
#endif  // TEST_EXAMPLES
