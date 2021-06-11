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

#include "examples.h"

using namespace vixl;
using namespace vixl::aarch32;

#define __ masm->

void GenerateMandelBrot(MacroAssembler* masm) {
  const QRegister kCReal = q0;
  const QRegister kCImag = q1;

  const QRegister kCRealStep = q13;
  const QRegister kCImagStep = q14;

  const QRegister kModSqLimit = q15;

  // Save register values.
  __ Push(RegisterList(r4, r5, r6));

  __ Vmov(F32, kCRealStep, 0.125);
  __ Vmov(F32, kCImagStep, 0.0625);

  const Register kZero = r2;
  __ Mov(kZero, 0);

  const DRegister kStars = d6;
  const DRegister kSpaces = d7;
  // Output characters - packed 4 characters into 32 bits.
  __ Vmov(I8, kStars, '*');
  __ Vmov(I8, kSpaces, ' ');

  const DRegisterLane kNegTwo = DRegisterLane(d7, 1);
  __ Vmov(s15, -2.0);

  // Imaginary part of c.
  __ Vdup(Untyped32, kCImag, kNegTwo);

  // Max modulus squared.
  __ Vmov(F32, kModSqLimit, 4.0);

  // Height of output in characters.
  __ Mov(r4, 64);

  // String length will be 129, so need 132 bytes of space.
  const uint32_t kStringLength = 132;

  // Make space for our string.
  __ Sub(sp, sp, kStringLength);

  // Set up a starting pointer for the string.
  const Register kStringPtr = r6;
  __ Mov(kStringPtr, sp);

  // Loop over imaginary values of c from -2 to 2, taking
  // 64 equally spaced values in the range.
  {
    Label c_imag_loop;

    __ Bind(&c_imag_loop);

    // Real part of c.
    // Store 4 equally spaced values in q0 (kCReal) to use SIMD.
    __ Vmov(s0, -2.0);
    __ Vmov(s1, -1.96875);
    __ Vmov(s2, -1.9375);
    __ Vmov(s3, -1.90625);

    // Width of output in terms of sets of 4 characters - twice that
    // of height to compensate for ratio of character height to width.
    __ Mov(r5, 32);

    const Register kWriteCursor = r3;
    // Set a cursor ready to write the next line.
    __ Mov(kWriteCursor, kStringPtr);

    // Loop over real values of c from -2 to 2, processing
    // 4 different values simultaneously using SIMD.
    {
      const QRegister kFlags = q2;
      const DRegister kLowerFlags = d4;

      Label c_real_loop;
      __ Bind(&c_real_loop);

      // Get number of iterations.
      __ Add(r1, r0, 1);

      // Perform the iterations of z(n+1) = zn^2 + c using SIMD.
      // If the result is that c is in the set, the element of
      // kFlags will be 0, else ~0.
      {
        const QRegister kZReal = q8;
        const QRegister kZImag = q9;

        // Real part of z.
        __ Vmov(F32, kZReal, 0.0);

        // Imaginary part of z.
        __ Vmov(F32, kZImag, 0.0);

        __ Vmov(F32, kFlags, 0.0);

        Label iterative_formula_start, iterative_formula_end;
        __ Bind(&iterative_formula_start);
        __ Subs(r1, r1, 1);
        __ B(le, &iterative_formula_end);

        // z(n+1) = zn^2 + c.
        // re(z(n+1)) = re(c) + re(zn)^2 - im(zn)^2.
        // im(z(n+1)) = im(c) + 2 * re(zn) * im(zn)

        __ Vmul(F32, q10, kZReal, kZImag);  // re(zn) * im(zn)

        __ Vmul(F32, kZReal, kZReal, kZReal);  // re(zn)^2
        __ Vadd(F32, kZReal, kCReal, kZReal);  // re(c) + re(zn)^2
        __ Vmls(F32, kZReal, kZImag, kZImag);  // re(c) + re(zn)^2 - im(zn)^2

        __ Vmov(F32, kZImag, kCImag);        // im(c)
        __ Vmls(F32, kZImag, q10, kNegTwo);  // im(c) + 2 * re(zn) * im(zn)

        __ Vmul(F32, q10, kZReal, kZReal);    // re(z(n+1))^2
        __ Vmla(F32, q10, kZImag, kZImag);    // re(z(n+1))^2 + im(z(n+1))^2
        __ Vcgt(F32, q10, q10, kModSqLimit);  // |z(n+1)|^2 > 4 ? ~0 : 0
        __ Vorr(F32, kFlags, kFlags, q10);    // (~0/0) | above result

        __ B(&iterative_formula_start);
        __ Bind(&iterative_formula_end);
      }

      // Narrow twice so that each mask is 8 bits, packed into
      // a single 32 bit register s4.
      // kLowerFlags is the lower half of kFlags, so the second narrow will
      // be working on the results of the first to halve the size of each
      // representation again.
      __ Vmovn(I32, kLowerFlags, kFlags);
      __ Vmovn(I16, kLowerFlags, kFlags);

      // '*' if in set, ' ' if not.
      __ Vbsl(Untyped32, kLowerFlags, kSpaces, kStars);

      // Add this to the string.
      __ Vst1(Untyped32,
              NeonRegisterList(kLowerFlags, 0),
              AlignedMemOperand(kWriteCursor, k32BitAlign, PostIndex));

      // Increase real part of c.
      __ Vadd(F32, kCReal, kCReal, kCRealStep);

      __ Subs(r5, r5, 1);
      __ B(ne, &c_real_loop);
    }

    // Put terminating character.
    __ Strb(kZero, MemOperand(kWriteCursor));

    // Print the string.
    __ Printf("%s\n", kStringPtr);

    // Increase imaginary part of c.
    __ Vadd(F32, kCImag, kCImag, kCImagStep);

    __ Subs(r4, r4, 1);
    __ B(ne, &c_imag_loop);
  }
  // Restore stack pointer.
  __ Add(sp, sp, kStringLength);
  // Restore register values.
  __ Pop(RegisterList(r4, r5, r6));
  __ Bx(lr);
}

#ifndef TEST_EXAMPLES
int main() {
  MacroAssembler masm;
  // Generate the code for the example function.
  Label mandelbrot;
  masm.Bind(&mandelbrot);
  GenerateMandelBrot(&masm);
  masm.FinalizeCode();
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH32
  // There is no simulator defined for VIXL AArch32.
  printf("This example cannot be simulated\n");
#else
  byte* code = masm.GetBuffer()->GetStartAddress<byte*>();
  uint32_t code_size = masm.GetSizeOfCodeGenerated();
  ExecutableMemory memory(code, code_size);
  // Run the example function.
  double (*mandelbrot_func)(uint32_t) =
      memory.GetEntryPoint<double (*)(uint32_t)>(mandelbrot,
                                                 masm.GetInstructionSetInUse());
  uint32_t iterations = 1000;
  (*mandelbrot_func)(iterations);
#endif
  return 0;
}
#endif  // TEST_EXAMPLES
