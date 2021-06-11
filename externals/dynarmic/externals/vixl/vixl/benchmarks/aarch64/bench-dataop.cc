// Copyright 2019, VIXL authors
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

#include "globals-vixl.h"

#include "aarch64/instructions-aarch64.h"
#include "aarch64/macro-assembler-aarch64.h"

#include "bench-utils.h"

using namespace vixl;
using namespace vixl::aarch64;

// This program focuses on emitting simple instructions.
//
// This code will emit a given number of 'add x0, x1, x2' in a fixed size
// buffer, looping over the buffer if necessary. This code therefore focuses
// on Emit and Operand.
int main(int argc, char* argv[]) {
  BenchCLI cli(argc, argv);
  if (cli.ShouldExitEarly()) return cli.GetExitCode();

  const size_t buffer_size = 256 * KBytes;
  const size_t buffer_instruction_count = buffer_size / kInstructionSize;
  MacroAssembler masm(buffer_size);

  BenchTimer timer;

  size_t iterations = 0;
  do {
    masm.Reset();
    {
      ExactAssemblyScope scope(&masm, buffer_size);
      for (size_t i = 0; i < buffer_instruction_count; ++i) {
        masm.add(x0, x1, Operand(x2));
      }
    }
    masm.FinalizeCode();
    iterations++;
  } while (!timer.HasRunFor(cli.GetRunTimeInSeconds()));

  cli.PrintResults(iterations, timer.GetElapsedSeconds());
  return cli.GetExitCode();
}
