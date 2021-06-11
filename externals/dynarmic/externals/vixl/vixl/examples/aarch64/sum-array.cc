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

#define ARRAY_SIZE(Array) (sizeof(Array) / sizeof((Array)[0]))
#define __ masm->

void GenerateSumArray(MacroAssembler* masm) {
  // uint32_t sum_array(uint8_t* array, uint32_t size)
  //  Argument locations:
  //    array (pointer) -> x0
  //    size            -> x1

  Label loop, end;

  __ Mov(x2, x0);
  __ Mov(w0, 0);

  // There's nothing to do if the array is empty.
  __ Cbz(w1, &end);

  // Go through the array and sum the elements.
  __ Bind(&loop);

  __ Ldrb(w3, MemOperand(x2, 1, PostIndex));  // w3 = *(x2++)
  __ Add(w0, w0, w3);

  __ Sub(w1, w1, 1);
  __ Cbnz(w1, &loop);

  __ Bind(&end);
  __ Ret();
}


#ifndef TEST_EXAMPLES
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
int main(void) {
  MacroAssembler masm;
  Decoder decoder;
  Simulator simulator(&decoder);

  // Generate the code for the example function.
  Label sum_array;
  masm.Bind(&sum_array);
  GenerateSumArray(&masm);
  masm.FinalizeCode();

  // Run the example function.
  uint8_t data[] = {2, 45, 63, 7, 245, 38};
  uintptr_t data_addr = reinterpret_cast<uintptr_t>(data);
  simulator.WriteXRegister(0, data_addr);
  simulator.WriteXRegister(1, ARRAY_SIZE(data));
  simulator.RunFrom(masm.GetLabelAddress<Instruction*>(&sum_array));

  unsigned int i;
  for (i = 0; i < ARRAY_SIZE(data) - 1; ++i) {
    printf("%d + ", data[i]);
  }
  printf("%d = %d\n", data[i], simulator.ReadWRegister(0));

  return 0;
}
#else
// Without the simulator there is nothing to test.
int main(void) { return 0; }
#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
#endif  // TEST_EXAMPLES
