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
// test/aarch32/config/template-simulator-aarch32.cc.in template file using
// tools/generate_tests.py.
//
// PLEASE DO NOT EDIT.
// -----------------------------------------------------------------------------


#include "test-runner.h"

#include "test-utils.h"
#include "test-utils-aarch32.h"

#include "aarch32/assembler-aarch32.h"
#include "aarch32/disasm-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"

#define __ masm.
#define BUF_SIZE (4096)

#ifdef VIXL_INCLUDE_SIMULATOR_AARCH32
// Run tests with the simulator.

#define SETUP() MacroAssembler masm(BUF_SIZE)

#define START() masm.GetBuffer()->Reset()

#define END() \
  __ Hlt(0);  \
  __ FinalizeCode();

// TODO: Run the tests in the simulator.
#define RUN()

#else  // ifdef VIXL_INCLUDE_SIMULATOR_AARCH32.

#define SETUP()                  \
  MacroAssembler masm(BUF_SIZE); \
  UseScratchRegisterScope harness_scratch;

#define START()                 \
  harness_scratch.Open(&masm);  \
  harness_scratch.ExcludeAll(); \
  masm.GetBuffer()->Reset();    \
  __ Push(r4);                  \
  __ Push(r5);                  \
  __ Push(r6);                  \
  __ Push(r7);                  \
  __ Push(r8);                  \
  __ Push(r9);                  \
  __ Push(r10);                 \
  __ Push(r11);                 \
  __ Push(lr);                  \
  harness_scratch.Include(ip);

#define END()                  \
  harness_scratch.Exclude(ip); \
  __ Pop(lr);                  \
  __ Pop(r11);                 \
  __ Pop(r10);                 \
  __ Pop(r9);                  \
  __ Pop(r8);                  \
  __ Pop(r7);                  \
  __ Pop(r6);                  \
  __ Pop(r5);                  \
  __ Pop(r4);                  \
  __ Bx(lr);                   \
  __ FinalizeCode();           \
  harness_scratch.Close();

#define RUN()                                                 \
  {                                                           \
    int pcs_offset = masm.IsUsingT32() ? 1 : 0;               \
    masm.GetBuffer()->SetExecutable();                        \
    ExecuteMemory(masm.GetBuffer()->GetStartAddress<byte*>(), \
                  masm.GetSizeOfCodeGenerated(),              \
                  pcs_offset);                                \
    masm.GetBuffer()->SetWritable();                          \
  }

#endif  // ifdef VIXL_INCLUDE_SIMULATOR_AARCH32

namespace vixl {
namespace aarch32 {

// List of instruction encodings:
#define FOREACH_INSTRUCTION(M) \
  M(Vadd)                      \
  M(Vsub)


// The following definitions are defined again in each generated test, therefore
// we need to place them in an anomymous namespace. It expresses that they are
// local to this file only, and the compiler is not allowed to share these types
// across test files during template instantiation. Specifically, `Operands` and
// `Inputs` have various layouts across generated tests so they absolutely
// cannot be shared.

#ifdef VIXL_INCLUDE_TARGET_T32
namespace {

// Values to be passed to the assembler to produce the instruction under test.
struct Operands {
  DataType dt;
  DRegister rd;
  DRegister rn;
  DRegister rm;
};

// Input data to feed to the instruction.
struct Inputs {
  uint32_t fpscr;
  uint64_t rd;
  uint64_t rn;
  uint64_t rm;
};

// This structure contains all input data needed to test one specific encoding.
// It used to generate a loop over an instruction.
struct TestLoopData {
  // The `operands` fields represents the values to pass to the assembler to
  // produce the instruction.
  Operands operands;
  // Description of the operands, used for error reporting.
  const char* operands_description;
  // Unique identifier, used for generating traces.
  const char* identifier;
  // Array of values to be fed to the instruction.
  size_t input_size;
  const Inputs* inputs;
};

static const Inputs kFloats[] =
    {{0x0, 0x3fefffffffffffff, 0x000fffffffffffff, 0xfff0000000000000},
     {0x0, 0xffefffffffffffff, 0xbfe0000000000000, 0x3ff0000000000000},
     {0x0, 0x7ff0000000000000, 0x3ff0000000000001, 0x0000000000000000},
     {0x0, 0xbff0000000000000, 0x4024000000000000, 0x4024000000000000},
     {0x0, 0xbff0000000000000, 0x3fdfffffffffffff, 0x0000000000000000},
     {0x0, 0x3fefffffffffffff, 0x7fefffffffffffff, 0xfff0000000000000},
     {0x0, 0xbfe0000000000000, 0xc024000000000000, 0x7ff123456789abcd},
     {0x0, 0x000fffffffffffff, 0x3ff0000000000001, 0xbff0000000000001},
     {0x0, 0x0000000000000001, 0x0000000000000000, 0x3fdfffffffffffff},
     {0x0, 0x8000000000000000, 0xfff123456789abcd, 0xfff0000000000000},
     {0x0, 0xfff923456789abcd, 0xc024000000000000, 0x3ff0000000000000},
     {0x0, 0x7ff0000000000000, 0x3ff8000000000000, 0x3fdfffffffffffff},
     {0x0, 0x7ff923456789abcd, 0x3ff0000000000001, 0x8000000000000001},
     {0x0, 0x0010000000000000, 0x0000000000000001, 0xbff0000000000000},
     {0x0, 0x3fefffffffffffff, 0x3fdfffffffffffff, 0xfff0000000000000},
     {0x0, 0x3fe0000000000001, 0x800fffffffffffff, 0xfff123456789abcd},
     {0x0, 0xffefffffffffffff, 0x3ff0000000000001, 0x800fffffffffffff},
     {0x0, 0x0010000000000000, 0xbfe0000000000001, 0xbfe0000000000000},
     {0x0, 0xc024000000000000, 0x0000000000000000, 0xbff0000000000000},
     {0x0, 0xfff0000000000000, 0x8000000000000001, 0x7ff0000000000000},
     {0x0, 0x3fdfffffffffffff, 0xbfdfffffffffffff, 0x3fe0000000000001},
     {0x0, 0x4024000000000000, 0x000fffffffffffff, 0x7ff923456789abcd},
     {0x0, 0xfff8000000000000, 0xbfe0000000000001, 0x3ff0000000000001},
     {0x0, 0x7fefffffffffffff, 0xbfe0000000000001, 0x3ff0000000000000},
     {0x0, 0x7fefffffffffffff, 0xbfe0000000000001, 0xfff123456789abcd},
     {0x0, 0xfff8000000000000, 0x7ff0000000000000, 0x7fefffffffffffff},
     {0x0, 0xbfe0000000000000, 0x7ff123456789abcd, 0x800123456789abcd},
     {0x0, 0x3ff0000000000001, 0xffefffffffffffff, 0xfff923456789abcd},
     {0x0, 0x3fe0000000000000, 0x0000000000000001, 0x3ff8000000000000},
     {0x0, 0x800123456789abcd, 0xbfe0000000000001, 0x8000000000000000},
     {0x0, 0xbfe0000000000000, 0x3fe0000000000000, 0x3ff8000000000000},
     {0x0, 0x8010000000000000, 0x0010000000000000, 0x8010000000000000},
     {0x0, 0x3fe0000000000001, 0x7ff923456789abcd, 0x000123456789abcd},
     {0x0, 0x3fe0000000000001, 0x3ff8000000000000, 0xbfe0000000000001},
     {0x0, 0xbfefffffffffffff, 0x0000000000000001, 0x0010000000000000},
     {0x0, 0xbfefffffffffffff, 0xbfdfffffffffffff, 0xbff8000000000000},
     {0x0, 0x0000000000000000, 0x3ff0000000000000, 0x3fe0000000000001},
     {0x0, 0xbff0000000000000, 0xffefffffffffffff, 0x3fefffffffffffff},
     {0x0, 0x3ff8000000000000, 0x7ff8000000000000, 0x7ff0000000000000},
     {0x0, 0x4024000000000000, 0xbfefffffffffffff, 0xfff8000000000000},
     {0x0, 0x0000000000000000, 0x000123456789abcd, 0x3fe0000000000000},
     {0x0, 0x7fefffffffffffff, 0x3fe0000000000001, 0xfff0000000000000},
     {0x0, 0xbfefffffffffffff, 0x000123456789abcd, 0x800fffffffffffff},
     {0x0, 0xfff8000000000000, 0x8000000000000001, 0x7fefffffffffffff},
     {0x0, 0x3ff8000000000000, 0x3ff0000000000001, 0xfff123456789abcd},
     {0x0, 0xbfe0000000000001, 0x0000000000000000, 0xbfefffffffffffff},
     {0x0, 0x7fefffffffffffff, 0xfff0000000000000, 0xbff0000000000000},
     {0x0, 0x7ff0000000000000, 0x3fe0000000000000, 0x7ff123456789abcd},
     {0x0, 0x3ff0000000000000, 0x3ff0000000000000, 0x8000000000000001},
     {0x0, 0xbff8000000000000, 0x4024000000000000, 0x3ff8000000000000},
     {0x0, 0xfff923456789abcd, 0xbff8000000000000, 0x000fffffffffffff},
     {0x0, 0x800123456789abcd, 0x0000000000000001, 0x4024000000000000},
     {0x0, 0x0000000000000000, 0xfff923456789abcd, 0x0000000000000000},
     {0x0, 0x3ff0000000000000, 0xfff8000000000000, 0xbfefffffffffffff},
     {0x0, 0x8000000000000000, 0x0000000000000001, 0x3fefffffffffffff},
     {0x0, 0xbff8000000000000, 0xbff0000000000000, 0x7ff8000000000000},
     {0x0, 0x3fdfffffffffffff, 0x8000000000000001, 0x3fdfffffffffffff},
     {0x0, 0x8010000000000000, 0x0000000000000001, 0x000123456789abcd},
     {0x0, 0xbfe0000000000001, 0x000fffffffffffff, 0xbfe0000000000001},
     {0x0, 0x7ff8000000000000, 0xbff0000000000001, 0x7ff0000000000000},
     {0x0, 0xbfdfffffffffffff, 0x7ff123456789abcd, 0x4024000000000000},
     {0x0, 0x3ff8000000000000, 0xbff0000000000000, 0xbfdfffffffffffff},
     {0x0, 0x000123456789abcd, 0x3fefffffffffffff, 0x0000000000000000},
     {0x0, 0x800fffffffffffff, 0xc024000000000000, 0x0000000000000001},
     {0x0, 0xbff8000000000000, 0xbfe0000000000000, 0x8010000000000000},
     {0x0, 0x3fdfffffffffffff, 0x000fffffffffffff, 0x3fdfffffffffffff},
     {0x0, 0x0010000000000000, 0x800fffffffffffff, 0x3ff0000000000001},
     {0x0, 0x3ff8000000000000, 0xfff0000000000000, 0x000fffffffffffff},
     {0x0, 0x3fefffffffffffff, 0xbfefffffffffffff, 0x3fe0000000000000},
     {0x0, 0x7ff0000000000000, 0xbff0000000000001, 0xbff8000000000000},
     {0x0, 0x000fffffffffffff, 0xfff0000000000000, 0xbfe0000000000000},
     {0x0, 0x0000000000000000, 0x3fdfffffffffffff, 0x7ff123456789abcd},
     {0x0, 0x3ff8000000000000, 0xbff8000000000000, 0x000fffffffffffff},
     {0x0, 0x7ff923456789abcd, 0x7ff0000000000000, 0x7ff923456789abcd},
     {0x0, 0x000fffffffffffff, 0x000fffffffffffff, 0x3fe0000000000000},
     {0x0, 0x7ff923456789abcd, 0xbff0000000000000, 0x8000000000000001},
     {0x0, 0x3fe0000000000001, 0x3ff0000000000000, 0x7fefffffffffffff},
     {0x0, 0xfff923456789abcd, 0x3fe0000000000000, 0xc024000000000000},
     {0x0, 0xbff0000000000000, 0x8010000000000000, 0x000fffffffffffff},
     {0x0, 0xbff0000000000001, 0x4024000000000000, 0x7fefffffffffffff},
     {0x0, 0xfff0000000000000, 0x0000000000000000, 0xffefffffffffffff},
     {0x0, 0xfff8000000000000, 0x3fe0000000000000, 0x800fffffffffffff},
     {0x0, 0x000fffffffffffff, 0xbfe0000000000000, 0x7fefffffffffffff},
     {0x0, 0x000fffffffffffff, 0xffefffffffffffff, 0x7ff0000000000000},
     {0x0, 0xbff0000000000001, 0x3ff0000000000001, 0x7ff123456789abcd},
     {0x0, 0xc024000000000000, 0x8000000000000001, 0x3ff8000000000000},
     {0x0, 0xbfe0000000000001, 0x8000000000000001, 0xbfdfffffffffffff},
     {0x0, 0xfff0000000000000, 0x0000000000000000, 0xfff923456789abcd},
     {0x0, 0x3fefffffffffffff, 0xbfdfffffffffffff, 0x8000000000000001},
     {0x0, 0x3fe0000000000000, 0xfff123456789abcd, 0xffefffffffffffff},
     {0x0, 0x3fefffffffffffff, 0xfff923456789abcd, 0x800123456789abcd},
     {0x0, 0x8010000000000000, 0x3ff0000000000000, 0xc024000000000000},
     {0x0, 0x7ff123456789abcd, 0x3fe0000000000001, 0x0000000000000001},
     {0x0, 0x7ff0000000000000, 0xffefffffffffffff, 0x3fe0000000000001},
     {0x0, 0x0010000000000000, 0xfff923456789abcd, 0x000fffffffffffff},
     {0x0, 0x4024000000000000, 0x7ff8000000000000, 0x000123456789abcd},
     {0x0, 0x8010000000000000, 0x8000000000000001, 0xbfe0000000000000},
     {0x0, 0x000fffffffffffff, 0x3ff0000000000000, 0x3ff0000000000001},
     {0x0, 0xbff0000000000000, 0x3fe0000000000000, 0xbff0000000000000},
     {0x0, 0x3fe0000000000001, 0xbfe0000000000000, 0x7ff0000000000000}};

static const Inputs kFloatsSameRegisters[] =
    {{0x0, 0x8000000000000000, 0xbfdfffffffffffff, 0xbfdfffffffffffff},
     {0x0, 0xbfe0000000000000, 0xfff0000000000000, 0xfff0000000000000},
     {0x0, 0x0000000000000001, 0x3fdfffffffffffff, 0x3fdfffffffffffff},
     {0x0, 0xfff8000000000000, 0x8000000000000001, 0x8000000000000001},
     {0x0, 0x7ff0000000000000, 0x7ff923456789abcd, 0x7ff923456789abcd},
     {0x0, 0x7ff123456789abcd, 0xfff0000000000000, 0xfff0000000000000},
     {0x0, 0xbff0000000000001, 0xfff123456789abcd, 0xfff123456789abcd},
     {0x0, 0xbfe0000000000001, 0x8000000000000000, 0x8000000000000000},
     {0x0, 0xc024000000000000, 0x3fe0000000000001, 0x3fe0000000000001},
     {0x0, 0xfff0000000000000, 0x8000000000000001, 0x8000000000000001},
     {0x0, 0x800fffffffffffff, 0x7ff8000000000000, 0x7ff8000000000000},
     {0x0, 0x800fffffffffffff, 0xfff8000000000000, 0xfff8000000000000},
     {0x0, 0xbfe0000000000001, 0x7ff8000000000000, 0x7ff8000000000000},
     {0x0, 0x0010000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff},
     {0x0, 0xfff0000000000000, 0xbff0000000000000, 0xbff0000000000000},
     {0x0, 0xc024000000000000, 0xfff8000000000000, 0xfff8000000000000},
     {0x0, 0xbfe0000000000001, 0xbfe0000000000000, 0xbfe0000000000000},
     {0x0, 0xbff0000000000001, 0x3ff0000000000000, 0x3ff0000000000000},
     {0x0, 0x3fdfffffffffffff, 0xbfe0000000000000, 0xbfe0000000000000},
     {0x0, 0xfff923456789abcd, 0xfff123456789abcd, 0xfff123456789abcd},
     {0x0, 0x7ff0000000000000, 0x800123456789abcd, 0x800123456789abcd},
     {0x0, 0x3ff8000000000000, 0xbfdfffffffffffff, 0xbfdfffffffffffff},
     {0x0, 0xffefffffffffffff, 0xbfe0000000000000, 0xbfe0000000000000},
     {0x0, 0x800123456789abcd, 0xbff0000000000001, 0xbff0000000000001},
     {0x0, 0x0000000000000000, 0x800fffffffffffff, 0x800fffffffffffff},
     {0x0, 0xfff0000000000000, 0x3ff8000000000000, 0x3ff8000000000000},
     {0x0, 0x7ff0000000000000, 0x7ff0000000000000, 0x7ff0000000000000},
     {0x0, 0x0010000000000000, 0xfff923456789abcd, 0xfff923456789abcd},
     {0x0, 0x3fefffffffffffff, 0xfff0000000000000, 0xfff0000000000000},
     {0x0, 0x000fffffffffffff, 0x7ff0000000000000, 0x7ff0000000000000},
     {0x0, 0x000123456789abcd, 0xbfe0000000000001, 0xbfe0000000000001},
     {0x0, 0xfff923456789abcd, 0x3fefffffffffffff, 0x3fefffffffffffff},
     {0x0, 0x3fdfffffffffffff, 0xbff0000000000001, 0xbff0000000000001},
     {0x0, 0x000123456789abcd, 0xbfe0000000000000, 0xbfe0000000000000},
     {0x0, 0xfff0000000000000, 0xbfefffffffffffff, 0xbfefffffffffffff},
     {0x0, 0xfff8000000000000, 0xfff0000000000000, 0xfff0000000000000},
     {0x0, 0x3fe0000000000000, 0xbfdfffffffffffff, 0xbfdfffffffffffff},
     {0x0, 0x0010000000000000, 0xfff0000000000000, 0xfff0000000000000},
     {0x0, 0xfff123456789abcd, 0x3fe0000000000000, 0x3fe0000000000000},
     {0x0, 0x3fe0000000000000, 0xfff0000000000000, 0xfff0000000000000},
     {0x0, 0x3fdfffffffffffff, 0xbff8000000000000, 0xbff8000000000000},
     {0x0, 0xbff0000000000001, 0x3fefffffffffffff, 0x3fefffffffffffff},
     {0x0, 0x0000000000000000, 0x7ff0000000000000, 0x7ff0000000000000},
     {0x0, 0x7ff0000000000000, 0xffefffffffffffff, 0xffefffffffffffff},
     {0x0, 0x000123456789abcd, 0x3fdfffffffffffff, 0x3fdfffffffffffff},
     {0x0, 0xc024000000000000, 0x4024000000000000, 0x4024000000000000},
     {0x0, 0x3fefffffffffffff, 0xbfdfffffffffffff, 0xbfdfffffffffffff},
     {0x0, 0x0010000000000000, 0x7ff8000000000000, 0x7ff8000000000000},
     {0x0, 0x3fe0000000000001, 0x7ff8000000000000, 0x7ff8000000000000},
     {0x0, 0x8000000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff},
     {0x0, 0x7ff123456789abcd, 0xfff0000000000000, 0xfff0000000000000},
     {0x0, 0x7ff0000000000000, 0x000fffffffffffff, 0x000fffffffffffff},
     {0x0, 0x7ff923456789abcd, 0xbff8000000000000, 0xbff8000000000000},
     {0x0, 0x7ff0000000000000, 0xbfefffffffffffff, 0xbfefffffffffffff},
     {0x0, 0xfff123456789abcd, 0x3ff0000000000000, 0x3ff0000000000000},
     {0x0, 0xfff923456789abcd, 0x3ff0000000000001, 0x3ff0000000000001},
     {0x0, 0xc024000000000000, 0xfff123456789abcd, 0xfff123456789abcd},
     {0x0, 0xbfe0000000000001, 0x8010000000000000, 0x8010000000000000},
     {0x0, 0xfff923456789abcd, 0xbfdfffffffffffff, 0xbfdfffffffffffff},
     {0x0, 0x8010000000000000, 0x3fe0000000000000, 0x3fe0000000000000},
     {0x0, 0xfff123456789abcd, 0x3ff8000000000000, 0x3ff8000000000000},
     {0x0, 0x800fffffffffffff, 0x000fffffffffffff, 0x000fffffffffffff},
     {0x0, 0xffefffffffffffff, 0xfff0000000000000, 0xfff0000000000000},
     {0x0, 0x4024000000000000, 0xbfe0000000000001, 0xbfe0000000000001},
     {0x0, 0xfff0000000000000, 0xfff923456789abcd, 0xfff923456789abcd},
     {0x0, 0x7ff123456789abcd, 0xfff0000000000000, 0xfff0000000000000},
     {0x0, 0x8000000000000000, 0x7ff123456789abcd, 0x7ff123456789abcd},
     {0x0, 0x0000000000000001, 0xc024000000000000, 0xc024000000000000},
     {0x0, 0x0010000000000000, 0x7ff0000000000000, 0x7ff0000000000000},
     {0x0, 0x3fe0000000000000, 0xbff0000000000000, 0xbff0000000000000},
     {0x0, 0xbfdfffffffffffff, 0x8010000000000000, 0x8010000000000000},
     {0x0, 0xbff8000000000000, 0x3fe0000000000001, 0x3fe0000000000001},
     {0x0, 0xffefffffffffffff, 0x8000000000000000, 0x8000000000000000},
     {0x0, 0x3fe0000000000000, 0x7ff8000000000000, 0x7ff8000000000000},
     {0x0, 0xbff0000000000001, 0xbff8000000000000, 0xbff8000000000000},
     {0x0, 0xbfe0000000000000, 0xbfe0000000000001, 0xbfe0000000000001},
     {0x0, 0xc024000000000000, 0x7ff0000000000000, 0x7ff0000000000000},
     {0x0, 0x8000000000000001, 0x7fefffffffffffff, 0x7fefffffffffffff},
     {0x0, 0x7ff923456789abcd, 0xfff0000000000000, 0xfff0000000000000},
     {0x0, 0x8010000000000000, 0x7ff0000000000000, 0x7ff0000000000000},
     {0x0, 0xc024000000000000, 0x800123456789abcd, 0x800123456789abcd},
     {0x0, 0x7ff0000000000000, 0xc024000000000000, 0xc024000000000000},
     {0x0, 0x4024000000000000, 0x0010000000000000, 0x0010000000000000},
     {0x0, 0x3ff0000000000000, 0x7ff0000000000000, 0x7ff0000000000000},
     {0x0, 0xbfe0000000000001, 0xc024000000000000, 0xc024000000000000},
     {0x0, 0xbfdfffffffffffff, 0x7ff0000000000000, 0x7ff0000000000000},
     {0x0, 0x000123456789abcd, 0xfff8000000000000, 0xfff8000000000000},
     {0x0, 0xbff0000000000001, 0x000123456789abcd, 0x000123456789abcd},
     {0x0, 0x0000000000000000, 0x7ff123456789abcd, 0x7ff123456789abcd},
     {0x0, 0x3ff0000000000001, 0x7ff8000000000000, 0x7ff8000000000000},
     {0x0, 0x3fe0000000000001, 0x3ff0000000000001, 0x3ff0000000000001},
     {0x0, 0x3ff0000000000001, 0x000123456789abcd, 0x000123456789abcd},
     {0x0, 0x3ff8000000000000, 0x8010000000000000, 0x8010000000000000},
     {0x0, 0x8000000000000000, 0x800fffffffffffff, 0x800fffffffffffff},
     {0x0, 0x7ff8000000000000, 0x7ff0000000000000, 0x7ff0000000000000},
     {0x0, 0xbff0000000000000, 0xfff8000000000000, 0xfff8000000000000},
     {0x0, 0x000fffffffffffff, 0xbff8000000000000, 0xbff8000000000000},
     {0x0, 0x7ff123456789abcd, 0x8010000000000000, 0x8010000000000000},
     {0x0, 0x8000000000000000, 0x3fefffffffffffff, 0x3fefffffffffffff},
     {0x0, 0xbfe0000000000000, 0x7ff0000000000000, 0x7ff0000000000000}};


// A loop will be generated for each element of this array.
const TestLoopData kTests[] = {{{F64, d3, d31, d11},
                                "F64 d3 d31 d11",
                                "Floats_F64_d3_d31_d11",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d21, d17, d0},
                                "F64 d21 d17 d0",
                                "Floats_F64_d21_d17_d0",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d8, d4, d16},
                                "F64 d8 d4 d16",
                                "Floats_F64_d8_d4_d16",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d18, d11, d29},
                                "F64 d18 d11 d29",
                                "Floats_F64_d18_d11_d29",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d18, d7, d16},
                                "F64 d18 d7 d16",
                                "Floats_F64_d18_d7_d16",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d31, d28, d30},
                                "F64 d31 d28 d30",
                                "Floats_F64_d31_d28_d30",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d3, d27, d4},
                                "F64 d3 d27 d4",
                                "Floats_F64_d3_d27_d4",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d16, d17, d25},
                                "F64 d16 d17 d25",
                                "Floats_F64_d16_d17_d25",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d12, d16, d21},
                                "F64 d12 d16 d21",
                                "Floats_F64_d12_d16_d21",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d13, d3, d9},
                                "F64 d13 d3 d9",
                                "Floats_F64_d13_d3_d9",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d14, d15, d9},
                                "F64 d14 d15 d9",
                                "Floats_F64_d14_d15_d9",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d23, d3, d9},
                                "F64 d23 d3 d9",
                                "Floats_F64_d23_d3_d9",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d8, d5, d4},
                                "F64 d8 d5 d4",
                                "Floats_F64_d8_d5_d4",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d28, d10, d8},
                                "F64 d28 d10 d8",
                                "Floats_F64_d28_d10_d8",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d8, d28, d11},
                                "F64 d8 d28 d11",
                                "Floats_F64_d8_d28_d11",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d1, d2, d23},
                                "F64 d1 d2 d23",
                                "Floats_F64_d1_d2_d23",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d27, d26, d30},
                                "F64 d27 d26 d30",
                                "Floats_F64_d27_d26_d30",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d3, d22, d7},
                                "F64 d3 d22 d7",
                                "Floats_F64_d3_d22_d7",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d3, d19, d24},
                                "F64 d3 d19 d24",
                                "Floats_F64_d3_d19_d24",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d27, d30, d22},
                                "F64 d27 d30 d22",
                                "Floats_F64_d27_d30_d22",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d27, d25, d7},
                                "F64 d27 d25 d7",
                                "Floats_F64_d27_d25_d7",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d21, d8, d10},
                                "F64 d21 d8 d10",
                                "Floats_F64_d21_d8_d10",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d28, d17, d14},
                                "F64 d28 d17 d14",
                                "Floats_F64_d28_d17_d14",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d1, d5, d24},
                                "F64 d1 d5 d24",
                                "Floats_F64_d1_d5_d24",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d20, d12, d17},
                                "F64 d20 d12 d17",
                                "Floats_F64_d20_d12_d17",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d30, d29, d10},
                                "F64 d30 d29 d10",
                                "Floats_F64_d30_d29_d10",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d25, d14, d25},
                                "F64 d25 d14 d25",
                                "Floats_F64_d25_d14_d25",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d1, d27, d16},
                                "F64 d1 d27 d16",
                                "Floats_F64_d1_d27_d16",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d6, d28, d7},
                                "F64 d6 d28 d7",
                                "Floats_F64_d6_d28_d7",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d23, d23, d15},
                                "F64 d23 d23 d15",
                                "Floats_F64_d23_d23_d15",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d7, d23, d3},
                                "F64 d7 d23 d3",
                                "Floats_F64_d7_d23_d3",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d7, d23, d16},
                                "F64 d7 d23 d16",
                                "Floats_F64_d7_d23_d16",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d23, d18, d21},
                                "F64 d23 d18 d21",
                                "Floats_F64_d23_d18_d21",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d16, d9, d17},
                                "F64 d16 d9 d17",
                                "Floats_F64_d16_d9_d17",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d5, d21, d9},
                                "F64 d5 d21 d9",
                                "Floats_F64_d5_d21_d9",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d28, d15, d11},
                                "F64 d28 d15 d11",
                                "Floats_F64_d28_d15_d11",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d2, d17, d1},
                                "F64 d2 d17 d1",
                                "Floats_F64_d2_d17_d1",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d25, d29, d17},
                                "F64 d25 d29 d17",
                                "Floats_F64_d25_d29_d17",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d16, d2, d12},
                                "F64 d16 d2 d12",
                                "Floats_F64_d16_d2_d12",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d14, d18, d24},
                                "F64 d14 d18 d24",
                                "Floats_F64_d14_d18_d24",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d3, d4, d25},
                                "F64 d3 d4 d25",
                                "Floats_F64_d3_d4_d25",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d29, d10, d21},
                                "F64 d29 d10 d21",
                                "Floats_F64_d29_d10_d21",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d29, d27, d24},
                                "F64 d29 d27 d24",
                                "Floats_F64_d29_d27_d24",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d30, d19, d23},
                                "F64 d30 d19 d23",
                                "Floats_F64_d30_d19_d23",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d3, d2, d14},
                                "F64 d3 d2 d14",
                                "Floats_F64_d3_d2_d14",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d17, d26, d0},
                                "F64 d17 d26 d0",
                                "Floats_F64_d17_d26_d0",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d17, d28, d8},
                                "F64 d17 d28 d8",
                                "Floats_F64_d17_d28_d8",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d0, d3, d24},
                                "F64 d0 d3 d24",
                                "Floats_F64_d0_d3_d24",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d30, d16, d21},
                                "F64 d30 d16 d21",
                                "Floats_F64_d30_d16_d21",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d18, d24, d3},
                                "F64 d18 d24 d3",
                                "Floats_F64_d18_d24_d3",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d6, d2, d15},
                                "F64 d6 d2 d15",
                                "Floats_F64_d6_d2_d15",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d7, d0, d27},
                                "F64 d7 d0 d27",
                                "Floats_F64_d7_d0_d27",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d0, d9, d27},
                                "F64 d0 d9 d27",
                                "Floats_F64_d0_d9_d27",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d7, d11, d27},
                                "F64 d7 d11 d27",
                                "Floats_F64_d7_d11_d27",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d17, d25, d11},
                                "F64 d17 d25 d11",
                                "Floats_F64_d17_d25_d11",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d31, d9, d22},
                                "F64 d31 d9 d22",
                                "Floats_F64_d31_d9_d22",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d24, d0, d5},
                                "F64 d24 d0 d5",
                                "Floats_F64_d24_d0_d5",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d5, d31, d4},
                                "F64 d5 d31 d4",
                                "Floats_F64_d5_d31_d4",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d16, d24, d2},
                                "F64 d16 d24 d2",
                                "Floats_F64_d16_d24_d2",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d7, d30, d4},
                                "F64 d7 d30 d4",
                                "Floats_F64_d7_d30_d4",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d31, d4, d28},
                                "F64 d31 d4 d28",
                                "Floats_F64_d31_d4_d28",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d10, d31, d13},
                                "F64 d10 d31 d13",
                                "Floats_F64_d10_d31_d13",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d28, d14, d1},
                                "F64 d28 d14 d1",
                                "Floats_F64_d28_d14_d1",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d4, d16, d0},
                                "F64 d4 d16 d0",
                                "Floats_F64_d4_d16_d0",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d19, d26, d15},
                                "F64 d19 d26 d15",
                                "Floats_F64_d19_d26_d15",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d23, d2, d26},
                                "F64 d23 d2 d26",
                                "Floats_F64_d23_d2_d26",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d25, d26, d10},
                                "F64 d25 d26 d10",
                                "Floats_F64_d25_d26_d10",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d0, d19, d0},
                                "F64 d0 d19 d0",
                                "Floats_F64_d0_d19_d0",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d4, d31, d23},
                                "F64 d4 d31 d23",
                                "Floats_F64_d4_d31_d23",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d30, d1, d23},
                                "F64 d30 d1 d23",
                                "Floats_F64_d30_d1_d23",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d14, d5, d20},
                                "F64 d14 d5 d20",
                                "Floats_F64_d14_d5_d20",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d20, d4, d12},
                                "F64 d20 d4 d12",
                                "Floats_F64_d20_d4_d12",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d2, d5, d10},
                                "F64 d2 d5 d10",
                                "Floats_F64_d2_d5_d10",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d14, d29, d3},
                                "F64 d14 d29 d3",
                                "Floats_F64_d14_d29_d3",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d17, d2, d15},
                                "F64 d17 d2 d15",
                                "Floats_F64_d17_d2_d15",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d9, d30, d29},
                                "F64 d9 d30 d29",
                                "Floats_F64_d9_d30_d29",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d30, d12, d30},
                                "F64 d30 d12 d30",
                                "Floats_F64_d30_d12_d30",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d28, d13, d21},
                                "F64 d28 d13 d21",
                                "Floats_F64_d28_d13_d21",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d15, d17, d26},
                                "F64 d15 d17 d26",
                                "Floats_F64_d15_d17_d26",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d30, d18, d9},
                                "F64 d30 d18 d9",
                                "Floats_F64_d30_d18_d9",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d6, d9, d31},
                                "F64 d6 d9 d31",
                                "Floats_F64_d6_d9_d31",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d11, d23, d22},
                                "F64 d11 d23 d22",
                                "Floats_F64_d11_d23_d22",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d26, d23, d29},
                                "F64 d26 d23 d29",
                                "Floats_F64_d26_d23_d29",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d20, d2, d21},
                                "F64 d20 d2 d21",
                                "Floats_F64_d20_d2_d21",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d1, d25, d1},
                                "F64 d1 d25 d1",
                                "Floats_F64_d1_d25_d1",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d1, d13, d15},
                                "F64 d1 d13 d15",
                                "Floats_F64_d1_d13_d15",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d6, d15, d13},
                                "F64 d6 d15 d13",
                                "Floats_F64_d6_d15_d13",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d4, d3, d7},
                                "F64 d4 d3 d7",
                                "Floats_F64_d4_d3_d7",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d8, d16, d18},
                                "F64 d8 d16 d18",
                                "Floats_F64_d8_d16_d18",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d12, d30, d21},
                                "F64 d12 d30 d21",
                                "Floats_F64_d12_d30_d21",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d0, d1, d15},
                                "F64 d0 d1 d15",
                                "Floats_F64_d0_d1_d15",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d6, d11, d4},
                                "F64 d6 d11 d4",
                                "Floats_F64_d6_d11_d4",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d8, d30, d13},
                                "F64 d8 d30 d13",
                                "Floats_F64_d8_d30_d13",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d12, d22, d12},
                                "F64 d12 d22 d12",
                                "Floats_F64_d12_d22_d12",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d9, d7, d13},
                                "F64 d9 d7 d13",
                                "Floats_F64_d9_d7_d13",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d3, d1, d0},
                                "F64 d3 d1 d0",
                                "Floats_F64_d3_d1_d0",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d28, d22, d20},
                                "F64 d28 d22 d20",
                                "Floats_F64_d28_d22_d20",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d22, d20, d9},
                                "F64 d22 d20 d9",
                                "Floats_F64_d22_d20_d9",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d18, d18, d25},
                                "F64 d18 d18 d25",
                                "Floats_F64_d18_d18_d25",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d19, d3, d8},
                                "F64 d19 d3 d8",
                                "Floats_F64_d19_d3_d8",
                                ARRAY_SIZE(kFloats),
                                kFloats},
                               {{F64, d25, d21, d21},
                                "F64 d25 d21 d21",
                                "FloatsSameRegisters_F64_d25_d21_d21",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d31, d4, d4},
                                "F64 d31 d4 d4",
                                "FloatsSameRegisters_F64_d31_d4_d4",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d14, d30, d30},
                                "F64 d14 d30 d30",
                                "FloatsSameRegisters_F64_d14_d30_d30",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d11, d27, d27},
                                "F64 d11 d27 d27",
                                "FloatsSameRegisters_F64_d11_d27_d27",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d21, d20, d20},
                                "F64 d21 d20 d20",
                                "FloatsSameRegisters_F64_d21_d20_d20",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d26, d14, d14},
                                "F64 d26 d14 d14",
                                "FloatsSameRegisters_F64_d26_d14_d14",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d10, d1, d1},
                                "F64 d10 d1 d1",
                                "FloatsSameRegisters_F64_d10_d1_d1",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d9, d24, d24},
                                "F64 d9 d24 d24",
                                "FloatsSameRegisters_F64_d9_d24_d24",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d17, d20, d20},
                                "F64 d17 d20 d20",
                                "FloatsSameRegisters_F64_d17_d20_d20",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d15, d13, d13},
                                "F64 d15 d13 d13",
                                "FloatsSameRegisters_F64_d15_d13_d13",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d18, d14, d14},
                                "F64 d18 d14 d14",
                                "FloatsSameRegisters_F64_d18_d14_d14",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d20, d10, d10},
                                "F64 d20 d10 d10",
                                "FloatsSameRegisters_F64_d20_d10_d10",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d23, d27, d27},
                                "F64 d23 d27 d27",
                                "FloatsSameRegisters_F64_d23_d27_d27",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d25, d26, d26},
                                "F64 d25 d26 d26",
                                "FloatsSameRegisters_F64_d25_d26_d26",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d24, d5, d5},
                                "F64 d24 d5 d5",
                                "FloatsSameRegisters_F64_d24_d5_d5",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d15, d10, d10},
                                "F64 d15 d10 d10",
                                "FloatsSameRegisters_F64_d15_d10_d10",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d0, d26, d26},
                                "F64 d0 d26 d26",
                                "FloatsSameRegisters_F64_d0_d26_d26",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d22, d23, d23},
                                "F64 d22 d23 d23",
                                "FloatsSameRegisters_F64_d22_d23_d23",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d18, d29, d29},
                                "F64 d18 d29 d29",
                                "FloatsSameRegisters_F64_d18_d29_d29",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d27, d25, d25},
                                "F64 d27 d25 d25",
                                "FloatsSameRegisters_F64_d27_d25_d25",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d15, d15, d15},
                                "F64 d15 d15 d15",
                                "FloatsSameRegisters_F64_d15_d15_d15",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d17, d5, d5},
                                "F64 d17 d5 d5",
                                "FloatsSameRegisters_F64_d17_d5_d5",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d1, d22, d22},
                                "F64 d1 d22 d22",
                                "FloatsSameRegisters_F64_d1_d22_d22",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d20, d29, d29},
                                "F64 d20 d29 d29",
                                "FloatsSameRegisters_F64_d20_d29_d29",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d7, d26, d26},
                                "F64 d7 d26 d26",
                                "FloatsSameRegisters_F64_d7_d26_d26",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d5, d19, d19},
                                "F64 d5 d19 d19",
                                "FloatsSameRegisters_F64_d5_d19_d19",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d19, d13, d13},
                                "F64 d19 d13 d13",
                                "FloatsSameRegisters_F64_d19_d13_d13",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d23, d13, d13},
                                "F64 d23 d13 d13",
                                "FloatsSameRegisters_F64_d23_d13_d13",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d0, d20, d20},
                                "F64 d0 d20 d20",
                                "FloatsSameRegisters_F64_d0_d20_d20",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d19, d26, d26},
                                "F64 d19 d26 d26",
                                "FloatsSameRegisters_F64_d19_d26_d26",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d7, d15, d15},
                                "F64 d7 d15 d15",
                                "FloatsSameRegisters_F64_d7_d15_d15",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d1, d6, d6},
                                "F64 d1 d6 d6",
                                "FloatsSameRegisters_F64_d1_d6_d6",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d3, d25, d25},
                                "F64 d3 d25 d25",
                                "FloatsSameRegisters_F64_d3_d25_d25",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d11, d11, d11},
                                "F64 d11 d11 d11",
                                "FloatsSameRegisters_F64_d11_d11_d11",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d10, d28, d28},
                                "F64 d10 d28 d28",
                                "FloatsSameRegisters_F64_d10_d28_d28",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d20, d13, d13},
                                "F64 d20 d13 d13",
                                "FloatsSameRegisters_F64_d20_d13_d13",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d1, d24, d24},
                                "F64 d1 d24 d24",
                                "FloatsSameRegisters_F64_d1_d24_d24",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d27, d6, d6},
                                "F64 d27 d6 d6",
                                "FloatsSameRegisters_F64_d27_d6_d6",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d30, d29, d29},
                                "F64 d30 d29 d29",
                                "FloatsSameRegisters_F64_d30_d29_d29",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d31, d14, d14},
                                "F64 d31 d14 d14",
                                "FloatsSameRegisters_F64_d31_d14_d14",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d25, d13, d13},
                                "F64 d25 d13 d13",
                                "FloatsSameRegisters_F64_d25_d13_d13",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d21, d16, d16},
                                "F64 d21 d16 d16",
                                "FloatsSameRegisters_F64_d21_d16_d16",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d12, d26, d26},
                                "F64 d12 d26 d26",
                                "FloatsSameRegisters_F64_d12_d26_d26",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d2, d10, d10},
                                "F64 d2 d10 d10",
                                "FloatsSameRegisters_F64_d2_d10_d10",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d1, d5, d5},
                                "F64 d1 d5 d5",
                                "FloatsSameRegisters_F64_d1_d5_d5",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d26, d10, d10},
                                "F64 d26 d10 d10",
                                "FloatsSameRegisters_F64_d26_d10_d10",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d21, d22, d22},
                                "F64 d21 d22 d22",
                                "FloatsSameRegisters_F64_d21_d22_d22",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d2, d15, d15},
                                "F64 d2 d15 d15",
                                "FloatsSameRegisters_F64_d2_d15_d15",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d30, d27, d27},
                                "F64 d30 d27 d27",
                                "FloatsSameRegisters_F64_d30_d27_d27",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d17, d4, d4},
                                "F64 d17 d4 d4",
                                "FloatsSameRegisters_F64_d17_d4_d4",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d0, d8, d8},
                                "F64 d0 d8 d8",
                                "FloatsSameRegisters_F64_d0_d8_d8",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d27, d28, d28},
                                "F64 d27 d28 d28",
                                "FloatsSameRegisters_F64_d27_d28_d28",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d10, d10, d10},
                                "F64 d10 d10 d10",
                                "FloatsSameRegisters_F64_d10_d10_d10",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d10, d17, d17},
                                "F64 d10 d17 d17",
                                "FloatsSameRegisters_F64_d10_d17_d17",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d18, d16, d16},
                                "F64 d18 d16 d16",
                                "FloatsSameRegisters_F64_d18_d16_d16",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d3, d20, d20},
                                "F64 d3 d20 d20",
                                "FloatsSameRegisters_F64_d3_d20_d20",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d0, d28, d28},
                                "F64 d0 d28 d28",
                                "FloatsSameRegisters_F64_d0_d28_d28",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d2, d27, d27},
                                "F64 d2 d27 d27",
                                "FloatsSameRegisters_F64_d2_d27_d27",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d12, d20, d20},
                                "F64 d12 d20 d20",
                                "FloatsSameRegisters_F64_d12_d20_d20",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d9, d22, d22},
                                "F64 d9 d22 d22",
                                "FloatsSameRegisters_F64_d9_d22_d22",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d10, d4, d4},
                                "F64 d10 d4 d4",
                                "FloatsSameRegisters_F64_d10_d4_d4",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d8, d10, d10},
                                "F64 d8 d10 d10",
                                "FloatsSameRegisters_F64_d8_d10_d10",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d26, d5, d5},
                                "F64 d26 d5 d5",
                                "FloatsSameRegisters_F64_d26_d5_d5",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d7, d20, d20},
                                "F64 d7 d20 d20",
                                "FloatsSameRegisters_F64_d7_d20_d20",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d21, d24, d24},
                                "F64 d21 d24 d24",
                                "FloatsSameRegisters_F64_d21_d24_d24",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d29, d9, d9},
                                "F64 d29 d9 d9",
                                "FloatsSameRegisters_F64_d29_d9_d9",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d20, d14, d14},
                                "F64 d20 d14 d14",
                                "FloatsSameRegisters_F64_d20_d14_d14",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d18, d30, d30},
                                "F64 d18 d30 d30",
                                "FloatsSameRegisters_F64_d18_d30_d30",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d15, d14, d14},
                                "F64 d15 d14 d14",
                                "FloatsSameRegisters_F64_d15_d14_d14",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d31, d21, d21},
                                "F64 d31 d21 d21",
                                "FloatsSameRegisters_F64_d31_d21_d21",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d20, d22, d22},
                                "F64 d20 d22 d22",
                                "FloatsSameRegisters_F64_d20_d22_d22",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d13, d5, d5},
                                "F64 d13 d5 d5",
                                "FloatsSameRegisters_F64_d13_d5_d5",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d28, d14, d14},
                                "F64 d28 d14 d14",
                                "FloatsSameRegisters_F64_d28_d14_d14",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d21, d25, d25},
                                "F64 d21 d25 d25",
                                "FloatsSameRegisters_F64_d21_d25_d25",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d23, d29, d29},
                                "F64 d23 d29 d29",
                                "FloatsSameRegisters_F64_d23_d29_d29",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d19, d17, d17},
                                "F64 d19 d17 d17",
                                "FloatsSameRegisters_F64_d19_d17_d17",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d30, d21, d21},
                                "F64 d30 d21 d21",
                                "FloatsSameRegisters_F64_d30_d21_d21",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d6, d9, d9},
                                "F64 d6 d9 d9",
                                "FloatsSameRegisters_F64_d6_d9_d9",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d22, d27, d27},
                                "F64 d22 d27 d27",
                                "FloatsSameRegisters_F64_d22_d27_d27",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d9, d25, d25},
                                "F64 d9 d25 d25",
                                "FloatsSameRegisters_F64_d9_d25_d25",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d12, d22, d22},
                                "F64 d12 d22 d22",
                                "FloatsSameRegisters_F64_d12_d22_d22",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d12, d9, d9},
                                "F64 d12 d9 d9",
                                "FloatsSameRegisters_F64_d12_d9_d9",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d0, d27, d27},
                                "F64 d0 d27 d27",
                                "FloatsSameRegisters_F64_d0_d27_d27",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d2, d12, d12},
                                "F64 d2 d12 d12",
                                "FloatsSameRegisters_F64_d2_d12_d12",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d27, d29, d29},
                                "F64 d27 d29 d29",
                                "FloatsSameRegisters_F64_d27_d29_d29",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d14, d4, d4},
                                "F64 d14 d4 d4",
                                "FloatsSameRegisters_F64_d14_d4_d4",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d17, d25, d25},
                                "F64 d17 d25 d25",
                                "FloatsSameRegisters_F64_d17_d25_d25",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d19, d11, d11},
                                "F64 d19 d11 d11",
                                "FloatsSameRegisters_F64_d19_d11_d11",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d2, d6, d6},
                                "F64 d2 d6 d6",
                                "FloatsSameRegisters_F64_d2_d6_d6",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d26, d24, d24},
                                "F64 d26 d24 d24",
                                "FloatsSameRegisters_F64_d26_d24_d24",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d17, d16, d16},
                                "F64 d17 d16 d16",
                                "FloatsSameRegisters_F64_d17_d16_d16",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d14, d26, d26},
                                "F64 d14 d26 d26",
                                "FloatsSameRegisters_F64_d14_d26_d26",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d18, d20, d20},
                                "F64 d18 d20 d20",
                                "FloatsSameRegisters_F64_d18_d20_d20",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d24, d14, d14},
                                "F64 d24 d14 d14",
                                "FloatsSameRegisters_F64_d24_d14_d14",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d8, d14, d14},
                                "F64 d8 d14 d14",
                                "FloatsSameRegisters_F64_d8_d14_d14",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d13, d9, d9},
                                "F64 d13 d9 d9",
                                "FloatsSameRegisters_F64_d13_d9_d9",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d18, d31, d31},
                                "F64 d18 d31 d31",
                                "FloatsSameRegisters_F64_d18_d31_d31",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d7, d22, d22},
                                "F64 d7 d22 d22",
                                "FloatsSameRegisters_F64_d7_d22_d22",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d29, d10, d10},
                                "F64 d29 d10 d10",
                                "FloatsSameRegisters_F64_d29_d10_d10",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters},
                               {{F64, d5, d29, d29},
                                "F64 d5 d29 d29",
                                "FloatsSameRegisters_F64_d5_d29_d29",
                                ARRAY_SIZE(kFloatsSameRegisters),
                                kFloatsSameRegisters}};

// We record all inputs to the instructions as outputs. This way, we also check
// that what shouldn't change didn't change.
struct TestResult {
  size_t output_size;
  const Inputs* outputs;
};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/simulator-cond-dt-drt-drd-drn-drm-float-f64-vadd-t32.h"
#include "aarch32/traces/simulator-cond-dt-drt-drd-drn-drm-float-f64-vsub-t32.h"


// The maximum number of errors to report in detail for each test.
const unsigned kErrorReportLimit = 8;

typedef void (MacroAssembler::*Fn)(DataType dt,
                                   DRegister rd,
                                   DRegister rn,
                                   DRegister rm);

void TestHelper(Fn instruction,
                const char* mnemonic,
                const TestResult reference[]) {
  SETUP();
  masm.UseT32();
  START();

  // Data to compare to `reference`.
  TestResult* results[ARRAY_SIZE(kTests)];

  // Test cases for memory bound instructions may allocate a buffer and save its
  // address in this array.
  byte* scratch_memory_buffers[ARRAY_SIZE(kTests)];

  // Generate a loop for each element in `kTests`. Each loop tests one specific
  // instruction.
  for (unsigned i = 0; i < ARRAY_SIZE(kTests); i++) {
    // Allocate results on the heap for this test.
    results[i] = new TestResult;
    results[i]->outputs = new Inputs[kTests[i].input_size];
    results[i]->output_size = kTests[i].input_size;

    size_t input_stride = sizeof(kTests[i].inputs[0]) * kTests[i].input_size;
    VIXL_ASSERT(IsUint32(input_stride));

    scratch_memory_buffers[i] = NULL;

    Label loop;
    UseScratchRegisterScope scratch_registers(&masm);
    // Include all registers from r0 ro r12.
    scratch_registers.Include(RegisterList(0x1fff));

    // Values to pass to the macro-assembler.
    DataType dt = kTests[i].operands.dt;
    DRegister rd = kTests[i].operands.rd;
    DRegister rn = kTests[i].operands.rn;
    DRegister rm = kTests[i].operands.rm;

    // Allocate reserved registers for our own use.
    Register input_ptr = scratch_registers.Acquire();
    Register input_end = scratch_registers.Acquire();
    Register result_ptr = scratch_registers.Acquire();

    // Initialize `input_ptr` to the first element and `input_end` the address
    // after the array.
    __ Mov(input_ptr, Operand::From(kTests[i].inputs));
    __ Add(input_end, input_ptr, static_cast<uint32_t>(input_stride));
    __ Mov(result_ptr, Operand::From(results[i]->outputs));
    __ Bind(&loop);

    {
      UseScratchRegisterScope temp_registers(&masm);
      Register fpsr_bits = temp_registers.Acquire();
      __ Ldr(fpsr_bits, MemOperand(input_ptr, offsetof(Inputs, fpscr)));
      __ Vmsr(FPSCR, fpsr_bits);
    }
    __ Vldr(rd, MemOperand(input_ptr, offsetof(Inputs, rd)));
    __ Vldr(rn, MemOperand(input_ptr, offsetof(Inputs, rn)));
    __ Vldr(rm, MemOperand(input_ptr, offsetof(Inputs, rm)));

    (masm.*instruction)(dt, rd, rn, rm);

    {
      UseScratchRegisterScope temp_registers(&masm);
      Register fpsr_bits = temp_registers.Acquire();
      __ Vmrs(RegisterOrAPSR_nzcv(fpsr_bits.GetCode()), FPSCR);
      __ Str(fpsr_bits, MemOperand(result_ptr, offsetof(Inputs, fpscr)));
    }
    __ Vstr(rd, MemOperand(result_ptr, offsetof(Inputs, rd)));
    __ Vstr(rn, MemOperand(result_ptr, offsetof(Inputs, rn)));
    __ Vstr(rm, MemOperand(result_ptr, offsetof(Inputs, rm)));

    // Advance the result pointer.
    __ Add(result_ptr, result_ptr, Operand::From(sizeof(kTests[i].inputs[0])));
    // Loop back until `input_ptr` is lower than `input_base`.
    __ Add(input_ptr, input_ptr, Operand::From(sizeof(kTests[i].inputs[0])));
    __ Cmp(input_ptr, input_end);
    __ B(ne, &loop);
  }

  END();

  RUN();

  if (Test::generate_test_trace()) {
    // Print the results.
    for (size_t i = 0; i < ARRAY_SIZE(kTests); i++) {
      printf("const Inputs kOutputs_%s_%s[] = {\n",
             mnemonic,
             kTests[i].identifier);
      for (size_t j = 0; j < results[i]->output_size; j++) {
        printf("  { ");
        printf("0x%08" PRIx32, results[i]->outputs[j].fpscr);
        printf(", ");
        printf("0x%016" PRIx64, results[i]->outputs[j].rd);
        printf(", ");
        printf("0x%016" PRIx64, results[i]->outputs[j].rn);
        printf(", ");
        printf("0x%016" PRIx64, results[i]->outputs[j].rm);
        printf(" },\n");
      }
      printf("};\n");
    }
    printf("const TestResult kReference%s[] = {\n", mnemonic);
    for (size_t i = 0; i < ARRAY_SIZE(kTests); i++) {
      printf("  {\n");
      printf("    ARRAY_SIZE(kOutputs_%s_%s),\n",
             mnemonic,
             kTests[i].identifier);
      printf("    kOutputs_%s_%s,\n", mnemonic, kTests[i].identifier);
      printf("  },\n");
    }
    printf("};\n");
  } else if (kCheckSimulatorTestResults) {
    // Check the results.
    unsigned total_error_count = 0;
    for (size_t i = 0; i < ARRAY_SIZE(kTests); i++) {
      bool instruction_has_errors = false;
      for (size_t j = 0; j < kTests[i].input_size; j++) {
        uint32_t fpscr = results[i]->outputs[j].fpscr;
        uint64_t rd = results[i]->outputs[j].rd;
        uint64_t rn = results[i]->outputs[j].rn;
        uint64_t rm = results[i]->outputs[j].rm;
        uint32_t fpscr_input = kTests[i].inputs[j].fpscr;
        uint64_t rd_input = kTests[i].inputs[j].rd;
        uint64_t rn_input = kTests[i].inputs[j].rn;
        uint64_t rm_input = kTests[i].inputs[j].rm;
        uint32_t fpscr_ref = reference[i].outputs[j].fpscr;
        uint64_t rd_ref = reference[i].outputs[j].rd;
        uint64_t rn_ref = reference[i].outputs[j].rn;
        uint64_t rm_ref = reference[i].outputs[j].rm;

        if (((fpscr != fpscr_ref) || (rd != rd_ref) || (rn != rn_ref) ||
             (rm != rm_ref)) &&
            (++total_error_count <= kErrorReportLimit)) {
          // Print the instruction once even if it triggered multiple failures.
          if (!instruction_has_errors) {
            printf("Error(s) when testing \"%s %s\":\n",
                   mnemonic,
                   kTests[i].operands_description);
            instruction_has_errors = true;
          }
          // Print subsequent errors.
          printf("  Input:    ");
          printf("0x%08" PRIx32, fpscr_input);
          printf(", ");
          printf("0x%016" PRIx64 "(%g)", rd_input, RawbitsToDouble(rd_input));
          printf(", ");
          printf("0x%016" PRIx64 "(%g)", rn_input, RawbitsToDouble(rn_input));
          printf(", ");
          printf("0x%016" PRIx64 "(%g)", rm_input, RawbitsToDouble(rm_input));
          printf("\n");
          printf("  Expected: ");
          printf("0x%08" PRIx32, fpscr_ref);
          printf(", ");
          printf("0x%016" PRIx64 "(%g)", rd_ref, RawbitsToDouble(rd_ref));
          printf(", ");
          printf("0x%016" PRIx64 "(%g)", rn_ref, RawbitsToDouble(rn_ref));
          printf(", ");
          printf("0x%016" PRIx64 "(%g)", rm_ref, RawbitsToDouble(rm_ref));
          printf("\n");
          printf("  Found:    ");
          printf("0x%08" PRIx32, fpscr);
          printf(", ");
          printf("0x%016" PRIx64 "(%g)", rd, RawbitsToDouble(rd));
          printf(", ");
          printf("0x%016" PRIx64 "(%g)", rn, RawbitsToDouble(rn));
          printf(", ");
          printf("0x%016" PRIx64 "(%g)", rm, RawbitsToDouble(rm));
          printf("\n\n");
        }
      }
    }

    if (total_error_count > kErrorReportLimit) {
      printf("%u other errors follow.\n",
             total_error_count - kErrorReportLimit);
    }
    VIXL_CHECK(total_error_count == 0);
  } else {
    VIXL_WARNING("Assembled the code, but did not run anything.\n");
  }

  for (size_t i = 0; i < ARRAY_SIZE(kTests); i++) {
    delete[] results[i]->outputs;
    delete results[i];
    delete[] scratch_memory_buffers[i];
  }
}

// Instantiate tests for each instruction in the list.
// TODO: Remove this limitation by having a sandboxing mechanism.
#if defined(VIXL_HOST_POINTER_32)
#define TEST(mnemonic)                                                         \
  void Test_##mnemonic() {                                                     \
    TestHelper(&MacroAssembler::mnemonic, #mnemonic, kReference##mnemonic);    \
  }                                                                            \
  Test test_##mnemonic(                                                        \
      "AARCH32_SIMULATOR_COND_DT_DRT_DRD_DRN_DRM_FLOAT_F64_" #mnemonic "_T32", \
      &Test_##mnemonic);
#else
#define TEST(mnemonic)                                                         \
  void Test_##mnemonic() {                                                     \
    VIXL_WARNING("This test can only run on a 32-bit host.\n");                \
    USE(TestHelper);                                                           \
  }                                                                            \
  Test test_##mnemonic(                                                        \
      "AARCH32_SIMULATOR_COND_DT_DRT_DRD_DRN_DRM_FLOAT_F64_" #mnemonic "_T32", \
      &Test_##mnemonic);
#endif

FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
