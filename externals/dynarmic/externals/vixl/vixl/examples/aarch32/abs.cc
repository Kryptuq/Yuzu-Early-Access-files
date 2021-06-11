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

void GenerateAbs(MacroAssembler* masm) {
  // int32_t abs(int32_t x)
  // Argument location:
  //   x -> r0

  __ Cmp(r0, 0);
  // If r0 is negative, negate r0.
  __ Rsb(mi, r0, r0, 0);
  __ Bx(lr);
}


#ifndef TEST_EXAMPLES
int main() {
  MacroAssembler masm(A32);
  // Generate the code for the example function.
  Label abs;
  masm.Bind(&abs);
  GenerateAbs(&masm);
  masm.FinalizeCode();
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH32
  // There is no simulator defined for VIXL AArch32.
  printf("This example cannot be simulated\n");
#else
  byte* code = masm.GetBuffer()->GetStartAddress<byte*>();
  uint32_t code_size = masm.GetSizeOfCodeGenerated();
  ExecutableMemory memory(code, code_size);
  // Run the example function.
  int32_t (*abs_function)(int32_t) =
      memory.GetEntryPoint<int32_t (*)(int32_t)>(abs,
                                                 masm.GetInstructionSetInUse());
  int32_t input_value = -42;
  int32_t output_value = (*abs_function)(input_value);
  printf("native: abs(%d) = %d\n", input_value, output_value);
#endif
  return 0;
}
#endif  // TEST_EXAMPLES
