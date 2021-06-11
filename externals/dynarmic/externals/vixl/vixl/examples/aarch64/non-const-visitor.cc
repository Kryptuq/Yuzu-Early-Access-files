// Copyright 2014, VIXL authors
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

#include "non-const-visitor.h"
#include "examples.h"

using namespace vixl;
using namespace vixl::aarch64;

#define __ masm->


void SwitchAddSubRegisterSources::VisitAddSubShifted(const Instruction* instr) {
  int rn = instr->GetRn();
  int rm = instr->GetRm();
  // Only non-const visitors are allowed to discard constness of the visited
  // instruction.
  Instruction* mutable_instr = MutableInstruction(instr);
  Instr instr_bits = mutable_instr->GetInstructionBits();

  // Switch the bitfields for the `rn` and `rm` registers.
  instr_bits &= ~(Rn_mask | Rm_mask);
  instr_bits |= (rn << Rm_offset) | (rm << Rn_offset);

  // Rewrite the instruction.
  mutable_instr->SetInstructionBits(instr_bits);
}


void GenerateNonConstVisitorTestCode(MacroAssembler* masm) {
  // int64_t foo(int64_t a, int64_t b)
  //  Argument locations:
  //    a -> x0
  //    b -> x1
  __ Sub(x0, x0, x1);
  // The return value is in x0.
  __ Ret();
}


int64_t RunNonConstVisitorTestGeneratedCode(const Instruction* start_instr) {
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
  Decoder simulator_decoder;
  Simulator simulator(&simulator_decoder);

  int64_t a = 5;
  int64_t b = 2;
  simulator.WriteXRegister(0, a);
  simulator.WriteXRegister(1, b);
  simulator.RunFrom(start_instr);
  int64_t res = simulator.ReadXRegister(0);
  printf("foo(%" PRId64 ", %" PRId64 ") = %" PRId64 "\n", a, b, res);

  return res;
#else
  // Without the simulator there is nothing to test.
  USE(start_instr);
  return 0;
#endif
}


#ifndef TEST_EXAMPLES
#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
int main(void) {
  MacroAssembler masm;

  // Generate the code.
  Label code_start, code_end;
  masm.Bind(&code_start);
  GenerateNonConstVisitorTestCode(&masm);
  masm.Bind(&code_end);
  masm.FinalizeCode();
  Instruction* instr_start = masm.GetLabelAddress<Instruction*>(&code_start);
  Instruction* instr_end = masm.GetLabelAddress<Instruction*>(&code_end);

  // Run the code a first time.
  RunNonConstVisitorTestGeneratedCode(instr_start);

  // Instantiate a decoder, disassembler, and our custom modifying visitor.
  Decoder decoder;
  PrintDisassembler disasm(stdout);
  SwitchAddSubRegisterSources modifying_visitor;

  // Register visitors in such a way that when visiting instructions, the
  // decoder will first disassemble the original instruction, modify it, and
  // then disassemble the modified instruction.
  decoder.AppendVisitor(&disasm);
  decoder.AppendVisitor(&modifying_visitor);
  decoder.AppendVisitor(&disasm);

  // Iterate through the instructions.
  Instruction* instr;
  for (instr = instr_start; instr < instr_end; instr += kInstructionSize) {
    printf("---\n");
    decoder.Decode(instr);
  }

  // Run the modified code and observe the different output from before.
  RunNonConstVisitorTestGeneratedCode(instr_start);

  return 0;
}
#else
// Without the simulator there is nothing to test.
int main(void) { return 0; }
#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
#endif  // TEST_EXAMPLES


// This is only used by the testing code.
void ModifyNonConstVisitorTestGeneratedCode(Instruction* start,
                                            Instruction* end) {
  Decoder decoder;
  SwitchAddSubRegisterSources modifying_visitor;
  decoder.AppendVisitor(&modifying_visitor);

  Instruction* instr;
  for (instr = start; instr < end; instr += kInstructionSize) {
    printf("---\n");
    decoder.Decode(instr);
  }
}
