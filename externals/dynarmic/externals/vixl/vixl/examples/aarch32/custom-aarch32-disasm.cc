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

#include <iostream>
#include <map>
#include <string>

#include "aarch32/constants-aarch32.h"
#include "aarch32/disasm-aarch32.h"
#include "aarch32/instructions-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"

#define __ masm.

namespace vixl {
namespace aarch32 {

// Example of disassembly customization.

class CustomStream : public Disassembler::DisassemblerStream {
  std::map<Location::Offset, const char*> symbols_;

 public:
  CustomStream() : Disassembler::DisassemblerStream(std::cout) {}
  std::map<Location::Offset, const char*>& GetSymbols() { return symbols_; }
  virtual DisassemblerStream& operator<<(const Disassembler::PrintLabel& label)
      VIXL_OVERRIDE {
    std::map<Location::Offset, const char*>::iterator symbol =
        symbols_.find(label.GetLocation());
    // If the label was named, print the name instead of the address.
    if (symbol != symbols_.end()) {
      os() << symbol->second;
      return *this;
    }
    os() << label;
    return *this;
  }
  virtual DisassemblerStream& operator<<(const Register reg) VIXL_OVERRIDE {
    // Print all the core registers with an upper-case letter instead of the
    // default lower-case.
    os() << "R" << reg.GetCode();
    return *this;
  }
};

class CustomDisassembler : public PrintDisassembler {
 public:
  explicit CustomDisassembler(CustomStream* stream)
      : PrintDisassembler(stream) {}
  CustomStream* GetStream() const {
    return reinterpret_cast<CustomStream*>(&os());
  }
  virtual void PrintCodeAddress(uint32_t pc) VIXL_OVERRIDE {
    // If the address matches a label, then print the label. Otherwise, print
    // nothing.
    std::map<Location::Offset, const char*>::iterator symbol =
        GetStream()->GetSymbols().find(pc);
    if (symbol != GetStream()->GetSymbols().end()) {
      os().os() << symbol->second << ":" << std::endl;
    }
    // Add indentation for instructions.
    os() << "  ";
  }
  virtual void PrintOpcode16(uint32_t opcode) VIXL_OVERRIDE {
    // Do not print opcodes.
    USE(opcode);
  }
  virtual void PrintOpcode32(uint32_t opcode) VIXL_OVERRIDE {
    // Do not print opcodes.
    USE(opcode);
  }
};

class NamedLabel : public Label {
  CustomStream* stream_;
  const char* name_;

 public:
  NamedLabel(CustomStream* stream, const char* name)
      : stream_(stream), name_(name) {}
  ~NamedLabel() {
    if (IsBound()) {
      stream_->GetSymbols().insert(
          std::pair<Location::Offset, const char*>(GetLocation(), name_));
    }
  }
};

void RunCustomDisassemblerTest() {
  CustomStream stream;
  MacroAssembler masm;
  {
    NamedLabel loop(&stream, "loop");
    NamedLabel end(&stream, "end");
    __ Mov(r0, 0);
    __ Mov(r1, 0);
    __ Bind(&loop);
    __ Cmp(r1, 20);
    __ B(gt, &end);
    __ Add(r0, r0, r1);
    __ Add(r1, r1, 1);
    __ B(&loop);
    __ Bind(&end);
    __ Bx(lr);
    __ FinalizeCode();
  }
  std::cout << "Custom disassembly:" << std::endl;
  CustomDisassembler custom_disassembler(&stream);
  custom_disassembler
      .DisassembleA32Buffer(masm.GetBuffer()->GetOffsetAddress<uint32_t*>(0),
                            masm.GetBuffer()->GetSizeInBytes());
  std::cout << std::endl;
  std::cout << "Standard disassembly:" << std::endl;
  PrintDisassembler print_disassembler(std::cout);
  print_disassembler
      .DisassembleA32Buffer(masm.GetBuffer()->GetOffsetAddress<uint32_t*>(0),
                            masm.GetBuffer()->GetSizeInBytes());
}

}  // namespace aarch32
}  // namespace vixl

#ifndef TEST_EXAMPLES
int main() {
  vixl::aarch32::RunCustomDisassemblerTest();
  return 0;
}
#endif  // TEST_EXAMPLES
