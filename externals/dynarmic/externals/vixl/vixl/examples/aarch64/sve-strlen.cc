// Copyright 2020, VIXL authors
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

// size_t sve_strlen(const char* str);
void GenerateSVEStrlen(MacroAssembler* masm) {
  // We will accumulate the length as we load each chunk.
  Register len = x1;
  __ Mov(len, 0);

  // We want to load as much as we can on each iteration, so set up an all-true
  // predicate for that purpose.
  PRegister all_true = p0;
  __ Ptrue(all_true.VnB());

  Label loop;
  __ Bind(&loop);
  // FFR is cumulative, so reset it to all-true for each iteration.
  __ Setffr();

  // Load as many characters as we can from &str[len]. We have to use a NF or FF
  // load, because we don't know how long the string is. An FF load is a good
  // choice, because we know that we will see at least a NULL termination, even
  // for an empty string.
  __ Ldff1b(z0.VnB(), all_true.Zeroing(), SVEMemOperand(x0, len));
  // For example, if str = "Test string.", and we load every byte:
  //   z0.b:      \0 . g n i r t s   t s e T

  // FFR now represents the number of bytes that we actually loaded, so use it
  // to predicate the data processing instructions.
  __ Rdffr(p1.VnB());

  // Find the NULL termination (if there is one), and set the flags.
  __ Cmpeq(p2.VnB(), p1.Zeroing(), z0.VnB(), 0);
  //   p2.b:       1 0 0 0 0 0 0 0 0 0 0 0 0

  // Activate every lane up to (but not including) the NULL termination. If we
  // found no NULL termination, this activates every lane, and indicates that we
  // have to load another vector of characters. Lanes activated in this way
  // represent string characters that we need to count.
  __ Brkb(p1.VnB(), p1.Zeroing(), p2.VnB());
  //   p1.b: 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1

  // Count the active lanes, and add them to the length count.
  __ Incp(len, p1.VnB());

  // Loop until `cmpeq` finds a NULL termination.
  __ B(sve_none, &loop);

  // Put the length in the AAPCS64 return register.
  __ Mov(x0, len);
  __ Ret();
}

#ifndef TEST_EXAMPLES
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
int main(void) {
  MacroAssembler masm;
  Decoder decoder;
  Simulator simulator(&decoder);

  // We're running on the simulator, so we can assume that SVE is present.
  masm.GetCPUFeatures()->Combine(CPUFeatures::kSVE);

  // Generate the code for the example function.
  Label sve_strlen;
  masm.Bind(&sve_strlen);
  GenerateSVEStrlen(&masm);
  masm.FinalizeCode();

  const char* example_input = "This is a string of exactly 42 characters.";
  VIXL_ASSERT(strlen(example_input) == 42);

  simulator.ResetState();
  simulator.WriteXRegister(0, reinterpret_cast<uintptr_t>(example_input));
  simulator.RunFrom(masm.GetLabelAddress<Instruction*>(&sve_strlen));

  printf("strlen(\"%s\") == %" PRIu64 "\n",
         example_input,
         simulator.ReadXRegister(0));

  return 0;
}
#else
// Without the simulator there is nothing to test.
int main(void) { return 0; }
#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
#endif  // TEST_EXAMPLES
