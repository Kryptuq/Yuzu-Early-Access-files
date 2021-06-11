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

#include "examples.h"

#include "aarch64/disasm-aarch64.h"
#include "aarch64/macro-assembler-aarch64.h"
#include "aarch64/simulator-aarch64.h"

using namespace vixl;
using namespace vixl::aarch64;

#ifdef VIXL_HAS_SIMULATED_RUNTIME_CALL_SUPPORT

#define __ masm->

int32_t add_int32s(int32_t a, int32_t b) {
  printf("add_int32s(%d, %d)\n", a, b);
  return a + b;
}

float int32_to_float(int32_t value) {
  printf("int32_to_float(%d)\n", value);
  return static_cast<float>(value);
}

int32_t float_to_int32(float value) {
  printf("float_to_int32(%f)\n", value);
  return static_cast<int32_t>(value);
}

void overload_function(float value) {
  printf("overload_function(float %f)\n", value);
}

void overload_function(int32_t value) {
  printf("overload_function(int32_t %d)\n", value);
}

void GenerateRuntimeCallExamples(MacroAssembler* masm) {
  // Preserve lr, since the calls will overwrite it.
  __ Push(xzr, lr);
  // The arguments are expected in the appropriate registers (following the
  // Aarch64 ABI).
  __ CallRuntime(add_int32s);
  // The result is in `w0`, as per the ABI.
  // Of course, one can use assembly to work with the results.
  __ Lsl(w0, w0, 2);
  __ CallRuntime(int32_to_float);
  // The result is in `s0`.
  // `CallRuntime` can infer template arguments when the call is not ambiguous.
  // Otherwise the template arguments must be specified in the form:
  // `__ CallRuntime<return_type, parameter_type_0, parameter_type_1, ...>();`
  __ CallRuntime<void, float>(overload_function);
  __ CallRuntime(float_to_int32);
  __ CallRuntime<void, int32_t>(overload_function);
  // Restore lr and return.
  __ Pop(lr, xzr);
  __ Ret();
}


#ifndef TEST_EXAMPLES
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64

int main(void) {
  MacroAssembler masm;

  // Generate the code for the example function.
  Label call_runtime_add_floats;
  masm.Bind(&call_runtime_add_floats);
  GenerateRuntimeCallExamples(&masm);
  masm.FinalizeCode();

  Instruction* start =
      masm.GetLabelAddress<Instruction*>(&call_runtime_add_floats);

  // Disassemble the generated code.
  PrintDisassembler disassembler(stdout);
  disassembler.DisassembleBuffer(start, masm.GetSizeOfCodeGenerated());

  Decoder decoder;
  Simulator simulator(&decoder);

  int32_t input_a = 1;
  int32_t input_b = 2;
  simulator.WriteWRegister(0, input_a);
  simulator.WriteWRegister(1, input_b);
  simulator.RunFrom(start);
  printf("The final result is %d\n", simulator.ReadWRegister(0));

  return 0;
}
#else
// TODO: Support running natively.
int main(void) { return 0; }
#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
#endif  // TEST_EXAMPLES
#else
#ifndef TEST_EXAMPLES
int main(void) { return 0; }
#endif  // TEST_EXAMPLES
#endif  // VIXL_HAS_SIMULATED_RUNTIME_CALL_SUPPORT
