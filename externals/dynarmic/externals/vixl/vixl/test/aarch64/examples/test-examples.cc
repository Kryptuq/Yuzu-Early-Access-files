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

#include "custom-disassembler.h"
#include "examples.h"
#include "non-const-visitor.h"
#include "test-runner.h"
#include "test-utils.h"
#include "../test-utils-aarch64.h"

#include "aarch64/macro-assembler-aarch64.h"
#include "aarch64/simulator-aarch64.h"
#define TEST(name) TEST_(EXAMPLE_##name)

using namespace vixl;
using namespace vixl::aarch64;


TEST(custom_disassembler) { TestCustomDisassembler(); }


// The tests below only work with the simulator.
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64

uint64_t FactorialC(uint64_t n) {
  uint64_t result = 1;

  while (n != 0) {
    result *= n;
    n--;
  }

  return result;
}

// Multiply two column-major 4x4 matrices of 32 bit floating point values.
// Return a column-major 4x4 matrix of 32 bit floating point values in 'C'.
void MatrixMultiplyC(float C[16], float A[16], float B[16]) {
  C[0] = A[0] * B[0] + A[4] * B[1] + A[8] * B[2] + A[12] * B[3];
  C[1] = A[1] * B[0] + A[5] * B[1] + A[9] * B[2] + A[13] * B[3];
  C[2] = A[2] * B[0] + A[6] * B[1] + A[10] * B[2] + A[14] * B[3];
  C[3] = A[3] * B[0] + A[7] * B[1] + A[11] * B[2] + A[15] * B[3];

  C[4] = A[0] * B[4] + A[4] * B[5] + A[8] * B[6] + A[12] * B[7];
  C[5] = A[1] * B[4] + A[5] * B[5] + A[9] * B[6] + A[13] * B[7];
  C[6] = A[2] * B[4] + A[6] * B[5] + A[10] * B[6] + A[14] * B[7];
  C[7] = A[3] * B[4] + A[7] * B[5] + A[11] * B[6] + A[15] * B[7];

  C[8] = A[0] * B[8] + A[4] * B[9] + A[8] * B[10] + A[12] * B[11];
  C[9] = A[1] * B[8] + A[5] * B[9] + A[9] * B[10] + A[13] * B[11];
  C[10] = A[2] * B[8] + A[6] * B[9] + A[10] * B[10] + A[14] * B[11];
  C[11] = A[3] * B[8] + A[7] * B[9] + A[11] * B[10] + A[15] * B[11];

  C[12] = A[0] * B[12] + A[4] * B[13] + A[8] * B[14] + A[12] * B[15];
  C[13] = A[1] * B[12] + A[5] * B[13] + A[9] * B[14] + A[13] * B[15];
  C[14] = A[2] * B[12] + A[6] * B[13] + A[10] * B[14] + A[14] * B[15];
  C[15] = A[3] * B[12] + A[7] * B[13] + A[11] * B[14] + A[15] * B[15];
}

double Add3DoubleC(double x, double y, double z) { return x + y + z; }

double Add4DoubleC(uint64_t a, double b, uint64_t c, double d) {
  return static_cast<double>(a) + b + static_cast<double>(c) + d;
}

uint32_t SumArrayC(uint8_t* array, uint32_t size) {
  uint32_t result = 0;

  for (uint32_t i = 0; i < size; ++i) {
    result += array[i];
  }

  return result;
}


#define TEST_FUNCTION(Func)                                                   \
  do {                                                                        \
    /* Record callee-saved registers, so we can check them after the test. */ \
    int64_t saved_xregs[13];                                                  \
    saved_xregs[0] = simulator.ReadXRegister(19);                             \
    saved_xregs[1] = simulator.ReadXRegister(20);                             \
    saved_xregs[2] = simulator.ReadXRegister(21);                             \
    saved_xregs[3] = simulator.ReadXRegister(22);                             \
    saved_xregs[4] = simulator.ReadXRegister(23);                             \
    saved_xregs[5] = simulator.ReadXRegister(24);                             \
    saved_xregs[6] = simulator.ReadXRegister(25);                             \
    saved_xregs[7] = simulator.ReadXRegister(26);                             \
    saved_xregs[8] = simulator.ReadXRegister(27);                             \
    saved_xregs[9] = simulator.ReadXRegister(28);                             \
    saved_xregs[10] = simulator.ReadXRegister(29);                            \
    saved_xregs[11] = simulator.ReadXRegister(30);                            \
    saved_xregs[12] = simulator.ReadXRegister(31);                            \
                                                                              \
    uint64_t saved_dregs[8];                                                  \
    saved_dregs[0] = simulator.ReadDRegisterBits(8);                          \
    saved_dregs[1] = simulator.ReadDRegisterBits(9);                          \
    saved_dregs[2] = simulator.ReadDRegisterBits(10);                         \
    saved_dregs[3] = simulator.ReadDRegisterBits(11);                         \
    saved_dregs[4] = simulator.ReadDRegisterBits(12);                         \
    saved_dregs[5] = simulator.ReadDRegisterBits(13);                         \
    saved_dregs[6] = simulator.ReadDRegisterBits(14);                         \
    saved_dregs[7] = simulator.ReadDRegisterBits(15);                         \
                                                                              \
    simulator.WriteXRegister(test_function_reg.GetCode(),                     \
                             masm.GetLabelAddress<uint64_t>(&Func));          \
    simulator.RunFrom(masm.GetLabelAddress<Instruction*>(&test));             \
                                                                              \
    /* Check that callee-saved regsiters are preserved. */                    \
    VIXL_CHECK(saved_xregs[0] == simulator.ReadXRegister(19));                \
    VIXL_CHECK(saved_xregs[1] == simulator.ReadXRegister(20));                \
    VIXL_CHECK(saved_xregs[2] == simulator.ReadXRegister(21));                \
    VIXL_CHECK(saved_xregs[3] == simulator.ReadXRegister(22));                \
    VIXL_CHECK(saved_xregs[4] == simulator.ReadXRegister(23));                \
    VIXL_CHECK(saved_xregs[5] == simulator.ReadXRegister(24));                \
    VIXL_CHECK(saved_xregs[6] == simulator.ReadXRegister(25));                \
    VIXL_CHECK(saved_xregs[7] == simulator.ReadXRegister(26));                \
    VIXL_CHECK(saved_xregs[8] == simulator.ReadXRegister(27));                \
    VIXL_CHECK(saved_xregs[9] == simulator.ReadXRegister(28));                \
    VIXL_CHECK(saved_xregs[10] == simulator.ReadXRegister(29));               \
    VIXL_CHECK(saved_xregs[11] == simulator.ReadXRegister(30));               \
    VIXL_CHECK(saved_xregs[12] == simulator.ReadXRegister(31));               \
                                                                              \
    VIXL_CHECK(saved_dregs[0] == simulator.ReadDRegisterBits(8));             \
    VIXL_CHECK(saved_dregs[1] == simulator.ReadDRegisterBits(9));             \
    VIXL_CHECK(saved_dregs[2] == simulator.ReadDRegisterBits(10));            \
    VIXL_CHECK(saved_dregs[3] == simulator.ReadDRegisterBits(11));            \
    VIXL_CHECK(saved_dregs[4] == simulator.ReadDRegisterBits(12));            \
    VIXL_CHECK(saved_dregs[5] == simulator.ReadDRegisterBits(13));            \
    VIXL_CHECK(saved_dregs[6] == simulator.ReadDRegisterBits(14));            \
    VIXL_CHECK(saved_dregs[7] == simulator.ReadDRegisterBits(15));            \
                                                                              \
  } while (0)

#define START()                                                          \
  MacroAssembler masm;                                                   \
  Decoder decoder;                                                       \
  Simulator simulator(&decoder);                                         \
  simulator.SetColouredTrace(Test::coloured_trace());                    \
  RegisterDump regs;                                                     \
                                                                         \
  Register test_function_reg = x15;                                      \
                                                                         \
  Label test;                                                            \
  masm.Bind(&test);                                                      \
  {                                                                      \
    int trace_parameters = 0;                                            \
    if (Test::trace_reg()) trace_parameters |= LOG_STATE;                \
    if (Test::trace_write()) trace_parameters |= LOG_WRITE;              \
    if (Test::trace_sim()) trace_parameters |= LOG_DISASM;               \
    if (Test::trace_branch()) trace_parameters |= LOG_BRANCH;            \
    if (trace_parameters != 0) {                                         \
      masm.Trace(static_cast<TraceParameters>(trace_parameters),         \
                 TRACE_ENABLE);                                          \
    }                                                                    \
  }                                                                      \
  masm.Blr(test_function_reg);                                           \
  masm.Trace(LOG_ALL, TRACE_DISABLE);                                    \
  regs.Dump(&masm);                                                      \
  masm.Mov(lr, reinterpret_cast<uint64_t>(Simulator::kEndOfSimAddress)); \
  masm.Ret();                                                            \
  masm.FinalizeCode()


#define FACTORIAL_DOTEST(N)                                           \
  do {                                                                \
    simulator.ResetState();                                           \
    simulator.WriteXRegister(0, N);                                   \
    TEST_FUNCTION(factorial);                                         \
    VIXL_CHECK(static_cast<uint64_t>(regs.xreg(0)) == FactorialC(N)); \
  } while (0)

TEST(factorial) {
  START();

  Label factorial;
  masm.Bind(&factorial);
  GenerateFactorial(&masm);
  masm.FinalizeCode();

  FACTORIAL_DOTEST(0);
  FACTORIAL_DOTEST(1);
  FACTORIAL_DOTEST(5);
  FACTORIAL_DOTEST(10);
  FACTORIAL_DOTEST(20);
  FACTORIAL_DOTEST(25);
}


#define FACTORIAL_REC_DOTEST(N)                                       \
  do {                                                                \
    simulator.ResetState();                                           \
    simulator.WriteXRegister(0, N);                                   \
    TEST_FUNCTION(factorial_rec);                                     \
    VIXL_CHECK(static_cast<uint64_t>(regs.xreg(0)) == FactorialC(N)); \
  } while (0)

TEST(factorial_rec) {
  START();

  Label factorial_rec;
  masm.Bind(&factorial_rec);
  GenerateFactorialRec(&masm);
  masm.FinalizeCode();

  FACTORIAL_REC_DOTEST(0);
  FACTORIAL_REC_DOTEST(1);
  FACTORIAL_REC_DOTEST(5);
  FACTORIAL_REC_DOTEST(10);
  FACTORIAL_REC_DOTEST(20);
  FACTORIAL_REC_DOTEST(25);
}

TEST(neon_matrix_multiply) {
  START();

  Label neon_matrix_multiply;
  masm.Bind(&neon_matrix_multiply);
  GenerateNEONMatrixMultiply(&masm);
  masm.FinalizeCode();

  {
    const int kRowSize = 4;
    const int kColSize = 4;
    const int kLength = kRowSize * kColSize;

    float mat1[kLength], mat2[kLength], expected[kLength], output[kLength];

    // Fill the two input matrices with some 32 bit floating point values.

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

    MatrixMultiplyC(expected, mat1, mat2);

    simulator.ResetState();
    simulator.WriteXRegister(0, reinterpret_cast<uintptr_t>(output));
    simulator.WriteXRegister(1, reinterpret_cast<uintptr_t>(mat1));
    simulator.WriteXRegister(2, reinterpret_cast<uintptr_t>(mat2));
    TEST_FUNCTION(neon_matrix_multiply);

    // Check that the results match what is expected.
    for (int i = 0; i < kLength; i++) {
      VIXL_CHECK(output[i] == expected[i]);
    }
  }
}

TEST(add2_vectors) {
  START();

  // Create and initialize the assembler and the simulator.
  Label add2_vectors;
  masm.Bind(&add2_vectors);
  GenerateAdd2Vectors(&masm);
  masm.FinalizeCode();

  // Initialize input data for the example function.
  uint8_t A[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 200};
  uint8_t B[] =
      {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 50};
  uint8_t D[ARRAY_SIZE(A)];
  uintptr_t A_addr = reinterpret_cast<uintptr_t>(A);
  uintptr_t B_addr = reinterpret_cast<uintptr_t>(B);

  // Check whether number of elements in vectors match.
  VIXL_STATIC_ASSERT(ARRAY_SIZE(A) == ARRAY_SIZE(B));
  VIXL_STATIC_ASSERT(ARRAY_SIZE(A) == ARRAY_SIZE(D));

  // Compute vector sum for comparison later.
  for (unsigned i = 0; i < ARRAY_SIZE(A); i++) {
    D[i] = A[i] + B[i];
  }

  // Set up simulator and run example function.
  simulator.ResetState();
  simulator.WriteXRegister(0, A_addr);
  simulator.WriteXRegister(1, B_addr);
  simulator.WriteXRegister(2, ARRAY_SIZE(A));
  TEST_FUNCTION(add2_vectors);

  // Compare vectors to ensure sums are equal.
  for (unsigned i = 0; i < ARRAY_SIZE(A); i++) {
    VIXL_CHECK(A[i] == D[i]);
  }
}

#define ADD3_DOUBLE_DOTEST(A, B, C)                   \
  do {                                                \
    simulator.ResetState();                           \
    simulator.WriteDRegister(0, A);                   \
    simulator.WriteDRegister(1, B);                   \
    simulator.WriteDRegister(2, C);                   \
    TEST_FUNCTION(add3_double);                       \
    VIXL_CHECK(regs.dreg(0) == Add3DoubleC(A, B, C)); \
  } while (0)

TEST(add3_double) {
  START();

  Label add3_double;
  masm.Bind(&add3_double);
  GenerateAdd3Double(&masm);
  masm.FinalizeCode();

  ADD3_DOUBLE_DOTEST(0.0, 0.0, 0.0);
  ADD3_DOUBLE_DOTEST(457.698, 14.36, 2.00025);
  ADD3_DOUBLE_DOTEST(-45.55, -98.9, -0.354);
  ADD3_DOUBLE_DOTEST(.55, .9, .12);
}


#define ADD4_DOUBLE_DOTEST(A, B, C, D)                   \
  do {                                                   \
    simulator.ResetState();                              \
    simulator.WriteXRegister(0, A);                      \
    simulator.WriteDRegister(0, B);                      \
    simulator.WriteXRegister(1, C);                      \
    simulator.WriteDRegister(1, D);                      \
    TEST_FUNCTION(add4_double);                          \
    VIXL_CHECK(regs.dreg(0) == Add4DoubleC(A, B, C, D)); \
  } while (0)

TEST(add4_double) {
  START();

  Label add4_double;
  masm.Bind(&add4_double);
  GenerateAdd4Double(&masm);
  masm.FinalizeCode();

  ADD4_DOUBLE_DOTEST(0, 0, 0, 0);
  ADD4_DOUBLE_DOTEST(4, 3.287, 6, 13.48);
  ADD4_DOUBLE_DOTEST(56, 665.368, 0, -4932.4697);
  ADD4_DOUBLE_DOTEST(56, 0, 546, 0);
  ADD4_DOUBLE_DOTEST(0, 0.658, 0, 0.00000011540026);
}


#define SUM_ARRAY_DOTEST(Array)                                      \
  do {                                                               \
    simulator.ResetState();                                          \
    uintptr_t addr = reinterpret_cast<uintptr_t>(Array);             \
    simulator.WriteXRegister(0, addr);                               \
    simulator.WriteXRegister(1, ARRAY_SIZE(Array));                  \
    TEST_FUNCTION(sum_array);                                        \
    VIXL_CHECK(regs.xreg(0) == SumArrayC(Array, ARRAY_SIZE(Array))); \
  } while (0)

TEST(sum_array) {
  START();

  Label sum_array;
  masm.Bind(&sum_array);
  GenerateSumArray(&masm);
  masm.FinalizeCode();

  uint8_t data1[] = {4, 9, 13, 3, 2, 6, 5};
  SUM_ARRAY_DOTEST(data1);

  uint8_t data2[] = {42};
  SUM_ARRAY_DOTEST(data2);

  uint8_t data3[1000];
  for (unsigned int i = 0; i < ARRAY_SIZE(data3); ++i) data3[i] = 255;
  SUM_ARRAY_DOTEST(data3);
}


#define ABS_DOTEST(X)                   \
  do {                                  \
    simulator.ResetState();             \
    simulator.WriteXRegister(0, X);     \
    TEST_FUNCTION(func_abs);            \
    VIXL_CHECK(regs.xreg(0) == abs(X)); \
  } while (0)

TEST(abs) {
  START();

  Label func_abs;
  masm.Bind(&func_abs);
  GenerateAbs(&masm);
  masm.FinalizeCode();

  ABS_DOTEST(-42);
  ABS_DOTEST(0);
  ABS_DOTEST(545);
  ABS_DOTEST(-428751489);
}


TEST(crc32) {
  START();

  Label crc32;
  masm.Bind(&crc32);
  GenerateCrc32(&masm);
  masm.FinalizeCode();

  const char* msg = "Hello World!";
  uintptr_t msg_addr = reinterpret_cast<uintptr_t>(msg);
  size_t msg_size = strlen(msg);
  int64_t chksum = INT64_C(0xe3d6e35c);
  simulator.WriteXRegister(0, msg_addr);
  simulator.WriteXRegister(1, msg_size);
  TEST_FUNCTION(crc32);
  VIXL_CHECK(regs.xreg(0) == chksum);
}


TEST(swap4) {
  START();

  Label swap4;
  masm.Bind(&swap4);
  GenerateSwap4(&masm);
  masm.FinalizeCode();

  int64_t a = 15;
  int64_t b = 26;
  int64_t c = 46;
  int64_t d = 79;

  simulator.WriteXRegister(0, a);
  simulator.WriteXRegister(1, b);
  simulator.WriteXRegister(2, c);
  simulator.WriteXRegister(3, d);
  TEST_FUNCTION(swap4);
  VIXL_CHECK(regs.xreg(0) == d);
  VIXL_CHECK(regs.xreg(1) == c);
  VIXL_CHECK(regs.xreg(2) == b);
  VIXL_CHECK(regs.xreg(3) == a);
}


TEST(swap_int32) {
  START();

  Label swap_int32;
  masm.Bind(&swap_int32);
  GenerateSwapInt32(&masm);
  masm.FinalizeCode();

  int32_t x = 168;
  int32_t y = 246;
  simulator.WriteWRegister(0, x);
  simulator.WriteWRegister(1, y);
  TEST_FUNCTION(swap_int32);
  VIXL_CHECK(regs.wreg(0) == y);
  VIXL_CHECK(regs.wreg(1) == x);
}


#define CHECKBOUNDS_DOTEST(Value, Low, High)                         \
  do {                                                               \
    simulator.ResetState();                                          \
    simulator.WriteXRegister(0, Value);                              \
    simulator.WriteXRegister(1, Low);                                \
    simulator.WriteXRegister(2, High);                               \
    TEST_FUNCTION(check_bounds);                                     \
    VIXL_CHECK(regs.xreg(0) == ((Low <= Value) && (Value <= High))); \
  } while (0)

TEST(check_bounds) {
  START();

  Label check_bounds;
  masm.Bind(&check_bounds);
  GenerateCheckBounds(&masm);
  masm.FinalizeCode();

  CHECKBOUNDS_DOTEST(0, 100, 200);
  CHECKBOUNDS_DOTEST(58, 100, 200);
  CHECKBOUNDS_DOTEST(99, 100, 200);
  CHECKBOUNDS_DOTEST(100, 100, 200);
  CHECKBOUNDS_DOTEST(101, 100, 200);
  CHECKBOUNDS_DOTEST(150, 100, 200);
  CHECKBOUNDS_DOTEST(199, 100, 200);
  CHECKBOUNDS_DOTEST(200, 100, 200);
  CHECKBOUNDS_DOTEST(201, 100, 200);
}


#define GETTING_STARTED_DOTEST(Value)                         \
  do {                                                        \
    simulator.ResetState();                                   \
    simulator.WriteXRegister(0, Value);                       \
    TEST_FUNCTION(demo_function);                             \
    VIXL_CHECK(regs.xreg(0) == (Value & 0x1122334455667788)); \
  } while (0)

TEST(getting_started) {
  START();

  Label demo_function;
  masm.Bind(&demo_function);
  GenerateDemoFunction(&masm);
  masm.FinalizeCode();

  GETTING_STARTED_DOTEST(0x8899aabbccddeeff);
  GETTING_STARTED_DOTEST(0x1122334455667788);
  GETTING_STARTED_DOTEST(0x0000000000000000);
  GETTING_STARTED_DOTEST(0xffffffffffffffff);
  GETTING_STARTED_DOTEST(0x5a5a5a5a5a5a5a5a);
}


TEST(non_const_visitor) {
  MacroAssembler masm;

  Label code_start, code_end;
  masm.Bind(&code_start);
  GenerateNonConstVisitorTestCode(&masm);
  masm.Bind(&code_end);
  masm.FinalizeCode();
  Instruction* instr_start = masm.GetLabelAddress<Instruction*>(&code_start);
  Instruction* instr_end = masm.GetLabelAddress<Instruction*>(&code_end);

  int64_t res_orig = RunNonConstVisitorTestGeneratedCode(instr_start);

  ModifyNonConstVisitorTestGeneratedCode(instr_start, instr_end);

  int64_t res_mod = RunNonConstVisitorTestGeneratedCode(instr_start);
  VIXL_CHECK(res_orig == -res_mod);
}


TEST(literal_example) {
  VIXL_ASSERT(LiteralExample(1, 2) == 3);
  VIXL_ASSERT(LiteralExample(INT64_C(0x100000000), 0x1) ==
              INT64_C(0x100000001));
}


#ifdef VIXL_HAS_SIMULATED_RUNTIME_CALL_SUPPORT

// This is an approximation of the result that works for the ranges tested
// below.
#define RUNTIME_CALLS_EXPECTED(A, B) ((A + B) * 4)

#define RUNTIME_CALLS_DOTEST(A, B)                            \
  do {                                                        \
    simulator.ResetState();                                   \
    simulator.WriteWRegister(0, A);                           \
    simulator.WriteWRegister(1, B);                           \
    TEST_FUNCTION(start);                                     \
    VIXL_CHECK(regs.wreg(0) == RUNTIME_CALLS_EXPECTED(A, B)); \
  } while (0)

TEST(runtime_calls) {
  START();

  Label start;
  masm.Bind(&start);
  GenerateRuntimeCallExamples(&masm);
  masm.FinalizeCode();

  RUNTIME_CALLS_DOTEST(0, 0);
  RUNTIME_CALLS_DOTEST(1, -2);
  RUNTIME_CALLS_DOTEST(123, 456);
}

#endif  // VIXL_HAS_SIMULATED_RUNTIME_CALL_SUPPORT

TEST(sve_strlen) {
  START();

  CPUFeatures cpu_features(CPUFeatures::kSVE);
  masm.SetCPUFeatures(cpu_features);

  Label sve_strlen;
  masm.Bind(&sve_strlen);
  GenerateSVEStrlen(&masm);
  masm.FinalizeCode();

  if (CanRun(cpu_features)) {
    const char* inputs[] =
        {"Exactly 15 chrs",
         "Exactly 16 chars",
         "Exactly 17 chars.",

         "This string is very long and will require multiple iterations, even "
         "with the maximum VL (256 bytes). This string is very long and will "
         "require multiple iterations, even with the maximum VL (256 bytes). "
         "This string is very long and will require multiple iterations, even "
         "with the maximum VL (256 bytes)."};

    for (size_t i = 0; i < ArrayLength(inputs); i++) {
      simulator.ResetState();
      simulator.WriteXRegister(0, reinterpret_cast<uintptr_t>(inputs[i]));
      TEST_FUNCTION(sve_strlen);
      VIXL_CHECK(static_cast<size_t>(regs.xreg(0)) == strlen(inputs[i]));
    }
  }
}

#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
