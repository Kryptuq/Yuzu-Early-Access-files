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


// -----------------------------------------------------------------------------
// This file is auto generated from the
// test/aarch32/config/template-assembler-aarch32.cc.in template file using
// tools/generate_tests.py.
//
// PLEASE DO NOT EDIT.
// -----------------------------------------------------------------------------


#include "test-runner.h"

#include "test-utils.h"
#include "test-utils-aarch32.h"

#include "aarch32/assembler-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"

#define BUF_SIZE (4096)

namespace vixl {
namespace aarch32 {

// List of instruction mnemonics.
#define FOREACH_INSTRUCTION(M) M(add)


// The following definitions are defined again in each generated test, therefore
// we need to place them in an anomymous namespace. It expresses that they are
// local to this file only, and the compiler is not allowed to share these types
// across test files during template instantiation. Specifically, `Operands` has
// various layouts across generated tests so it absolutely cannot be shared.

#ifdef VIXL_INCLUDE_TARGET_T32
namespace {

// Values to be passed to the assembler to produce the instruction under test.
struct Operands {
  Condition cond;
  Register rd;
  Register rn;
  uint32_t immediate;
};

// This structure contains all data needed to test one specific
// instruction.
struct TestData {
  // The `operands` field represents what to pass to the assembler to
  // produce the instruction.
  Operands operands;
  // True if we need to generate an IT instruction for this test to be valid.
  bool in_it_block;
  // The condition to give the IT instruction, this will be set to "al" by
  // default.
  Condition it_condition;
  // Description of the operands, used for error reporting.
  const char* operands_description;
  // Unique identifier, used for generating traces.
  const char* identifier;
};

struct TestResult {
  size_t size;
  const byte* encoding;
};

// Each element of this array produce one instruction encoding.
const TestData kTests[] =
    {{{al, r0, sp, 0x0}, false, al, "al r0 sp 0x0", "al_r0_sp_0x0"},
     {{al, r0, sp, 0x4}, false, al, "al r0 sp 0x4", "al_r0_sp_0x4"},
     {{al, r0, sp, 0x8}, false, al, "al r0 sp 0x8", "al_r0_sp_0x8"},
     {{al, r0, sp, 0xc}, false, al, "al r0 sp 0xc", "al_r0_sp_0xc"},
     {{al, r0, sp, 0x10}, false, al, "al r0 sp 0x10", "al_r0_sp_0x10"},
     {{al, r0, sp, 0x14}, false, al, "al r0 sp 0x14", "al_r0_sp_0x14"},
     {{al, r0, sp, 0x18}, false, al, "al r0 sp 0x18", "al_r0_sp_0x18"},
     {{al, r0, sp, 0x1c}, false, al, "al r0 sp 0x1c", "al_r0_sp_0x1c"},
     {{al, r0, sp, 0x20}, false, al, "al r0 sp 0x20", "al_r0_sp_0x20"},
     {{al, r0, sp, 0x24}, false, al, "al r0 sp 0x24", "al_r0_sp_0x24"},
     {{al, r0, sp, 0x28}, false, al, "al r0 sp 0x28", "al_r0_sp_0x28"},
     {{al, r0, sp, 0x2c}, false, al, "al r0 sp 0x2c", "al_r0_sp_0x2c"},
     {{al, r0, sp, 0x30}, false, al, "al r0 sp 0x30", "al_r0_sp_0x30"},
     {{al, r0, sp, 0x34}, false, al, "al r0 sp 0x34", "al_r0_sp_0x34"},
     {{al, r0, sp, 0x38}, false, al, "al r0 sp 0x38", "al_r0_sp_0x38"},
     {{al, r0, sp, 0x3c}, false, al, "al r0 sp 0x3c", "al_r0_sp_0x3c"},
     {{al, r0, sp, 0x40}, false, al, "al r0 sp 0x40", "al_r0_sp_0x40"},
     {{al, r0, sp, 0x44}, false, al, "al r0 sp 0x44", "al_r0_sp_0x44"},
     {{al, r0, sp, 0x48}, false, al, "al r0 sp 0x48", "al_r0_sp_0x48"},
     {{al, r0, sp, 0x4c}, false, al, "al r0 sp 0x4c", "al_r0_sp_0x4c"},
     {{al, r0, sp, 0x50}, false, al, "al r0 sp 0x50", "al_r0_sp_0x50"},
     {{al, r0, sp, 0x54}, false, al, "al r0 sp 0x54", "al_r0_sp_0x54"},
     {{al, r0, sp, 0x58}, false, al, "al r0 sp 0x58", "al_r0_sp_0x58"},
     {{al, r0, sp, 0x5c}, false, al, "al r0 sp 0x5c", "al_r0_sp_0x5c"},
     {{al, r0, sp, 0x60}, false, al, "al r0 sp 0x60", "al_r0_sp_0x60"},
     {{al, r0, sp, 0x64}, false, al, "al r0 sp 0x64", "al_r0_sp_0x64"},
     {{al, r0, sp, 0x68}, false, al, "al r0 sp 0x68", "al_r0_sp_0x68"},
     {{al, r0, sp, 0x6c}, false, al, "al r0 sp 0x6c", "al_r0_sp_0x6c"},
     {{al, r0, sp, 0x70}, false, al, "al r0 sp 0x70", "al_r0_sp_0x70"},
     {{al, r0, sp, 0x74}, false, al, "al r0 sp 0x74", "al_r0_sp_0x74"},
     {{al, r0, sp, 0x78}, false, al, "al r0 sp 0x78", "al_r0_sp_0x78"},
     {{al, r0, sp, 0x7c}, false, al, "al r0 sp 0x7c", "al_r0_sp_0x7c"},
     {{al, r0, sp, 0x80}, false, al, "al r0 sp 0x80", "al_r0_sp_0x80"},
     {{al, r0, sp, 0x84}, false, al, "al r0 sp 0x84", "al_r0_sp_0x84"},
     {{al, r0, sp, 0x88}, false, al, "al r0 sp 0x88", "al_r0_sp_0x88"},
     {{al, r0, sp, 0x8c}, false, al, "al r0 sp 0x8c", "al_r0_sp_0x8c"},
     {{al, r0, sp, 0x90}, false, al, "al r0 sp 0x90", "al_r0_sp_0x90"},
     {{al, r0, sp, 0x94}, false, al, "al r0 sp 0x94", "al_r0_sp_0x94"},
     {{al, r0, sp, 0x98}, false, al, "al r0 sp 0x98", "al_r0_sp_0x98"},
     {{al, r0, sp, 0x9c}, false, al, "al r0 sp 0x9c", "al_r0_sp_0x9c"},
     {{al, r0, sp, 0xa0}, false, al, "al r0 sp 0xa0", "al_r0_sp_0xa0"},
     {{al, r0, sp, 0xa4}, false, al, "al r0 sp 0xa4", "al_r0_sp_0xa4"},
     {{al, r0, sp, 0xa8}, false, al, "al r0 sp 0xa8", "al_r0_sp_0xa8"},
     {{al, r0, sp, 0xac}, false, al, "al r0 sp 0xac", "al_r0_sp_0xac"},
     {{al, r0, sp, 0xb0}, false, al, "al r0 sp 0xb0", "al_r0_sp_0xb0"},
     {{al, r0, sp, 0xb4}, false, al, "al r0 sp 0xb4", "al_r0_sp_0xb4"},
     {{al, r0, sp, 0xb8}, false, al, "al r0 sp 0xb8", "al_r0_sp_0xb8"},
     {{al, r0, sp, 0xbc}, false, al, "al r0 sp 0xbc", "al_r0_sp_0xbc"},
     {{al, r0, sp, 0xc0}, false, al, "al r0 sp 0xc0", "al_r0_sp_0xc0"},
     {{al, r0, sp, 0xc4}, false, al, "al r0 sp 0xc4", "al_r0_sp_0xc4"},
     {{al, r0, sp, 0xc8}, false, al, "al r0 sp 0xc8", "al_r0_sp_0xc8"},
     {{al, r0, sp, 0xcc}, false, al, "al r0 sp 0xcc", "al_r0_sp_0xcc"},
     {{al, r0, sp, 0xd0}, false, al, "al r0 sp 0xd0", "al_r0_sp_0xd0"},
     {{al, r0, sp, 0xd4}, false, al, "al r0 sp 0xd4", "al_r0_sp_0xd4"},
     {{al, r0, sp, 0xd8}, false, al, "al r0 sp 0xd8", "al_r0_sp_0xd8"},
     {{al, r0, sp, 0xdc}, false, al, "al r0 sp 0xdc", "al_r0_sp_0xdc"},
     {{al, r0, sp, 0xe0}, false, al, "al r0 sp 0xe0", "al_r0_sp_0xe0"},
     {{al, r0, sp, 0xe4}, false, al, "al r0 sp 0xe4", "al_r0_sp_0xe4"},
     {{al, r0, sp, 0xe8}, false, al, "al r0 sp 0xe8", "al_r0_sp_0xe8"},
     {{al, r0, sp, 0xec}, false, al, "al r0 sp 0xec", "al_r0_sp_0xec"},
     {{al, r0, sp, 0xf0}, false, al, "al r0 sp 0xf0", "al_r0_sp_0xf0"},
     {{al, r0, sp, 0xf4}, false, al, "al r0 sp 0xf4", "al_r0_sp_0xf4"},
     {{al, r0, sp, 0xf8}, false, al, "al r0 sp 0xf8", "al_r0_sp_0xf8"},
     {{al, r0, sp, 0xfc}, false, al, "al r0 sp 0xfc", "al_r0_sp_0xfc"},
     {{al, r0, sp, 0x100}, false, al, "al r0 sp 0x100", "al_r0_sp_0x100"},
     {{al, r0, sp, 0x104}, false, al, "al r0 sp 0x104", "al_r0_sp_0x104"},
     {{al, r0, sp, 0x108}, false, al, "al r0 sp 0x108", "al_r0_sp_0x108"},
     {{al, r0, sp, 0x10c}, false, al, "al r0 sp 0x10c", "al_r0_sp_0x10c"},
     {{al, r0, sp, 0x110}, false, al, "al r0 sp 0x110", "al_r0_sp_0x110"},
     {{al, r0, sp, 0x114}, false, al, "al r0 sp 0x114", "al_r0_sp_0x114"},
     {{al, r0, sp, 0x118}, false, al, "al r0 sp 0x118", "al_r0_sp_0x118"},
     {{al, r0, sp, 0x11c}, false, al, "al r0 sp 0x11c", "al_r0_sp_0x11c"},
     {{al, r0, sp, 0x120}, false, al, "al r0 sp 0x120", "al_r0_sp_0x120"},
     {{al, r0, sp, 0x124}, false, al, "al r0 sp 0x124", "al_r0_sp_0x124"},
     {{al, r0, sp, 0x128}, false, al, "al r0 sp 0x128", "al_r0_sp_0x128"},
     {{al, r0, sp, 0x12c}, false, al, "al r0 sp 0x12c", "al_r0_sp_0x12c"},
     {{al, r0, sp, 0x130}, false, al, "al r0 sp 0x130", "al_r0_sp_0x130"},
     {{al, r0, sp, 0x134}, false, al, "al r0 sp 0x134", "al_r0_sp_0x134"},
     {{al, r0, sp, 0x138}, false, al, "al r0 sp 0x138", "al_r0_sp_0x138"},
     {{al, r0, sp, 0x13c}, false, al, "al r0 sp 0x13c", "al_r0_sp_0x13c"},
     {{al, r0, sp, 0x140}, false, al, "al r0 sp 0x140", "al_r0_sp_0x140"},
     {{al, r0, sp, 0x144}, false, al, "al r0 sp 0x144", "al_r0_sp_0x144"},
     {{al, r0, sp, 0x148}, false, al, "al r0 sp 0x148", "al_r0_sp_0x148"},
     {{al, r0, sp, 0x14c}, false, al, "al r0 sp 0x14c", "al_r0_sp_0x14c"},
     {{al, r0, sp, 0x150}, false, al, "al r0 sp 0x150", "al_r0_sp_0x150"},
     {{al, r0, sp, 0x154}, false, al, "al r0 sp 0x154", "al_r0_sp_0x154"},
     {{al, r0, sp, 0x158}, false, al, "al r0 sp 0x158", "al_r0_sp_0x158"},
     {{al, r0, sp, 0x15c}, false, al, "al r0 sp 0x15c", "al_r0_sp_0x15c"},
     {{al, r0, sp, 0x160}, false, al, "al r0 sp 0x160", "al_r0_sp_0x160"},
     {{al, r0, sp, 0x164}, false, al, "al r0 sp 0x164", "al_r0_sp_0x164"},
     {{al, r0, sp, 0x168}, false, al, "al r0 sp 0x168", "al_r0_sp_0x168"},
     {{al, r0, sp, 0x16c}, false, al, "al r0 sp 0x16c", "al_r0_sp_0x16c"},
     {{al, r0, sp, 0x170}, false, al, "al r0 sp 0x170", "al_r0_sp_0x170"},
     {{al, r0, sp, 0x174}, false, al, "al r0 sp 0x174", "al_r0_sp_0x174"},
     {{al, r0, sp, 0x178}, false, al, "al r0 sp 0x178", "al_r0_sp_0x178"},
     {{al, r0, sp, 0x17c}, false, al, "al r0 sp 0x17c", "al_r0_sp_0x17c"},
     {{al, r0, sp, 0x180}, false, al, "al r0 sp 0x180", "al_r0_sp_0x180"},
     {{al, r0, sp, 0x184}, false, al, "al r0 sp 0x184", "al_r0_sp_0x184"},
     {{al, r0, sp, 0x188}, false, al, "al r0 sp 0x188", "al_r0_sp_0x188"},
     {{al, r0, sp, 0x18c}, false, al, "al r0 sp 0x18c", "al_r0_sp_0x18c"},
     {{al, r0, sp, 0x190}, false, al, "al r0 sp 0x190", "al_r0_sp_0x190"},
     {{al, r0, sp, 0x194}, false, al, "al r0 sp 0x194", "al_r0_sp_0x194"},
     {{al, r0, sp, 0x198}, false, al, "al r0 sp 0x198", "al_r0_sp_0x198"},
     {{al, r0, sp, 0x19c}, false, al, "al r0 sp 0x19c", "al_r0_sp_0x19c"},
     {{al, r0, sp, 0x1a0}, false, al, "al r0 sp 0x1a0", "al_r0_sp_0x1a0"},
     {{al, r0, sp, 0x1a4}, false, al, "al r0 sp 0x1a4", "al_r0_sp_0x1a4"},
     {{al, r0, sp, 0x1a8}, false, al, "al r0 sp 0x1a8", "al_r0_sp_0x1a8"},
     {{al, r0, sp, 0x1ac}, false, al, "al r0 sp 0x1ac", "al_r0_sp_0x1ac"},
     {{al, r0, sp, 0x1b0}, false, al, "al r0 sp 0x1b0", "al_r0_sp_0x1b0"},
     {{al, r0, sp, 0x1b4}, false, al, "al r0 sp 0x1b4", "al_r0_sp_0x1b4"},
     {{al, r0, sp, 0x1b8}, false, al, "al r0 sp 0x1b8", "al_r0_sp_0x1b8"},
     {{al, r0, sp, 0x1bc}, false, al, "al r0 sp 0x1bc", "al_r0_sp_0x1bc"},
     {{al, r0, sp, 0x1c0}, false, al, "al r0 sp 0x1c0", "al_r0_sp_0x1c0"},
     {{al, r0, sp, 0x1c4}, false, al, "al r0 sp 0x1c4", "al_r0_sp_0x1c4"},
     {{al, r0, sp, 0x1c8}, false, al, "al r0 sp 0x1c8", "al_r0_sp_0x1c8"},
     {{al, r0, sp, 0x1cc}, false, al, "al r0 sp 0x1cc", "al_r0_sp_0x1cc"},
     {{al, r0, sp, 0x1d0}, false, al, "al r0 sp 0x1d0", "al_r0_sp_0x1d0"},
     {{al, r0, sp, 0x1d4}, false, al, "al r0 sp 0x1d4", "al_r0_sp_0x1d4"},
     {{al, r0, sp, 0x1d8}, false, al, "al r0 sp 0x1d8", "al_r0_sp_0x1d8"},
     {{al, r0, sp, 0x1dc}, false, al, "al r0 sp 0x1dc", "al_r0_sp_0x1dc"},
     {{al, r0, sp, 0x1e0}, false, al, "al r0 sp 0x1e0", "al_r0_sp_0x1e0"},
     {{al, r0, sp, 0x1e4}, false, al, "al r0 sp 0x1e4", "al_r0_sp_0x1e4"},
     {{al, r0, sp, 0x1e8}, false, al, "al r0 sp 0x1e8", "al_r0_sp_0x1e8"},
     {{al, r0, sp, 0x1ec}, false, al, "al r0 sp 0x1ec", "al_r0_sp_0x1ec"},
     {{al, r0, sp, 0x1f0}, false, al, "al r0 sp 0x1f0", "al_r0_sp_0x1f0"},
     {{al, r0, sp, 0x1f4}, false, al, "al r0 sp 0x1f4", "al_r0_sp_0x1f4"},
     {{al, r0, sp, 0x1f8}, false, al, "al r0 sp 0x1f8", "al_r0_sp_0x1f8"},
     {{al, r0, sp, 0x1fc}, false, al, "al r0 sp 0x1fc", "al_r0_sp_0x1fc"},
     {{al, r0, sp, 0x200}, false, al, "al r0 sp 0x200", "al_r0_sp_0x200"},
     {{al, r0, sp, 0x204}, false, al, "al r0 sp 0x204", "al_r0_sp_0x204"},
     {{al, r0, sp, 0x208}, false, al, "al r0 sp 0x208", "al_r0_sp_0x208"},
     {{al, r0, sp, 0x20c}, false, al, "al r0 sp 0x20c", "al_r0_sp_0x20c"},
     {{al, r0, sp, 0x210}, false, al, "al r0 sp 0x210", "al_r0_sp_0x210"},
     {{al, r0, sp, 0x214}, false, al, "al r0 sp 0x214", "al_r0_sp_0x214"},
     {{al, r0, sp, 0x218}, false, al, "al r0 sp 0x218", "al_r0_sp_0x218"},
     {{al, r0, sp, 0x21c}, false, al, "al r0 sp 0x21c", "al_r0_sp_0x21c"},
     {{al, r0, sp, 0x220}, false, al, "al r0 sp 0x220", "al_r0_sp_0x220"},
     {{al, r0, sp, 0x224}, false, al, "al r0 sp 0x224", "al_r0_sp_0x224"},
     {{al, r0, sp, 0x228}, false, al, "al r0 sp 0x228", "al_r0_sp_0x228"},
     {{al, r0, sp, 0x22c}, false, al, "al r0 sp 0x22c", "al_r0_sp_0x22c"},
     {{al, r0, sp, 0x230}, false, al, "al r0 sp 0x230", "al_r0_sp_0x230"},
     {{al, r0, sp, 0x234}, false, al, "al r0 sp 0x234", "al_r0_sp_0x234"},
     {{al, r0, sp, 0x238}, false, al, "al r0 sp 0x238", "al_r0_sp_0x238"},
     {{al, r0, sp, 0x23c}, false, al, "al r0 sp 0x23c", "al_r0_sp_0x23c"},
     {{al, r0, sp, 0x240}, false, al, "al r0 sp 0x240", "al_r0_sp_0x240"},
     {{al, r0, sp, 0x244}, false, al, "al r0 sp 0x244", "al_r0_sp_0x244"},
     {{al, r0, sp, 0x248}, false, al, "al r0 sp 0x248", "al_r0_sp_0x248"},
     {{al, r0, sp, 0x24c}, false, al, "al r0 sp 0x24c", "al_r0_sp_0x24c"},
     {{al, r0, sp, 0x250}, false, al, "al r0 sp 0x250", "al_r0_sp_0x250"},
     {{al, r0, sp, 0x254}, false, al, "al r0 sp 0x254", "al_r0_sp_0x254"},
     {{al, r0, sp, 0x258}, false, al, "al r0 sp 0x258", "al_r0_sp_0x258"},
     {{al, r0, sp, 0x25c}, false, al, "al r0 sp 0x25c", "al_r0_sp_0x25c"},
     {{al, r0, sp, 0x260}, false, al, "al r0 sp 0x260", "al_r0_sp_0x260"},
     {{al, r0, sp, 0x264}, false, al, "al r0 sp 0x264", "al_r0_sp_0x264"},
     {{al, r0, sp, 0x268}, false, al, "al r0 sp 0x268", "al_r0_sp_0x268"},
     {{al, r0, sp, 0x26c}, false, al, "al r0 sp 0x26c", "al_r0_sp_0x26c"},
     {{al, r0, sp, 0x270}, false, al, "al r0 sp 0x270", "al_r0_sp_0x270"},
     {{al, r0, sp, 0x274}, false, al, "al r0 sp 0x274", "al_r0_sp_0x274"},
     {{al, r0, sp, 0x278}, false, al, "al r0 sp 0x278", "al_r0_sp_0x278"},
     {{al, r0, sp, 0x27c}, false, al, "al r0 sp 0x27c", "al_r0_sp_0x27c"},
     {{al, r0, sp, 0x280}, false, al, "al r0 sp 0x280", "al_r0_sp_0x280"},
     {{al, r0, sp, 0x284}, false, al, "al r0 sp 0x284", "al_r0_sp_0x284"},
     {{al, r0, sp, 0x288}, false, al, "al r0 sp 0x288", "al_r0_sp_0x288"},
     {{al, r0, sp, 0x28c}, false, al, "al r0 sp 0x28c", "al_r0_sp_0x28c"},
     {{al, r0, sp, 0x290}, false, al, "al r0 sp 0x290", "al_r0_sp_0x290"},
     {{al, r0, sp, 0x294}, false, al, "al r0 sp 0x294", "al_r0_sp_0x294"},
     {{al, r0, sp, 0x298}, false, al, "al r0 sp 0x298", "al_r0_sp_0x298"},
     {{al, r0, sp, 0x29c}, false, al, "al r0 sp 0x29c", "al_r0_sp_0x29c"},
     {{al, r0, sp, 0x2a0}, false, al, "al r0 sp 0x2a0", "al_r0_sp_0x2a0"},
     {{al, r0, sp, 0x2a4}, false, al, "al r0 sp 0x2a4", "al_r0_sp_0x2a4"},
     {{al, r0, sp, 0x2a8}, false, al, "al r0 sp 0x2a8", "al_r0_sp_0x2a8"},
     {{al, r0, sp, 0x2ac}, false, al, "al r0 sp 0x2ac", "al_r0_sp_0x2ac"},
     {{al, r0, sp, 0x2b0}, false, al, "al r0 sp 0x2b0", "al_r0_sp_0x2b0"},
     {{al, r0, sp, 0x2b4}, false, al, "al r0 sp 0x2b4", "al_r0_sp_0x2b4"},
     {{al, r0, sp, 0x2b8}, false, al, "al r0 sp 0x2b8", "al_r0_sp_0x2b8"},
     {{al, r0, sp, 0x2bc}, false, al, "al r0 sp 0x2bc", "al_r0_sp_0x2bc"},
     {{al, r0, sp, 0x2c0}, false, al, "al r0 sp 0x2c0", "al_r0_sp_0x2c0"},
     {{al, r0, sp, 0x2c4}, false, al, "al r0 sp 0x2c4", "al_r0_sp_0x2c4"},
     {{al, r0, sp, 0x2c8}, false, al, "al r0 sp 0x2c8", "al_r0_sp_0x2c8"},
     {{al, r0, sp, 0x2cc}, false, al, "al r0 sp 0x2cc", "al_r0_sp_0x2cc"},
     {{al, r0, sp, 0x2d0}, false, al, "al r0 sp 0x2d0", "al_r0_sp_0x2d0"},
     {{al, r0, sp, 0x2d4}, false, al, "al r0 sp 0x2d4", "al_r0_sp_0x2d4"},
     {{al, r0, sp, 0x2d8}, false, al, "al r0 sp 0x2d8", "al_r0_sp_0x2d8"},
     {{al, r0, sp, 0x2dc}, false, al, "al r0 sp 0x2dc", "al_r0_sp_0x2dc"},
     {{al, r0, sp, 0x2e0}, false, al, "al r0 sp 0x2e0", "al_r0_sp_0x2e0"},
     {{al, r0, sp, 0x2e4}, false, al, "al r0 sp 0x2e4", "al_r0_sp_0x2e4"},
     {{al, r0, sp, 0x2e8}, false, al, "al r0 sp 0x2e8", "al_r0_sp_0x2e8"},
     {{al, r0, sp, 0x2ec}, false, al, "al r0 sp 0x2ec", "al_r0_sp_0x2ec"},
     {{al, r0, sp, 0x2f0}, false, al, "al r0 sp 0x2f0", "al_r0_sp_0x2f0"},
     {{al, r0, sp, 0x2f4}, false, al, "al r0 sp 0x2f4", "al_r0_sp_0x2f4"},
     {{al, r0, sp, 0x2f8}, false, al, "al r0 sp 0x2f8", "al_r0_sp_0x2f8"},
     {{al, r0, sp, 0x2fc}, false, al, "al r0 sp 0x2fc", "al_r0_sp_0x2fc"},
     {{al, r0, sp, 0x300}, false, al, "al r0 sp 0x300", "al_r0_sp_0x300"},
     {{al, r0, sp, 0x304}, false, al, "al r0 sp 0x304", "al_r0_sp_0x304"},
     {{al, r0, sp, 0x308}, false, al, "al r0 sp 0x308", "al_r0_sp_0x308"},
     {{al, r0, sp, 0x30c}, false, al, "al r0 sp 0x30c", "al_r0_sp_0x30c"},
     {{al, r0, sp, 0x310}, false, al, "al r0 sp 0x310", "al_r0_sp_0x310"},
     {{al, r0, sp, 0x314}, false, al, "al r0 sp 0x314", "al_r0_sp_0x314"},
     {{al, r0, sp, 0x318}, false, al, "al r0 sp 0x318", "al_r0_sp_0x318"},
     {{al, r0, sp, 0x31c}, false, al, "al r0 sp 0x31c", "al_r0_sp_0x31c"},
     {{al, r0, sp, 0x320}, false, al, "al r0 sp 0x320", "al_r0_sp_0x320"},
     {{al, r0, sp, 0x324}, false, al, "al r0 sp 0x324", "al_r0_sp_0x324"},
     {{al, r0, sp, 0x328}, false, al, "al r0 sp 0x328", "al_r0_sp_0x328"},
     {{al, r0, sp, 0x32c}, false, al, "al r0 sp 0x32c", "al_r0_sp_0x32c"},
     {{al, r0, sp, 0x330}, false, al, "al r0 sp 0x330", "al_r0_sp_0x330"},
     {{al, r0, sp, 0x334}, false, al, "al r0 sp 0x334", "al_r0_sp_0x334"},
     {{al, r0, sp, 0x338}, false, al, "al r0 sp 0x338", "al_r0_sp_0x338"},
     {{al, r0, sp, 0x33c}, false, al, "al r0 sp 0x33c", "al_r0_sp_0x33c"},
     {{al, r0, sp, 0x340}, false, al, "al r0 sp 0x340", "al_r0_sp_0x340"},
     {{al, r0, sp, 0x344}, false, al, "al r0 sp 0x344", "al_r0_sp_0x344"},
     {{al, r0, sp, 0x348}, false, al, "al r0 sp 0x348", "al_r0_sp_0x348"},
     {{al, r0, sp, 0x34c}, false, al, "al r0 sp 0x34c", "al_r0_sp_0x34c"},
     {{al, r0, sp, 0x350}, false, al, "al r0 sp 0x350", "al_r0_sp_0x350"},
     {{al, r0, sp, 0x354}, false, al, "al r0 sp 0x354", "al_r0_sp_0x354"},
     {{al, r0, sp, 0x358}, false, al, "al r0 sp 0x358", "al_r0_sp_0x358"},
     {{al, r0, sp, 0x35c}, false, al, "al r0 sp 0x35c", "al_r0_sp_0x35c"},
     {{al, r0, sp, 0x360}, false, al, "al r0 sp 0x360", "al_r0_sp_0x360"},
     {{al, r0, sp, 0x364}, false, al, "al r0 sp 0x364", "al_r0_sp_0x364"},
     {{al, r0, sp, 0x368}, false, al, "al r0 sp 0x368", "al_r0_sp_0x368"},
     {{al, r0, sp, 0x36c}, false, al, "al r0 sp 0x36c", "al_r0_sp_0x36c"},
     {{al, r0, sp, 0x370}, false, al, "al r0 sp 0x370", "al_r0_sp_0x370"},
     {{al, r0, sp, 0x374}, false, al, "al r0 sp 0x374", "al_r0_sp_0x374"},
     {{al, r0, sp, 0x378}, false, al, "al r0 sp 0x378", "al_r0_sp_0x378"},
     {{al, r0, sp, 0x37c}, false, al, "al r0 sp 0x37c", "al_r0_sp_0x37c"},
     {{al, r0, sp, 0x380}, false, al, "al r0 sp 0x380", "al_r0_sp_0x380"},
     {{al, r0, sp, 0x384}, false, al, "al r0 sp 0x384", "al_r0_sp_0x384"},
     {{al, r0, sp, 0x388}, false, al, "al r0 sp 0x388", "al_r0_sp_0x388"},
     {{al, r0, sp, 0x38c}, false, al, "al r0 sp 0x38c", "al_r0_sp_0x38c"},
     {{al, r0, sp, 0x390}, false, al, "al r0 sp 0x390", "al_r0_sp_0x390"},
     {{al, r0, sp, 0x394}, false, al, "al r0 sp 0x394", "al_r0_sp_0x394"},
     {{al, r0, sp, 0x398}, false, al, "al r0 sp 0x398", "al_r0_sp_0x398"},
     {{al, r0, sp, 0x39c}, false, al, "al r0 sp 0x39c", "al_r0_sp_0x39c"},
     {{al, r0, sp, 0x3a0}, false, al, "al r0 sp 0x3a0", "al_r0_sp_0x3a0"},
     {{al, r0, sp, 0x3a4}, false, al, "al r0 sp 0x3a4", "al_r0_sp_0x3a4"},
     {{al, r0, sp, 0x3a8}, false, al, "al r0 sp 0x3a8", "al_r0_sp_0x3a8"},
     {{al, r0, sp, 0x3ac}, false, al, "al r0 sp 0x3ac", "al_r0_sp_0x3ac"},
     {{al, r0, sp, 0x3b0}, false, al, "al r0 sp 0x3b0", "al_r0_sp_0x3b0"},
     {{al, r0, sp, 0x3b4}, false, al, "al r0 sp 0x3b4", "al_r0_sp_0x3b4"},
     {{al, r0, sp, 0x3b8}, false, al, "al r0 sp 0x3b8", "al_r0_sp_0x3b8"},
     {{al, r0, sp, 0x3bc}, false, al, "al r0 sp 0x3bc", "al_r0_sp_0x3bc"},
     {{al, r0, sp, 0x3c0}, false, al, "al r0 sp 0x3c0", "al_r0_sp_0x3c0"},
     {{al, r0, sp, 0x3c4}, false, al, "al r0 sp 0x3c4", "al_r0_sp_0x3c4"},
     {{al, r0, sp, 0x3c8}, false, al, "al r0 sp 0x3c8", "al_r0_sp_0x3c8"},
     {{al, r0, sp, 0x3cc}, false, al, "al r0 sp 0x3cc", "al_r0_sp_0x3cc"},
     {{al, r0, sp, 0x3d0}, false, al, "al r0 sp 0x3d0", "al_r0_sp_0x3d0"},
     {{al, r0, sp, 0x3d4}, false, al, "al r0 sp 0x3d4", "al_r0_sp_0x3d4"},
     {{al, r0, sp, 0x3d8}, false, al, "al r0 sp 0x3d8", "al_r0_sp_0x3d8"},
     {{al, r0, sp, 0x3dc}, false, al, "al r0 sp 0x3dc", "al_r0_sp_0x3dc"},
     {{al, r0, sp, 0x3e0}, false, al, "al r0 sp 0x3e0", "al_r0_sp_0x3e0"},
     {{al, r0, sp, 0x3e4}, false, al, "al r0 sp 0x3e4", "al_r0_sp_0x3e4"},
     {{al, r0, sp, 0x3e8}, false, al, "al r0 sp 0x3e8", "al_r0_sp_0x3e8"},
     {{al, r0, sp, 0x3ec}, false, al, "al r0 sp 0x3ec", "al_r0_sp_0x3ec"},
     {{al, r0, sp, 0x3f0}, false, al, "al r0 sp 0x3f0", "al_r0_sp_0x3f0"},
     {{al, r0, sp, 0x3f4}, false, al, "al r0 sp 0x3f4", "al_r0_sp_0x3f4"},
     {{al, r0, sp, 0x3f8}, false, al, "al r0 sp 0x3f8", "al_r0_sp_0x3f8"},
     {{al, r0, sp, 0x3fc}, false, al, "al r0 sp 0x3fc", "al_r0_sp_0x3fc"}};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/assembler-cond-rd-sp-operand-imm8-add-t32.h"


// The maximum number of errors to report in detail for each test.
const unsigned kErrorReportLimit = 8;

typedef void (MacroAssembler::*Fn)(Condition cond,
                                   Register rd,
                                   Register rn,
                                   const Operand& op);

void TestHelper(Fn instruction,
                const char* mnemonic,
                const TestResult reference[]) {
  unsigned total_error_count = 0;
  MacroAssembler masm(BUF_SIZE);

  masm.UseT32();

  for (unsigned i = 0; i < ARRAY_SIZE(kTests); i++) {
    // Values to pass to the macro-assembler.
    Condition cond = kTests[i].operands.cond;
    Register rd = kTests[i].operands.rd;
    Register rn = kTests[i].operands.rn;
    uint32_t immediate = kTests[i].operands.immediate;
    Operand op(immediate);

    int32_t start = masm.GetCursorOffset();
    {
      // We never generate more that 4 bytes, as IT instructions are only
      // allowed for narrow encodings.
      ExactAssemblyScope scope(&masm, 4, ExactAssemblyScope::kMaximumSize);
      if (kTests[i].in_it_block) {
        masm.it(kTests[i].it_condition);
      }
      (masm.*instruction)(cond, rd, rn, op);
    }
    int32_t end = masm.GetCursorOffset();

    const byte* result_ptr =
        masm.GetBuffer()->GetOffsetAddress<const byte*>(start);
    VIXL_ASSERT(start < end);
    uint32_t result_size = end - start;

    if (Test::generate_test_trace()) {
      // Print the result bytes.
      printf("const byte kInstruction_%s_%s[] = {\n",
             mnemonic,
             kTests[i].identifier);
      for (uint32_t j = 0; j < result_size; j++) {
        if (j == 0) {
          printf("  0x%02" PRIx8, result_ptr[j]);
        } else {
          printf(", 0x%02" PRIx8, result_ptr[j]);
        }
      }
      // This comment is meant to be used by external tools to validate
      // the encoding. We can parse the comment to figure out what
      // instruction this corresponds to.
      if (kTests[i].in_it_block) {
        printf(" // It %s; %s %s\n};\n",
               kTests[i].it_condition.GetName(),
               mnemonic,
               kTests[i].operands_description);
      } else {
        printf(" // %s %s\n};\n", mnemonic, kTests[i].operands_description);
      }
    } else {
      // Check we've emitted the exact same encoding as present in the
      // trace file. Only print up to `kErrorReportLimit` errors.
      if (((result_size != reference[i].size) ||
           (memcmp(result_ptr, reference[i].encoding, reference[i].size) !=
            0)) &&
          (++total_error_count <= kErrorReportLimit)) {
        printf("Error when testing \"%s\" with operands \"%s\":\n",
               mnemonic,
               kTests[i].operands_description);
        printf("  Expected: ");
        for (uint32_t j = 0; j < reference[i].size; j++) {
          if (j == 0) {
            printf("0x%02" PRIx8, reference[i].encoding[j]);
          } else {
            printf(", 0x%02" PRIx8, reference[i].encoding[j]);
          }
        }
        printf("\n");
        printf("  Found:    ");
        for (uint32_t j = 0; j < result_size; j++) {
          if (j == 0) {
            printf("0x%02" PRIx8, result_ptr[j]);
          } else {
            printf(", 0x%02" PRIx8, result_ptr[j]);
          }
        }
        printf("\n");
      }
    }
  }

  masm.FinalizeCode();

  if (Test::generate_test_trace()) {
    // Finalize the trace file by writing the final `TestResult` array
    // which links all generated instruction encodings.
    printf("const TestResult kReference%s[] = {\n", mnemonic);
    for (unsigned i = 0; i < ARRAY_SIZE(kTests); i++) {
      printf("  {\n");
      printf("    ARRAY_SIZE(kInstruction_%s_%s),\n",
             mnemonic,
             kTests[i].identifier);
      printf("    kInstruction_%s_%s,\n", mnemonic, kTests[i].identifier);
      printf("  },\n");
    }
    printf("};\n");
  } else {
    if (total_error_count > kErrorReportLimit) {
      printf("%u other errors follow.\n",
             total_error_count - kErrorReportLimit);
    }
    // Crash if the test failed.
    VIXL_CHECK(total_error_count == 0);
  }
}

// Instantiate tests for each instruction in the list.
#define TEST(mnemonic)                                                        \
  void Test_##mnemonic() {                                                    \
    TestHelper(&MacroAssembler::mnemonic, #mnemonic, kReference##mnemonic);   \
  }                                                                           \
  Test test_##mnemonic("AARCH32_ASSEMBLER_COND_RD_SP_OPERAND_IMM8_" #mnemonic \
                       "_T32",                                                \
                       &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
