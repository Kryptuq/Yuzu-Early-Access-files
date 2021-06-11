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
  M(Crc32b)                    \
  M(Crc32cb)                   \
  M(Crc32ch)                   \
  M(Crc32cw)                   \
  M(Crc32h)                    \
  M(Crc32w)


// The following definitions are defined again in each generated test, therefore
// we need to place them in an anomymous namespace. It expresses that they are
// local to this file only, and the compiler is not allowed to share these types
// across test files during template instantiation. Specifically, `Operands` and
// `Inputs` have various layouts across generated tests so they absolutely
// cannot be shared.

#ifdef VIXL_INCLUDE_TARGET_A32
namespace {

// Values to be passed to the assembler to produce the instruction under test.
struct Operands {
  Register rd;
  Register rn;
  Register rm;
};

// Input data to feed to the instruction.
struct Inputs {
  uint32_t rd;
  uint32_t rn;
  uint32_t rm;
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

static const Inputs kRnIsRm[] = {
    {0xffffffe0, 0x00007fff, 0x00007fff}, {0xaaaaaaaa, 0x00007fff, 0x00007fff},
    {0xffffff82, 0x7ffffffe, 0x7ffffffe}, {0x7ffffffd, 0x33333333, 0x33333333},
    {0x00007ffd, 0x00000002, 0x00000002}, {0xffffff80, 0xfffffffd, 0xfffffffd},
    {0xffffff82, 0x33333333, 0x33333333}, {0xcccccccc, 0xffff8002, 0xffff8002},
    {0x55555555, 0xffffffe0, 0xffffffe0}, {0x00007fff, 0xffffffff, 0xffffffff},
    {0x55555555, 0x33333333, 0x33333333}, {0x7ffffffd, 0xffff8002, 0xffff8002},
    {0x00000020, 0xffffff83, 0xffffff83}, {0x0000007e, 0x00007fff, 0x00007fff},
    {0xfffffffe, 0xffffff82, 0xffffff82}, {0xffffff82, 0x00007fff, 0x00007fff},
    {0xffffff82, 0xfffffffd, 0xfffffffd}, {0x80000000, 0x7fffffff, 0x7fffffff},
    {0x55555555, 0x00000002, 0x00000002}, {0xfffffffe, 0xffffff80, 0xffffff80},
    {0xfffffffe, 0x00000002, 0x00000002}, {0xfffffffe, 0x00000020, 0x00000020},
    {0xfffffffe, 0x00007ffd, 0x00007ffd}, {0x0000007f, 0x33333333, 0x33333333},
    {0xfffffffd, 0xfffffffe, 0xfffffffe}, {0x80000001, 0x7ffffffd, 0x7ffffffd},
    {0x7ffffffe, 0xffffff83, 0xffffff83}, {0xffffffe0, 0x00000002, 0x00000002},
    {0x00007ffe, 0xffffff83, 0xffffff83}, {0xffffff82, 0xffffff80, 0xffffff80},
    {0x0000007e, 0x33333333, 0x33333333}, {0xffff8001, 0xfffffffe, 0xfffffffe},
    {0xfffffffe, 0xffffff81, 0xffffff81}, {0x00000001, 0xffffff83, 0xffffff83},
    {0xfffffffe, 0xaaaaaaaa, 0xaaaaaaaa}, {0xcccccccc, 0xffffff80, 0xffffff80},
    {0xffffff83, 0x0000007f, 0x0000007f}, {0x00007ffd, 0x00000020, 0x00000020},
    {0x0000007d, 0xfffffffd, 0xfffffffd}, {0x7ffffffe, 0xaaaaaaaa, 0xaaaaaaaa},
    {0xffffff83, 0x00000001, 0x00000001}, {0x0000007d, 0xffff8000, 0xffff8000},
    {0xffffffe0, 0x00000020, 0x00000020}, {0xffff8003, 0xffffff80, 0xffffff80},
    {0x00000000, 0xaaaaaaaa, 0xaaaaaaaa}, {0x00007fff, 0x00000020, 0x00000020},
    {0x80000000, 0x0000007d, 0x0000007d}, {0xffffff81, 0x0000007e, 0x0000007e},
    {0xcccccccc, 0x00000020, 0x00000020}, {0xffffff81, 0x33333333, 0x33333333},
    {0xffff8002, 0xffffffe0, 0xffffffe0}, {0xffffffe0, 0x00007ffe, 0x00007ffe},
    {0xffff8002, 0xaaaaaaaa, 0xaaaaaaaa}, {0xffff8001, 0x00007ffe, 0x00007ffe},
    {0x80000000, 0xcccccccc, 0xcccccccc}, {0x7ffffffd, 0x00000002, 0x00000002},
    {0xcccccccc, 0x0000007f, 0x0000007f}, {0x7ffffffe, 0xffffff82, 0xffffff82},
    {0x00000001, 0x0000007e, 0x0000007e}, {0x00007ffd, 0x00007ffe, 0x00007ffe},
    {0x7ffffffd, 0xfffffffe, 0xfffffffe}, {0x00000001, 0x80000001, 0x80000001},
    {0x00007ffe, 0x7fffffff, 0x7fffffff}, {0xffff8001, 0xffff8002, 0xffff8002},
    {0x00000001, 0xffff8003, 0xffff8003}, {0x00000002, 0xfffffffe, 0xfffffffe},
    {0x00000002, 0x00007ffd, 0x00007ffd}, {0x7ffffffe, 0xfffffffd, 0xfffffffd},
    {0xffff8001, 0x7ffffffd, 0x7ffffffd}, {0x7ffffffd, 0x55555555, 0x55555555},
    {0x80000000, 0xffffff80, 0xffffff80}, {0xffff8002, 0xffff8003, 0xffff8003},
    {0xffffffff, 0x00000020, 0x00000020}, {0xfffffffe, 0x00000001, 0x00000001},
    {0x00007fff, 0x00007fff, 0x00007fff}, {0x00000000, 0xffffff83, 0xffffff83},
    {0x33333333, 0xaaaaaaaa, 0xaaaaaaaa}, {0x0000007d, 0x7ffffffe, 0x7ffffffe},
    {0xffff8003, 0xffff8001, 0xffff8001}, {0xfffffffd, 0xcccccccc, 0xcccccccc},
    {0x80000000, 0x80000001, 0x80000001}, {0xffff8002, 0x00000002, 0x00000002},
    {0xfffffffd, 0xffffffe0, 0xffffffe0}, {0xffffffe0, 0xffffff82, 0xffffff82},
    {0x33333333, 0x00007ffe, 0x00007ffe}, {0xffff8002, 0xffffff80, 0xffffff80},
    {0xffffff81, 0x80000000, 0x80000000}, {0x00000000, 0x80000000, 0x80000000},
    {0x00000000, 0xcccccccc, 0xcccccccc}, {0x00000020, 0xffffffe0, 0xffffffe0},
    {0x00007ffe, 0xffffffe0, 0xffffffe0}, {0xffffff81, 0xfffffffe, 0xfffffffe},
    {0x00000002, 0x00007ffe, 0x00007ffe}, {0xffff8000, 0x7fffffff, 0x7fffffff},
    {0x00000002, 0x55555555, 0x55555555}, {0xffff8000, 0x55555555, 0x55555555},
    {0xffff8001, 0x00000020, 0x00000020}, {0xffffffff, 0x55555555, 0x55555555},
    {0x33333333, 0x00000020, 0x00000020}, {0xfffffffd, 0x7fffffff, 0x7fffffff},
    {0x7ffffffd, 0x7fffffff, 0x7fffffff}, {0xaaaaaaaa, 0xffffff81, 0xffffff81},
    {0x00007ffd, 0x80000001, 0x80000001}, {0x7ffffffe, 0xffffffff, 0xffffffff},
    {0x00007ffe, 0x7ffffffe, 0x7ffffffe}, {0xffff8001, 0x55555555, 0x55555555},
    {0xffffff80, 0xfffffffe, 0xfffffffe}, {0x0000007e, 0x7ffffffd, 0x7ffffffd},
    {0x00000000, 0x00007ffd, 0x00007ffd}, {0x00000020, 0x80000001, 0x80000001},
    {0xffffff83, 0xffffffe0, 0xffffffe0}, {0x55555555, 0xffff8000, 0xffff8000},
    {0x33333333, 0xffff8002, 0xffff8002}, {0x7ffffffd, 0x0000007f, 0x0000007f},
    {0x00000002, 0xffffff82, 0xffffff82}, {0xfffffffe, 0xcccccccc, 0xcccccccc},
    {0x33333333, 0xffffff82, 0xffffff82}, {0x7ffffffd, 0x00007ffd, 0x00007ffd},
    {0x00000002, 0xffffff80, 0xffffff80}, {0xffff8001, 0x00000000, 0x00000000},
    {0x7ffffffe, 0x0000007d, 0x0000007d}, {0x00000001, 0x00000020, 0x00000020},
    {0xaaaaaaaa, 0xffffff82, 0xffffff82}, {0x00000001, 0xfffffffd, 0xfffffffd},
    {0x80000001, 0xfffffffe, 0xfffffffe}, {0xffffff80, 0x80000000, 0x80000000},
    {0x80000000, 0xffffffff, 0xffffffff}, {0x00000001, 0x00000001, 0x00000001},
    {0x80000001, 0xffff8003, 0xffff8003}, {0xffffffff, 0xffffff83, 0xffffff83},
    {0x00007ffe, 0x00007ffe, 0x00007ffe}, {0x7ffffffd, 0xcccccccc, 0xcccccccc},
    {0xffff8003, 0xcccccccc, 0xcccccccc}, {0xcccccccc, 0x00007fff, 0x00007fff},
    {0x00000002, 0x7ffffffd, 0x7ffffffd}, {0x00000002, 0xffff8002, 0xffff8002},
    {0xffffff80, 0x33333333, 0x33333333}, {0x55555555, 0xffffff82, 0xffffff82},
    {0x7fffffff, 0x00007fff, 0x00007fff}, {0x33333333, 0x00000001, 0x00000001},
    {0x33333333, 0xffffffe0, 0xffffffe0}, {0xffffff83, 0x0000007e, 0x0000007e},
    {0x00000002, 0xffff8001, 0xffff8001}, {0xcccccccc, 0xffffffe0, 0xffffffe0},
    {0xffff8000, 0x7ffffffe, 0x7ffffffe}, {0x00007ffe, 0xffffff81, 0xffffff81},
    {0x7ffffffd, 0xffffffe0, 0xffffffe0}, {0x00007fff, 0xffffff81, 0xffffff81},
    {0xffffffff, 0x00007ffd, 0x00007ffd}, {0x00007fff, 0x80000001, 0x80000001},
    {0xffffffff, 0xcccccccc, 0xcccccccc}, {0x00007ffd, 0x0000007d, 0x0000007d},
    {0x0000007e, 0x00000000, 0x00000000}, {0x0000007f, 0x7ffffffe, 0x7ffffffe},
    {0x00000020, 0xffffff82, 0xffffff82}, {0xcccccccc, 0x80000001, 0x80000001},
    {0xffff8002, 0x00007ffd, 0x00007ffd}, {0xffff8000, 0x00000001, 0x00000001},
    {0xffffff83, 0xffffff83, 0xffffff83}, {0x00000002, 0xffffffe0, 0xffffffe0},
    {0xfffffffe, 0xffff8000, 0xffff8000}, {0x7ffffffe, 0xffffff80, 0xffffff80},
    {0x0000007f, 0xffffff81, 0xffffff81}, {0xffffffff, 0x0000007f, 0x0000007f},
    {0x7ffffffe, 0x0000007e, 0x0000007e}, {0x80000000, 0xfffffffd, 0xfffffffd},
    {0x33333333, 0x0000007d, 0x0000007d}, {0x80000001, 0xcccccccc, 0xcccccccc},
    {0xaaaaaaaa, 0x7ffffffd, 0x7ffffffd}, {0x7fffffff, 0x0000007f, 0x0000007f},
    {0x0000007e, 0xffff8001, 0xffff8001}, {0x0000007d, 0xaaaaaaaa, 0xaaaaaaaa},
    {0xffffff82, 0x55555555, 0x55555555}, {0x55555555, 0xffffff81, 0xffffff81},
    {0xcccccccc, 0xffffff82, 0xffffff82}, {0x0000007d, 0x0000007d, 0x0000007d},
    {0xfffffffd, 0xfffffffd, 0xfffffffd}, {0xffffff82, 0xffff8002, 0xffff8002},
    {0xffffffe0, 0x00000001, 0x00000001}, {0x0000007d, 0x00007ffe, 0x00007ffe},
    {0xfffffffe, 0xffff8003, 0xffff8003}, {0x00000001, 0xffff8001, 0xffff8001},
    {0xffff8003, 0x00007ffe, 0x00007ffe}, {0x7ffffffd, 0x00000020, 0x00000020},
    {0xfffffffd, 0x00007ffe, 0x00007ffe}, {0x7fffffff, 0xfffffffd, 0xfffffffd},
    {0x7fffffff, 0xffff8001, 0xffff8001}, {0xffff8003, 0xffffffff, 0xffffffff},
    {0x0000007d, 0x33333333, 0x33333333}, {0xffffffff, 0x00000000, 0x00000000},
    {0xffffffe0, 0xffffff81, 0xffffff81}, {0xffffffe0, 0xffffffff, 0xffffffff},
    {0x00007ffd, 0x00007ffd, 0x00007ffd}, {0x00000002, 0x33333333, 0x33333333},
    {0x0000007f, 0x00000002, 0x00000002}, {0xffff8000, 0x0000007e, 0x0000007e},
    {0x80000000, 0x00007fff, 0x00007fff}, {0xffff8000, 0xfffffffd, 0xfffffffd},
    {0xffff8002, 0xcccccccc, 0xcccccccc}, {0xffff8003, 0xffffff82, 0xffffff82}};

static const Inputs kRnIsNotRm[] = {
    {0x00007ffe, 0x00007ffe, 0x80000000}, {0x80000001, 0xcccccccc, 0x00000001},
    {0x0000007d, 0x00000020, 0xcccccccc}, {0x7ffffffd, 0x55555555, 0x0000007f},
    {0x00000020, 0xffff8001, 0x80000001}, {0xffffffe0, 0xffffff81, 0xffffff83},
    {0x00000002, 0xffffff80, 0xaaaaaaaa}, {0x00000002, 0x00000001, 0x0000007d},
    {0xffffff83, 0x00007ffd, 0x00000002}, {0xffff8000, 0x00000020, 0x55555555},
    {0x00007ffd, 0x0000007f, 0x7ffffffd}, {0xfffffffd, 0x80000000, 0x00000000},
    {0x0000007f, 0x33333333, 0x7fffffff}, {0x00000000, 0x33333333, 0x55555555},
    {0xffffff81, 0x00007fff, 0xaaaaaaaa}, {0x33333333, 0x00000000, 0x7ffffffe},
    {0xfffffffe, 0x00000001, 0xffffffff}, {0xaaaaaaaa, 0xffffffe0, 0xffffff80},
    {0x33333333, 0xffffffe0, 0x7fffffff}, {0x55555555, 0x7ffffffd, 0xffffff81},
    {0xaaaaaaaa, 0xffffffe0, 0x0000007f}, {0x00007fff, 0xfffffffd, 0xcccccccc},
    {0x7fffffff, 0xffff8002, 0x00000001}, {0x0000007d, 0x00000002, 0x7fffffff},
    {0x00000002, 0x55555555, 0x00007ffe}, {0x00007ffd, 0x0000007d, 0xfffffffe},
    {0x7ffffffd, 0x55555555, 0xffffff83}, {0x7ffffffe, 0x80000001, 0x00000001},
    {0xfffffffd, 0x80000001, 0x80000000}, {0xffff8002, 0x00000000, 0x80000001},
    {0x0000007d, 0x00000020, 0x0000007e}, {0x00000020, 0x0000007f, 0x80000001},
    {0x7ffffffe, 0x80000001, 0xffff8002}, {0xffffff81, 0xffffff80, 0xffffff81},
    {0x7ffffffe, 0xffffff80, 0x0000007d}, {0x00007ffd, 0x00000000, 0x55555555},
    {0xcccccccc, 0xcccccccc, 0x7ffffffe}, {0xffff8002, 0x00000001, 0xffff8000},
    {0x00007ffe, 0x0000007d, 0xffffffe0}, {0x00007ffd, 0x7ffffffe, 0xffffffe0},
    {0x80000001, 0x7ffffffe, 0xffff8001}, {0x55555555, 0xffffff80, 0xffff8003},
    {0xfffffffd, 0xffff8002, 0x55555555}, {0x33333333, 0xffff8000, 0xffff8003},
    {0x7ffffffd, 0xffff8002, 0x0000007e}, {0x7fffffff, 0xffffff80, 0xfffffffd},
    {0xffff8002, 0x00000001, 0xaaaaaaaa}, {0xffffff83, 0xffff8001, 0x33333333},
    {0x00000000, 0xfffffffe, 0x0000007e}, {0x80000000, 0xffffffe0, 0x0000007d},
    {0xffff8000, 0x33333333, 0x00000002}, {0xfffffffe, 0x00000002, 0xffffffe0},
    {0x00007ffd, 0x0000007e, 0x7ffffffe}, {0xffffff81, 0xffffffff, 0x00000000},
    {0x0000007e, 0x7ffffffe, 0x33333333}, {0x00000002, 0x80000001, 0xffffff80},
    {0x7ffffffd, 0xffffff82, 0x7ffffffe}, {0xffffff81, 0x0000007e, 0xffffff81},
    {0xffff8002, 0x00007fff, 0x0000007f}, {0xcccccccc, 0xcccccccc, 0x0000007d},
    {0x7ffffffd, 0xffffff80, 0x00007ffe}, {0x55555555, 0x55555555, 0xfffffffe},
    {0x00000001, 0x00000020, 0x00000001}, {0x0000007f, 0x7ffffffd, 0x80000001},
    {0x55555555, 0xaaaaaaaa, 0xfffffffd}, {0x00000020, 0x00007fff, 0xffffff81},
    {0xffffff80, 0x33333333, 0xffffff81}, {0x00000000, 0xffff8001, 0xffffff83},
    {0x0000007e, 0x33333333, 0x7ffffffd}, {0xaaaaaaaa, 0x55555555, 0xffffffff},
    {0xfffffffd, 0xffff8000, 0x00000000}, {0xffff8000, 0xfffffffe, 0x7fffffff},
    {0xffff8001, 0xcccccccc, 0x33333333}, {0x00007ffd, 0xffff8001, 0x0000007f},
    {0xffffff83, 0x7ffffffd, 0x7fffffff}, {0xffffff81, 0xffffffe0, 0x00007fff},
    {0xffffffff, 0x00000002, 0xffffff82}, {0xffff8001, 0xffffff81, 0x55555555},
    {0x00007fff, 0xffffff83, 0x00007ffe}, {0x00007fff, 0xcccccccc, 0x00000020},
    {0x7fffffff, 0x7ffffffd, 0xffff8000}, {0x0000007f, 0x80000000, 0x7ffffffd},
    {0x0000007e, 0x0000007e, 0x0000007d}, {0xfffffffe, 0x00000020, 0xffffff80},
    {0x00007ffd, 0x00007ffe, 0xffffff82}, {0x00000020, 0x80000000, 0x0000007e},
    {0x00000020, 0x00000002, 0xffffffff}, {0x00007fff, 0xffff8000, 0x00007fff},
    {0xcccccccc, 0x7ffffffd, 0x80000000}, {0xffffff80, 0x00000002, 0xffffff81},
    {0xfffffffe, 0xffffff83, 0x33333333}, {0xffff8000, 0x7ffffffe, 0x55555555},
    {0x00007fff, 0x0000007e, 0xffffff81}, {0xaaaaaaaa, 0xcccccccc, 0xffffffe0},
    {0x7ffffffd, 0xffffffff, 0xffff8003}, {0xffffff81, 0xffff8002, 0xffffff82},
    {0x00000002, 0xffffffe0, 0x80000001}, {0x7ffffffe, 0x33333333, 0xaaaaaaaa},
    {0x00007fff, 0x00000020, 0xfffffffe}, {0xcccccccc, 0xfffffffe, 0x00000000},
    {0x7ffffffe, 0x80000001, 0xfffffffd}, {0xffffffe0, 0x00000000, 0xaaaaaaaa},
    {0x80000000, 0xffffff83, 0xffff8000}, {0xffffff81, 0xfffffffe, 0xffffff83},
    {0x80000000, 0x0000007e, 0xffff8002}, {0x80000001, 0x00000020, 0x00007ffe},
    {0x00000000, 0x00000020, 0x0000007e}, {0x80000001, 0xffffffff, 0x7ffffffe},
    {0xffffff83, 0x33333333, 0x00000001}, {0x00000020, 0xffffff80, 0xffffff81},
    {0xcccccccc, 0xffff8000, 0x00007ffd}, {0xffff8003, 0x00000000, 0x0000007f},
    {0xffff8003, 0x00000001, 0x0000007d}, {0x0000007e, 0x00000020, 0x55555555},
    {0xffffffff, 0xfffffffe, 0xffffff83}, {0xffffff80, 0x80000000, 0xfffffffe},
    {0x0000007e, 0x0000007e, 0x00000000}, {0xaaaaaaaa, 0x00000002, 0xffffffe0},
    {0xffff8001, 0x00007ffe, 0x00000020}, {0x7ffffffe, 0xffff8000, 0x00007ffe},
    {0xffffff80, 0x33333333, 0x00007fff}, {0x33333333, 0xffffff81, 0x00000001},
    {0x00000000, 0x33333333, 0x00000020}, {0x00007fff, 0x00000020, 0x0000007e},
    {0xcccccccc, 0x55555555, 0xffffffe0}, {0xffffff80, 0x0000007e, 0x33333333},
    {0x0000007e, 0xffff8002, 0x00000000}, {0xffffff83, 0xffffffe0, 0xfffffffd},
    {0x00000001, 0x0000007f, 0xffff8001}, {0xffffffff, 0x00000020, 0xcccccccc},
    {0x7ffffffd, 0xfffffffd, 0xffff8000}, {0xffffffff, 0x80000000, 0x00007fff},
    {0x00000020, 0xffffffe0, 0xfffffffd}, {0x55555555, 0xffff8002, 0x0000007f},
    {0xffff8002, 0x55555555, 0x0000007f}, {0xffff8003, 0xffff8000, 0x0000007d},
    {0x00007fff, 0x00007ffe, 0x00000000}, {0xcccccccc, 0xffffff81, 0x00007fff},
    {0x7ffffffd, 0xcccccccc, 0xffff8000}, {0x7ffffffe, 0xffff8001, 0x80000000},
    {0x0000007f, 0xffffffe0, 0x80000001}, {0xffff8000, 0x00007ffd, 0xffff8003},
    {0xffff8001, 0xffffff80, 0x00000001}, {0x33333333, 0x00000000, 0x80000000},
    {0xfffffffe, 0x7ffffffd, 0x80000001}, {0x00000000, 0xffffffe0, 0x80000001},
    {0xffff8002, 0x7ffffffe, 0x33333333}, {0xffffffe0, 0x0000007e, 0x00000001},
    {0xffffff80, 0xffffffe0, 0x00007ffe}, {0xffff8000, 0x0000007f, 0x80000001},
    {0xffffff80, 0xffffff80, 0xffff8002}, {0x7fffffff, 0x0000007f, 0xfffffffd},
    {0x00000001, 0xffffffe0, 0xffffffff}, {0x55555555, 0x0000007f, 0xffff8002},
    {0xffff8001, 0xfffffffd, 0x00000020}, {0x00007ffd, 0xfffffffd, 0x0000007e},
    {0xfffffffd, 0xffff8003, 0x55555555}, {0xfffffffe, 0xffffff80, 0x0000007f},
    {0x0000007f, 0x0000007f, 0xfffffffe}, {0x33333333, 0x55555555, 0xffff8003},
    {0x0000007d, 0x00000020, 0x55555555}, {0xffff8003, 0x33333333, 0xffffff83},
    {0xffffffe0, 0x00007fff, 0xffffff80}, {0x00000020, 0x00000001, 0x0000007d},
    {0xffffff83, 0x00007fff, 0xaaaaaaaa}, {0xfffffffd, 0xffffff83, 0x55555555},
    {0xffff8002, 0x00007fff, 0xffffffe0}, {0x80000001, 0xffffff82, 0x00007ffd},
    {0xffff8000, 0x00000002, 0xffffffe0}, {0xfffffffd, 0xffffff80, 0x7ffffffe},
    {0xffffffff, 0xaaaaaaaa, 0xffff8000}, {0x7ffffffd, 0x00000002, 0xffff8002},
    {0xffff8001, 0x00007ffe, 0xffffffff}, {0xffff8001, 0x00000001, 0x00000000},
    {0x80000001, 0xffffff80, 0x7ffffffd}, {0xffffff80, 0x0000007d, 0xffffff81},
    {0xffffff81, 0xcccccccc, 0xffffff81}, {0xaaaaaaaa, 0x00000000, 0xffffff81},
    {0x00000002, 0x00000020, 0xffffff81}, {0x7ffffffd, 0x7ffffffe, 0xffffff83},
    {0x7fffffff, 0x55555555, 0x0000007f}, {0xfffffffd, 0xffffff82, 0xfffffffd},
    {0x00000020, 0x00007ffd, 0xffffff81}, {0x0000007e, 0xffffff83, 0x33333333},
    {0xffffff83, 0x00007fff, 0x00000002}, {0xffffffe0, 0xffffff80, 0xffff8001},
    {0x00000000, 0x00000000, 0x00000002}, {0xfffffffd, 0x7ffffffe, 0x00000001},
    {0xfffffffd, 0xfffffffd, 0xffffff80}, {0xffffff83, 0x00000000, 0x00007ffe},
    {0xffff8002, 0xcccccccc, 0x33333333}, {0xffffff80, 0xffffff81, 0x0000007d},
    {0x0000007e, 0xffff8000, 0x00007ffe}, {0x0000007e, 0x80000000, 0x80000001},
    {0xffffff83, 0x00007fff, 0xffffffe0}, {0xcccccccc, 0x00007fff, 0xffffff80},
    {0x0000007f, 0x0000007e, 0xfffffffd}, {0xcccccccc, 0xffffffff, 0x00000001},
    {0xffffffff, 0xffff8000, 0xffffff82}, {0xfffffffe, 0x00000002, 0xffffffff}};


// A loop will be generated for each element of this array.
const TestLoopData kTests[] = {
    {{r9, r8, r8}, "r9 r8 r8", "RnIsRm_r9_r8_r8", ARRAY_SIZE(kRnIsRm), kRnIsRm},
    {{r6, r11, r11},
     "r6 r11 r11",
     "RnIsRm_r6_r11_r11",
     ARRAY_SIZE(kRnIsRm),
     kRnIsRm},
    {{r3, r12, r12},
     "r3 r12 r12",
     "RnIsRm_r3_r12_r12",
     ARRAY_SIZE(kRnIsRm),
     kRnIsRm},
    {{r14, r3, r3},
     "r14 r3 r3",
     "RnIsRm_r14_r3_r3",
     ARRAY_SIZE(kRnIsRm),
     kRnIsRm},
    {{r10, r4, r4},
     "r10 r4 r4",
     "RnIsRm_r10_r4_r4",
     ARRAY_SIZE(kRnIsRm),
     kRnIsRm},
    {{r6, r8, r8}, "r6 r8 r8", "RnIsRm_r6_r8_r8", ARRAY_SIZE(kRnIsRm), kRnIsRm},
    {{r5, r7, r7}, "r5 r7 r7", "RnIsRm_r5_r7_r7", ARRAY_SIZE(kRnIsRm), kRnIsRm},
    {{r6, r3, r3}, "r6 r3 r3", "RnIsRm_r6_r3_r3", ARRAY_SIZE(kRnIsRm), kRnIsRm},
    {{r7, r0, r0}, "r7 r0 r0", "RnIsRm_r7_r0_r0", ARRAY_SIZE(kRnIsRm), kRnIsRm},
    {{r2, r1, r1}, "r2 r1 r1", "RnIsRm_r2_r1_r1", ARRAY_SIZE(kRnIsRm), kRnIsRm},
    {{r5, r3, r14},
     "r5 r3 r14",
     "RnIsNotRm_r5_r3_r14",
     ARRAY_SIZE(kRnIsNotRm),
     kRnIsNotRm},
    {{r10, r8, r3},
     "r10 r8 r3",
     "RnIsNotRm_r10_r8_r3",
     ARRAY_SIZE(kRnIsNotRm),
     kRnIsNotRm},
    {{r2, r8, r11},
     "r2 r8 r11",
     "RnIsNotRm_r2_r8_r11",
     ARRAY_SIZE(kRnIsNotRm),
     kRnIsNotRm},
    {{r7, r12, r7},
     "r7 r12 r7",
     "RnIsNotRm_r7_r12_r7",
     ARRAY_SIZE(kRnIsNotRm),
     kRnIsNotRm},
    {{r2, r4, r10},
     "r2 r4 r10",
     "RnIsNotRm_r2_r4_r10",
     ARRAY_SIZE(kRnIsNotRm),
     kRnIsNotRm},
    {{r1, r10, r8},
     "r1 r10 r8",
     "RnIsNotRm_r1_r10_r8",
     ARRAY_SIZE(kRnIsNotRm),
     kRnIsNotRm},
    {{r1, r4, r2},
     "r1 r4 r2",
     "RnIsNotRm_r1_r4_r2",
     ARRAY_SIZE(kRnIsNotRm),
     kRnIsNotRm},
    {{r12, r2, r12},
     "r12 r2 r12",
     "RnIsNotRm_r12_r2_r12",
     ARRAY_SIZE(kRnIsNotRm),
     kRnIsNotRm},
    {{r4, r8, r5},
     "r4 r8 r5",
     "RnIsNotRm_r4_r8_r5",
     ARRAY_SIZE(kRnIsNotRm),
     kRnIsNotRm},
    {{r4, r0, r9},
     "r4 r0 r9",
     "RnIsNotRm_r4_r0_r9",
     ARRAY_SIZE(kRnIsNotRm),
     kRnIsNotRm}};

// We record all inputs to the instructions as outputs. This way, we also check
// that what shouldn't change didn't change.
struct TestResult {
  size_t output_size;
  const Inputs* outputs;
};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/simulator-rd-rn-rm-crc32b-a32.h"
#include "aarch32/traces/simulator-rd-rn-rm-crc32cb-a32.h"
#include "aarch32/traces/simulator-rd-rn-rm-crc32ch-a32.h"
#include "aarch32/traces/simulator-rd-rn-rm-crc32cw-a32.h"
#include "aarch32/traces/simulator-rd-rn-rm-crc32h-a32.h"
#include "aarch32/traces/simulator-rd-rn-rm-crc32w-a32.h"


// The maximum number of errors to report in detail for each test.
const unsigned kErrorReportLimit = 8;

typedef void (MacroAssembler::*Fn)(Register rd, Register rn, Register rm);

void TestHelper(Fn instruction,
                const char* mnemonic,
                const TestResult reference[]) {
  SETUP();
  masm.UseA32();
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
    Register rd = kTests[i].operands.rd;
    Register rn = kTests[i].operands.rn;
    Register rm = kTests[i].operands.rm;
    scratch_registers.Exclude(rd);
    scratch_registers.Exclude(rn);
    scratch_registers.Exclude(rm);

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

    __ Ldr(rd, MemOperand(input_ptr, offsetof(Inputs, rd)));
    __ Ldr(rn, MemOperand(input_ptr, offsetof(Inputs, rn)));
    __ Ldr(rm, MemOperand(input_ptr, offsetof(Inputs, rm)));

    (masm.*instruction)(rd, rn, rm);

    __ Str(rd, MemOperand(result_ptr, offsetof(Inputs, rd)));
    __ Str(rn, MemOperand(result_ptr, offsetof(Inputs, rn)));
    __ Str(rm, MemOperand(result_ptr, offsetof(Inputs, rm)));

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
        printf("0x%08" PRIx32, results[i]->outputs[j].rd);
        printf(", ");
        printf("0x%08" PRIx32, results[i]->outputs[j].rn);
        printf(", ");
        printf("0x%08" PRIx32, results[i]->outputs[j].rm);
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
        uint32_t rd = results[i]->outputs[j].rd;
        uint32_t rn = results[i]->outputs[j].rn;
        uint32_t rm = results[i]->outputs[j].rm;
        uint32_t rd_input = kTests[i].inputs[j].rd;
        uint32_t rn_input = kTests[i].inputs[j].rn;
        uint32_t rm_input = kTests[i].inputs[j].rm;
        uint32_t rd_ref = reference[i].outputs[j].rd;
        uint32_t rn_ref = reference[i].outputs[j].rn;
        uint32_t rm_ref = reference[i].outputs[j].rm;

        if (((rd != rd_ref) || (rn != rn_ref) || (rm != rm_ref)) &&
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
          printf("0x%08" PRIx32, rd_input);
          printf(", ");
          printf("0x%08" PRIx32, rn_input);
          printf(", ");
          printf("0x%08" PRIx32, rm_input);
          printf("\n");
          printf("  Expected: ");
          printf("0x%08" PRIx32, rd_ref);
          printf(", ");
          printf("0x%08" PRIx32, rn_ref);
          printf(", ");
          printf("0x%08" PRIx32, rm_ref);
          printf("\n");
          printf("  Found:    ");
          printf("0x%08" PRIx32, rd);
          printf(", ");
          printf("0x%08" PRIx32, rn);
          printf(", ");
          printf("0x%08" PRIx32, rm);
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
  Test test_##mnemonic("AARCH32_SIMULATOR_RD_RN_RM_" #mnemonic "_A32",      \
                       &Test_##mnemonic);
#else
#define TEST(mnemonic)                                                 \
  void Test_##mnemonic() {                                             \
    VIXL_WARNING("This test can only run on a 32-bit host.\n");        \
    USE(TestHelper);                                                   \
  }                                                                    \
  Test test_##mnemonic("AARCH32_SIMULATOR_RD_RN_RM_" #mnemonic "_A32", \
                       &Test_##mnemonic);
#endif

FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
