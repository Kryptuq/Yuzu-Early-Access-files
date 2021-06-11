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

void GenerateDemo(MacroAssembler* masm) {
  // uint32_t demo(uint32_t x)

  // Load a constant in r1 using the literal pool.
  __ Ldr(r1, 0x12345678);
  __ And(r0, r0, r1);
  __ Bx(lr);
}


#ifndef TEST_EXAMPLES
int main() {
  MacroAssembler masm;
  // Generate the code for the example function.
  Label demo;
  // Tell the macro assembler that the label "demo" refer to the current
  // location in the buffer.
  masm.Bind(&demo);
  GenerateDemo(&masm);
  // Ensure that everything is generated and that the generated buffer is
  // ready to use.
  masm.FinalizeCode();
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH32
// There is no simulator defined for VIXL AArch32.
#else
  byte* code = masm.GetBuffer()->GetStartAddress<byte*>();
  uint32_t code_size = masm.GetSizeOfCodeGenerated();
  ExecutableMemory memory(code, code_size);
  // Run the example function.
  uint32_t (*demo_function)(uint32_t) =
      memory
          .GetEntryPoint<uint32_t (*)(uint32_t)>(demo,
                                                 masm.GetInstructionSetInUse());
  uint32_t input_value = 0x89abcdef;
  uint32_t output_value = (*demo_function)(input_value);
  printf("native: demo(0x%08x) = 0x%08x\n", input_value, output_value);
#endif
  return 0;
}
#endif  // TEST_EXAMPLES
