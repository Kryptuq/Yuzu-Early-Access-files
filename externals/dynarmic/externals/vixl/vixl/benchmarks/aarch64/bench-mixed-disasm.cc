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

// Like PrintDisassembler, but to prevent the I/O overhead from dominating the
// benchmark, don't actually print anything.
class BenchDisassembler : public Disassembler {
 public:
  BenchDisassembler() : Disassembler(), generated_chars_(0) {}

  size_t GetGeneratedCharCount() const { return generated_chars_; }

 protected:
  virtual void ProcessOutput(const Instruction* instr) VIXL_OVERRIDE {
    USE(instr);
    generated_chars_ += strlen(GetOutput());
  }

  size_t generated_chars_;
};

// This program measures the performance of the disassembler, using the same
// code sequence used in bench-mixed-masm.cc.
int main(int argc, char* argv[]) {
  BenchCLI cli(argc, argv);
  if (cli.ShouldExitEarly()) return cli.GetExitCode();

  const size_t buffer_size = 256 * KBytes;
  MacroAssembler masm(buffer_size);
  masm.SetCPUFeatures(CPUFeatures::All());
  BenchCodeGenerator generator(&masm);

  masm.Reset();
  generator.Generate(buffer_size);
  masm.FinalizeCode();

  const Instruction* start =
      masm.GetBuffer()->GetStartAddress<const Instruction*>();
  const Instruction* end =
      masm.GetBuffer()->GetEndAddress<const Instruction*>();

  Decoder decoder;
  BenchDisassembler disasm;
  decoder.AppendVisitor(&disasm);

  BenchTimer timer;

  size_t iterations = 0;
  size_t generated_chars = 0;
  do {
    decoder.Decode(start, end);
    generated_chars += disasm.GetGeneratedCharCount();

    iterations++;
  } while (!timer.HasRunFor(cli.GetRunTimeInSeconds()));

  printf("Disassembled %" PRIu64 " characters.\n",
         static_cast<uint64_t>(generated_chars));
  cli.PrintResults(iterations, timer.GetElapsedSeconds());
  return cli.GetExitCode();
}
