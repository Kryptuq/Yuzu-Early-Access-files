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

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

#include "aarch32/constants-aarch32.h"
#include "aarch32/instructions-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"

using namespace vixl;
using namespace vixl::aarch32;

static const unsigned kDefaultInstructionCount = 100000;

// This program focuses on emitting simple instructions.
//
// This code will emit a given number of 'add r0, r1, r2' in a buffer.
// This code therefore focuses on Emit and Operand.
void benchmark(unsigned instructions, InstructionSet isa) {
  const unsigned buffer_size = 256 * KBytes;
  timeval start;
  gettimeofday(&start, NULL);

  MacroAssembler masm(buffer_size);
  masm.UseInstructionSet(isa);

#define __ masm.

  for (unsigned i = 0; i < instructions; ++i) {
    __ Add(r0, r1, Operand(r2));
  }

  masm.FinalizeCode();

  timeval end;
  gettimeofday(&end, NULL);
  double delta = (end.tv_sec - start.tv_sec) +
                 static_cast<double>(end.tv_usec - start.tv_usec) / 1000000;
  printf("%s: time for %d instructions: %gs\n",
         isa == T32 ? "T32" : "A32",
         instructions,
         delta);
}

int main(int argc, char* argv[]) {
  unsigned instructions = 0;

  switch (argc) {
    case 1:
      instructions = kDefaultInstructionCount;
      break;
    case 2:
      instructions = atoi(argv[1]);
      break;
    default:
      printf("Usage: %s [#instructions]\n", argv[0]);
      exit(1);
  }

#ifdef VIXL_INCLUDE_TARGET_A32
  benchmark(instructions, A32);
#endif
#ifdef VIXL_INCLUDE_TARGET_T32
  benchmark(instructions, T32);
#endif

  return 0;
}
