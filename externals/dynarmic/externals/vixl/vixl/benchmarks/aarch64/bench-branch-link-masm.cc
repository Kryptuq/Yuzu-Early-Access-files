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

// This program focuses on the emission of branches and veneers.
int main(int argc, char* argv[]) {
  BenchCLI cli(argc, argv);
  if (cli.ShouldExitEarly()) return cli.GetExitCode();

  const size_t buffer_size = 256 * KBytes;
  const size_t instructions_per_iteration = 4;
  const size_t max_buffer_iterations =
      buffer_size / (instructions_per_iteration * kInstructionSize);
  MacroAssembler masm(buffer_size);

  BenchTimer timer;

  size_t iterations = 0;
  do {
    masm.Reset();

    Label target_1, target_2, target_3, target_4;
    for (size_t i = 0; i < max_buffer_iterations; i++) {
      masm.B(&target_1);
    }
    masm.Bind(&target_1);
    for (size_t i = 0; i < max_buffer_iterations; i++) {
      masm.B(eq, &target_2);
    }
    masm.Bind(&target_2);
    for (size_t i = 0; i < max_buffer_iterations; i++) {
      masm.Cbz(x2, &target_3);
    }
    masm.Bind(&target_3);
    for (size_t i = 0; i < max_buffer_iterations; i++) {
      masm.Tbz(x3, 2, &target_4);
    }
    masm.Bind(&target_4);

    masm.FinalizeCode();
    iterations++;
  } while (!timer.HasRunFor(cli.GetRunTimeInSeconds()));

  cli.PrintResults(iterations, timer.GetElapsedSeconds());
  return cli.GetExitCode();
}
