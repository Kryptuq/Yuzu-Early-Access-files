// Copyright 2017, VIXL authors
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

static const int kDefaultIterationCount = 1000;
static const int kDefaultLiteralCount = 100;

// This program focuses on emitting branch instructions targeting a label bound
// very closely. Here the MacroAssembler is used. This exercises label binding
// and patching mechanisms, as well as the veneer resolving mechanisms for
// branches not requiring veneers.
void benchmark(int iterations, int literals, InstructionSet isa) {
  const int buffer_size = 256 * KBytes;

  timeval start;
  gettimeofday(&start, NULL);
  MacroAssembler masm(buffer_size);
  masm.UseInstructionSet(isa);

#define __ masm.

  // Load a number of distinct literals, for a number of iterations, forcing
  // pool emission in between.
  for (int i = 0; i < iterations; i++) {
    for (int j = 0; j < literals; j++) {
      __ Ldr(r0, j);
      __ FinalizeCode();
    }
  }

  timeval end;
  gettimeofday(&end, NULL);
  double delta = (end.tv_sec - start.tv_sec) +
                 static_cast<double>(end.tv_usec - start.tv_usec) / 1000000;
  printf("%s: time for %d iterations: %gs\n",
         isa == T32 ? "T32" : "A32",
         iterations,
         delta);
}


int main(int argc, char* argv[]) {
  int iterations = 0;
  int literals = 0;

  switch (argc) {
    case 1:
      iterations = kDefaultIterationCount;
      literals = kDefaultLiteralCount;
      break;
    case 2:
      iterations = atoi(argv[1]);
      literals = kDefaultLiteralCount;
      break;
    case 3:
      iterations = atoi(argv[1]);
      literals = atoi(argv[2]);
      break;
    default:
      printf("Usage: %s [#iterations] [#literals]\n", argv[0]);
      exit(1);
  }

#ifdef VIXL_INCLUDE_TARGET_A32
  benchmark(iterations, literals, A32);
#endif
#ifdef VIXL_INCLUDE_TARGET_T32
  benchmark(iterations, literals, T32);
#endif

  return 0;
}
