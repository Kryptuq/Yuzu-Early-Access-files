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

#ifndef VIXL_AARCH32_TEST_UTILS_AARCH32_H_
#define VIXL_AARCH32_TEST_UTILS_AARCH32_H_

#include "../test-pool-manager.h"
#include "../test-runner.h"
#include "aarch32/constants-aarch32.h"
#include "aarch32/instructions-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"

namespace vixl {

namespace aarch32 {

class TestMacroAssembler {
 public:
  explicit TestMacroAssembler(MacroAssembler* masm)
      : test(&masm->pool_manager_) {}
  int32_t GetPoolCheckpoint() const { return test.GetPoolCheckpoint(); }
  int GetPoolSize() const { return test.GetPoolSize(); }
  bool PoolIsEmpty() const { return test.PoolIsEmpty(); }

 private:
  TestPoolManager test;
};

// Only check the simulator tests when we can actually run them.
// TODO: Improve this.
#if defined(__arm__)
static const bool kCheckSimulatorTestResults = true;
#else
static const bool kCheckSimulatorTestResults = false;
#endif

// Helper constants used to check for condition code combinations.  These are
// not part of instruction definitions as no instruction uses them directly.
const uint32_t NoFlag = 0x0;
const uint32_t NFlag = 0x80000000;
const uint32_t ZFlag = 0x40000000;
const uint32_t CFlag = 0x20000000;
const uint32_t VFlag = 0x10000000;
const uint32_t NZFlag = NFlag | ZFlag;
const uint32_t NCFlag = NFlag | CFlag;
const uint32_t NVFlag = NFlag | VFlag;
const uint32_t ZCFlag = ZFlag | CFlag;
const uint32_t ZVFlag = ZFlag | VFlag;
const uint32_t CVFlag = CFlag | VFlag;
const uint32_t NZCFlag = NFlag | ZFlag | CFlag;
const uint32_t NZVFlag = NFlag | ZFlag | VFlag;
const uint32_t NCVFlag = NFlag | CFlag | VFlag;
const uint32_t ZCVFlag = ZFlag | CFlag | VFlag;
const uint32_t NZCVFlag = NFlag | ZFlag | CFlag | VFlag;
const uint32_t QFlag = 0x08000000;

const uint32_t GE0Flag = 0x00010000;
const uint32_t GE1Flag = 0x00020000;
const uint32_t GE2Flag = 0x00040000;
const uint32_t GE3Flag = 0x00080000;
const uint32_t GE01Flag = GE0Flag | GE1Flag;
const uint32_t GE02Flag = GE0Flag | GE2Flag;
const uint32_t GE03Flag = GE0Flag | GE3Flag;
const uint32_t GE12Flag = GE1Flag | GE2Flag;
const uint32_t GE13Flag = GE1Flag | GE3Flag;
const uint32_t GE23Flag = GE2Flag | GE3Flag;
const uint32_t GE012Flag = GE0Flag | GE1Flag | GE2Flag;
const uint32_t GE013Flag = GE0Flag | GE1Flag | GE3Flag;
const uint32_t GE023Flag = GE0Flag | GE2Flag | GE3Flag;
const uint32_t GE123Flag = GE1Flag | GE2Flag | GE3Flag;
const uint32_t GE0123Flag = GE0Flag | GE1Flag | GE2Flag | GE3Flag;
const uint32_t GEFlags = GE0123Flag;

struct vec128_t {
  uint64_t l;
  uint64_t h;
};

class RegisterDump {
 public:
  RegisterDump() : completed_(false) {
    VIXL_ASSERT(sizeof(dump_.r_[0]) == kRegSizeInBytes);
  }

  // The Dump method generates code to store a snapshot of the register values.
  // It needs to be able to use the stack temporarily.
  //
  // The dumping code is generated though the given MacroAssembler. No registers
  // are corrupted in the process apart for the program counter, but the stack
  // is used briefly.  Note the program counter cannot be retrieved from the
  // register dump anyway.
  void Dump(MacroAssembler* masm);

  // Register accessors.
  int32_t reg(unsigned code) const {
    VIXL_ASSERT(IsComplete());
    // The collected program counter should not be accessed.
    VIXL_ASSERT(code != kPcCode);
    return dump_.r_[code];
  }

  // QRegister accessors
  vec128_t GetQRegisterBits(unsigned code) const {
    VIXL_ASSERT(IsComplete());
    VIXL_ASSERT(code < kNumberOfQRegisters);
    vec128_t content = {dump_.d_[code * 2], dump_.d_[(code * 2) + 1]};
    return content;
  }

  // DRegister accessors
  uint64_t GetDRegisterBits(unsigned code) const {
    VIXL_ASSERT(IsComplete());
    VIXL_ASSERT(code < kMaxNumberOfDRegisters);
    return dump_.d_[code];
  }

  // SRegister accessors
  uint32_t GetSRegisterBits(unsigned code) const {
    VIXL_ASSERT(IsComplete());
    VIXL_ASSERT(code < kNumberOfSRegisters);
    if ((code % 2) == 0) {
      return GetDRegisterBits(code / 2) & 0xffffffff;
    } else {
      return GetDRegisterBits(code / 2) >> 32;
    }
    VIXL_UNREACHABLE();
    return 0;
  }

  // Stack pointer accessors.
  int32_t spreg() const { return reg(kSPRegNum); }

  // Flags accessors.
  uint32_t flags_nzcv() const {
    VIXL_ASSERT(IsComplete());
    return dump_.flags_ & NZCVFlag;
  }

  bool IsComplete() const { return completed_; }

 private:
  // Indicate whether the dump operation has been completed.
  bool completed_;

  // Store all the dumped elements in a simple struct so the implementation can
  // use offsetof to quickly find the correct field.
  struct dump_t {
    // Core registers, except for PC.
    uint32_t r_[kNumberOfRegisters - 1];
    uint64_t d_[kMaxNumberOfDRegisters];

    // NZCV flags, stored in bits 28 to 31.
    // bit[31] : Negative
    // bit[30] : Zero
    // bit[29] : Carry
    // bit[28] : oVerflow
    uint32_t flags_;
  } dump_;
};

bool Equal32(uint32_t expected, const RegisterDump* core, const Register& reg);
bool Equal32(uint32_t expected, const RegisterDump* core, uint32_t result);
bool Equal32(uint32_t expected,
             const RegisterDump* core,
             const SRegister& sreg);
bool Equal64(uint64_t expected,
             const RegisterDump* core,
             const DRegister& dreg);
bool Equal128(uint64_t expected_h,
              uint64_t expected_l,
              const RegisterDump* core,
              const QRegister& qreg);
bool EqualFP32(float expected, const RegisterDump* core, const SRegister& dreg);
bool EqualFP64(double expected,
               const RegisterDump* core,
               const DRegister& dreg);
bool EqualNzcv(uint32_t expected, uint32_t result);

}  // namespace aarch32
}  // namespace vixl

#endif  // VIXL_AARCH32_TEST_UTILS_AARCH32_H_
