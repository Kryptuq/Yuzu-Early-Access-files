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

// A vector by scalar multiply helper routine to generate code for
// the multiplication of each column of the resulting 4x4 matrix.
// This function provides a template for the following pattern:
//
// __ Fmul(v<v_out>.V4S(), v4.V4S(),  v<s_column>.S(), 0);
// __ Fmla(v<v_out>.V4S(), v5.V4S(),  v<s_column>.S(), 1);
// __ Fmla(v<v_out>.V4S(), v6.V4S(),  v<s_column>.S(), 2);
// __ Fmla(v<v_out>.V4S(), v7.V4S(),  v<s_column>.S(), 3);
//
// v<v_out> corresponds to a column of the output matrix (v0, v1, v2 or v3).
// v<s_column> corresponds to a column of the 2nd input (v16, v17, v18 or v19).
//
static void GenerateMultiplyColumn(MacroAssembler* masm,
                                   unsigned out_column,
                                   unsigned in_column) {
  // 'v_out' splits a Q register into 4 lanes of 32 bits each.
  VRegister v_out = VRegister(out_column, kQRegSize, 4);
  // 'v_in' refers to a single 32 bit 'S' lane.
  VRegister v_in = VRegister(in_column, kSRegSize);

  __ Fmul(v_out, v4.V4S(), v_in, 0);  // e.g. (v0.V4S(), v4.V4S(),  v8.S(), 0).
  __ Fmla(v_out, v5.V4S(), v_in, 1);
  __ Fmla(v_out, v6.V4S(), v_in, 2);
  __ Fmla(v_out, v7.V4S(), v_in, 3);
}

void GenerateNEONMatrixMultiply(MacroAssembler* masm) {
  // Argument location:
  //   dst  -> x0
  //   mat1 -> x1
  //   mat2 -> x2

  Label end;

  __ And(x3, x0, x1);
  __ And(x3, x3, x2);
  __ Cbz(x3, &end);  // Nothing to do if an input is null.

  // Load the first matrix into v4, v5, v6 and v7.
  __ Ld1(v4.V4S(), v5.V4S(), v6.V4S(), v7.V4S(), MemOperand(x1));
  // Load the first matrix into v16, v17, v18 and v19.
  __ Ld1(v16.V4S(), v17.V4S(), v18.V4S(), v19.V4S(), MemOperand(x2));

  // Initialise vectors of the output matrix with zeros.
  // This is only for the purposes of showing how this can be achived
  // but technically this is not required because we overwrite all lanes
  // of the output vectors.
  __ Movi(v0.V16B(), 0);
  __ Movi(v1.V16B(), 0);
  __ Movi(v2.V16B(), 0);
  __ Movi(v3.V16B(), 0);

  GenerateMultiplyColumn(masm, 0, 16);
  GenerateMultiplyColumn(masm, 1, 17);
  GenerateMultiplyColumn(masm, 2, 18);
  GenerateMultiplyColumn(masm, 3, 19);

  // Store the resulting matrix from v0, v1, v2 and v3.
  __ St1(v0.V4S(), v1.V4S(), v2.V4S(), v3.V4S(), MemOperand(x0));

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
  Label neon_matrix_multiply;
  masm.Bind(&neon_matrix_multiply);
  GenerateNEONMatrixMultiply(&masm);
  masm.FinalizeCode();

  // Define the required variables and run the example function.
  const int kRowSize = 4;
  const int kColSize = 4;
  const int kLength = kRowSize * kColSize;

  float mat1[kLength], mat2[kLength], output[kLength];

  // Initialise the output matrix to the zero matrix.
  memset(output, 0, sizeof(output[0]) * kLength);

  // Fill the two input matrices with some 32 bit floating point values.
  // Array initialisation using curly brackets is also possible like so:
  //   float mat1[kLength] = { 1.0f, 52.03f, 4.43f, ... };
  // However, the following way better shows the "column-major" arrangement.

  mat1[0] = 1.0f;
  mat1[4] = 2.0f;
  mat1[8] = 3.0f;
  mat1[12] = 4.0f;
  mat1[1] = 52.03f;
  mat1[5] = 12.24f;
  mat1[9] = 53.56f;
  mat1[13] = 22.22f;
  mat1[2] = 4.43f;
  mat1[6] = 5.00f;
  mat1[10] = 7.00f;
  mat1[14] = 3.11f;
  mat1[3] = 43.47f;
  mat1[7] = 10.97f;
  mat1[11] = 37.78f;
  mat1[15] = 90.91f;

  mat2[0] = 1.0f;
  mat2[4] = 11.24f;
  mat2[8] = 21.00f;
  mat2[12] = 21.31f;
  mat2[1] = 2.0f;
  mat2[5] = 2.24f;
  mat2[9] = 8.56f;
  mat2[13] = 52.03f;
  mat2[2] = 3.0f;
  mat2[6] = 51.00f;
  mat2[10] = 21.00f;
  mat2[14] = 33.11f;
  mat2[3] = 4.0f;
  mat2[7] = 0.00f;
  mat2[11] = 84.00f;
  mat2[15] = 1.97f;

  simulator.ResetState();
  simulator.WriteXRegister(0, reinterpret_cast<uintptr_t>(output));
  simulator.WriteXRegister(1, reinterpret_cast<uintptr_t>(mat1));
  simulator.WriteXRegister(2, reinterpret_cast<uintptr_t>(mat2));
  simulator.RunFrom(masm.GetLabelAddress<Instruction*>(&neon_matrix_multiply));

  // Print the 4x4 output matrix along with both 4x4 input matrices.
  for (int i = 0; i < kRowSize; i++) {
    printf(
        "| %8.2f %8.2f %8.2f %8.2f |   "
        "| %8.2f %8.2f %8.2f %8.2f |       "
        "| %8.2f %8.2f %8.2f %8.2f |\n",
        mat1[i],
        mat1[4 + i],
        mat1[8 + i],
        mat1[12 + i],
        mat2[i],
        mat2[4 + i],
        mat2[8 + i],
        mat2[12 + i],
        output[i],
        output[4 + i],
        output[8 + i],
        output[12 + i]);
    if (i == 0 || i == 2) {
      printf(
          "|                                     |   "
          "|                                     |       "
          "|                                     |\n");
    } else if (i == 1) {
      printf(
          "|                                     | x "
          "|                                     |   =   "
          "|                                     |\n");
    }
  }

  return 0;
}
#else
// Without the simulator there is nothing to test.
int main(void) { return 0; }
#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
#endif  // TEST_EXAMPLES
