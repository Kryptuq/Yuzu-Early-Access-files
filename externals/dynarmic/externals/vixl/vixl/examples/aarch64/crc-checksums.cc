// Copyright 2015, VIXL authors
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

void GenerateCrc32(MacroAssembler* masm) {
  // uint32_t crc32(const char *msg, size_t length)
  // Argument location:
  //   msg (pointer)    -> x0
  //   length (integer) -> x1

  // This example computes CRC32CB on an input array of a given size
  // and returns the resulting checksum in w0.

  Label loop, end;

  // Move input array to temp register so we can re-use w0 as return register.
  __ Mov(x2, x0);

  // Initial remainder for the checksum. If length=0, then this value will be
  // returned.
  __ Mov(w0, 0xffffffff);

  // Loop for iterating through the array, starting at msg[0].
  __ Bind(&loop);

  // If no more elements to process, then exit function.
  __ Cbz(x1, &end);
  __ Sub(x1, x1, 1);

  // Compute checksum for msg[i].
  __ Ldrb(w3, MemOperand(x2, 1, PostIndex));
  __ Crc32b(w0, w0, w3);
  __ B(&loop);

  __ Bind(&end);

  __ Ret();
}

#ifndef TEST_EXAMPLES

void RunExample(const char* msg) {
  MacroAssembler masm;

  // Generate the code for the example function.
  Label func;
  masm.Bind(&func);
  GenerateCrc32(&masm);
  masm.FinalizeCode();

#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
  // Run example function in the simulator.
  uintptr_t msg_addr = reinterpret_cast<uintptr_t>(msg);
  size_t msg_size = strlen(msg);
  Decoder decoder;
  Simulator simulator(&decoder);
  simulator.WriteXRegister(0, msg_addr);
  simulator.WriteXRegister(1, msg_size);
  simulator.RunFrom(masm.GetLabelAddress<Instruction*>(&func));
  printf("crc32(\"%s\")=0x%x\n", msg, simulator.ReadWRegister(0));
#else
  // Run example function natively.
  printf("Not yet implemented.\n");
  USE(msg);
#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
}


int main(void) {
  RunExample("Hello World!");
  RunExample("do");
  RunExample("1");
  RunExample("");

  return 0;
}
#endif  // TEST_EXAMPLES
