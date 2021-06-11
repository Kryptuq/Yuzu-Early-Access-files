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

// Macro to compute the number of elements in a vector.
#define ARRAY_SIZE(Array) (sizeof(Array) / sizeof((Array)[0]))
#define __ masm->

/*
 * This example adds two vectors with 1-byte elements using NEON instructions,
 * and returns the results in the first vector.
 */
void GenerateAdd2Vectors(MacroAssembler* masm) {
  // void add2_vectors(uint8_t *vec_a, const uint8_t *vec_b, unsigned size)
  // Argument locations:
  //    vec_a (pointer) -> x0
  //    vec_b (pointer) -> x1
  //    size (integer) -> w2
  // Result returned in vec_a.

  Label loop16, loopr, end;

  // Loop to add vector elements in 16-byte chunks.
  __ Bind(&loop16);

  // Handle vectors smaller than 16-bytes in the remainder loop.
  __ Cmp(w2, 16);
  __ B(lo, &loopr);
  __ Sub(w2, w2, 16);

  // Add vectors in 16-byte chunks.
  __ Ld1(v0.V16B(), MemOperand(x0));
  __ Ld1(v1.V16B(), MemOperand(x1, 16, PostIndex));
  __ Add(v0.V16B(), v0.V16B(), v1.V16B());
  __ St1(v0.V16B(), MemOperand(x0, 16, PostIndex));

  __ B(&loop16);

  // Loop to add the remaining vector elements.
  __ Bind(&loopr);

  // If there are no more vector elements to process, then exit.
  __ Cbz(w2, &end);
  __ Sub(w2, w2, 1);

  // Add remaining vector elements in 1-byte chunks.
  __ Ldrb(w5, MemOperand(x0));
  __ Ldrb(w6, MemOperand(x1, 1, PostIndex));
  __ Add(w5, w5, w6);
  __ Strb(w5, MemOperand(x0, 1, PostIndex));

  __ B(&loopr);

  __ Bind(&end);

  __ Ret();
}


void PrintVector(const uint8_t* vec, unsigned num) {
  unsigned i;
  printf("( ");
  if (num > 0) {
    for (i = 0; i < num - 1; ++i) {
      printf("%d, ", vec[i]);
    }
    printf("%d", vec[i]);
  }
  printf(" )\n");
}


#ifndef TEST_EXAMPLES
int main(void) {
  MacroAssembler masm;

  // Generate native code for the example function.
  Label add2_vectors;
  masm.Bind(&add2_vectors);
  GenerateAdd2Vectors(&masm);
  masm.FinalizeCode();

  // Initialize input data for the example function.
  // clang-format: off
  uint8_t vec_a[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                     13, 14, 15, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
                     10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
  uint8_t vec_b[] = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
                     29, 30, 31, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
                     26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};
  // clang-format on
  uint8_t vec_c[ARRAY_SIZE(vec_a)];

  // Check whether the number of elements in both vectors match.
  VIXL_CHECK(ARRAY_SIZE(vec_a) == ARRAY_SIZE(vec_b));

  // Compute the result in C.
  for (unsigned i = 0; i < ARRAY_SIZE(vec_a); i++) {
    vec_c[i] = vec_a[i] + vec_b[i];
  }

#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
  uintptr_t vec_a_addr = reinterpret_cast<uintptr_t>(vec_a);
  uintptr_t vec_b_addr = reinterpret_cast<uintptr_t>(vec_b);

  // Configure register environment in the simulator.
  Decoder decoder;
  Simulator simulator(&decoder);
  simulator.WriteXRegister(0, vec_a_addr);
  simulator.WriteXRegister(1, vec_b_addr);
  simulator.WriteXRegister(2, ARRAY_SIZE(vec_a));
  PrintVector(vec_a, ARRAY_SIZE(vec_a));
  printf(" +\n");
  PrintVector(vec_b, ARRAY_SIZE(vec_b));

  // Run the example function in the simulator.
  simulator.RunFrom(masm.GetLabelAddress<Instruction*>(&add2_vectors));
  printf(" =\n");
  PrintVector(vec_a, ARRAY_SIZE(vec_a));

  // Check that the computed value in NEON matches the C version.
  for (unsigned i = 0; i < ARRAY_SIZE(vec_a); i++) {
    VIXL_CHECK(vec_c[i] == vec_a[i]);
  }
#else
  USE(vec_c);

  // Placeholder to run test natively.
  printf("Running tests natively is not supported yet.\n");
#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64

  return 0;
}
#endif  // TEST_EXAMPLES
