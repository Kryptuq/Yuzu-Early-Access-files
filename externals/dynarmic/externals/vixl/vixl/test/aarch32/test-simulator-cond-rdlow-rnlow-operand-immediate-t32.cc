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
  M(Add)                       \
  M(Adds)                      \
  M(Rsb)                       \
  M(Rsbs)                      \
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
  int32_t immediate;
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
    {{ZFlag, 0x00007fff, 0x00007fff},    {CFlag, 0xffffff83, 0xffffff83},
     {NCFlag, 0x00000000, 0x00000000},   {NCVFlag, 0x00000000, 0x00000000},
     {NZFlag, 0x00000000, 0x00000000},   {VFlag, 0x00000002, 0x00000002},
     {NCFlag, 0xfffffffe, 0xfffffffe},   {NCVFlag, 0x00007ffd, 0x00007ffd},
     {NZCVFlag, 0xffffffff, 0xffffffff}, {ZVFlag, 0xffffffff, 0xffffffff},
     {CFlag, 0x00000002, 0x00000002},    {ZFlag, 0x80000001, 0x80000001},
     {ZCFlag, 0x00007ffe, 0x00007ffe},   {ZVFlag, 0xffff8000, 0xffff8000},
     {CFlag, 0x0000007e, 0x0000007e},    {ZFlag, 0xcccccccc, 0xcccccccc},
     {NVFlag, 0xffff8000, 0xffff8000},   {CFlag, 0x00000001, 0x00000001},
     {NFlag, 0x00000001, 0x00000001},    {NZFlag, 0xffffffe0, 0xffffffe0},
     {CVFlag, 0xfffffffd, 0xfffffffd},   {ZVFlag, 0x00007ffe, 0x00007ffe},
     {ZCVFlag, 0x55555555, 0x55555555},  {NCFlag, 0x00000020, 0x00000020},
     {ZCFlag, 0xffffff81, 0xffffff81},   {ZCFlag, 0xcccccccc, 0xcccccccc},
     {ZCFlag, 0x00000020, 0x00000020},   {NCVFlag, 0xffff8000, 0xffff8000},
     {NZVFlag, 0xffff8001, 0xffff8001},  {ZVFlag, 0xffffff81, 0xffffff81},
     {NZVFlag, 0xffffff81, 0xffffff81},  {NZVFlag, 0x00000000, 0x00000000},
     {NVFlag, 0x00000001, 0x00000001},   {NCVFlag, 0x33333333, 0x33333333},
     {NZCVFlag, 0xffff8001, 0xffff8001}, {NZCFlag, 0xffffffff, 0xffffffff},
     {NCVFlag, 0x80000000, 0x80000000},  {ZCFlag, 0x00000001, 0x00000001},
     {CVFlag, 0x00000020, 0x00000020},   {NCFlag, 0xffff8003, 0xffff8003},
     {CVFlag, 0x00000002, 0x00000002},   {NZCFlag, 0x80000000, 0x80000000},
     {VFlag, 0xffffff83, 0xffffff83},    {NZFlag, 0x33333333, 0x33333333},
     {NCVFlag, 0x00007ffe, 0x00007ffe},  {NFlag, 0xffffff81, 0xffffff81},
     {NZVFlag, 0x00000020, 0x00000020},  {CVFlag, 0x00007fff, 0x00007fff},
     {NZCFlag, 0xffff8003, 0xffff8003},  {ZCFlag, 0xfffffffd, 0xfffffffd},
     {NZFlag, 0xcccccccc, 0xcccccccc},   {ZCFlag, 0xffffffff, 0xffffffff},
     {ZVFlag, 0xffffffe0, 0xffffffe0},   {ZCFlag, 0x55555555, 0x55555555},
     {NVFlag, 0x00000020, 0x00000020},   {NFlag, 0xffff8000, 0xffff8000},
     {ZFlag, 0x00000000, 0x00000000},    {VFlag, 0xffffffe0, 0xffffffe0},
     {CFlag, 0xffff8001, 0xffff8001},    {NZCVFlag, 0xfffffffd, 0xfffffffd},
     {NCVFlag, 0x0000007d, 0x0000007d},  {NCVFlag, 0x0000007f, 0x0000007f},
     {NZFlag, 0xffff8003, 0xffff8003},   {ZCFlag, 0xffffff82, 0xffffff82},
     {ZFlag, 0xffff8001, 0xffff8001},    {NZFlag, 0xffff8002, 0xffff8002},
     {NZFlag, 0x00000020, 0x00000020},   {NCFlag, 0x33333333, 0x33333333},
     {ZCVFlag, 0x80000000, 0x80000000},  {NZCFlag, 0xffffffe0, 0xffffffe0},
     {NZFlag, 0x00007fff, 0x00007fff},   {NZVFlag, 0x00000002, 0x00000002},
     {NFlag, 0x55555555, 0x55555555},    {NVFlag, 0xffffffff, 0xffffffff},
     {NCFlag, 0x00007fff, 0x00007fff},   {NZCVFlag, 0xffffff81, 0xffffff81},
     {ZCVFlag, 0x00007fff, 0x00007fff},  {NZFlag, 0x0000007d, 0x0000007d},
     {VFlag, 0x00007ffe, 0x00007ffe},    {CVFlag, 0xffffff83, 0xffffff83},
     {NZFlag, 0x80000000, 0x80000000},   {ZCFlag, 0xaaaaaaaa, 0xaaaaaaaa},
     {NCFlag, 0x0000007f, 0x0000007f},   {CFlag, 0x00000020, 0x00000020},
     {NZCFlag, 0x00000001, 0x00000001},  {NZCFlag, 0xcccccccc, 0xcccccccc},
     {NZCFlag, 0xffffff80, 0xffffff80},  {NCVFlag, 0xffff8003, 0xffff8003},
     {NFlag, 0x0000007d, 0x0000007d},    {NZVFlag, 0x55555555, 0x55555555},
     {NZCVFlag, 0xfffffffe, 0xfffffffe}, {NCFlag, 0xffff8000, 0xffff8000},
     {CFlag, 0x80000001, 0x80000001},    {NVFlag, 0xffffffe0, 0xffffffe0},
     {CFlag, 0xffffff82, 0xffffff82},    {NZCVFlag, 0x00007ffd, 0x00007ffd},
     {CFlag, 0xaaaaaaaa, 0xaaaaaaaa},    {NZCFlag, 0xffffff81, 0xffffff81},
     {ZFlag, 0x00007ffd, 0x00007ffd},    {NZVFlag, 0xffffffe0, 0xffffffe0},
     {NZVFlag, 0x00007ffe, 0x00007ffe},  {NZFlag, 0x7ffffffd, 0x7ffffffd},
     {NZCFlag, 0xfffffffe, 0xfffffffe},  {NZCVFlag, 0x7ffffffd, 0x7ffffffd},
     {ZVFlag, 0xaaaaaaaa, 0xaaaaaaaa},   {CFlag, 0x00000000, 0x00000000},
     {ZCVFlag, 0xaaaaaaaa, 0xaaaaaaaa},  {NZFlag, 0x00007ffd, 0x00007ffd},
     {VFlag, 0xffff8000, 0xffff8000},    {CVFlag, 0xcccccccc, 0xcccccccc},
     {NZCVFlag, 0xffff8003, 0xffff8003}, {NVFlag, 0x00007ffe, 0x00007ffe},
     {ZVFlag, 0xfffffffd, 0xfffffffd},   {NZCFlag, 0x0000007e, 0x0000007e},
     {VFlag, 0x00000000, 0x00000000},    {NZFlag, 0x00000002, 0x00000002},
     {ZCFlag, 0x7fffffff, 0x7fffffff},   {ZFlag, 0x0000007e, 0x0000007e},
     {NZCFlag, 0x7fffffff, 0x7fffffff},  {ZVFlag, 0xffff8002, 0xffff8002},
     {ZFlag, 0x00000020, 0x00000020},    {NZFlag, 0xffffff81, 0xffffff81},
     {ZCVFlag, 0x00000000, 0x00000000},  {CFlag, 0xffffff81, 0xffffff81},
     {NVFlag, 0x0000007e, 0x0000007e},   {NZFlag, 0x55555555, 0x55555555},
     {CFlag, 0x0000007d, 0x0000007d},    {NCFlag, 0x7fffffff, 0x7fffffff},
     {NCFlag, 0xffff8001, 0xffff8001},   {CFlag, 0xffffffe0, 0xffffffe0},
     {ZVFlag, 0x7ffffffe, 0x7ffffffe},   {VFlag, 0x7ffffffd, 0x7ffffffd},
     {CFlag, 0xffffffff, 0xffffffff},    {ZCFlag, 0xffff8000, 0xffff8000},
     {NZVFlag, 0xffff8003, 0xffff8003},  {NCFlag, 0x0000007d, 0x0000007d},
     {NZVFlag, 0x0000007f, 0x0000007f},  {CFlag, 0xcccccccc, 0xcccccccc},
     {CVFlag, 0x00000001, 0x00000001},   {NZCFlag, 0xffff8001, 0xffff8001},
     {NCVFlag, 0x00000020, 0x00000020},  {NZVFlag, 0xffffff82, 0xffffff82},
     {NCFlag, 0x00007ffd, 0x00007ffd},   {ZVFlag, 0x0000007e, 0x0000007e},
     {NZFlag, 0x80000001, 0x80000001},   {NVFlag, 0x7ffffffe, 0x7ffffffe},
     {NZCVFlag, 0x00000000, 0x00000000}, {CVFlag, 0x55555555, 0x55555555},
     {ZVFlag, 0x00000020, 0x00000020},   {VFlag, 0x00000020, 0x00000020},
     {NFlag, 0xcccccccc, 0xcccccccc},    {NVFlag, 0x0000007d, 0x0000007d},
     {CVFlag, 0x0000007f, 0x0000007f},   {NZCFlag, 0x00000002, 0x00000002},
     {NZCFlag, 0x00007fff, 0x00007fff},  {NZVFlag, 0x0000007d, 0x0000007d},
     {NZVFlag, 0xfffffffd, 0xfffffffd},  {NFlag, 0x00000002, 0x00000002},
     {NZCVFlag, 0x0000007d, 0x0000007d}, {NVFlag, 0xffffff82, 0xffffff82},
     {VFlag, 0x00007fff, 0x00007fff},    {NZCVFlag, 0x00000001, 0x00000001},
     {ZFlag, 0xaaaaaaaa, 0xaaaaaaaa},    {CVFlag, 0xffff8002, 0xffff8002},
     {NFlag, 0x7fffffff, 0x7fffffff},    {NCVFlag, 0xffffff80, 0xffffff80},
     {ZFlag, 0x33333333, 0x33333333},    {NZFlag, 0x00000001, 0x00000001},
     {NCVFlag, 0x7ffffffe, 0x7ffffffe},  {VFlag, 0x00000001, 0x00000001},
     {ZCFlag, 0x0000007f, 0x0000007f},   {ZVFlag, 0x80000001, 0x80000001},
     {ZVFlag, 0xffff8003, 0xffff8003},   {NCVFlag, 0xaaaaaaaa, 0xaaaaaaaa},
     {NZCVFlag, 0xffffffe0, 0xffffffe0}, {VFlag, 0x33333333, 0x33333333},
     {NCFlag, 0xffffff80, 0xffffff80},   {ZFlag, 0x00007ffe, 0x00007ffe},
     {NVFlag, 0x00000000, 0x00000000},   {NZCFlag, 0x00007ffd, 0x00007ffd},
     {NZFlag, 0xfffffffd, 0xfffffffd},   {VFlag, 0xffffffff, 0xffffffff},
     {NZCFlag, 0xffffff83, 0xffffff83},  {NZVFlag, 0xffff8002, 0xffff8002},
     {NZCVFlag, 0x33333333, 0x33333333}, {ZCVFlag, 0xfffffffd, 0xfffffffd},
     {ZCFlag, 0x80000001, 0x80000001},   {NCFlag, 0x00007ffe, 0x00007ffe},
     {NFlag, 0xfffffffd, 0xfffffffd},    {NZCVFlag, 0x7fffffff, 0x7fffffff},
     {VFlag, 0xffff8001, 0xffff8001},    {NZVFlag, 0xffff8000, 0xffff8000},
     {ZCVFlag, 0xcccccccc, 0xcccccccc},  {VFlag, 0x7ffffffe, 0x7ffffffe},
     {ZFlag, 0xffff8002, 0xffff8002},    {ZCFlag, 0x0000007d, 0x0000007d},
     {NZCVFlag, 0xffffff83, 0xffffff83}, {ZVFlag, 0x00000001, 0x00000001},
     {NZCVFlag, 0xffff8002, 0xffff8002}, {ZCFlag, 0xffffff83, 0xffffff83},
     {CFlag, 0xffff8003, 0xffff8003},    {NCVFlag, 0xffffffff, 0xffffffff},
     {CFlag, 0xfffffffe, 0xfffffffe},    {NZFlag, 0xffffff80, 0xffffff80},
     {CVFlag, 0x00007ffd, 0x00007ffd},   {ZCVFlag, 0x0000007d, 0x0000007d},
     {ZVFlag, 0xcccccccc, 0xcccccccc},   {NVFlag, 0x00007ffd, 0x00007ffd},
     {NZFlag, 0xaaaaaaaa, 0xaaaaaaaa},   {ZFlag, 0x0000007f, 0x0000007f},
     {CVFlag, 0xffff8003, 0xffff8003},   {ZFlag, 0xffffffe0, 0xffffffe0},
     {NZVFlag, 0xcccccccc, 0xcccccccc},  {NFlag, 0x00007ffe, 0x00007ffe},
     {NFlag, 0xffff8003, 0xffff8003},    {NZVFlag, 0xffffffff, 0xffffffff},
     {CFlag, 0x00007fff, 0x00007fff},    {NCVFlag, 0x7fffffff, 0x7fffffff},
     {CFlag, 0x33333333, 0x33333333},    {ZCFlag, 0x80000000, 0x80000000},
     {ZCFlag, 0x0000007e, 0x0000007e},   {NVFlag, 0x7fffffff, 0x7fffffff},
     {NZFlag, 0x7ffffffe, 0x7ffffffe},   {CFlag, 0xffff8002, 0xffff8002},
     {NCFlag, 0x80000000, 0x80000000},   {VFlag, 0x7fffffff, 0x7fffffff},
     {NZFlag, 0xffffffff, 0xffffffff},   {NFlag, 0x7ffffffd, 0x7ffffffd},
     {ZFlag, 0x00000002, 0x00000002},    {ZCVFlag, 0xffff8003, 0xffff8003},
     {CFlag, 0x7fffffff, 0x7fffffff},    {CFlag, 0x0000007f, 0x0000007f},
     {VFlag, 0x80000001, 0x80000001},    {ZCFlag, 0x00007fff, 0x00007fff},
     {CFlag, 0xfffffffd, 0xfffffffd},    {NZFlag, 0x7fffffff, 0x7fffffff},
     {CVFlag, 0xffff8001, 0xffff8001},   {NZVFlag, 0x00007fff, 0x00007fff},
     {NZVFlag, 0x7ffffffe, 0x7ffffffe},  {ZCVFlag, 0x0000007e, 0x0000007e},
     {NZVFlag, 0x00000001, 0x00000001},  {ZFlag, 0xffffffff, 0xffffffff},
     {NZFlag, 0xfffffffe, 0xfffffffe},   {NZCVFlag, 0x0000007f, 0x0000007f},
     {NZCVFlag, 0x80000000, 0x80000000}, {VFlag, 0x55555555, 0x55555555},
     {NVFlag, 0xffff8001, 0xffff8001},   {NFlag, 0xffffff83, 0xffffff83},
     {NZVFlag, 0x7ffffffd, 0x7ffffffd},  {ZVFlag, 0x00000000, 0x00000000},
     {NCVFlag, 0xfffffffd, 0xfffffffd},  {CFlag, 0x00007ffe, 0x00007ffe},
     {NVFlag, 0xffffff83, 0xffffff83},   {NZFlag, 0x0000007e, 0x0000007e},
     {VFlag, 0x80000000, 0x80000000},    {NZCFlag, 0xffff8000, 0xffff8000},
     {ZCFlag, 0xffff8001, 0xffff8001},   {NVFlag, 0x00007fff, 0x00007fff},
     {ZCVFlag, 0xffffff81, 0xffffff81},  {NZVFlag, 0x0000007e, 0x0000007e},
     {CVFlag, 0xffff8000, 0xffff8000},   {VFlag, 0xffffff82, 0xffffff82},
     {VFlag, 0xaaaaaaaa, 0xaaaaaaaa},    {NZCVFlag, 0xcccccccc, 0xcccccccc},
     {CVFlag, 0x33333333, 0x33333333},   {NCFlag, 0xffffffff, 0xffffffff},
     {VFlag, 0xffffff80, 0xffffff80},    {NVFlag, 0xffff8002, 0xffff8002},
     {ZCFlag, 0x00000000, 0x00000000},   {ZCVFlag, 0x80000001, 0x80000001},
     {NZCVFlag, 0x80000001, 0x80000001}, {NCFlag, 0x55555555, 0x55555555},
     {CVFlag, 0x7ffffffe, 0x7ffffffe},   {ZVFlag, 0x7ffffffd, 0x7ffffffd},
     {ZCVFlag, 0xffff8000, 0xffff8000},  {ZCFlag, 0xffff8002, 0xffff8002},
     {NFlag, 0xfffffffe, 0xfffffffe},    {ZCVFlag, 0x0000007f, 0x0000007f},
     {NCVFlag, 0xfffffffe, 0xfffffffe},  {ZCVFlag, 0x7fffffff, 0x7fffffff},
     {CVFlag, 0x0000007e, 0x0000007e},   {NZCVFlag, 0x7ffffffe, 0x7ffffffe},
     {CFlag, 0xffffff80, 0xffffff80},    {VFlag, 0x00007ffd, 0x00007ffd},
     {CVFlag, 0xffffffe0, 0xffffffe0},   {CFlag, 0x7ffffffe, 0x7ffffffe},
     {CFlag, 0x80000000, 0x80000000},    {CVFlag, 0xfffffffe, 0xfffffffe},
     {NCVFlag, 0xffff8002, 0xffff8002},  {ZCVFlag, 0x33333333, 0x33333333},
     {NZCFlag, 0xaaaaaaaa, 0xaaaaaaaa},  {NVFlag, 0x55555555, 0x55555555},
     {NVFlag, 0x80000000, 0x80000000},   {NZCVFlag, 0x00007ffe, 0x00007ffe},
     {ZFlag, 0xffffff83, 0xffffff83},    {NCFlag, 0xaaaaaaaa, 0xaaaaaaaa},
     {NZVFlag, 0xfffffffe, 0xfffffffe},  {CVFlag, 0xaaaaaaaa, 0xaaaaaaaa},
     {NZCFlag, 0xffff8002, 0xffff8002},  {VFlag, 0x0000007d, 0x0000007d}};

static const Inputs kRdIsNotRn[] =
    {{NZCVFlag, 0xffff8000, 0xffffffff}, {CVFlag, 0xcccccccc, 0xffffff80},
     {NCFlag, 0xfffffffe, 0xfffffffd},   {NCFlag, 0xcccccccc, 0xcccccccc},
     {ZFlag, 0x80000001, 0xffffffff},    {NCFlag, 0xffffff82, 0x0000007d},
     {NCVFlag, 0x0000007d, 0xffff8001},  {NVFlag, 0x00007fff, 0x7fffffff},
     {NVFlag, 0x80000001, 0xffffff82},   {ZCVFlag, 0xffffffff, 0x0000007e},
     {ZCVFlag, 0xffffff80, 0x0000007d},  {ZCVFlag, 0x00000020, 0xaaaaaaaa},
     {CVFlag, 0x00007fff, 0x00000020},   {NVFlag, 0xffff8000, 0xffffff82},
     {NZFlag, 0xffffff82, 0x7ffffffe},   {NZVFlag, 0xffff8003, 0x33333333},
     {ZCVFlag, 0x00007ffe, 0x7ffffffe},  {NFlag, 0x0000007f, 0xffffffff},
     {NZCFlag, 0x33333333, 0x80000000},  {ZCFlag, 0x80000001, 0xffffffff},
     {VFlag, 0x00007ffd, 0x00000001},    {ZVFlag, 0xffffffff, 0x0000007e},
     {NZFlag, 0x00000001, 0xcccccccc},   {NZVFlag, 0x00000020, 0xffff8001},
     {NCVFlag, 0x7ffffffe, 0xffffff80},  {ZCFlag, 0x33333333, 0x00007fff},
     {CFlag, 0xffffff82, 0x0000007e},    {ZCFlag, 0x55555555, 0xffff8003},
     {NFlag, 0x0000007f, 0x00000001},    {ZCVFlag, 0x00000020, 0xfffffffd},
     {ZVFlag, 0x0000007e, 0xffff8003},   {ZCVFlag, 0x00000020, 0x80000001},
     {ZCFlag, 0xfffffffe, 0x55555555},   {NFlag, 0xfffffffd, 0xfffffffe},
     {VFlag, 0xffffff82, 0x7ffffffe},    {CVFlag, 0xffffff80, 0x0000007f},
     {NVFlag, 0x00000020, 0xffff8001},   {NCFlag, 0x0000007d, 0x80000000},
     {CVFlag, 0x00000020, 0xffffff83},   {NZCVFlag, 0x0000007f, 0x00000000},
     {ZVFlag, 0xffffff82, 0x7ffffffd},   {ZVFlag, 0xffff8000, 0x80000001},
     {NFlag, 0xcccccccc, 0x0000007e},    {NFlag, 0x55555555, 0xffffff80},
     {NCFlag, 0x80000000, 0x00007ffd},   {ZVFlag, 0x00007ffd, 0x00007ffe},
     {ZCVFlag, 0x00000000, 0x7ffffffe},  {ZFlag, 0xffff8001, 0xffffffff},
     {NCFlag, 0xfffffffe, 0x55555555},   {VFlag, 0x7ffffffd, 0x80000001},
     {VFlag, 0x80000000, 0xffffffff},    {CFlag, 0xffffff82, 0xffffff80},
     {NZCFlag, 0xffff8003, 0xfffffffd},  {CFlag, 0x00000020, 0x00007fff},
     {NZVFlag, 0xffff8002, 0xfffffffe},  {NVFlag, 0xffffff80, 0x0000007e},
     {NVFlag, 0xcccccccc, 0x0000007d},   {CFlag, 0x0000007f, 0x00000001},
     {ZVFlag, 0xffff8002, 0xffffff81},   {NZVFlag, 0x80000000, 0x00007ffe},
     {NZCVFlag, 0x80000000, 0x7ffffffe}, {ZVFlag, 0x7ffffffe, 0x80000000},
     {ZFlag, 0xffff8000, 0xffffff80},    {VFlag, 0x00007ffd, 0x00007ffd},
     {ZCFlag, 0x0000007d, 0x0000007e},   {NCFlag, 0x00000000, 0x7ffffffe},
     {NVFlag, 0x00000020, 0xaaaaaaaa},   {NVFlag, 0xffffff83, 0x0000007e},
     {NZFlag, 0xffff8002, 0x7ffffffd},   {CVFlag, 0xcccccccc, 0x00000001},
     {NZFlag, 0x33333333, 0x0000007e},   {NZCFlag, 0x00000002, 0x7ffffffd},
     {NZFlag, 0x00007ffd, 0xffffff80},   {ZVFlag, 0x00007ffd, 0xfffffffe},
     {NZCVFlag, 0xffff8000, 0x80000000}, {NZFlag, 0xffff8002, 0xffffffff},
     {CFlag, 0x00000002, 0xffff8000},    {NZCFlag, 0x0000007d, 0xffffff80},
     {ZVFlag, 0x0000007d, 0xffff8003},   {VFlag, 0x55555555, 0xffff8000},
     {CFlag, 0x00007fff, 0x00000001},    {VFlag, 0x80000001, 0x33333333},
     {ZCFlag, 0xaaaaaaaa, 0xffffff82},   {VFlag, 0x00000002, 0x00007ffd},
     {ZVFlag, 0x7ffffffe, 0xffff8001},   {ZCFlag, 0x7fffffff, 0x00000000},
     {CVFlag, 0x55555555, 0x0000007e},   {VFlag, 0x55555555, 0x0000007e},
     {NZCVFlag, 0x80000000, 0x7ffffffd}, {ZFlag, 0x0000007d, 0x00000000},
     {NZVFlag, 0xffffff80, 0x00000002},  {NVFlag, 0xfffffffd, 0x00000001},
     {ZVFlag, 0x7ffffffd, 0xfffffffd},   {VFlag, 0xffff8002, 0x55555555},
     {ZCVFlag, 0x00000001, 0xffff8002},  {NVFlag, 0xffff8002, 0x00000020},
     {ZFlag, 0x33333333, 0xffff8002},    {ZCVFlag, 0x55555555, 0xffffff82},
     {CVFlag, 0x7ffffffe, 0xffffffe0},   {NVFlag, 0xcccccccc, 0x00007ffd},
     {NZFlag, 0x00000001, 0x0000007f},   {ZFlag, 0xfffffffe, 0x00007fff},
     {NCVFlag, 0x80000001, 0xffffff83},  {NCVFlag, 0xaaaaaaaa, 0xffff8002},
     {ZVFlag, 0x00007fff, 0xffff8000},   {ZFlag, 0xffff8001, 0xfffffffd},
     {CFlag, 0x55555555, 0x80000000},    {ZCVFlag, 0xcccccccc, 0xfffffffe},
     {NZCVFlag, 0xffff8003, 0x7fffffff}, {NZCVFlag, 0x00007ffe, 0xffffff81},
     {NZCFlag, 0xfffffffd, 0xaaaaaaaa},  {NVFlag, 0x00000002, 0x00000020},
     {ZCVFlag, 0xffff8003, 0xfffffffd},  {NFlag, 0xffff8001, 0x00007ffe},
     {ZCFlag, 0xaaaaaaaa, 0x00000002},   {NCVFlag, 0xffff8000, 0xfffffffe},
     {CFlag, 0x00000020, 0x00000000},    {NZVFlag, 0x00007ffd, 0xffffff81},
     {NVFlag, 0x00007fff, 0x00000020},   {ZFlag, 0x0000007e, 0x80000001},
     {CVFlag, 0x0000007d, 0xaaaaaaaa},   {NZVFlag, 0x7fffffff, 0x55555555},
     {VFlag, 0xffffffff, 0x00007ffd},    {NFlag, 0x80000000, 0xffffffe0},
     {ZVFlag, 0xffffff82, 0xfffffffe},   {CVFlag, 0x0000007d, 0x00007fff},
     {CFlag, 0x7ffffffe, 0x55555555},    {ZFlag, 0xffffff82, 0x7fffffff},
     {NZCFlag, 0xfffffffd, 0xffffff83},  {CVFlag, 0x80000001, 0x80000000},
     {ZVFlag, 0x00000001, 0xfffffffe},   {CVFlag, 0x00000001, 0x33333333},
     {NZFlag, 0x7ffffffd, 0x80000000},   {NZVFlag, 0x00007ffd, 0xaaaaaaaa},
     {CVFlag, 0x0000007e, 0xffffff82},   {CVFlag, 0x7ffffffe, 0xffff8002},
     {NVFlag, 0xfffffffe, 0xffff8003},   {NFlag, 0x7fffffff, 0x80000001},
     {CFlag, 0xffffff82, 0x00000002},    {CVFlag, 0xcccccccc, 0x0000007e},
     {NZCVFlag, 0x00000001, 0x7ffffffe}, {NFlag, 0x00000000, 0x55555555},
     {NZFlag, 0xffffff81, 0x00000000},   {NZVFlag, 0xffffffe0, 0x0000007e},
     {ZCFlag, 0xffff8002, 0xaaaaaaaa},   {NZVFlag, 0x7fffffff, 0x0000007d},
     {ZVFlag, 0x0000007e, 0xffff8001},   {NCVFlag, 0xffffff83, 0xaaaaaaaa},
     {ZFlag, 0xffffff82, 0xffffff83},    {VFlag, 0x00000001, 0x55555555},
     {NFlag, 0x00000020, 0x80000000},    {NZFlag, 0x00000000, 0xfffffffe},
     {VFlag, 0xffffff83, 0xaaaaaaaa},    {ZFlag, 0xffff8002, 0xffffff83},
     {NZVFlag, 0xcccccccc, 0x7ffffffe},  {ZVFlag, 0x00000000, 0x55555555},
     {NCFlag, 0x33333333, 0xffff8003},   {NZCVFlag, 0xfffffffd, 0x00000000},
     {NZCVFlag, 0x00000020, 0xffff8000}, {CVFlag, 0xffff8001, 0xffffff83},
     {CFlag, 0xffffff83, 0x33333333},    {CVFlag, 0x7ffffffd, 0x00000020},
     {NCVFlag, 0xffffffe0, 0x80000000},  {NCVFlag, 0xffffff82, 0xcccccccc},
     {NZCVFlag, 0xaaaaaaaa, 0x00007fff}, {VFlag, 0xcccccccc, 0xffff8003},
     {ZCVFlag, 0x55555555, 0xffffffe0},  {NZCFlag, 0xffffff83, 0xffff8000},
     {ZVFlag, 0xffff8001, 0x0000007d},   {CVFlag, 0xffffff83, 0x00000002},
     {NVFlag, 0x7fffffff, 0x0000007d},   {VFlag, 0x55555555, 0x33333333},
     {NCVFlag, 0x00000002, 0xffffff80},  {ZFlag, 0xfffffffd, 0x00007fff},
     {ZCFlag, 0xffff8003, 0x0000007d},   {NZCFlag, 0xffff8002, 0xffff8000},
     {NCVFlag, 0x00000020, 0xffff8002},  {ZVFlag, 0xffff8000, 0x00000020},
     {ZCVFlag, 0x0000007f, 0xffffffe0},  {NZCFlag, 0xffffff80, 0x33333333},
     {NCVFlag, 0x00007fff, 0xaaaaaaaa},  {ZCVFlag, 0x00000020, 0xffffff82},
     {NFlag, 0x00000000, 0x0000007d},    {NCVFlag, 0x00000001, 0x00007ffe},
     {ZFlag, 0x80000001, 0x00000002},    {NZVFlag, 0xffff8000, 0x00000020},
     {CVFlag, 0xffff8002, 0xcccccccc},   {NVFlag, 0xffffff83, 0x80000000},
     {ZCFlag, 0x0000007f, 0xffffff80},   {NZFlag, 0xcccccccc, 0xffffffff},
     {NZFlag, 0x00007fff, 0x0000007f},   {NZFlag, 0xfffffffd, 0xffff8001},
     {CFlag, 0x0000007d, 0x7ffffffd},    {ZCFlag, 0xffff8000, 0xffff8003},
     {ZVFlag, 0xffffffff, 0xffffffe0},   {NZCFlag, 0x00007ffd, 0xcccccccc},
     {NCVFlag, 0x0000007d, 0x00000020},  {ZCFlag, 0xaaaaaaaa, 0x80000000},
     {ZCVFlag, 0xfffffffd, 0x0000007d},  {ZFlag, 0xffffffe0, 0xfffffffe},
     {VFlag, 0xffff8003, 0x80000000},    {NZCFlag, 0x00007fff, 0xffffff82},
     {NCVFlag, 0x00000002, 0x33333333},  {NZCVFlag, 0x00000000, 0xffffff82},
     {VFlag, 0x80000000, 0x80000000},    {ZVFlag, 0x80000001, 0x33333333},
     {ZFlag, 0x7ffffffe, 0x00007fff},    {NZCVFlag, 0x7fffffff, 0x00007fff},
     {ZCFlag, 0xffffff83, 0xaaaaaaaa},   {NZVFlag, 0xffff8002, 0xffffffe0},
     {VFlag, 0xfffffffd, 0xffffff81},    {NZFlag, 0x7fffffff, 0x7ffffffd},
     {NZCVFlag, 0x0000007f, 0xcccccccc}, {NZCFlag, 0xffff8001, 0x00000020},
     {ZFlag, 0x00000020, 0xffff8002},    {ZVFlag, 0x55555555, 0x00007ffd},
     {NZVFlag, 0xcccccccc, 0x33333333},  {CFlag, 0x80000000, 0x80000000},
     {ZCVFlag, 0xcccccccc, 0x00007ffd},  {ZFlag, 0xffff8002, 0xffffff80},
     {CVFlag, 0x7ffffffd, 0xffffff80},   {NZCVFlag, 0x00000001, 0x00000001},
     {NFlag, 0xfffffffe, 0xffffffe0},    {ZVFlag, 0x0000007e, 0x0000007f},
     {NCVFlag, 0xffff8002, 0xffff8002},  {ZCVFlag, 0xffffff82, 0x80000001},
     {CVFlag, 0x0000007d, 0x0000007d},   {NZVFlag, 0xcccccccc, 0xffffff83},
     {VFlag, 0xffffff82, 0xfffffffd},    {CFlag, 0x0000007d, 0x0000007f},
     {NFlag, 0x0000007d, 0x00000001},    {NVFlag, 0x00007ffd, 0xfffffffe},
     {NZVFlag, 0x33333333, 0xffffff80},  {NVFlag, 0x0000007e, 0x00007ffe},
     {ZFlag, 0xffff8001, 0x0000007e},    {VFlag, 0x80000000, 0x00000002},
     {NVFlag, 0x00000020, 0x00007ffe},   {CFlag, 0x00007ffe, 0x0000007f},
     {ZCFlag, 0xffff8002, 0x00000020},   {NCVFlag, 0x00000001, 0xffff8000},
     {NZCVFlag, 0x0000007e, 0xffff8001}, {ZCFlag, 0x0000007f, 0xffffff81},
     {ZVFlag, 0xfffffffd, 0x00007ffd},   {NCFlag, 0x0000007d, 0xcccccccc},
     {NZFlag, 0xfffffffd, 0xfffffffe},   {ZCFlag, 0xffff8003, 0x80000000},
     {CFlag, 0x0000007d, 0xffff8002},    {ZVFlag, 0x0000007f, 0x00007ffe},
     {NCVFlag, 0xffff8002, 0x00000001},  {ZCFlag, 0x80000001, 0x0000007e},
     {NCFlag, 0x00000000, 0xaaaaaaaa},   {ZCVFlag, 0x33333333, 0x80000001},
     {CFlag, 0x80000000, 0x0000007d},    {NCVFlag, 0x80000001, 0x80000000},
     {ZCVFlag, 0x7ffffffe, 0xffffff82},  {ZCFlag, 0xfffffffd, 0xffff8000},
     {ZFlag, 0x80000000, 0x55555555},    {ZVFlag, 0x0000007d, 0xffff8000},
     {ZCFlag, 0x80000001, 0xffff8001},   {NVFlag, 0xffff8002, 0x00007ffe},
     {ZCFlag, 0x00000000, 0xffff8000},   {CFlag, 0xffffff81, 0xffffffff},
     {CFlag, 0xffffffff, 0x80000001},    {CFlag, 0x80000001, 0x7ffffffd},
     {ZFlag, 0x80000001, 0x00007fff},    {ZVFlag, 0xffff8002, 0xffff8001},
     {NZFlag, 0x80000000, 0x00007ffd},   {NZCFlag, 0x33333333, 0xcccccccc},
     {ZCFlag, 0xffffffff, 0x33333333},   {VFlag, 0x80000001, 0x55555555},
     {CFlag, 0xffffff82, 0x00000001},    {ZVFlag, 0xffff8003, 0xffff8001},
     {NCVFlag, 0xffff8003, 0xffffffe0},  {ZCVFlag, 0xffffffe0, 0x7ffffffd},
     {NFlag, 0xffff8003, 0x7ffffffe},    {VFlag, 0xffffffff, 0x00000001},
     {CVFlag, 0xcccccccc, 0xfffffffe},   {NZVFlag, 0xffff8003, 0x0000007f},
     {NZVFlag, 0x0000007d, 0x00000002},  {NVFlag, 0x0000007d, 0x00000000},
     {NVFlag, 0x00000002, 0xffff8002},   {ZCVFlag, 0xffff8001, 0xfffffffd},
     {CVFlag, 0x00007fff, 0x0000007d},   {NCFlag, 0x33333333, 0x00000002},
     {NCVFlag, 0xcccccccc, 0xaaaaaaaa},  {CVFlag, 0x80000000, 0x00007ffd},
     {NZFlag, 0xffffff81, 0x55555555},   {CFlag, 0xffff8003, 0x33333333},
     {NZCVFlag, 0xffffffe0, 0xffffff82}, {NVFlag, 0x80000001, 0xfffffffd},
     {CFlag, 0xffff8000, 0x33333333},    {NCVFlag, 0xffff8003, 0x0000007f},
     {CFlag, 0x00000000, 0x0000007d},    {NVFlag, 0x0000007e, 0xcccccccc},
     {NZFlag, 0x7ffffffe, 0xffffffe0},   {ZVFlag, 0xffffffe0, 0xffffff82},
     {ZCVFlag, 0x80000001, 0x55555555},  {NZFlag, 0xcccccccc, 0xffff8001},
     {NFlag, 0x55555555, 0x00000020},    {ZVFlag, 0x00007ffd, 0xffffff83}};

static const Inputs kImmediate[] =
    {{CFlag, 0xabababab, 0xffffff82},    {CVFlag, 0xabababab, 0x0000007f},
     {NFlag, 0xabababab, 0x80000001},    {NZFlag, 0xabababab, 0xfffffffd},
     {VFlag, 0xabababab, 0x7ffffffd},    {CFlag, 0xabababab, 0x7ffffffe},
     {ZFlag, 0xabababab, 0xaaaaaaaa},    {NZVFlag, 0xabababab, 0x0000007e},
     {NZVFlag, 0xabababab, 0x80000001},  {CFlag, 0xabababab, 0xffffff81},
     {NZVFlag, 0xabababab, 0xfffffffd},  {NZFlag, 0xabababab, 0x00007fff},
     {VFlag, 0xabababab, 0x00000001},    {CVFlag, 0xabababab, 0xffffffff},
     {NZCVFlag, 0xabababab, 0xffffffff}, {CFlag, 0xabababab, 0x00000000},
     {NZFlag, 0xabababab, 0xffffff83},   {NCFlag, 0xabababab, 0x0000007e},
     {NZCVFlag, 0xabababab, 0x7fffffff}, {ZCVFlag, 0xabababab, 0x00000002},
     {CFlag, 0xabababab, 0x80000000},    {NZCFlag, 0xabababab, 0x7ffffffd},
     {ZFlag, 0xabababab, 0xffff8000},    {CFlag, 0xabababab, 0x7ffffffd},
     {NVFlag, 0xabababab, 0x55555555},   {NZCFlag, 0xabababab, 0xfffffffd},
     {CVFlag, 0xabababab, 0xaaaaaaaa},   {NZCVFlag, 0xabababab, 0xffff8003},
     {NZFlag, 0xabababab, 0xffffffe0},   {NZCFlag, 0xabababab, 0x00007ffd},
     {ZCVFlag, 0xabababab, 0xffffff80},  {NZFlag, 0xabababab, 0x7ffffffd},
     {ZCFlag, 0xabababab, 0x7fffffff},   {ZVFlag, 0xabababab, 0xffffff81},
     {VFlag, 0xabababab, 0x7fffffff},    {NCVFlag, 0xabababab, 0xcccccccc},
     {ZVFlag, 0xabababab, 0x00007fff},   {NZFlag, 0xabababab, 0x00000002},
     {NVFlag, 0xabababab, 0x00000002},   {ZVFlag, 0xabababab, 0xffff8002},
     {NZVFlag, 0xabababab, 0x00000020},  {ZCVFlag, 0xabababab, 0xaaaaaaaa},
     {ZCFlag, 0xabababab, 0x00000000},   {NZCVFlag, 0xabababab, 0xaaaaaaaa},
     {NZFlag, 0xabababab, 0xfffffffe},   {NZCFlag, 0xabababab, 0xffffffe0},
     {NFlag, 0xabababab, 0xaaaaaaaa},    {ZVFlag, 0xabababab, 0xffffff80},
     {VFlag, 0xabababab, 0x0000007f},    {ZVFlag, 0xabababab, 0x33333333},
     {NZFlag, 0xabababab, 0x00007ffd},   {NCFlag, 0xabababab, 0x00000002},
     {NVFlag, 0xabababab, 0x00007ffd},   {ZFlag, 0xabababab, 0x00000001},
     {CVFlag, 0xabababab, 0x7fffffff},   {CFlag, 0xabababab, 0xaaaaaaaa},
     {NZCVFlag, 0xabababab, 0x80000001}, {CVFlag, 0xabababab, 0x00000002},
     {ZVFlag, 0xabababab, 0x0000007e},   {VFlag, 0xabababab, 0xffffffff},
     {NCFlag, 0xabababab, 0x00000001},   {NZCFlag, 0xabababab, 0xcccccccc},
     {CVFlag, 0xabababab, 0x80000001},   {NVFlag, 0xabababab, 0xffffff82},
     {NZCVFlag, 0xabababab, 0x0000007e}, {CFlag, 0xabababab, 0xffffffe0},
     {ZCFlag, 0xabababab, 0xffff8002},   {NZVFlag, 0xabababab, 0x7fffffff},
     {NZVFlag, 0xabababab, 0x33333333},  {NZCFlag, 0xabababab, 0x0000007d},
     {NFlag, 0xabababab, 0x7ffffffe},    {ZCVFlag, 0xabababab, 0xcccccccc},
     {ZCFlag, 0xabababab, 0xfffffffe},   {NVFlag, 0xabababab, 0x00007ffe},
     {NZFlag, 0xabababab, 0x00007ffe},   {NCVFlag, 0xabababab, 0xffffff82},
     {NZVFlag, 0xabababab, 0x00000002},  {ZVFlag, 0xabababab, 0x7fffffff},
     {NZCFlag, 0xabababab, 0xffff8001},  {VFlag, 0xabababab, 0xffffff83},
     {ZCVFlag, 0xabababab, 0x0000007e},  {NZCFlag, 0xabababab, 0xffffff83},
     {NCFlag, 0xabababab, 0x00007fff},   {NCVFlag, 0xabababab, 0x7ffffffe},
     {CFlag, 0xabababab, 0x00000020},    {NFlag, 0xabababab, 0x00007ffd},
     {NZFlag, 0xabababab, 0x7fffffff},   {NZCFlag, 0xabababab, 0xffff8002},
     {ZVFlag, 0xabababab, 0x0000007f},   {NZVFlag, 0xabababab, 0xffffff83},
     {NZVFlag, 0xabababab, 0xffffffff},  {ZFlag, 0xabababab, 0x80000000},
     {ZVFlag, 0xabababab, 0xffffff82},   {ZVFlag, 0xabababab, 0x80000000},
     {NZFlag, 0xabababab, 0xaaaaaaaa},   {CFlag, 0xabababab, 0xfffffffe},
     {NZCVFlag, 0xabababab, 0x00000000}, {VFlag, 0xabababab, 0x80000001},
     {ZCVFlag, 0xabababab, 0xfffffffd},  {NFlag, 0xabababab, 0xffffffff},
     {VFlag, 0xabababab, 0xcccccccc},    {NFlag, 0xabababab, 0xffff8003},
     {NZVFlag, 0xabababab, 0xfffffffe},  {CVFlag, 0xabababab, 0xffff8001},
     {NZVFlag, 0xabababab, 0x00007fff},  {VFlag, 0xabababab, 0x33333333},
     {ZCVFlag, 0xabababab, 0x00007ffd},  {NCFlag, 0xabababab, 0xfffffffe},
     {ZVFlag, 0xabababab, 0xffffff83},   {NFlag, 0xabababab, 0x00007fff},
     {NVFlag, 0xabababab, 0x80000000},   {NCVFlag, 0xabababab, 0xffffff80},
     {ZFlag, 0xabababab, 0x7fffffff},    {NFlag, 0xabababab, 0x00000020},
     {NFlag, 0xabababab, 0x7ffffffd},    {NFlag, 0xabababab, 0x0000007f},
     {NZCVFlag, 0xabababab, 0x00000020}, {NCVFlag, 0xabababab, 0xfffffffe},
     {ZVFlag, 0xabababab, 0xcccccccc},   {NCFlag, 0xabababab, 0xffffffe0},
     {CVFlag, 0xabababab, 0xffffff82},   {NCFlag, 0xabababab, 0x00000000},
     {VFlag, 0xabababab, 0xffff8000},    {VFlag, 0xabababab, 0xffff8003},
     {NCFlag, 0xabababab, 0xffffffff},   {NZCFlag, 0xabababab, 0x55555555},
     {ZVFlag, 0xabababab, 0xfffffffe},   {NZFlag, 0xabababab, 0x7ffffffe},
     {ZCVFlag, 0xabababab, 0x7ffffffe},  {ZVFlag, 0xabababab, 0xffffffe0},
     {NZCFlag, 0xabababab, 0x0000007e},  {CFlag, 0xabababab, 0x7fffffff},
     {NZVFlag, 0xabababab, 0x55555555},  {NZCVFlag, 0xabababab, 0xcccccccc},
     {ZCVFlag, 0xabababab, 0x80000000},  {NFlag, 0xabababab, 0x55555555},
     {ZCVFlag, 0xabababab, 0xffffffe0},  {NCFlag, 0xabababab, 0xffff8000},
     {NCFlag, 0xabababab, 0xffff8001},   {NVFlag, 0xabababab, 0x7ffffffe},
     {ZVFlag, 0xabababab, 0x00007ffe},   {NVFlag, 0xabababab, 0x00000020},
     {NFlag, 0xabababab, 0x7fffffff},    {NZFlag, 0xabababab, 0x33333333},
     {ZCFlag, 0xabababab, 0xffff8001},   {ZFlag, 0xabababab, 0xffffffe0},
     {VFlag, 0xabababab, 0xffffffe0},    {VFlag, 0xabababab, 0xffffff80},
     {NCVFlag, 0xabababab, 0x80000000},  {ZCFlag, 0xabababab, 0x55555555},
     {CFlag, 0xabababab, 0xffff8001},    {CFlag, 0xabababab, 0xffff8002},
     {ZVFlag, 0xabababab, 0xffff8000},   {CFlag, 0xabababab, 0x80000001},
     {NZCVFlag, 0xabababab, 0x0000007d}, {NZCVFlag, 0xabababab, 0xfffffffd},
     {CVFlag, 0xabababab, 0xffffff81},   {NVFlag, 0xabababab, 0xffff8002},
     {ZCFlag, 0xabababab, 0x7ffffffe},   {ZCVFlag, 0xabababab, 0xffffff82},
     {ZCVFlag, 0xabababab, 0x33333333},  {NCFlag, 0xabababab, 0x33333333},
     {NZCVFlag, 0xabababab, 0x00000002}, {NFlag, 0xabababab, 0x00000001},
     {NVFlag, 0xabababab, 0xffff8003},   {NZCVFlag, 0xabababab, 0x00000001},
     {CFlag, 0xabababab, 0xffff8003},    {NVFlag, 0xabababab, 0x0000007e},
     {CVFlag, 0xabababab, 0x80000000},   {ZFlag, 0xabababab, 0xffffffff},
     {NFlag, 0xabababab, 0xfffffffd},    {NVFlag, 0xabababab, 0x00007fff},
     {NZCVFlag, 0xabababab, 0x7ffffffd}, {NZVFlag, 0xabababab, 0x00000000},
     {NZCFlag, 0xabababab, 0xfffffffe},  {ZVFlag, 0xabababab, 0x7ffffffe},
     {CFlag, 0xabababab, 0xffffffff},    {ZCFlag, 0xabababab, 0xffffff82},
     {ZCFlag, 0xabababab, 0xffffff83},   {ZCFlag, 0xabababab, 0x00000002},
     {NZFlag, 0xabababab, 0x0000007e},   {NZCVFlag, 0xabababab, 0xffffffe0},
     {NZFlag, 0xabababab, 0x00000001},   {ZVFlag, 0xabababab, 0x55555555},
     {ZCVFlag, 0xabababab, 0x00007fff},  {NFlag, 0xabababab, 0x33333333},
     {ZFlag, 0xabababab, 0x00000000},    {NVFlag, 0xabababab, 0x7fffffff},
     {ZVFlag, 0xabababab, 0xfffffffd},   {ZFlag, 0xabababab, 0x00000020},
     {NCVFlag, 0xabababab, 0xfffffffd},  {NZCFlag, 0xabababab, 0x80000000},
     {NVFlag, 0xabababab, 0x80000001},   {ZFlag, 0xabababab, 0xffff8002},
     {NZCVFlag, 0xabababab, 0xffff8001}, {NFlag, 0xabababab, 0xfffffffe},
     {ZVFlag, 0xabababab, 0x0000007d},   {NCFlag, 0xabababab, 0x00007ffd},
     {NFlag, 0xabababab, 0xffff8001},    {NZCFlag, 0xabababab, 0xaaaaaaaa},
     {NZCFlag, 0xabababab, 0x00000020},  {ZCVFlag, 0xabababab, 0x7fffffff},
     {ZCVFlag, 0xabababab, 0x00000001},  {NVFlag, 0xabababab, 0xcccccccc},
     {NFlag, 0xabababab, 0x80000000},    {NFlag, 0xabababab, 0xffffffe0},
     {ZCFlag, 0xabababab, 0xffffffff},   {CVFlag, 0xabababab, 0x00000000},
     {ZCFlag, 0xabababab, 0xffff8000},   {ZCFlag, 0xabababab, 0x00007ffd},
     {NCFlag, 0xabababab, 0x0000007f},   {ZVFlag, 0xabababab, 0x00000020},
     {VFlag, 0xabababab, 0xffff8002},    {ZFlag, 0xabababab, 0xffffff81},
     {CVFlag, 0xabababab, 0xffff8003},   {NFlag, 0xabababab, 0xffffff80},
     {NVFlag, 0xabababab, 0xaaaaaaaa},   {CFlag, 0xabababab, 0x00000001},
     {ZCVFlag, 0xabababab, 0xffff8003},  {NCFlag, 0xabababab, 0xfffffffd},
     {CVFlag, 0xabababab, 0xffffff83},   {ZCFlag, 0xabababab, 0xfffffffd},
     {CVFlag, 0xabababab, 0x0000007d},   {ZCVFlag, 0xabababab, 0xffffff83},
     {NCFlag, 0xabababab, 0xffffff81},   {NFlag, 0xabababab, 0xffff8000},
     {NZFlag, 0xabababab, 0x80000001},   {NCFlag, 0xabababab, 0x55555555},
     {NCVFlag, 0xabababab, 0xaaaaaaaa},  {NZCFlag, 0xabababab, 0xffffffff},
     {ZFlag, 0xabababab, 0x33333333},    {NCFlag, 0xabababab, 0xffffff82},
     {NZFlag, 0xabababab, 0xffff8001},   {VFlag, 0xabababab, 0x7ffffffe},
     {NZCVFlag, 0xabababab, 0x7ffffffe}, {ZFlag, 0xabababab, 0x80000001},
     {NVFlag, 0xabababab, 0xffffff80},   {NFlag, 0xabababab, 0x0000007e},
     {NCVFlag, 0xabababab, 0x00007ffd},  {CFlag, 0xabababab, 0x0000007e},
     {NZVFlag, 0xabababab, 0xcccccccc},  {NZVFlag, 0xabababab, 0x0000007f},
     {ZFlag, 0xabababab, 0xffffff83},    {VFlag, 0xabababab, 0xaaaaaaaa},
     {ZVFlag, 0xabababab, 0x7ffffffd},   {ZVFlag, 0xabababab, 0xffffffff},
     {NZCVFlag, 0xabababab, 0x0000007f}, {NCVFlag, 0xabababab, 0x0000007e},
     {NZVFlag, 0xabababab, 0xffffff82},  {CFlag, 0xabababab, 0xffffff80},
     {NZCVFlag, 0xabababab, 0x80000000}, {NZVFlag, 0xabababab, 0x7ffffffe},
     {NZFlag, 0xabababab, 0xffffff80},   {NZFlag, 0xabababab, 0xffff8000},
     {NCFlag, 0xabababab, 0xaaaaaaaa},   {NFlag, 0xabababab, 0x00000000},
     {NZFlag, 0xabababab, 0xcccccccc},   {ZCFlag, 0xabababab, 0xffffffe0},
     {CVFlag, 0xabababab, 0xcccccccc},   {VFlag, 0xabababab, 0xfffffffd},
     {CVFlag, 0xabababab, 0x7ffffffd},   {ZCFlag, 0xabababab, 0x00000020},
     {CVFlag, 0xabababab, 0x00007ffe},   {ZCFlag, 0xabababab, 0xffffff80},
     {NVFlag, 0xabababab, 0xfffffffd},   {NZCFlag, 0xabababab, 0x00000001},
     {CVFlag, 0xabababab, 0x00000001},   {NVFlag, 0xabababab, 0xffffff83},
     {ZCVFlag, 0xabababab, 0x7ffffffd},  {NZVFlag, 0xabababab, 0xffff8000},
     {CVFlag, 0xabababab, 0xffffffe0},   {NCFlag, 0xabababab, 0x0000007d},
     {VFlag, 0xabababab, 0x00007ffd},    {NZCFlag, 0xabababab, 0xffffff81},
     {NCVFlag, 0xabababab, 0x00007fff},  {NVFlag, 0xabababab, 0x0000007f},
     {CFlag, 0xabababab, 0xcccccccc},    {ZFlag, 0xabababab, 0xcccccccc},
     {ZCFlag, 0xabababab, 0x80000000},   {VFlag, 0xabababab, 0x0000007d},
     {NFlag, 0xabababab, 0xcccccccc},    {NCFlag, 0xabababab, 0x7fffffff},
     {VFlag, 0xabababab, 0xffffff81},    {NCVFlag, 0xabababab, 0xffffffff},
     {NZFlag, 0xabababab, 0xffff8002},   {NZVFlag, 0xabababab, 0x80000000},
     {ZCVFlag, 0xabababab, 0x0000007f},  {ZFlag, 0xabababab, 0x0000007f},
     {VFlag, 0xabababab, 0xffff8001},    {NVFlag, 0xabababab, 0x00000000},
     {ZFlag, 0xabababab, 0x00007ffd},    {NCVFlag, 0xabababab, 0x80000001},
     {NCVFlag, 0xabababab, 0xffff8002},  {NCFlag, 0xabababab, 0x7ffffffe},
     {ZCFlag, 0xabababab, 0x7ffffffd},   {NZCFlag, 0xabababab, 0x80000001},
     {ZCFlag, 0xabababab, 0x0000007f},   {VFlag, 0xabababab, 0x80000000},
     {NCFlag, 0xabababab, 0x80000001},   {NVFlag, 0xabababab, 0xffffff81}};


// A loop will be generated for each element of this array.
const TestLoopData kTests[] = {{{ne, r2, r2, 45},
                                "ne r2 r2 45",
                                "RdIsRn_ne_r2_r2_45",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{cs, r6, r6, 59},
                                "cs r6 r6 59",
                                "RdIsRn_cs_r6_r6_59",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{mi, r6, r6, 211},
                                "mi r6 r6 211",
                                "RdIsRn_mi_r6_r6_211",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{lt, r5, r5, 36},
                                "lt r5 r5 36",
                                "RdIsRn_lt_r5_r5_36",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{cc, r7, r7, 7},
                                "cc r7 r7 7",
                                "RdIsRn_cc_r7_r7_7",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{cs, r7, r7, 215},
                                "cs r7 r7 215",
                                "RdIsRn_cs_r7_r7_215",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{pl, r6, r6, 54},
                                "pl r6 r6 54",
                                "RdIsRn_pl_r6_r6_54",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{lt, r6, r6, 221},
                                "lt r6 r6 221",
                                "RdIsRn_lt_r6_r6_221",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{ne, r2, r2, 40},
                                "ne r2 r2 40",
                                "RdIsRn_ne_r2_r2_40",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{hi, r5, r5, 158},
                                "hi r5 r5 158",
                                "RdIsRn_hi_r5_r5_158",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{cs, r0, r0, 18},
                                "cs r0 r0 18",
                                "RdIsRn_cs_r0_r0_18",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{ne, r3, r3, 205},
                                "ne r3 r3 205",
                                "RdIsRn_ne_r3_r3_205",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{vc, r0, r0, 97},
                                "vc r0 r0 97",
                                "RdIsRn_vc_r0_r0_97",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{hi, r2, r2, 197},
                                "hi r2 r2 197",
                                "RdIsRn_hi_r2_r2_197",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{cs, r0, r0, 248},
                                "cs r0 r0 248",
                                "RdIsRn_cs_r0_r0_248",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{ne, r4, r4, 108},
                                "ne r4 r4 108",
                                "RdIsRn_ne_r4_r4_108",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{vs, r3, r3, 73},
                                "vs r3 r3 73",
                                "RdIsRn_vs_r3_r3_73",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{ne, r7, r7, 230},
                                "ne r7 r7 230",
                                "RdIsRn_ne_r7_r7_230",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{eq, r0, r0, 100},
                                "eq r0 r0 100",
                                "RdIsRn_eq_r0_r0_100",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{mi, r5, r5, 211},
                                "mi r5 r5 211",
                                "RdIsRn_mi_r5_r5_211",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{ge, r2, r3, 225},
                                "ge r2 r3 225",
                                "RdIsNotRn_ge_r2_r3_225",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{vs, r3, r1, 64},
                                "vs r3 r1 64",
                                "RdIsNotRn_vs_r3_r1_64",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{vs, r0, r1, 93},
                                "vs r0 r1 93",
                                "RdIsNotRn_vs_r0_r1_93",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{ne, r5, r1, 247},
                                "ne r5 r1 247",
                                "RdIsNotRn_ne_r5_r1_247",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{vs, r1, r7, 9},
                                "vs r1 r7 9",
                                "RdIsNotRn_vs_r1_r7_9",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{le, r0, r2, 201},
                                "le r0 r2 201",
                                "RdIsNotRn_le_r0_r2_201",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{vs, r6, r0, 176},
                                "vs r6 r0 176",
                                "RdIsNotRn_vs_r6_r0_176",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{vc, r0, r1, 248},
                                "vc r0 r1 248",
                                "RdIsNotRn_vc_r0_r1_248",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r7, r6, 64},
                                "al r7 r6 64",
                                "RdIsNotRn_al_r7_r6_64",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r5, r7, 175},
                                "al r5 r7 175",
                                "RdIsNotRn_al_r5_r7_175",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r0, r4, 182},
                                "al r0 r4 182",
                                "RdIsNotRn_al_r0_r4_182",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{ls, r7, r5, 58},
                                "ls r7 r5 58",
                                "RdIsNotRn_ls_r7_r5_58",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{vc, r0, r7, 205},
                                "vc r0 r7 205",
                                "RdIsNotRn_vc_r0_r7_205",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{pl, r1, r3, 135},
                                "pl r1 r3 135",
                                "RdIsNotRn_pl_r1_r3_135",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{gt, r4, r3, 253},
                                "gt r4 r3 253",
                                "RdIsNotRn_gt_r4_r3_253",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r1, r7, 32},
                                "al r1 r7 32",
                                "RdIsNotRn_al_r1_r7_32",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{eq, r1, r7, 139},
                                "eq r1 r7 139",
                                "RdIsNotRn_eq_r1_r7_139",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{lt, r0, r5, 248},
                                "lt r0 r5 248",
                                "RdIsNotRn_lt_r0_r5_248",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{hi, r0, r6, 89},
                                "hi r0 r6 89",
                                "RdIsNotRn_hi_r0_r6_89",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{cc, r3, r5, 210},
                                "cc r3 r5 210",
                                "RdIsNotRn_cc_r3_r5_210",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{cs, r0, r0, 187},
                                "cs r0 r0 187",
                                "Immediate_cs_r0_r0_187",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{hi, r0, r0, 236},
                                "hi r0 r0 236",
                                "Immediate_hi_r0_r0_236",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{eq, r0, r0, 133},
                                "eq r0 r0 133",
                                "Immediate_eq_r0_r0_133",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{mi, r0, r0, 193},
                                "mi r0 r0 193",
                                "Immediate_mi_r0_r0_193",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{cc, r0, r0, 73},
                                "cc r0 r0 73",
                                "Immediate_cc_r0_r0_73",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{cs, r0, r0, 93},
                                "cs r0 r0 93",
                                "Immediate_cs_r0_r0_93",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{ne, r0, r0, 130},
                                "ne r0 r0 130",
                                "Immediate_ne_r0_r0_130",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{ge, r0, r0, 209},
                                "ge r0 r0 209",
                                "Immediate_ge_r0_r0_209",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{lt, r0, r0, 42},
                                "lt r0 r0 42",
                                "Immediate_lt_r0_r0_42",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{cs, r0, r0, 177},
                                "cs r0 r0 177",
                                "Immediate_cs_r0_r0_177",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{lt, r0, r0, 139},
                                "lt r0 r0 139",
                                "Immediate_lt_r0_r0_139",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{mi, r0, r0, 43},
                                "mi r0 r0 43",
                                "Immediate_mi_r0_r0_43",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{cs, r0, r0, 247},
                                "cs r0 r0 247",
                                "Immediate_cs_r0_r0_247",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{ls, r0, r0, 170},
                                "ls r0 r0 170",
                                "Immediate_ls_r0_r0_170",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{cs, r0, r0, 185},
                                "cs r0 r0 185",
                                "Immediate_cs_r0_r0_185",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{ne, r0, r0, 246},
                                "ne r0 r0 246",
                                "Immediate_ne_r0_r0_246",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{mi, r0, r0, 179},
                                "mi r0 r0 179",
                                "Immediate_mi_r0_r0_179",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{pl, r0, r0, 7},
                                "pl r0 r0 7",
                                "Immediate_pl_r0_r0_7",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{mi, r0, r0, 177},
                                "mi r0 r0 177",
                                "Immediate_mi_r0_r0_177",
                                ARRAY_SIZE(kImmediate),
                                kImmediate},
                               {{gt, r0, r0, 175},
                                "gt r0 r0 175",
                                "Immediate_gt_r0_r0_175",
                                ARRAY_SIZE(kImmediate),
                                kImmediate}};

// We record all inputs to the instructions as outputs. This way, we also check
// that what shouldn't change didn't change.
struct TestResult {
  size_t output_size;
  const Inputs* outputs;
};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/simulator-cond-rdlow-rnlow-operand-immediate-add-t32.h"
#include "aarch32/traces/simulator-cond-rdlow-rnlow-operand-immediate-adds-t32.h"
#include "aarch32/traces/simulator-cond-rdlow-rnlow-operand-immediate-rsb-t32.h"
#include "aarch32/traces/simulator-cond-rdlow-rnlow-operand-immediate-rsbs-t32.h"
#include "aarch32/traces/simulator-cond-rdlow-rnlow-operand-immediate-sub-t32.h"
#include "aarch32/traces/simulator-cond-rdlow-rnlow-operand-immediate-subs-t32.h"


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
    int32_t immediate = kTests[i].operands.immediate;
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
#define TEST(mnemonic)                                                      \
  void Test_##mnemonic() {                                                  \
    TestHelper(&MacroAssembler::mnemonic, #mnemonic, kReference##mnemonic); \
  }                                                                         \
  Test test_##mnemonic(                                                     \
      "AARCH32_SIMULATOR_COND_RDLOW_RNLOW_OPERAND_IMMEDIATE_" #mnemonic     \
      "_T32",                                                               \
      &Test_##mnemonic);
#else
#define TEST(mnemonic)                                                  \
  void Test_##mnemonic() {                                              \
    VIXL_WARNING("This test can only run on a 32-bit host.\n");         \
    USE(TestHelper);                                                    \
  }                                                                     \
  Test test_##mnemonic(                                                 \
      "AARCH32_SIMULATOR_COND_RDLOW_RNLOW_OPERAND_IMMEDIATE_" #mnemonic \
      "_T32",                                                           \
      &Test_##mnemonic);
#endif

FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
