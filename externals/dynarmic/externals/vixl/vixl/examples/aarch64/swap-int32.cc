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

void GenerateSwapInt32(MacroAssembler* masm) {
  {
    // In this scope the register x2 will be used by the macro-assembler
    // as the stack pointer (via peek, poke, push, etc).
    const Register old_stack_pointer = __ StackPointer();
    __ Mov(x2, __ StackPointer());
    __ SetStackPointer(x2);

    // This call to Claim is not 16-byte aligned and would have failed
    // if the current stack pointer was sp.
    __ Claim(8);

    __ Poke(w0, 0);
    __ Poke(w1, 4);
    __ Peek(w1, 0);
    __ Peek(w0, 4);

    __ Drop(8);

    // Even if we didn't use the system stack pointer, sp might have been
    // modified because the ABI forbids access to memory below the stack
    // pointer.
    __ Mov(old_stack_pointer, __ StackPointer());
    __ SetStackPointer(old_stack_pointer);
  }

  // The stack pointer has now been switched back to sp.
  __ Ret();
}


#ifndef TEST_EXAMPLES
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
int main(void) {
  MacroAssembler masm;
  Decoder decoder;
  Simulator simulator(&decoder);

  // Generate the code for the example function.
  Label swap_int32;
  masm.Bind(&swap_int32);
  GenerateSwapInt32(&masm);
  masm.FinalizeCode();

  // Run the example function.
  simulator.WriteWRegister(0, 0x11111111);
  simulator.WriteWRegister(1, 0x22222222);

  // clang-format off
  printf("Before swap_int32:\n"
         "x0 = 0x%" PRIx32 "\n"
         "x1 = 0x%" PRIx32 "\n",
         simulator.ReadWRegister(0), simulator.ReadWRegister(1));
  // clang-format on

  simulator.RunFrom(masm.GetLabelAddress<Instruction*>(&swap_int32));

  // clang-format off
  printf("After swap_int32:\n"
         "x0 = 0x%" PRIx32 "\n"
         "x1 = 0x%" PRIx32 "\n",
         simulator.ReadWRegister(0), simulator.ReadWRegister(1));
  // clang-format on

  return 0;
}
#else
// Without the simulator there is nothing to test.
int main(void) { return 0; }
#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
#endif  // TEST_EXAMPLES
