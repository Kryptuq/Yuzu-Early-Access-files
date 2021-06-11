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
  M(Adc)                       \
  M(Adcs)                      \
  M(Add)                       \
  M(Adds)                      \
  M(And)                       \
  M(Ands)                      \
  M(Bic)                       \
  M(Bics)                      \
  M(Eor)                       \
  M(Eors)                      \
  M(Orn)                       \
  M(Orns)                      \
  M(Orr)                       \
  M(Orrs)                      \
  M(Rsb)                       \
  M(Rsbs)                      \
  M(Sbc)                       \
  M(Sbcs)                      \
  M(Sub)                       \
  M(Subs)


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
  Condition cond;
  Register rd;
  Register rn;
  uint32_t immediate;
};

// Input data to feed to the instruction.
struct Inputs {
  uint32_t apsr;
  uint32_t rd;
  uint32_t rn;
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

static const Inputs kRdIsRn[] =
    {{NoFlag, 0x00000000, 0x00000000}, {NoFlag, 0x00000001, 0x00000001},
     {NoFlag, 0x00000002, 0x00000002}, {NoFlag, 0x00000020, 0x00000020},
     {NoFlag, 0x0000007d, 0x0000007d}, {NoFlag, 0x0000007e, 0x0000007e},
     {NoFlag, 0x0000007f, 0x0000007f}, {NoFlag, 0x00007ffd, 0x00007ffd},
     {NoFlag, 0x00007ffe, 0x00007ffe}, {NoFlag, 0x00007fff, 0x00007fff},
     {NoFlag, 0x33333333, 0x33333333}, {NoFlag, 0x55555555, 0x55555555},
     {NoFlag, 0x7ffffffd, 0x7ffffffd}, {NoFlag, 0x7ffffffe, 0x7ffffffe},
     {NoFlag, 0x7fffffff, 0x7fffffff}, {NoFlag, 0x80000000, 0x80000000},
     {NoFlag, 0x80000001, 0x80000001}, {NoFlag, 0xaaaaaaaa, 0xaaaaaaaa},
     {NoFlag, 0xcccccccc, 0xcccccccc}, {NoFlag, 0xffff8000, 0xffff8000},
     {NoFlag, 0xffff8001, 0xffff8001}, {NoFlag, 0xffff8002, 0xffff8002},
     {NoFlag, 0xffff8003, 0xffff8003}, {NoFlag, 0xffffff80, 0xffffff80},
     {NoFlag, 0xffffff81, 0xffffff81}, {NoFlag, 0xffffff82, 0xffffff82},
     {NoFlag, 0xffffff83, 0xffffff83}, {NoFlag, 0xffffffe0, 0xffffffe0},
     {NoFlag, 0xfffffffd, 0xfffffffd}, {NoFlag, 0xfffffffe, 0xfffffffe},
     {NoFlag, 0xffffffff, 0xffffffff}};

static const Inputs kRdIsNotRn[] =
    {{NoFlag, 0x00000000, 0x00000000}, {NoFlag, 0x00000000, 0x00000001},
     {NoFlag, 0x00000000, 0x00000002}, {NoFlag, 0x00000000, 0x00000020},
     {NoFlag, 0x00000000, 0x0000007d}, {NoFlag, 0x00000000, 0x0000007e},
     {NoFlag, 0x00000000, 0x0000007f}, {NoFlag, 0x00000000, 0x00007ffd},
     {NoFlag, 0x00000000, 0x00007ffe}, {NoFlag, 0x00000000, 0x00007fff},
     {NoFlag, 0x00000000, 0x33333333}, {NoFlag, 0x00000000, 0x55555555},
     {NoFlag, 0x00000000, 0x7ffffffd}, {NoFlag, 0x00000000, 0x7ffffffe},
     {NoFlag, 0x00000000, 0x7fffffff}, {NoFlag, 0x00000000, 0x80000000},
     {NoFlag, 0x00000000, 0x80000001}, {NoFlag, 0x00000000, 0xaaaaaaaa},
     {NoFlag, 0x00000000, 0xcccccccc}, {NoFlag, 0x00000000, 0xffff8000},
     {NoFlag, 0x00000000, 0xffff8001}, {NoFlag, 0x00000000, 0xffff8002},
     {NoFlag, 0x00000000, 0xffff8003}, {NoFlag, 0x00000000, 0xffffff80},
     {NoFlag, 0x00000000, 0xffffff81}, {NoFlag, 0x00000000, 0xffffff82},
     {NoFlag, 0x00000000, 0xffffff83}, {NoFlag, 0x00000000, 0xffffffe0},
     {NoFlag, 0x00000000, 0xfffffffd}, {NoFlag, 0x00000000, 0xfffffffe},
     {NoFlag, 0x00000000, 0xffffffff}, {NoFlag, 0x00000001, 0x00000000},
     {NoFlag, 0x00000001, 0x00000001}, {NoFlag, 0x00000001, 0x00000002},
     {NoFlag, 0x00000001, 0x00000020}, {NoFlag, 0x00000001, 0x0000007d},
     {NoFlag, 0x00000001, 0x0000007e}, {NoFlag, 0x00000001, 0x0000007f},
     {NoFlag, 0x00000001, 0x00007ffd}, {NoFlag, 0x00000001, 0x00007ffe},
     {NoFlag, 0x00000001, 0x00007fff}, {NoFlag, 0x00000001, 0x33333333},
     {NoFlag, 0x00000001, 0x55555555}, {NoFlag, 0x00000001, 0x7ffffffd},
     {NoFlag, 0x00000001, 0x7ffffffe}, {NoFlag, 0x00000001, 0x7fffffff},
     {NoFlag, 0x00000001, 0x80000000}, {NoFlag, 0x00000001, 0x80000001},
     {NoFlag, 0x00000001, 0xaaaaaaaa}, {NoFlag, 0x00000001, 0xcccccccc},
     {NoFlag, 0x00000001, 0xffff8000}, {NoFlag, 0x00000001, 0xffff8001},
     {NoFlag, 0x00000001, 0xffff8002}, {NoFlag, 0x00000001, 0xffff8003},
     {NoFlag, 0x00000001, 0xffffff80}, {NoFlag, 0x00000001, 0xffffff81},
     {NoFlag, 0x00000001, 0xffffff82}, {NoFlag, 0x00000001, 0xffffff83},
     {NoFlag, 0x00000001, 0xffffffe0}, {NoFlag, 0x00000001, 0xfffffffd},
     {NoFlag, 0x00000001, 0xfffffffe}, {NoFlag, 0x00000001, 0xffffffff},
     {NoFlag, 0x00000002, 0x00000000}, {NoFlag, 0x00000002, 0x00000001},
     {NoFlag, 0x00000002, 0x00000002}, {NoFlag, 0x00000002, 0x00000020},
     {NoFlag, 0x00000002, 0x0000007d}, {NoFlag, 0x00000002, 0x0000007e},
     {NoFlag, 0x00000002, 0x0000007f}, {NoFlag, 0x00000002, 0x00007ffd},
     {NoFlag, 0x00000002, 0x00007ffe}, {NoFlag, 0x00000002, 0x00007fff},
     {NoFlag, 0x00000002, 0x33333333}, {NoFlag, 0x00000002, 0x55555555},
     {NoFlag, 0x00000002, 0x7ffffffd}, {NoFlag, 0x00000002, 0x7ffffffe},
     {NoFlag, 0x00000002, 0x7fffffff}, {NoFlag, 0x00000002, 0x80000000},
     {NoFlag, 0x00000002, 0x80000001}, {NoFlag, 0x00000002, 0xaaaaaaaa},
     {NoFlag, 0x00000002, 0xcccccccc}, {NoFlag, 0x00000002, 0xffff8000},
     {NoFlag, 0x00000002, 0xffff8001}, {NoFlag, 0x00000002, 0xffff8002},
     {NoFlag, 0x00000002, 0xffff8003}, {NoFlag, 0x00000002, 0xffffff80},
     {NoFlag, 0x00000002, 0xffffff81}, {NoFlag, 0x00000002, 0xffffff82},
     {NoFlag, 0x00000002, 0xffffff83}, {NoFlag, 0x00000002, 0xffffffe0},
     {NoFlag, 0x00000002, 0xfffffffd}, {NoFlag, 0x00000002, 0xfffffffe},
     {NoFlag, 0x00000002, 0xffffffff}, {NoFlag, 0x00000020, 0x00000000},
     {NoFlag, 0x00000020, 0x00000001}, {NoFlag, 0x00000020, 0x00000002},
     {NoFlag, 0x00000020, 0x00000020}, {NoFlag, 0x00000020, 0x0000007d},
     {NoFlag, 0x00000020, 0x0000007e}, {NoFlag, 0x00000020, 0x0000007f},
     {NoFlag, 0x00000020, 0x00007ffd}, {NoFlag, 0x00000020, 0x00007ffe},
     {NoFlag, 0x00000020, 0x00007fff}, {NoFlag, 0x00000020, 0x33333333},
     {NoFlag, 0x00000020, 0x55555555}, {NoFlag, 0x00000020, 0x7ffffffd},
     {NoFlag, 0x00000020, 0x7ffffffe}, {NoFlag, 0x00000020, 0x7fffffff},
     {NoFlag, 0x00000020, 0x80000000}, {NoFlag, 0x00000020, 0x80000001},
     {NoFlag, 0x00000020, 0xaaaaaaaa}, {NoFlag, 0x00000020, 0xcccccccc},
     {NoFlag, 0x00000020, 0xffff8000}, {NoFlag, 0x00000020, 0xffff8001},
     {NoFlag, 0x00000020, 0xffff8002}, {NoFlag, 0x00000020, 0xffff8003},
     {NoFlag, 0x00000020, 0xffffff80}, {NoFlag, 0x00000020, 0xffffff81},
     {NoFlag, 0x00000020, 0xffffff82}, {NoFlag, 0x00000020, 0xffffff83},
     {NoFlag, 0x00000020, 0xffffffe0}, {NoFlag, 0x00000020, 0xfffffffd},
     {NoFlag, 0x00000020, 0xfffffffe}, {NoFlag, 0x00000020, 0xffffffff},
     {NoFlag, 0x0000007d, 0x00000000}, {NoFlag, 0x0000007d, 0x00000001},
     {NoFlag, 0x0000007d, 0x00000002}, {NoFlag, 0x0000007d, 0x00000020},
     {NoFlag, 0x0000007d, 0x0000007d}, {NoFlag, 0x0000007d, 0x0000007e},
     {NoFlag, 0x0000007d, 0x0000007f}, {NoFlag, 0x0000007d, 0x00007ffd},
     {NoFlag, 0x0000007d, 0x00007ffe}, {NoFlag, 0x0000007d, 0x00007fff},
     {NoFlag, 0x0000007d, 0x33333333}, {NoFlag, 0x0000007d, 0x55555555},
     {NoFlag, 0x0000007d, 0x7ffffffd}, {NoFlag, 0x0000007d, 0x7ffffffe},
     {NoFlag, 0x0000007d, 0x7fffffff}, {NoFlag, 0x0000007d, 0x80000000},
     {NoFlag, 0x0000007d, 0x80000001}, {NoFlag, 0x0000007d, 0xaaaaaaaa},
     {NoFlag, 0x0000007d, 0xcccccccc}, {NoFlag, 0x0000007d, 0xffff8000},
     {NoFlag, 0x0000007d, 0xffff8001}, {NoFlag, 0x0000007d, 0xffff8002},
     {NoFlag, 0x0000007d, 0xffff8003}, {NoFlag, 0x0000007d, 0xffffff80},
     {NoFlag, 0x0000007d, 0xffffff81}, {NoFlag, 0x0000007d, 0xffffff82},
     {NoFlag, 0x0000007d, 0xffffff83}, {NoFlag, 0x0000007d, 0xffffffe0},
     {NoFlag, 0x0000007d, 0xfffffffd}, {NoFlag, 0x0000007d, 0xfffffffe},
     {NoFlag, 0x0000007d, 0xffffffff}, {NoFlag, 0x0000007e, 0x00000000},
     {NoFlag, 0x0000007e, 0x00000001}, {NoFlag, 0x0000007e, 0x00000002},
     {NoFlag, 0x0000007e, 0x00000020}, {NoFlag, 0x0000007e, 0x0000007d},
     {NoFlag, 0x0000007e, 0x0000007e}, {NoFlag, 0x0000007e, 0x0000007f},
     {NoFlag, 0x0000007e, 0x00007ffd}, {NoFlag, 0x0000007e, 0x00007ffe},
     {NoFlag, 0x0000007e, 0x00007fff}, {NoFlag, 0x0000007e, 0x33333333},
     {NoFlag, 0x0000007e, 0x55555555}, {NoFlag, 0x0000007e, 0x7ffffffd},
     {NoFlag, 0x0000007e, 0x7ffffffe}, {NoFlag, 0x0000007e, 0x7fffffff},
     {NoFlag, 0x0000007e, 0x80000000}, {NoFlag, 0x0000007e, 0x80000001},
     {NoFlag, 0x0000007e, 0xaaaaaaaa}, {NoFlag, 0x0000007e, 0xcccccccc},
     {NoFlag, 0x0000007e, 0xffff8000}, {NoFlag, 0x0000007e, 0xffff8001},
     {NoFlag, 0x0000007e, 0xffff8002}, {NoFlag, 0x0000007e, 0xffff8003},
     {NoFlag, 0x0000007e, 0xffffff80}, {NoFlag, 0x0000007e, 0xffffff81},
     {NoFlag, 0x0000007e, 0xffffff82}, {NoFlag, 0x0000007e, 0xffffff83},
     {NoFlag, 0x0000007e, 0xffffffe0}, {NoFlag, 0x0000007e, 0xfffffffd},
     {NoFlag, 0x0000007e, 0xfffffffe}, {NoFlag, 0x0000007e, 0xffffffff},
     {NoFlag, 0x0000007f, 0x00000000}, {NoFlag, 0x0000007f, 0x00000001},
     {NoFlag, 0x0000007f, 0x00000002}, {NoFlag, 0x0000007f, 0x00000020},
     {NoFlag, 0x0000007f, 0x0000007d}, {NoFlag, 0x0000007f, 0x0000007e},
     {NoFlag, 0x0000007f, 0x0000007f}, {NoFlag, 0x0000007f, 0x00007ffd},
     {NoFlag, 0x0000007f, 0x00007ffe}, {NoFlag, 0x0000007f, 0x00007fff},
     {NoFlag, 0x0000007f, 0x33333333}, {NoFlag, 0x0000007f, 0x55555555},
     {NoFlag, 0x0000007f, 0x7ffffffd}, {NoFlag, 0x0000007f, 0x7ffffffe},
     {NoFlag, 0x0000007f, 0x7fffffff}, {NoFlag, 0x0000007f, 0x80000000},
     {NoFlag, 0x0000007f, 0x80000001}, {NoFlag, 0x0000007f, 0xaaaaaaaa},
     {NoFlag, 0x0000007f, 0xcccccccc}, {NoFlag, 0x0000007f, 0xffff8000},
     {NoFlag, 0x0000007f, 0xffff8001}, {NoFlag, 0x0000007f, 0xffff8002},
     {NoFlag, 0x0000007f, 0xffff8003}, {NoFlag, 0x0000007f, 0xffffff80},
     {NoFlag, 0x0000007f, 0xffffff81}, {NoFlag, 0x0000007f, 0xffffff82},
     {NoFlag, 0x0000007f, 0xffffff83}, {NoFlag, 0x0000007f, 0xffffffe0},
     {NoFlag, 0x0000007f, 0xfffffffd}, {NoFlag, 0x0000007f, 0xfffffffe},
     {NoFlag, 0x0000007f, 0xffffffff}, {NoFlag, 0x00007ffd, 0x00000000},
     {NoFlag, 0x00007ffd, 0x00000001}, {NoFlag, 0x00007ffd, 0x00000002},
     {NoFlag, 0x00007ffd, 0x00000020}, {NoFlag, 0x00007ffd, 0x0000007d},
     {NoFlag, 0x00007ffd, 0x0000007e}, {NoFlag, 0x00007ffd, 0x0000007f},
     {NoFlag, 0x00007ffd, 0x00007ffd}, {NoFlag, 0x00007ffd, 0x00007ffe},
     {NoFlag, 0x00007ffd, 0x00007fff}, {NoFlag, 0x00007ffd, 0x33333333},
     {NoFlag, 0x00007ffd, 0x55555555}, {NoFlag, 0x00007ffd, 0x7ffffffd},
     {NoFlag, 0x00007ffd, 0x7ffffffe}, {NoFlag, 0x00007ffd, 0x7fffffff},
     {NoFlag, 0x00007ffd, 0x80000000}, {NoFlag, 0x00007ffd, 0x80000001},
     {NoFlag, 0x00007ffd, 0xaaaaaaaa}, {NoFlag, 0x00007ffd, 0xcccccccc},
     {NoFlag, 0x00007ffd, 0xffff8000}, {NoFlag, 0x00007ffd, 0xffff8001},
     {NoFlag, 0x00007ffd, 0xffff8002}, {NoFlag, 0x00007ffd, 0xffff8003},
     {NoFlag, 0x00007ffd, 0xffffff80}, {NoFlag, 0x00007ffd, 0xffffff81},
     {NoFlag, 0x00007ffd, 0xffffff82}, {NoFlag, 0x00007ffd, 0xffffff83},
     {NoFlag, 0x00007ffd, 0xffffffe0}, {NoFlag, 0x00007ffd, 0xfffffffd},
     {NoFlag, 0x00007ffd, 0xfffffffe}, {NoFlag, 0x00007ffd, 0xffffffff},
     {NoFlag, 0x00007ffe, 0x00000000}, {NoFlag, 0x00007ffe, 0x00000001},
     {NoFlag, 0x00007ffe, 0x00000002}, {NoFlag, 0x00007ffe, 0x00000020},
     {NoFlag, 0x00007ffe, 0x0000007d}, {NoFlag, 0x00007ffe, 0x0000007e},
     {NoFlag, 0x00007ffe, 0x0000007f}, {NoFlag, 0x00007ffe, 0x00007ffd},
     {NoFlag, 0x00007ffe, 0x00007ffe}, {NoFlag, 0x00007ffe, 0x00007fff},
     {NoFlag, 0x00007ffe, 0x33333333}, {NoFlag, 0x00007ffe, 0x55555555},
     {NoFlag, 0x00007ffe, 0x7ffffffd}, {NoFlag, 0x00007ffe, 0x7ffffffe},
     {NoFlag, 0x00007ffe, 0x7fffffff}, {NoFlag, 0x00007ffe, 0x80000000},
     {NoFlag, 0x00007ffe, 0x80000001}, {NoFlag, 0x00007ffe, 0xaaaaaaaa},
     {NoFlag, 0x00007ffe, 0xcccccccc}, {NoFlag, 0x00007ffe, 0xffff8000},
     {NoFlag, 0x00007ffe, 0xffff8001}, {NoFlag, 0x00007ffe, 0xffff8002},
     {NoFlag, 0x00007ffe, 0xffff8003}, {NoFlag, 0x00007ffe, 0xffffff80},
     {NoFlag, 0x00007ffe, 0xffffff81}, {NoFlag, 0x00007ffe, 0xffffff82},
     {NoFlag, 0x00007ffe, 0xffffff83}, {NoFlag, 0x00007ffe, 0xffffffe0},
     {NoFlag, 0x00007ffe, 0xfffffffd}, {NoFlag, 0x00007ffe, 0xfffffffe},
     {NoFlag, 0x00007ffe, 0xffffffff}, {NoFlag, 0x00007fff, 0x00000000},
     {NoFlag, 0x00007fff, 0x00000001}, {NoFlag, 0x00007fff, 0x00000002},
     {NoFlag, 0x00007fff, 0x00000020}, {NoFlag, 0x00007fff, 0x0000007d},
     {NoFlag, 0x00007fff, 0x0000007e}, {NoFlag, 0x00007fff, 0x0000007f},
     {NoFlag, 0x00007fff, 0x00007ffd}, {NoFlag, 0x00007fff, 0x00007ffe},
     {NoFlag, 0x00007fff, 0x00007fff}, {NoFlag, 0x00007fff, 0x33333333},
     {NoFlag, 0x00007fff, 0x55555555}, {NoFlag, 0x00007fff, 0x7ffffffd},
     {NoFlag, 0x00007fff, 0x7ffffffe}, {NoFlag, 0x00007fff, 0x7fffffff},
     {NoFlag, 0x00007fff, 0x80000000}, {NoFlag, 0x00007fff, 0x80000001},
     {NoFlag, 0x00007fff, 0xaaaaaaaa}, {NoFlag, 0x00007fff, 0xcccccccc},
     {NoFlag, 0x00007fff, 0xffff8000}, {NoFlag, 0x00007fff, 0xffff8001},
     {NoFlag, 0x00007fff, 0xffff8002}, {NoFlag, 0x00007fff, 0xffff8003},
     {NoFlag, 0x00007fff, 0xffffff80}, {NoFlag, 0x00007fff, 0xffffff81},
     {NoFlag, 0x00007fff, 0xffffff82}, {NoFlag, 0x00007fff, 0xffffff83},
     {NoFlag, 0x00007fff, 0xffffffe0}, {NoFlag, 0x00007fff, 0xfffffffd},
     {NoFlag, 0x00007fff, 0xfffffffe}, {NoFlag, 0x00007fff, 0xffffffff},
     {NoFlag, 0x33333333, 0x00000000}, {NoFlag, 0x33333333, 0x00000001},
     {NoFlag, 0x33333333, 0x00000002}, {NoFlag, 0x33333333, 0x00000020},
     {NoFlag, 0x33333333, 0x0000007d}, {NoFlag, 0x33333333, 0x0000007e},
     {NoFlag, 0x33333333, 0x0000007f}, {NoFlag, 0x33333333, 0x00007ffd},
     {NoFlag, 0x33333333, 0x00007ffe}, {NoFlag, 0x33333333, 0x00007fff},
     {NoFlag, 0x33333333, 0x33333333}, {NoFlag, 0x33333333, 0x55555555},
     {NoFlag, 0x33333333, 0x7ffffffd}, {NoFlag, 0x33333333, 0x7ffffffe},
     {NoFlag, 0x33333333, 0x7fffffff}, {NoFlag, 0x33333333, 0x80000000},
     {NoFlag, 0x33333333, 0x80000001}, {NoFlag, 0x33333333, 0xaaaaaaaa},
     {NoFlag, 0x33333333, 0xcccccccc}, {NoFlag, 0x33333333, 0xffff8000},
     {NoFlag, 0x33333333, 0xffff8001}, {NoFlag, 0x33333333, 0xffff8002},
     {NoFlag, 0x33333333, 0xffff8003}, {NoFlag, 0x33333333, 0xffffff80},
     {NoFlag, 0x33333333, 0xffffff81}, {NoFlag, 0x33333333, 0xffffff82},
     {NoFlag, 0x33333333, 0xffffff83}, {NoFlag, 0x33333333, 0xffffffe0},
     {NoFlag, 0x33333333, 0xfffffffd}, {NoFlag, 0x33333333, 0xfffffffe},
     {NoFlag, 0x33333333, 0xffffffff}, {NoFlag, 0x55555555, 0x00000000},
     {NoFlag, 0x55555555, 0x00000001}, {NoFlag, 0x55555555, 0x00000002},
     {NoFlag, 0x55555555, 0x00000020}, {NoFlag, 0x55555555, 0x0000007d},
     {NoFlag, 0x55555555, 0x0000007e}, {NoFlag, 0x55555555, 0x0000007f},
     {NoFlag, 0x55555555, 0x00007ffd}, {NoFlag, 0x55555555, 0x00007ffe},
     {NoFlag, 0x55555555, 0x00007fff}, {NoFlag, 0x55555555, 0x33333333},
     {NoFlag, 0x55555555, 0x55555555}, {NoFlag, 0x55555555, 0x7ffffffd},
     {NoFlag, 0x55555555, 0x7ffffffe}, {NoFlag, 0x55555555, 0x7fffffff},
     {NoFlag, 0x55555555, 0x80000000}, {NoFlag, 0x55555555, 0x80000001},
     {NoFlag, 0x55555555, 0xaaaaaaaa}, {NoFlag, 0x55555555, 0xcccccccc},
     {NoFlag, 0x55555555, 0xffff8000}, {NoFlag, 0x55555555, 0xffff8001},
     {NoFlag, 0x55555555, 0xffff8002}, {NoFlag, 0x55555555, 0xffff8003},
     {NoFlag, 0x55555555, 0xffffff80}, {NoFlag, 0x55555555, 0xffffff81},
     {NoFlag, 0x55555555, 0xffffff82}, {NoFlag, 0x55555555, 0xffffff83},
     {NoFlag, 0x55555555, 0xffffffe0}, {NoFlag, 0x55555555, 0xfffffffd},
     {NoFlag, 0x55555555, 0xfffffffe}, {NoFlag, 0x55555555, 0xffffffff},
     {NoFlag, 0x7ffffffd, 0x00000000}, {NoFlag, 0x7ffffffd, 0x00000001},
     {NoFlag, 0x7ffffffd, 0x00000002}, {NoFlag, 0x7ffffffd, 0x00000020},
     {NoFlag, 0x7ffffffd, 0x0000007d}, {NoFlag, 0x7ffffffd, 0x0000007e},
     {NoFlag, 0x7ffffffd, 0x0000007f}, {NoFlag, 0x7ffffffd, 0x00007ffd},
     {NoFlag, 0x7ffffffd, 0x00007ffe}, {NoFlag, 0x7ffffffd, 0x00007fff},
     {NoFlag, 0x7ffffffd, 0x33333333}, {NoFlag, 0x7ffffffd, 0x55555555},
     {NoFlag, 0x7ffffffd, 0x7ffffffd}, {NoFlag, 0x7ffffffd, 0x7ffffffe},
     {NoFlag, 0x7ffffffd, 0x7fffffff}, {NoFlag, 0x7ffffffd, 0x80000000},
     {NoFlag, 0x7ffffffd, 0x80000001}, {NoFlag, 0x7ffffffd, 0xaaaaaaaa},
     {NoFlag, 0x7ffffffd, 0xcccccccc}, {NoFlag, 0x7ffffffd, 0xffff8000},
     {NoFlag, 0x7ffffffd, 0xffff8001}, {NoFlag, 0x7ffffffd, 0xffff8002},
     {NoFlag, 0x7ffffffd, 0xffff8003}, {NoFlag, 0x7ffffffd, 0xffffff80},
     {NoFlag, 0x7ffffffd, 0xffffff81}, {NoFlag, 0x7ffffffd, 0xffffff82},
     {NoFlag, 0x7ffffffd, 0xffffff83}, {NoFlag, 0x7ffffffd, 0xffffffe0},
     {NoFlag, 0x7ffffffd, 0xfffffffd}, {NoFlag, 0x7ffffffd, 0xfffffffe},
     {NoFlag, 0x7ffffffd, 0xffffffff}, {NoFlag, 0x7ffffffe, 0x00000000},
     {NoFlag, 0x7ffffffe, 0x00000001}, {NoFlag, 0x7ffffffe, 0x00000002},
     {NoFlag, 0x7ffffffe, 0x00000020}, {NoFlag, 0x7ffffffe, 0x0000007d},
     {NoFlag, 0x7ffffffe, 0x0000007e}, {NoFlag, 0x7ffffffe, 0x0000007f},
     {NoFlag, 0x7ffffffe, 0x00007ffd}, {NoFlag, 0x7ffffffe, 0x00007ffe},
     {NoFlag, 0x7ffffffe, 0x00007fff}, {NoFlag, 0x7ffffffe, 0x33333333},
     {NoFlag, 0x7ffffffe, 0x55555555}, {NoFlag, 0x7ffffffe, 0x7ffffffd},
     {NoFlag, 0x7ffffffe, 0x7ffffffe}, {NoFlag, 0x7ffffffe, 0x7fffffff},
     {NoFlag, 0x7ffffffe, 0x80000000}, {NoFlag, 0x7ffffffe, 0x80000001},
     {NoFlag, 0x7ffffffe, 0xaaaaaaaa}, {NoFlag, 0x7ffffffe, 0xcccccccc},
     {NoFlag, 0x7ffffffe, 0xffff8000}, {NoFlag, 0x7ffffffe, 0xffff8001},
     {NoFlag, 0x7ffffffe, 0xffff8002}, {NoFlag, 0x7ffffffe, 0xffff8003},
     {NoFlag, 0x7ffffffe, 0xffffff80}, {NoFlag, 0x7ffffffe, 0xffffff81},
     {NoFlag, 0x7ffffffe, 0xffffff82}, {NoFlag, 0x7ffffffe, 0xffffff83},
     {NoFlag, 0x7ffffffe, 0xffffffe0}, {NoFlag, 0x7ffffffe, 0xfffffffd},
     {NoFlag, 0x7ffffffe, 0xfffffffe}, {NoFlag, 0x7ffffffe, 0xffffffff},
     {NoFlag, 0x7fffffff, 0x00000000}, {NoFlag, 0x7fffffff, 0x00000001},
     {NoFlag, 0x7fffffff, 0x00000002}, {NoFlag, 0x7fffffff, 0x00000020},
     {NoFlag, 0x7fffffff, 0x0000007d}, {NoFlag, 0x7fffffff, 0x0000007e},
     {NoFlag, 0x7fffffff, 0x0000007f}, {NoFlag, 0x7fffffff, 0x00007ffd},
     {NoFlag, 0x7fffffff, 0x00007ffe}, {NoFlag, 0x7fffffff, 0x00007fff},
     {NoFlag, 0x7fffffff, 0x33333333}, {NoFlag, 0x7fffffff, 0x55555555},
     {NoFlag, 0x7fffffff, 0x7ffffffd}, {NoFlag, 0x7fffffff, 0x7ffffffe},
     {NoFlag, 0x7fffffff, 0x7fffffff}, {NoFlag, 0x7fffffff, 0x80000000},
     {NoFlag, 0x7fffffff, 0x80000001}, {NoFlag, 0x7fffffff, 0xaaaaaaaa},
     {NoFlag, 0x7fffffff, 0xcccccccc}, {NoFlag, 0x7fffffff, 0xffff8000},
     {NoFlag, 0x7fffffff, 0xffff8001}, {NoFlag, 0x7fffffff, 0xffff8002},
     {NoFlag, 0x7fffffff, 0xffff8003}, {NoFlag, 0x7fffffff, 0xffffff80},
     {NoFlag, 0x7fffffff, 0xffffff81}, {NoFlag, 0x7fffffff, 0xffffff82},
     {NoFlag, 0x7fffffff, 0xffffff83}, {NoFlag, 0x7fffffff, 0xffffffe0},
     {NoFlag, 0x7fffffff, 0xfffffffd}, {NoFlag, 0x7fffffff, 0xfffffffe},
     {NoFlag, 0x7fffffff, 0xffffffff}, {NoFlag, 0x80000000, 0x00000000},
     {NoFlag, 0x80000000, 0x00000001}, {NoFlag, 0x80000000, 0x00000002},
     {NoFlag, 0x80000000, 0x00000020}, {NoFlag, 0x80000000, 0x0000007d},
     {NoFlag, 0x80000000, 0x0000007e}, {NoFlag, 0x80000000, 0x0000007f},
     {NoFlag, 0x80000000, 0x00007ffd}, {NoFlag, 0x80000000, 0x00007ffe},
     {NoFlag, 0x80000000, 0x00007fff}, {NoFlag, 0x80000000, 0x33333333},
     {NoFlag, 0x80000000, 0x55555555}, {NoFlag, 0x80000000, 0x7ffffffd},
     {NoFlag, 0x80000000, 0x7ffffffe}, {NoFlag, 0x80000000, 0x7fffffff},
     {NoFlag, 0x80000000, 0x80000000}, {NoFlag, 0x80000000, 0x80000001},
     {NoFlag, 0x80000000, 0xaaaaaaaa}, {NoFlag, 0x80000000, 0xcccccccc},
     {NoFlag, 0x80000000, 0xffff8000}, {NoFlag, 0x80000000, 0xffff8001},
     {NoFlag, 0x80000000, 0xffff8002}, {NoFlag, 0x80000000, 0xffff8003},
     {NoFlag, 0x80000000, 0xffffff80}, {NoFlag, 0x80000000, 0xffffff81},
     {NoFlag, 0x80000000, 0xffffff82}, {NoFlag, 0x80000000, 0xffffff83},
     {NoFlag, 0x80000000, 0xffffffe0}, {NoFlag, 0x80000000, 0xfffffffd},
     {NoFlag, 0x80000000, 0xfffffffe}, {NoFlag, 0x80000000, 0xffffffff},
     {NoFlag, 0x80000001, 0x00000000}, {NoFlag, 0x80000001, 0x00000001},
     {NoFlag, 0x80000001, 0x00000002}, {NoFlag, 0x80000001, 0x00000020},
     {NoFlag, 0x80000001, 0x0000007d}, {NoFlag, 0x80000001, 0x0000007e},
     {NoFlag, 0x80000001, 0x0000007f}, {NoFlag, 0x80000001, 0x00007ffd},
     {NoFlag, 0x80000001, 0x00007ffe}, {NoFlag, 0x80000001, 0x00007fff},
     {NoFlag, 0x80000001, 0x33333333}, {NoFlag, 0x80000001, 0x55555555},
     {NoFlag, 0x80000001, 0x7ffffffd}, {NoFlag, 0x80000001, 0x7ffffffe},
     {NoFlag, 0x80000001, 0x7fffffff}, {NoFlag, 0x80000001, 0x80000000},
     {NoFlag, 0x80000001, 0x80000001}, {NoFlag, 0x80000001, 0xaaaaaaaa},
     {NoFlag, 0x80000001, 0xcccccccc}, {NoFlag, 0x80000001, 0xffff8000},
     {NoFlag, 0x80000001, 0xffff8001}, {NoFlag, 0x80000001, 0xffff8002},
     {NoFlag, 0x80000001, 0xffff8003}, {NoFlag, 0x80000001, 0xffffff80},
     {NoFlag, 0x80000001, 0xffffff81}, {NoFlag, 0x80000001, 0xffffff82},
     {NoFlag, 0x80000001, 0xffffff83}, {NoFlag, 0x80000001, 0xffffffe0},
     {NoFlag, 0x80000001, 0xfffffffd}, {NoFlag, 0x80000001, 0xfffffffe},
     {NoFlag, 0x80000001, 0xffffffff}, {NoFlag, 0xaaaaaaaa, 0x00000000},
     {NoFlag, 0xaaaaaaaa, 0x00000001}, {NoFlag, 0xaaaaaaaa, 0x00000002},
     {NoFlag, 0xaaaaaaaa, 0x00000020}, {NoFlag, 0xaaaaaaaa, 0x0000007d},
     {NoFlag, 0xaaaaaaaa, 0x0000007e}, {NoFlag, 0xaaaaaaaa, 0x0000007f},
     {NoFlag, 0xaaaaaaaa, 0x00007ffd}, {NoFlag, 0xaaaaaaaa, 0x00007ffe},
     {NoFlag, 0xaaaaaaaa, 0x00007fff}, {NoFlag, 0xaaaaaaaa, 0x33333333},
     {NoFlag, 0xaaaaaaaa, 0x55555555}, {NoFlag, 0xaaaaaaaa, 0x7ffffffd},
     {NoFlag, 0xaaaaaaaa, 0x7ffffffe}, {NoFlag, 0xaaaaaaaa, 0x7fffffff},
     {NoFlag, 0xaaaaaaaa, 0x80000000}, {NoFlag, 0xaaaaaaaa, 0x80000001},
     {NoFlag, 0xaaaaaaaa, 0xaaaaaaaa}, {NoFlag, 0xaaaaaaaa, 0xcccccccc},
     {NoFlag, 0xaaaaaaaa, 0xffff8000}, {NoFlag, 0xaaaaaaaa, 0xffff8001},
     {NoFlag, 0xaaaaaaaa, 0xffff8002}, {NoFlag, 0xaaaaaaaa, 0xffff8003},
     {NoFlag, 0xaaaaaaaa, 0xffffff80}, {NoFlag, 0xaaaaaaaa, 0xffffff81},
     {NoFlag, 0xaaaaaaaa, 0xffffff82}, {NoFlag, 0xaaaaaaaa, 0xffffff83},
     {NoFlag, 0xaaaaaaaa, 0xffffffe0}, {NoFlag, 0xaaaaaaaa, 0xfffffffd},
     {NoFlag, 0xaaaaaaaa, 0xfffffffe}, {NoFlag, 0xaaaaaaaa, 0xffffffff},
     {NoFlag, 0xcccccccc, 0x00000000}, {NoFlag, 0xcccccccc, 0x00000001},
     {NoFlag, 0xcccccccc, 0x00000002}, {NoFlag, 0xcccccccc, 0x00000020},
     {NoFlag, 0xcccccccc, 0x0000007d}, {NoFlag, 0xcccccccc, 0x0000007e},
     {NoFlag, 0xcccccccc, 0x0000007f}, {NoFlag, 0xcccccccc, 0x00007ffd},
     {NoFlag, 0xcccccccc, 0x00007ffe}, {NoFlag, 0xcccccccc, 0x00007fff},
     {NoFlag, 0xcccccccc, 0x33333333}, {NoFlag, 0xcccccccc, 0x55555555},
     {NoFlag, 0xcccccccc, 0x7ffffffd}, {NoFlag, 0xcccccccc, 0x7ffffffe},
     {NoFlag, 0xcccccccc, 0x7fffffff}, {NoFlag, 0xcccccccc, 0x80000000},
     {NoFlag, 0xcccccccc, 0x80000001}, {NoFlag, 0xcccccccc, 0xaaaaaaaa},
     {NoFlag, 0xcccccccc, 0xcccccccc}, {NoFlag, 0xcccccccc, 0xffff8000},
     {NoFlag, 0xcccccccc, 0xffff8001}, {NoFlag, 0xcccccccc, 0xffff8002},
     {NoFlag, 0xcccccccc, 0xffff8003}, {NoFlag, 0xcccccccc, 0xffffff80},
     {NoFlag, 0xcccccccc, 0xffffff81}, {NoFlag, 0xcccccccc, 0xffffff82},
     {NoFlag, 0xcccccccc, 0xffffff83}, {NoFlag, 0xcccccccc, 0xffffffe0},
     {NoFlag, 0xcccccccc, 0xfffffffd}, {NoFlag, 0xcccccccc, 0xfffffffe},
     {NoFlag, 0xcccccccc, 0xffffffff}, {NoFlag, 0xffff8000, 0x00000000},
     {NoFlag, 0xffff8000, 0x00000001}, {NoFlag, 0xffff8000, 0x00000002},
     {NoFlag, 0xffff8000, 0x00000020}, {NoFlag, 0xffff8000, 0x0000007d},
     {NoFlag, 0xffff8000, 0x0000007e}, {NoFlag, 0xffff8000, 0x0000007f},
     {NoFlag, 0xffff8000, 0x00007ffd}, {NoFlag, 0xffff8000, 0x00007ffe},
     {NoFlag, 0xffff8000, 0x00007fff}, {NoFlag, 0xffff8000, 0x33333333},
     {NoFlag, 0xffff8000, 0x55555555}, {NoFlag, 0xffff8000, 0x7ffffffd},
     {NoFlag, 0xffff8000, 0x7ffffffe}, {NoFlag, 0xffff8000, 0x7fffffff},
     {NoFlag, 0xffff8000, 0x80000000}, {NoFlag, 0xffff8000, 0x80000001},
     {NoFlag, 0xffff8000, 0xaaaaaaaa}, {NoFlag, 0xffff8000, 0xcccccccc},
     {NoFlag, 0xffff8000, 0xffff8000}, {NoFlag, 0xffff8000, 0xffff8001},
     {NoFlag, 0xffff8000, 0xffff8002}, {NoFlag, 0xffff8000, 0xffff8003},
     {NoFlag, 0xffff8000, 0xffffff80}, {NoFlag, 0xffff8000, 0xffffff81},
     {NoFlag, 0xffff8000, 0xffffff82}, {NoFlag, 0xffff8000, 0xffffff83},
     {NoFlag, 0xffff8000, 0xffffffe0}, {NoFlag, 0xffff8000, 0xfffffffd},
     {NoFlag, 0xffff8000, 0xfffffffe}, {NoFlag, 0xffff8000, 0xffffffff},
     {NoFlag, 0xffff8001, 0x00000000}, {NoFlag, 0xffff8001, 0x00000001},
     {NoFlag, 0xffff8001, 0x00000002}, {NoFlag, 0xffff8001, 0x00000020},
     {NoFlag, 0xffff8001, 0x0000007d}, {NoFlag, 0xffff8001, 0x0000007e},
     {NoFlag, 0xffff8001, 0x0000007f}, {NoFlag, 0xffff8001, 0x00007ffd},
     {NoFlag, 0xffff8001, 0x00007ffe}, {NoFlag, 0xffff8001, 0x00007fff},
     {NoFlag, 0xffff8001, 0x33333333}, {NoFlag, 0xffff8001, 0x55555555},
     {NoFlag, 0xffff8001, 0x7ffffffd}, {NoFlag, 0xffff8001, 0x7ffffffe},
     {NoFlag, 0xffff8001, 0x7fffffff}, {NoFlag, 0xffff8001, 0x80000000},
     {NoFlag, 0xffff8001, 0x80000001}, {NoFlag, 0xffff8001, 0xaaaaaaaa},
     {NoFlag, 0xffff8001, 0xcccccccc}, {NoFlag, 0xffff8001, 0xffff8000},
     {NoFlag, 0xffff8001, 0xffff8001}, {NoFlag, 0xffff8001, 0xffff8002},
     {NoFlag, 0xffff8001, 0xffff8003}, {NoFlag, 0xffff8001, 0xffffff80},
     {NoFlag, 0xffff8001, 0xffffff81}, {NoFlag, 0xffff8001, 0xffffff82},
     {NoFlag, 0xffff8001, 0xffffff83}, {NoFlag, 0xffff8001, 0xffffffe0},
     {NoFlag, 0xffff8001, 0xfffffffd}, {NoFlag, 0xffff8001, 0xfffffffe},
     {NoFlag, 0xffff8001, 0xffffffff}, {NoFlag, 0xffff8002, 0x00000000},
     {NoFlag, 0xffff8002, 0x00000001}, {NoFlag, 0xffff8002, 0x00000002},
     {NoFlag, 0xffff8002, 0x00000020}, {NoFlag, 0xffff8002, 0x0000007d},
     {NoFlag, 0xffff8002, 0x0000007e}, {NoFlag, 0xffff8002, 0x0000007f},
     {NoFlag, 0xffff8002, 0x00007ffd}, {NoFlag, 0xffff8002, 0x00007ffe},
     {NoFlag, 0xffff8002, 0x00007fff}, {NoFlag, 0xffff8002, 0x33333333},
     {NoFlag, 0xffff8002, 0x55555555}, {NoFlag, 0xffff8002, 0x7ffffffd},
     {NoFlag, 0xffff8002, 0x7ffffffe}, {NoFlag, 0xffff8002, 0x7fffffff},
     {NoFlag, 0xffff8002, 0x80000000}, {NoFlag, 0xffff8002, 0x80000001},
     {NoFlag, 0xffff8002, 0xaaaaaaaa}, {NoFlag, 0xffff8002, 0xcccccccc},
     {NoFlag, 0xffff8002, 0xffff8000}, {NoFlag, 0xffff8002, 0xffff8001},
     {NoFlag, 0xffff8002, 0xffff8002}, {NoFlag, 0xffff8002, 0xffff8003},
     {NoFlag, 0xffff8002, 0xffffff80}, {NoFlag, 0xffff8002, 0xffffff81},
     {NoFlag, 0xffff8002, 0xffffff82}, {NoFlag, 0xffff8002, 0xffffff83},
     {NoFlag, 0xffff8002, 0xffffffe0}, {NoFlag, 0xffff8002, 0xfffffffd},
     {NoFlag, 0xffff8002, 0xfffffffe}, {NoFlag, 0xffff8002, 0xffffffff},
     {NoFlag, 0xffff8003, 0x00000000}, {NoFlag, 0xffff8003, 0x00000001},
     {NoFlag, 0xffff8003, 0x00000002}, {NoFlag, 0xffff8003, 0x00000020},
     {NoFlag, 0xffff8003, 0x0000007d}, {NoFlag, 0xffff8003, 0x0000007e},
     {NoFlag, 0xffff8003, 0x0000007f}, {NoFlag, 0xffff8003, 0x00007ffd},
     {NoFlag, 0xffff8003, 0x00007ffe}, {NoFlag, 0xffff8003, 0x00007fff},
     {NoFlag, 0xffff8003, 0x33333333}, {NoFlag, 0xffff8003, 0x55555555},
     {NoFlag, 0xffff8003, 0x7ffffffd}, {NoFlag, 0xffff8003, 0x7ffffffe},
     {NoFlag, 0xffff8003, 0x7fffffff}, {NoFlag, 0xffff8003, 0x80000000},
     {NoFlag, 0xffff8003, 0x80000001}, {NoFlag, 0xffff8003, 0xaaaaaaaa},
     {NoFlag, 0xffff8003, 0xcccccccc}, {NoFlag, 0xffff8003, 0xffff8000},
     {NoFlag, 0xffff8003, 0xffff8001}, {NoFlag, 0xffff8003, 0xffff8002},
     {NoFlag, 0xffff8003, 0xffff8003}, {NoFlag, 0xffff8003, 0xffffff80},
     {NoFlag, 0xffff8003, 0xffffff81}, {NoFlag, 0xffff8003, 0xffffff82},
     {NoFlag, 0xffff8003, 0xffffff83}, {NoFlag, 0xffff8003, 0xffffffe0},
     {NoFlag, 0xffff8003, 0xfffffffd}, {NoFlag, 0xffff8003, 0xfffffffe},
     {NoFlag, 0xffff8003, 0xffffffff}, {NoFlag, 0xffffff80, 0x00000000},
     {NoFlag, 0xffffff80, 0x00000001}, {NoFlag, 0xffffff80, 0x00000002},
     {NoFlag, 0xffffff80, 0x00000020}, {NoFlag, 0xffffff80, 0x0000007d},
     {NoFlag, 0xffffff80, 0x0000007e}, {NoFlag, 0xffffff80, 0x0000007f},
     {NoFlag, 0xffffff80, 0x00007ffd}, {NoFlag, 0xffffff80, 0x00007ffe},
     {NoFlag, 0xffffff80, 0x00007fff}, {NoFlag, 0xffffff80, 0x33333333},
     {NoFlag, 0xffffff80, 0x55555555}, {NoFlag, 0xffffff80, 0x7ffffffd},
     {NoFlag, 0xffffff80, 0x7ffffffe}, {NoFlag, 0xffffff80, 0x7fffffff},
     {NoFlag, 0xffffff80, 0x80000000}, {NoFlag, 0xffffff80, 0x80000001},
     {NoFlag, 0xffffff80, 0xaaaaaaaa}, {NoFlag, 0xffffff80, 0xcccccccc},
     {NoFlag, 0xffffff80, 0xffff8000}, {NoFlag, 0xffffff80, 0xffff8001},
     {NoFlag, 0xffffff80, 0xffff8002}, {NoFlag, 0xffffff80, 0xffff8003},
     {NoFlag, 0xffffff80, 0xffffff80}, {NoFlag, 0xffffff80, 0xffffff81},
     {NoFlag, 0xffffff80, 0xffffff82}, {NoFlag, 0xffffff80, 0xffffff83},
     {NoFlag, 0xffffff80, 0xffffffe0}, {NoFlag, 0xffffff80, 0xfffffffd},
     {NoFlag, 0xffffff80, 0xfffffffe}, {NoFlag, 0xffffff80, 0xffffffff},
     {NoFlag, 0xffffff81, 0x00000000}, {NoFlag, 0xffffff81, 0x00000001},
     {NoFlag, 0xffffff81, 0x00000002}, {NoFlag, 0xffffff81, 0x00000020},
     {NoFlag, 0xffffff81, 0x0000007d}, {NoFlag, 0xffffff81, 0x0000007e},
     {NoFlag, 0xffffff81, 0x0000007f}, {NoFlag, 0xffffff81, 0x00007ffd},
     {NoFlag, 0xffffff81, 0x00007ffe}, {NoFlag, 0xffffff81, 0x00007fff},
     {NoFlag, 0xffffff81, 0x33333333}, {NoFlag, 0xffffff81, 0x55555555},
     {NoFlag, 0xffffff81, 0x7ffffffd}, {NoFlag, 0xffffff81, 0x7ffffffe},
     {NoFlag, 0xffffff81, 0x7fffffff}, {NoFlag, 0xffffff81, 0x80000000},
     {NoFlag, 0xffffff81, 0x80000001}, {NoFlag, 0xffffff81, 0xaaaaaaaa},
     {NoFlag, 0xffffff81, 0xcccccccc}, {NoFlag, 0xffffff81, 0xffff8000},
     {NoFlag, 0xffffff81, 0xffff8001}, {NoFlag, 0xffffff81, 0xffff8002},
     {NoFlag, 0xffffff81, 0xffff8003}, {NoFlag, 0xffffff81, 0xffffff80},
     {NoFlag, 0xffffff81, 0xffffff81}, {NoFlag, 0xffffff81, 0xffffff82},
     {NoFlag, 0xffffff81, 0xffffff83}, {NoFlag, 0xffffff81, 0xffffffe0},
     {NoFlag, 0xffffff81, 0xfffffffd}, {NoFlag, 0xffffff81, 0xfffffffe},
     {NoFlag, 0xffffff81, 0xffffffff}, {NoFlag, 0xffffff82, 0x00000000},
     {NoFlag, 0xffffff82, 0x00000001}, {NoFlag, 0xffffff82, 0x00000002},
     {NoFlag, 0xffffff82, 0x00000020}, {NoFlag, 0xffffff82, 0x0000007d},
     {NoFlag, 0xffffff82, 0x0000007e}, {NoFlag, 0xffffff82, 0x0000007f},
     {NoFlag, 0xffffff82, 0x00007ffd}, {NoFlag, 0xffffff82, 0x00007ffe},
     {NoFlag, 0xffffff82, 0x00007fff}, {NoFlag, 0xffffff82, 0x33333333},
     {NoFlag, 0xffffff82, 0x55555555}, {NoFlag, 0xffffff82, 0x7ffffffd},
     {NoFlag, 0xffffff82, 0x7ffffffe}, {NoFlag, 0xffffff82, 0x7fffffff},
     {NoFlag, 0xffffff82, 0x80000000}, {NoFlag, 0xffffff82, 0x80000001},
     {NoFlag, 0xffffff82, 0xaaaaaaaa}, {NoFlag, 0xffffff82, 0xcccccccc},
     {NoFlag, 0xffffff82, 0xffff8000}, {NoFlag, 0xffffff82, 0xffff8001},
     {NoFlag, 0xffffff82, 0xffff8002}, {NoFlag, 0xffffff82, 0xffff8003},
     {NoFlag, 0xffffff82, 0xffffff80}, {NoFlag, 0xffffff82, 0xffffff81},
     {NoFlag, 0xffffff82, 0xffffff82}, {NoFlag, 0xffffff82, 0xffffff83},
     {NoFlag, 0xffffff82, 0xffffffe0}, {NoFlag, 0xffffff82, 0xfffffffd},
     {NoFlag, 0xffffff82, 0xfffffffe}, {NoFlag, 0xffffff82, 0xffffffff},
     {NoFlag, 0xffffff83, 0x00000000}, {NoFlag, 0xffffff83, 0x00000001},
     {NoFlag, 0xffffff83, 0x00000002}, {NoFlag, 0xffffff83, 0x00000020},
     {NoFlag, 0xffffff83, 0x0000007d}, {NoFlag, 0xffffff83, 0x0000007e},
     {NoFlag, 0xffffff83, 0x0000007f}, {NoFlag, 0xffffff83, 0x00007ffd},
     {NoFlag, 0xffffff83, 0x00007ffe}, {NoFlag, 0xffffff83, 0x00007fff},
     {NoFlag, 0xffffff83, 0x33333333}, {NoFlag, 0xffffff83, 0x55555555},
     {NoFlag, 0xffffff83, 0x7ffffffd}, {NoFlag, 0xffffff83, 0x7ffffffe},
     {NoFlag, 0xffffff83, 0x7fffffff}, {NoFlag, 0xffffff83, 0x80000000},
     {NoFlag, 0xffffff83, 0x80000001}, {NoFlag, 0xffffff83, 0xaaaaaaaa},
     {NoFlag, 0xffffff83, 0xcccccccc}, {NoFlag, 0xffffff83, 0xffff8000},
     {NoFlag, 0xffffff83, 0xffff8001}, {NoFlag, 0xffffff83, 0xffff8002},
     {NoFlag, 0xffffff83, 0xffff8003}, {NoFlag, 0xffffff83, 0xffffff80},
     {NoFlag, 0xffffff83, 0xffffff81}, {NoFlag, 0xffffff83, 0xffffff82},
     {NoFlag, 0xffffff83, 0xffffff83}, {NoFlag, 0xffffff83, 0xffffffe0},
     {NoFlag, 0xffffff83, 0xfffffffd}, {NoFlag, 0xffffff83, 0xfffffffe},
     {NoFlag, 0xffffff83, 0xffffffff}, {NoFlag, 0xffffffe0, 0x00000000},
     {NoFlag, 0xffffffe0, 0x00000001}, {NoFlag, 0xffffffe0, 0x00000002},
     {NoFlag, 0xffffffe0, 0x00000020}, {NoFlag, 0xffffffe0, 0x0000007d},
     {NoFlag, 0xffffffe0, 0x0000007e}, {NoFlag, 0xffffffe0, 0x0000007f},
     {NoFlag, 0xffffffe0, 0x00007ffd}, {NoFlag, 0xffffffe0, 0x00007ffe},
     {NoFlag, 0xffffffe0, 0x00007fff}, {NoFlag, 0xffffffe0, 0x33333333},
     {NoFlag, 0xffffffe0, 0x55555555}, {NoFlag, 0xffffffe0, 0x7ffffffd},
     {NoFlag, 0xffffffe0, 0x7ffffffe}, {NoFlag, 0xffffffe0, 0x7fffffff},
     {NoFlag, 0xffffffe0, 0x80000000}, {NoFlag, 0xffffffe0, 0x80000001},
     {NoFlag, 0xffffffe0, 0xaaaaaaaa}, {NoFlag, 0xffffffe0, 0xcccccccc},
     {NoFlag, 0xffffffe0, 0xffff8000}, {NoFlag, 0xffffffe0, 0xffff8001},
     {NoFlag, 0xffffffe0, 0xffff8002}, {NoFlag, 0xffffffe0, 0xffff8003},
     {NoFlag, 0xffffffe0, 0xffffff80}, {NoFlag, 0xffffffe0, 0xffffff81},
     {NoFlag, 0xffffffe0, 0xffffff82}, {NoFlag, 0xffffffe0, 0xffffff83},
     {NoFlag, 0xffffffe0, 0xffffffe0}, {NoFlag, 0xffffffe0, 0xfffffffd},
     {NoFlag, 0xffffffe0, 0xfffffffe}, {NoFlag, 0xffffffe0, 0xffffffff},
     {NoFlag, 0xfffffffd, 0x00000000}, {NoFlag, 0xfffffffd, 0x00000001},
     {NoFlag, 0xfffffffd, 0x00000002}, {NoFlag, 0xfffffffd, 0x00000020},
     {NoFlag, 0xfffffffd, 0x0000007d}, {NoFlag, 0xfffffffd, 0x0000007e},
     {NoFlag, 0xfffffffd, 0x0000007f}, {NoFlag, 0xfffffffd, 0x00007ffd},
     {NoFlag, 0xfffffffd, 0x00007ffe}, {NoFlag, 0xfffffffd, 0x00007fff},
     {NoFlag, 0xfffffffd, 0x33333333}, {NoFlag, 0xfffffffd, 0x55555555},
     {NoFlag, 0xfffffffd, 0x7ffffffd}, {NoFlag, 0xfffffffd, 0x7ffffffe},
     {NoFlag, 0xfffffffd, 0x7fffffff}, {NoFlag, 0xfffffffd, 0x80000000},
     {NoFlag, 0xfffffffd, 0x80000001}, {NoFlag, 0xfffffffd, 0xaaaaaaaa},
     {NoFlag, 0xfffffffd, 0xcccccccc}, {NoFlag, 0xfffffffd, 0xffff8000},
     {NoFlag, 0xfffffffd, 0xffff8001}, {NoFlag, 0xfffffffd, 0xffff8002},
     {NoFlag, 0xfffffffd, 0xffff8003}, {NoFlag, 0xfffffffd, 0xffffff80},
     {NoFlag, 0xfffffffd, 0xffffff81}, {NoFlag, 0xfffffffd, 0xffffff82},
     {NoFlag, 0xfffffffd, 0xffffff83}, {NoFlag, 0xfffffffd, 0xffffffe0},
     {NoFlag, 0xfffffffd, 0xfffffffd}, {NoFlag, 0xfffffffd, 0xfffffffe},
     {NoFlag, 0xfffffffd, 0xffffffff}, {NoFlag, 0xfffffffe, 0x00000000},
     {NoFlag, 0xfffffffe, 0x00000001}, {NoFlag, 0xfffffffe, 0x00000002},
     {NoFlag, 0xfffffffe, 0x00000020}, {NoFlag, 0xfffffffe, 0x0000007d},
     {NoFlag, 0xfffffffe, 0x0000007e}, {NoFlag, 0xfffffffe, 0x0000007f},
     {NoFlag, 0xfffffffe, 0x00007ffd}, {NoFlag, 0xfffffffe, 0x00007ffe},
     {NoFlag, 0xfffffffe, 0x00007fff}, {NoFlag, 0xfffffffe, 0x33333333},
     {NoFlag, 0xfffffffe, 0x55555555}, {NoFlag, 0xfffffffe, 0x7ffffffd},
     {NoFlag, 0xfffffffe, 0x7ffffffe}, {NoFlag, 0xfffffffe, 0x7fffffff},
     {NoFlag, 0xfffffffe, 0x80000000}, {NoFlag, 0xfffffffe, 0x80000001},
     {NoFlag, 0xfffffffe, 0xaaaaaaaa}, {NoFlag, 0xfffffffe, 0xcccccccc},
     {NoFlag, 0xfffffffe, 0xffff8000}, {NoFlag, 0xfffffffe, 0xffff8001},
     {NoFlag, 0xfffffffe, 0xffff8002}, {NoFlag, 0xfffffffe, 0xffff8003},
     {NoFlag, 0xfffffffe, 0xffffff80}, {NoFlag, 0xfffffffe, 0xffffff81},
     {NoFlag, 0xfffffffe, 0xffffff82}, {NoFlag, 0xfffffffe, 0xffffff83},
     {NoFlag, 0xfffffffe, 0xffffffe0}, {NoFlag, 0xfffffffe, 0xfffffffd},
     {NoFlag, 0xfffffffe, 0xfffffffe}, {NoFlag, 0xfffffffe, 0xffffffff},
     {NoFlag, 0xffffffff, 0x00000000}, {NoFlag, 0xffffffff, 0x00000001},
     {NoFlag, 0xffffffff, 0x00000002}, {NoFlag, 0xffffffff, 0x00000020},
     {NoFlag, 0xffffffff, 0x0000007d}, {NoFlag, 0xffffffff, 0x0000007e},
     {NoFlag, 0xffffffff, 0x0000007f}, {NoFlag, 0xffffffff, 0x00007ffd},
     {NoFlag, 0xffffffff, 0x00007ffe}, {NoFlag, 0xffffffff, 0x00007fff},
     {NoFlag, 0xffffffff, 0x33333333}, {NoFlag, 0xffffffff, 0x55555555},
     {NoFlag, 0xffffffff, 0x7ffffffd}, {NoFlag, 0xffffffff, 0x7ffffffe},
     {NoFlag, 0xffffffff, 0x7fffffff}, {NoFlag, 0xffffffff, 0x80000000},
     {NoFlag, 0xffffffff, 0x80000001}, {NoFlag, 0xffffffff, 0xaaaaaaaa},
     {NoFlag, 0xffffffff, 0xcccccccc}, {NoFlag, 0xffffffff, 0xffff8000},
     {NoFlag, 0xffffffff, 0xffff8001}, {NoFlag, 0xffffffff, 0xffff8002},
     {NoFlag, 0xffffffff, 0xffff8003}, {NoFlag, 0xffffffff, 0xffffff80},
     {NoFlag, 0xffffffff, 0xffffff81}, {NoFlag, 0xffffffff, 0xffffff82},
     {NoFlag, 0xffffffff, 0xffffff83}, {NoFlag, 0xffffffff, 0xffffffe0},
     {NoFlag, 0xffffffff, 0xfffffffd}, {NoFlag, 0xffffffff, 0xfffffffe},
     {NoFlag, 0xffffffff, 0xffffffff}};

static const Inputs kModifiedImmediate[] =
    {{NoFlag, 0xabababab, 0x00000000}, {NoFlag, 0xabababab, 0x00000001},
     {NoFlag, 0xabababab, 0x00000002}, {NoFlag, 0xabababab, 0x00000020},
     {NoFlag, 0xabababab, 0x0000007d}, {NoFlag, 0xabababab, 0x0000007e},
     {NoFlag, 0xabababab, 0x0000007f}, {NoFlag, 0xabababab, 0x00007ffd},
     {NoFlag, 0xabababab, 0x00007ffe}, {NoFlag, 0xabababab, 0x00007fff},
     {NoFlag, 0xabababab, 0x33333333}, {NoFlag, 0xabababab, 0x55555555},
     {NoFlag, 0xabababab, 0x7ffffffd}, {NoFlag, 0xabababab, 0x7ffffffe},
     {NoFlag, 0xabababab, 0x7fffffff}, {NoFlag, 0xabababab, 0x80000000},
     {NoFlag, 0xabababab, 0x80000001}, {NoFlag, 0xabababab, 0xaaaaaaaa},
     {NoFlag, 0xabababab, 0xcccccccc}, {NoFlag, 0xabababab, 0xffff8000},
     {NoFlag, 0xabababab, 0xffff8001}, {NoFlag, 0xabababab, 0xffff8002},
     {NoFlag, 0xabababab, 0xffff8003}, {NoFlag, 0xabababab, 0xffffff80},
     {NoFlag, 0xabababab, 0xffffff81}, {NoFlag, 0xabababab, 0xffffff82},
     {NoFlag, 0xabababab, 0xffffff83}, {NoFlag, 0xabababab, 0xffffffe0},
     {NoFlag, 0xabababab, 0xfffffffd}, {NoFlag, 0xabababab, 0xfffffffe},
     {NoFlag, 0xabababab, 0xffffffff}};


// A loop will be generated for each element of this array.
const TestLoopData kTests[] = {{{al, r5, r5, 0x00001560},
                                "al r5 r5 0x00001560",
                                "RdIsRn_al_r5_r5_0x00001560",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r0, r0, 0x00007f80},
                                "al r0 r0 0x00007f80",
                                "RdIsRn_al_r0_r0_0x00007f80",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r1, r1, 0x002ac000},
                                "al r1 r1 0x002ac000",
                                "RdIsRn_al_r1_r1_0x002ac000",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r10, r10, 0x15600000},
                                "al r10 r10 0x15600000",
                                "RdIsRn_al_r10_r10_0x15600000",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r2, r2, 0x00001fe0},
                                "al r2 r2 0x00001fe0",
                                "RdIsRn_al_r2_r2_0x00001fe0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r12, r12, 0x2ac00000},
                                "al r12 r12 0x2ac00000",
                                "RdIsRn_al_r12_r12_0x2ac00000",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r8, r8, 0x00001fe0},
                                "al r8 r8 0x00001fe0",
                                "RdIsRn_al_r8_r8_0x00001fe0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r4, r4, 0x00ff0000},
                                "al r4 r4 0x00ff0000",
                                "RdIsRn_al_r4_r4_0x00ff0000",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r8, r8, 0x0007f800},
                                "al r8 r8 0x0007f800",
                                "RdIsRn_al_r8_r8_0x0007f800",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r6, r6, 0x00005580},
                                "al r6 r6 0x00005580",
                                "RdIsRn_al_r6_r6_0x00005580",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r5, r8, 0x0000ab00},
                                "al r5 r8 0x0000ab00",
                                "RdIsNotRn_al_r5_r8_0x0000ab00",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r0, r4, 0x00005580},
                                "al r0 r4 0x00005580",
                                "RdIsNotRn_al_r0_r4_0x00005580",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r9, r5, 0x0002ac00},
                                "al r9 r5 0x0002ac00",
                                "RdIsNotRn_al_r9_r5_0x0002ac00",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r5, r14, 0x000002ac},
                                "al r5 r14 0x000002ac",
                                "RdIsNotRn_al_r5_r14_0x000002ac",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r10, r7, 0x0002ac00},
                                "al r10 r7 0x0002ac00",
                                "RdIsNotRn_al_r10_r7_0x0002ac00",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r3, r5, 0x00ff00ff},
                                "al r3 r5 0x00ff00ff",
                                "RdIsNotRn_al_r3_r5_0x00ff00ff",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r1, r3, 0x02ac0000},
                                "al r1 r3 0x02ac0000",
                                "RdIsNotRn_al_r1_r3_0x02ac0000",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r1, r10, 0x00558000},
                                "al r1 r10 0x00558000",
                                "RdIsNotRn_al_r1_r10_0x00558000",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r14, r9, 0x01560000},
                                "al r14 r9 0x01560000",
                                "RdIsNotRn_al_r14_r9_0x01560000",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r8, r0, 0x00002ac0},
                                "al r8 r0 0x00002ac0",
                                "RdIsNotRn_al_r8_r0_0x00002ac0",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r0, r0, 0x000001fe},
                                "al r0 r0 0x000001fe",
                                "ModifiedImmediate_al_r0_r0_0x000001fe",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x000003fc},
                                "al r0 r0 0x000003fc",
                                "ModifiedImmediate_al_r0_r0_0x000003fc",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x000007f8},
                                "al r0 r0 0x000007f8",
                                "ModifiedImmediate_al_r0_r0_0x000007f8",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00000ff0},
                                "al r0 r0 0x00000ff0",
                                "ModifiedImmediate_al_r0_r0_0x00000ff0",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00001fe0},
                                "al r0 r0 0x00001fe0",
                                "ModifiedImmediate_al_r0_r0_0x00001fe0",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00003fc0},
                                "al r0 r0 0x00003fc0",
                                "ModifiedImmediate_al_r0_r0_0x00003fc0",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00007f80},
                                "al r0 r0 0x00007f80",
                                "ModifiedImmediate_al_r0_r0_0x00007f80",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x0000ff00},
                                "al r0 r0 0x0000ff00",
                                "ModifiedImmediate_al_r0_r0_0x0000ff00",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x0001fe00},
                                "al r0 r0 0x0001fe00",
                                "ModifiedImmediate_al_r0_r0_0x0001fe00",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x0003fc00},
                                "al r0 r0 0x0003fc00",
                                "ModifiedImmediate_al_r0_r0_0x0003fc00",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x0007f800},
                                "al r0 r0 0x0007f800",
                                "ModifiedImmediate_al_r0_r0_0x0007f800",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x000ff000},
                                "al r0 r0 0x000ff000",
                                "ModifiedImmediate_al_r0_r0_0x000ff000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x001fe000},
                                "al r0 r0 0x001fe000",
                                "ModifiedImmediate_al_r0_r0_0x001fe000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x003fc000},
                                "al r0 r0 0x003fc000",
                                "ModifiedImmediate_al_r0_r0_0x003fc000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x007f8000},
                                "al r0 r0 0x007f8000",
                                "ModifiedImmediate_al_r0_r0_0x007f8000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00ff0000},
                                "al r0 r0 0x00ff0000",
                                "ModifiedImmediate_al_r0_r0_0x00ff0000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x01fe0000},
                                "al r0 r0 0x01fe0000",
                                "ModifiedImmediate_al_r0_r0_0x01fe0000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x03fc0000},
                                "al r0 r0 0x03fc0000",
                                "ModifiedImmediate_al_r0_r0_0x03fc0000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x07f80000},
                                "al r0 r0 0x07f80000",
                                "ModifiedImmediate_al_r0_r0_0x07f80000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x0ff00000},
                                "al r0 r0 0x0ff00000",
                                "ModifiedImmediate_al_r0_r0_0x0ff00000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x1fe00000},
                                "al r0 r0 0x1fe00000",
                                "ModifiedImmediate_al_r0_r0_0x1fe00000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x3fc00000},
                                "al r0 r0 0x3fc00000",
                                "ModifiedImmediate_al_r0_r0_0x3fc00000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x7f800000},
                                "al r0 r0 0x7f800000",
                                "ModifiedImmediate_al_r0_r0_0x7f800000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0xff000000},
                                "al r0 r0 0xff000000",
                                "ModifiedImmediate_al_r0_r0_0xff000000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x000000ff},
                                "al r0 r0 0x000000ff",
                                "ModifiedImmediate_al_r0_r0_0x000000ff",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00ff00ff},
                                "al r0 r0 0x00ff00ff",
                                "ModifiedImmediate_al_r0_r0_0x00ff00ff",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0xff00ff00},
                                "al r0 r0 0xff00ff00",
                                "ModifiedImmediate_al_r0_r0_0xff00ff00",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0xffffffff},
                                "al r0 r0 0xffffffff",
                                "ModifiedImmediate_al_r0_r0_0xffffffff",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00000156},
                                "al r0 r0 0x00000156",
                                "ModifiedImmediate_al_r0_r0_0x00000156",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x000002ac},
                                "al r0 r0 0x000002ac",
                                "ModifiedImmediate_al_r0_r0_0x000002ac",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00000558},
                                "al r0 r0 0x00000558",
                                "ModifiedImmediate_al_r0_r0_0x00000558",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00000ab0},
                                "al r0 r0 0x00000ab0",
                                "ModifiedImmediate_al_r0_r0_0x00000ab0",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00001560},
                                "al r0 r0 0x00001560",
                                "ModifiedImmediate_al_r0_r0_0x00001560",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00002ac0},
                                "al r0 r0 0x00002ac0",
                                "ModifiedImmediate_al_r0_r0_0x00002ac0",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00005580},
                                "al r0 r0 0x00005580",
                                "ModifiedImmediate_al_r0_r0_0x00005580",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x0000ab00},
                                "al r0 r0 0x0000ab00",
                                "ModifiedImmediate_al_r0_r0_0x0000ab00",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00015600},
                                "al r0 r0 0x00015600",
                                "ModifiedImmediate_al_r0_r0_0x00015600",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x0002ac00},
                                "al r0 r0 0x0002ac00",
                                "ModifiedImmediate_al_r0_r0_0x0002ac00",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00055800},
                                "al r0 r0 0x00055800",
                                "ModifiedImmediate_al_r0_r0_0x00055800",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x000ab000},
                                "al r0 r0 0x000ab000",
                                "ModifiedImmediate_al_r0_r0_0x000ab000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00156000},
                                "al r0 r0 0x00156000",
                                "ModifiedImmediate_al_r0_r0_0x00156000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x002ac000},
                                "al r0 r0 0x002ac000",
                                "ModifiedImmediate_al_r0_r0_0x002ac000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00558000},
                                "al r0 r0 0x00558000",
                                "ModifiedImmediate_al_r0_r0_0x00558000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00ab0000},
                                "al r0 r0 0x00ab0000",
                                "ModifiedImmediate_al_r0_r0_0x00ab0000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x01560000},
                                "al r0 r0 0x01560000",
                                "ModifiedImmediate_al_r0_r0_0x01560000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x02ac0000},
                                "al r0 r0 0x02ac0000",
                                "ModifiedImmediate_al_r0_r0_0x02ac0000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x05580000},
                                "al r0 r0 0x05580000",
                                "ModifiedImmediate_al_r0_r0_0x05580000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x0ab00000},
                                "al r0 r0 0x0ab00000",
                                "ModifiedImmediate_al_r0_r0_0x0ab00000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x15600000},
                                "al r0 r0 0x15600000",
                                "ModifiedImmediate_al_r0_r0_0x15600000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x2ac00000},
                                "al r0 r0 0x2ac00000",
                                "ModifiedImmediate_al_r0_r0_0x2ac00000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x55800000},
                                "al r0 r0 0x55800000",
                                "ModifiedImmediate_al_r0_r0_0x55800000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0xab000000},
                                "al r0 r0 0xab000000",
                                "ModifiedImmediate_al_r0_r0_0xab000000",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x000000ab},
                                "al r0 r0 0x000000ab",
                                "ModifiedImmediate_al_r0_r0_0x000000ab",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0x00ab00ab},
                                "al r0 r0 0x00ab00ab",
                                "ModifiedImmediate_al_r0_r0_0x00ab00ab",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0xab00ab00},
                                "al r0 r0 0xab00ab00",
                                "ModifiedImmediate_al_r0_r0_0xab00ab00",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, r0, 0xabababab},
                                "al r0 r0 0xabababab",
                                "ModifiedImmediate_al_r0_r0_0xabababab",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate}};

// We record all inputs to the instructions as outputs. This way, we also check
// that what shouldn't change didn't change.
struct TestResult {
  size_t output_size;
  const Inputs* outputs;
};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-adc-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-adcs-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-add-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-adds-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-and-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-ands-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-bic-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-bics-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-eor-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-eors-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-orn-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-orns-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-orr-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-orrs-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-rsb-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-rsbs-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-sbc-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-sbcs-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-sub-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-const-subs-t32.h"


// The maximum number of errors to report in detail for each test.
const unsigned kErrorReportLimit = 8;

typedef void (MacroAssembler::*Fn)(Condition cond,
                                   Register rd,
                                   Register rn,
                                   const Operand& op);

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
    Condition cond = kTests[i].operands.cond;
    Register rd = kTests[i].operands.rd;
    Register rn = kTests[i].operands.rn;
    uint32_t immediate = kTests[i].operands.immediate;
    Operand op(immediate);
    scratch_registers.Exclude(rd);
    scratch_registers.Exclude(rn);

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
      Register nzcv_bits = temp_registers.Acquire();
      Register saved_q_bit = temp_registers.Acquire();
      // Save the `Q` bit flag.
      __ Mrs(saved_q_bit, APSR);
      __ And(saved_q_bit, saved_q_bit, QFlag);
      // Set the `NZCV` and `Q` flags together.
      __ Ldr(nzcv_bits, MemOperand(input_ptr, offsetof(Inputs, apsr)));
      __ Orr(nzcv_bits, nzcv_bits, saved_q_bit);
      __ Msr(APSR_nzcvq, nzcv_bits);
    }
    __ Ldr(rd, MemOperand(input_ptr, offsetof(Inputs, rd)));
    __ Ldr(rn, MemOperand(input_ptr, offsetof(Inputs, rn)));

    (masm.*instruction)(cond, rd, rn, op);

    {
      UseScratchRegisterScope temp_registers(&masm);
      Register nzcv_bits = temp_registers.Acquire();
      __ Mrs(nzcv_bits, APSR);
      // Only record the NZCV bits.
      __ And(nzcv_bits, nzcv_bits, NZCVFlag);
      __ Str(nzcv_bits, MemOperand(result_ptr, offsetof(Inputs, apsr)));
    }
    __ Str(rd, MemOperand(result_ptr, offsetof(Inputs, rd)));
    __ Str(rn, MemOperand(result_ptr, offsetof(Inputs, rn)));

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
        printf("0x%08" PRIx32, results[i]->outputs[j].apsr);
        printf(", ");
        printf("0x%08" PRIx32, results[i]->outputs[j].rd);
        printf(", ");
        printf("0x%08" PRIx32, results[i]->outputs[j].rn);
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
        uint32_t apsr = results[i]->outputs[j].apsr;
        uint32_t rd = results[i]->outputs[j].rd;
        uint32_t rn = results[i]->outputs[j].rn;
        uint32_t apsr_input = kTests[i].inputs[j].apsr;
        uint32_t rd_input = kTests[i].inputs[j].rd;
        uint32_t rn_input = kTests[i].inputs[j].rn;
        uint32_t apsr_ref = reference[i].outputs[j].apsr;
        uint32_t rd_ref = reference[i].outputs[j].rd;
        uint32_t rn_ref = reference[i].outputs[j].rn;

        if (((apsr != apsr_ref) || (rd != rd_ref) || (rn != rn_ref)) &&
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
          printf("0x%08" PRIx32, apsr_input);
          printf(", ");
          printf("0x%08" PRIx32, rd_input);
          printf(", ");
          printf("0x%08" PRIx32, rn_input);
          printf("\n");
          printf("  Expected: ");
          printf("0x%08" PRIx32, apsr_ref);
          printf(", ");
          printf("0x%08" PRIx32, rd_ref);
          printf(", ");
          printf("0x%08" PRIx32, rn_ref);
          printf("\n");
          printf("  Found:    ");
          printf("0x%08" PRIx32, apsr);
          printf(", ");
          printf("0x%08" PRIx32, rd);
          printf(", ");
          printf("0x%08" PRIx32, rn);
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
  Test test_##mnemonic("AARCH32_SIMULATOR_COND_RD_RN_OPERAND_CONST_" #mnemonic \
                       "_T32",                                                 \
                       &Test_##mnemonic);
#else
#define TEST(mnemonic)                                                         \
  void Test_##mnemonic() {                                                     \
    VIXL_WARNING("This test can only run on a 32-bit host.\n");                \
    USE(TestHelper);                                                           \
  }                                                                            \
  Test test_##mnemonic("AARCH32_SIMULATOR_COND_RD_RN_OPERAND_CONST_" #mnemonic \
                       "_T32",                                                 \
                       &Test_##mnemonic);
#endif

FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
