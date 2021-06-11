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

#ifndef VIXL_EXAMPLE_EXAMPLES_H_
#define VIXL_EXAMPLE_EXAMPLES_H_

#include "aarch64/macro-assembler-aarch64.h"
#include "aarch64/simulator-aarch64.h"

// Generate a function with the following prototype:
//   uint64_t factorial(uint64_t n)
//
// It provides an iterative implementation of the factorial computation.
void GenerateFactorial(vixl::aarch64::MacroAssembler* masm);

// Generate a function with the following prototype:
//   uint64_t factorial_rec(uint64_t n)
//
// It provides a recursive implementation of the factorial computation.
void GenerateFactorialRec(vixl::aarch64::MacroAssembler* masm);

// Generate a function with the following prototype:
//   void neon_matrix_multiply(float* dst, float* mat1, float* mat2)
//
// It provides an implementation of a column-major 4x4 matrix multiplication.
void GenerateNEONMatrixMultiply(vixl::aarch64::MacroAssembler* masm);

// Generate a function with the following prototype:
//   void add2_vectors(int8_t *vec_a, const int8_t *vec_b, unsigned size)
//
// Demonstrate how to add two vectors using NEON. The result is stored in vec_a.
void GenerateAdd2Vectors(vixl::aarch64::MacroAssembler* masm);

// Generate a function with the following prototype:
//   double add3_double(double x, double y, double z)
//
// This example is intended to show the calling convention with double
// floating point arguments.
void GenerateAdd3Double(vixl::aarch64::MacroAssembler* masm);

// Generate a function with the following prototype:
//   double add4_double(uint64_t a, double b, uint64_t c, double d)
//
// The generated function pictures the calling convention for functions
// mixing integer and floating point arguments.
void GenerateAdd4Double(vixl::aarch64::MacroAssembler* masm);

// Generate a function with the following prototype:
//   uint32_t sum_array(uint8_t* array, uint32_t size)
//
// The generated function computes the sum of all the elements in
// the given array.
void GenerateSumArray(vixl::aarch64::MacroAssembler* masm);

// Generate a function with the following prototype:
//   int64_t abs(int64_t x)
//
// The generated function computes the absolute value of an integer.
void GenerateAbs(vixl::aarch64::MacroAssembler* masm);

// Generate a function with the following prototype:
//   uint64_t check_bounds(uint64_t value, uint64_t low, uint64_t high)
//
// The goal of this example is to illustrate the use of conditional
// instructions. The generated function will check that the given value is
// contained within the given boundaries. It returns 1 if 'value' is between
// 'low' and 'high' (ie. low <= value <= high).
void GenerateCheckBounds(vixl::aarch64::MacroAssembler* masm);

// Generate a function with the following prototype:
//   uint32_t crc32(const char *msg, size_t msg_length)
//
// The generated function computes the CRC-32 checksum on the input msg
// with specified length, and returns the result.
void GenerateCrc32(vixl::aarch64::MacroAssembler* masm);

// Generate a function which uses the stack to swap the content of the x0, x1,
// x2 and x3 registers.
void GenerateSwap4(vixl::aarch64::MacroAssembler* masm);

// Generate a function which swaps the content of w0 and w1.
// This example demonstrates some interesting features of VIXL's stack
// operations.
void GenerateSwapInt32(vixl::aarch64::MacroAssembler* masm);

// Generate a function with the following prototype:
//   uint64_t demo_function(uint64_t x)
//
// This is the example used in doc/getting-started-aarch64.txt
void GenerateDemoFunction(vixl::aarch64::MacroAssembler* masm);

// This function generates and runs code that uses literals to sum the `a` and
// `b` inputs.
int64_t LiteralExample(int64_t a, int64_t b);

// Generate a few examples of runtime calls.
void GenerateRuntimeCallExamples(vixl::aarch64::MacroAssembler* masm);

// Generate a function with the following prototype:
//    size_t sve_strlen(const char* str);
//
// The function implements the standard `strlen` using SVE.
void GenerateSVEStrlen(vixl::aarch64::MacroAssembler* masm);

#endif  // VIXL_EXAMPLE_EXAMPLES_H_
