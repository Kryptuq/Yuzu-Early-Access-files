// Copyright 2020, VIXL authors
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code-buffer-vixl.h"
#include "aarch64/decoder-aarch64.h"
#include "aarch64/disasm-aarch64.h"

// This example is interactive, and isn't tested systematically.
#ifndef TEST_EXAMPLES

using namespace vixl;
using namespace vixl::aarch64;

void PrintUsage(char const* name) {
  printf("Usage: %s [OPTION]... <INSTRUCTION>...\n", name);
  printf("\n");
  printf("Disassemble ad-hoc A64 instructions.\n");
  printf("\n");
  printf(
      "Options:\n"
      "  --start-at <address>\n"
      "    Start disassembling from <address> Any signed 64-bit value\n"
      "    accepted by strtoll can be specified. The address is printed\n"
      "    alongside each instruction, and it is also used to decode\n"
      "    PC-relative offsets.\n"
      "\n"
      "    Defaults to 0.\n"
      "\n");
  printf(
      "<instruction>\n"
      "  A hexadecimal representation of an A64 instruction. The leading '0x'\n"
      "  (or '0X') is optional.\n"
      "\n"
      "  Multiple instructions can be provided; they will be disassembled as\n"
      "  if they were read sequentially from memory.\n"
      "\n");
  printf("Examples:\n");
  printf("  $ %s d2824685\n", name);
  printf("   0x0000000000000000:  d2824685  movz x5, #0x1234\n");
  printf("\n");
  printf("  $ %s --start-at -4 0x10fffe85 0xd61f00a0\n", name);
  printf("  -0x0000000000000004:  10fffe85  adr x5, #-0x30 (addr -0x34)\n");
  printf("   0x0000000000000000:  d61f00a0  br x5\n");
}

Instr ParseInstr(char const* arg) {
  // TODO: Error handling for out-of-range inputs.
  return (Instr)strtoul(arg, NULL, 16);
}

int64_t ParseInt64(char const* arg) {
  // TODO: Error handling for out-of-range inputs.
  return (int64_t)strtoll(arg, NULL, 0);
}

int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; i++) {
    char const* arg = argv[i];
    if ((strcmp(arg, "--help") == 0) || (strcmp(arg, "-h") == 0)) {
      PrintUsage(argv[0]);
      return 0;
    }
  }

  // Assume an address of 0, unless otherwise specified.
  int64_t start_address = 0;
  // Allocate space for one instruction per argument.
  CodeBuffer buffer((argc - 1) * kInstructionSize);

  bool expect_start_at = false;
  for (int i = 1; i < argc; i++) {
    char* arg = argv[i];
    if (expect_start_at) {
      start_address = ParseInt64(arg);
      expect_start_at = false;
    } else if (strcmp(arg, "--start-at") == 0) {
      expect_start_at = true;
    } else {
      // Assume that everything else is an instruction.
      buffer.Emit(ParseInstr(arg));
    }
  }
  buffer.SetClean();

  if (expect_start_at) {
    printf("No address given. Use: --start-at <address>\n");
    return 1;
  }

  if (buffer.GetSizeInBytes() == 0) {
    printf("Nothing to disassemble.\n");
    return 0;
  }

  // Disassemble the buffer.
  const Instruction* start = buffer.GetStartAddress<Instruction*>();
  const Instruction* end = buffer.GetEndAddress<Instruction*>();
  vixl::aarch64::PrintDisassembler disasm(stdout);
  disasm.PrintSignedAddresses(true);
  disasm.MapCodeAddress(start_address, start);
  disasm.DisassembleBuffer(start, end);

  return 0;
}

#endif  // TEST_EXAMPLES
