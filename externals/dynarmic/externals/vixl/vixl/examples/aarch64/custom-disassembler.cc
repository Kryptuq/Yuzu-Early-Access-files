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

#include "custom-disassembler.h"
#include "examples.h"

using namespace vixl;
using namespace vixl::aarch64;

#define __ masm->


// We override this method to specify how register names should be disassembled.
void CustomDisassembler::AppendRegisterNameToOutput(const Instruction* instr,
                                                    const CPURegister& reg) {
  USE(instr);
  if (reg.IsRegister()) {
    switch (reg.GetCode()) {
      case 16:
        AppendToOutput(reg.Is64Bits() ? "ip0" : "wip0");
        return;
      case 17:
        AppendToOutput(reg.Is64Bits() ? "ip1" : "wip1");
        return;
      case 30:
        AppendToOutput(reg.Is64Bits() ? "lr" : "w30");
        return;
      case kSPRegInternalCode:
        AppendToOutput(reg.Is64Bits() ? "x_stack_pointer" : "w_stack_pointer");
        return;
      case 31:
        AppendToOutput(reg.Is64Bits() ? "x_zero_reg" : "w_zero_reg");
        return;
      default:
        // Fall through.
        break;
    }
  }
  // Print other register names as usual.
  Disassembler::AppendRegisterNameToOutput(instr, reg);
}


static const char* FakeLookupTargetDescription(const void* address) {
  USE(address);
  // We fake looking up the address.
  static int i = 0;
  const char* desc = NULL;
  if (i == 0) {
    desc = "label: somewhere";
  } else if (i == 2) {
    desc = "label: somewhere else";
  }
  i++;
  return desc;
}


// We override this method to add a description to addresses that we know about.
// In this example we fake looking up a description, but in practice one could
// for example use a table mapping addresses to function names.
void CustomDisassembler::AppendCodeRelativeCodeAddressToOutput(
    const Instruction* instr, const void* addr) {
  USE(instr);
  // Print the address.
  int64_t rel_addr = CodeRelativeAddress(addr);
  if (rel_addr >= 0) {
    AppendToOutput("(addr 0x%" PRIx64, rel_addr);
  } else {
    AppendToOutput("(addr -0x%" PRIx64, -rel_addr);
  }

  // If available, print a description of the address.
  const char* address_desc = FakeLookupTargetDescription(addr);
  if (address_desc != NULL) {
    Disassembler::AppendToOutput(" ; %s", address_desc);
  }
  AppendToOutput(")");
}


// We override this method to add a comment to this type of instruction. Helpers
// from the vixl::Instruction class can be used to analyse the instruction being
// disasssembled.
void CustomDisassembler::VisitAddSubShifted(const Instruction* instr) {
  vixl::aarch64::Disassembler::VisitAddSubShifted(instr);
  if (instr->GetRd() == 10) {
    AppendToOutput(" // add/sub to x10");
  }
  ProcessOutput(instr);
}


void GenerateCustomDisassemblerTestCode(MacroAssembler* masm) {
  // Generate some code to illustrate how the modified disassembler changes the
  // disassembly output.
  Label begin, end;
  __ Bind(&begin);
  __ Add(x10, x16, x17);
  __ Cbz(x10, &end);
  __ Add(x11, ip0, ip1);
  __ Add(w5, w6, w30);
  __ Tbz(x10, 2, &begin);
  __ Tbnz(x10, 3, &begin);
  __ Br(x30);
  __ Br(lr);
  __ Fadd(d30, d16, d17);
  __ Push(xzr, xzr);
  __ Pop(x16, x20);
  __ Bind(&end);
}


void TestCustomDisassembler() {
  MacroAssembler masm;

  // Generate the code.
  Label code_start, code_end;
  masm.Bind(&code_start);
  GenerateCustomDisassemblerTestCode(&masm);
  masm.Bind(&code_end);
  masm.FinalizeCode();
  Instruction* instr_start = masm.GetLabelAddress<Instruction*>(&code_start);
  Instruction* instr_end = masm.GetLabelAddress<Instruction*>(&code_end);

  // Instantiate a standard disassembler, our custom disassembler, and register
  // them with a decoder.
  Decoder decoder;
  Disassembler disasm;
  CustomDisassembler custom_disasm;
  decoder.AppendVisitor(&disasm);
  decoder.AppendVisitor(&custom_disasm);

  // In our custom disassembler, disassemble as if the base address was -0x8.
  // Note that this can also be achieved with
  //   custom_disasm.MapCodeAddress(0x0, instr_start + 2 * kInstructionSize);
  // Users may generally want to map the start address to 0x0. Mapping to a
  // negative offset can be used to focus on the section of the
  // disassembly at address 0x0.
  custom_disasm.MapCodeAddress(-0x8, instr_start);

  // Iterate through the instructions to show the difference in the disassembly.
  Instruction* instr;
  for (instr = instr_start; instr < instr_end; instr += kInstructionSize) {
    decoder.Decode(instr);
    printf("\n");
    printf("VIXL disasm\t %p:\t%s\n",
           reinterpret_cast<void*>(instr),
           disasm.GetOutput());
    int64_t rel_addr =
        custom_disasm.CodeRelativeAddress(reinterpret_cast<void*>(instr));
    char rel_addr_sign_char = ' ';
    if (rel_addr < 0) {
      rel_addr_sign_char = '-';
      rel_addr = -rel_addr;
    }
    printf("custom disasm\t%c0x%" PRIx64 ":\t%s\n",
           rel_addr_sign_char,
           rel_addr,
           custom_disasm.GetOutput());
  }
}


#ifndef TEST_EXAMPLES
int main() {
  TestCustomDisassembler();
  return 0;
}
#endif
