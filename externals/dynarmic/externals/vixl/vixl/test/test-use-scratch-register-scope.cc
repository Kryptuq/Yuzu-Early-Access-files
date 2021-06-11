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

#include <stdlib.h>

#include "test-runner.h"

#ifdef VIXL_INCLUDE_TARGET_AARCH32
#include "aarch32/macro-assembler-aarch32.h"
#endif

#ifdef VIXL_INCLUDE_TARGET_AARCH64
#include "aarch64/macro-assembler-aarch64.h"
#endif

#define STRINGIFY(x) #x

#define TEST_AARCH32(Name)                                                \
  namespace aarch32 {                                                     \
  void Test_##Name##_AArch32_Impl();                                      \
  }                                                                       \
  void Test_##Name##_AArch32() { aarch32::Test_##Name##_AArch32_Impl(); } \
  Test test_##Name##_AArch32(STRINGIFY(AARCH32_SCRATCH_##Name),           \
                             &Test_##Name##_AArch32);                     \
  void aarch32::Test_##Name##_AArch32_Impl()

#define TEST_AARCH64(Name)                                                \
  namespace aarch64 {                                                     \
  void Test_##Name##_AArch64_Impl();                                      \
  }                                                                       \
  void Test_##Name##_AArch64() { aarch64::Test_##Name##_AArch64_Impl(); } \
  Test test_##Name##_AArch64(STRINGIFY(AARCH64_SCRATCH_##Name),           \
                             &Test_##Name##_AArch64);                     \
  void aarch64::Test_##Name##_AArch64_Impl()

#define SETUP() MacroAssembler masm

#define __ masm.

namespace vixl {

// UseScratchRegisterScopes must be able to nest perfectly. That is, they may
// nest, but nested scopes must not outlive less-nested scopes.
template <typename MacroAssembler, typename UseScratchRegisterScope>
class PerfectNestingTestHelper {
 public:
  explicit PerfectNestingTestHelper(MacroAssembler* masm) : masm_(masm) {
    uint16_t seed[3] = {4, 5, 6};
    seed48(seed);
  }
  void Run() {
    UseScratchRegisterScope* top_scope =
        masm_->GetCurrentScratchRegisterScope();
    int descendents = 0;
    while (descendents < kMinimumDescendentScopeCount) descendents += Run(0);
    VIXL_CHECK(masm_->GetCurrentScratchRegisterScope() == top_scope);
  }

 private:
  int Run(int depth) {
    // As the depth increases, the probability of recursion decreases.
    // At depth = kDepthLimit, we never recurse.
    int max_children = static_cast<int>(std::abs(mrand48()) % kDepthLimit);
    int children = std::max(0, max_children - depth);
    int descendents = children;
    while (children-- > 0) {
      UseScratchRegisterScope scope(masm_);
      VIXL_CHECK(masm_->GetCurrentScratchRegisterScope() == &scope);
      descendents += Run(depth + 1);
      VIXL_CHECK(masm_->GetCurrentScratchRegisterScope() == &scope);
    }
    return descendents;
  }

  MacroAssembler* masm_;
  static const int kDepthLimit = 12;
  static const int kMinimumDescendentScopeCount = 10000;
};

#ifdef VIXL_INCLUDE_TARGET_AARCH32
TEST_AARCH32(perfect_nesting) {
  SETUP();
  PerfectNestingTestHelper<MacroAssembler, UseScratchRegisterScope>(&masm)
      .Run();
}
#endif  // VIXL_INCLUDE_TARGET_AARCH32

#ifdef VIXL_INCLUDE_TARGET_AARCH64
TEST_AARCH64(perfect_nesting) {
  SETUP();
  PerfectNestingTestHelper<MacroAssembler, UseScratchRegisterScope>(&masm)
      .Run();
}
#endif  // VIXL_INCLUDE_TARGET_AARCH64


#ifdef VIXL_INCLUDE_TARGET_AARCH32
TEST_AARCH32(v_registers) {
  SETUP();
  {
    UseScratchRegisterScope temps(&masm);
    temps.Include(VRegisterList(q0, q1, q2, q3));

    // This test assumes that low-numbered registers are allocated first. The
    // implementation is allowed to use a different strategy; if it does, the
    // test will need to be updated.
    // TODO: Write more flexible (and thorough) tests.

    VIXL_CHECK(q0.Is(temps.AcquireQ()));
    VIXL_CHECK(!temps.IsAvailable(q0));
    VIXL_CHECK(!temps.IsAvailable(d0));
    VIXL_CHECK(!temps.IsAvailable(d1));
    VIXL_CHECK(!temps.IsAvailable(s0));
    VIXL_CHECK(!temps.IsAvailable(s1));
    VIXL_CHECK(!temps.IsAvailable(s2));
    VIXL_CHECK(!temps.IsAvailable(s3));

    VIXL_CHECK(d2.Is(temps.AcquireV(64)));
    VIXL_CHECK(!temps.IsAvailable(q1));
    VIXL_CHECK(!temps.IsAvailable(d2));
    VIXL_CHECK(temps.IsAvailable(d3));
    VIXL_CHECK(!temps.IsAvailable(s4));
    VIXL_CHECK(!temps.IsAvailable(s5));
    VIXL_CHECK(temps.IsAvailable(s6));
    VIXL_CHECK(temps.IsAvailable(s7));

    VIXL_CHECK(s6.Is(temps.AcquireS()));
    VIXL_CHECK(!temps.IsAvailable(d3));
    VIXL_CHECK(!temps.IsAvailable(s6));
    VIXL_CHECK(temps.IsAvailable(s7));

    VIXL_CHECK(q2.Is(temps.AcquireV(128)));
    VIXL_CHECK(!temps.IsAvailable(q2));
    VIXL_CHECK(!temps.IsAvailable(d4));
    VIXL_CHECK(!temps.IsAvailable(d5));
    VIXL_CHECK(!temps.IsAvailable(s8));
    VIXL_CHECK(!temps.IsAvailable(s9));
    VIXL_CHECK(!temps.IsAvailable(s10));
    VIXL_CHECK(!temps.IsAvailable(s11));
    VIXL_CHECK(temps.IsAvailable(s7));

    VIXL_CHECK(d6.Is(temps.AcquireD()));
    VIXL_CHECK(!temps.IsAvailable(q3));
    VIXL_CHECK(!temps.IsAvailable(d6));
    VIXL_CHECK(temps.IsAvailable(d7));
    VIXL_CHECK(!temps.IsAvailable(s12));
    VIXL_CHECK(!temps.IsAvailable(s13));
    VIXL_CHECK(temps.IsAvailable(s14));
    VIXL_CHECK(temps.IsAvailable(s15));
    VIXL_CHECK(temps.IsAvailable(s7));

    VIXL_CHECK(s7.Is(temps.AcquireS()));
  }
}
#endif  // VIXL_INCLUDE_TARGET_AARCH32


#ifdef VIXL_INCLUDE_TARGET_AARCH32
TEST_AARCH32(include_exclude) {
  SETUP();
  {
    UseScratchRegisterScope temps(&masm);
    temps.Include(r0, r1, r2, r3);
    temps.Include(s0, s1, d1, q1);

    VIXL_CHECK(temps.IsAvailable(r0));
    VIXL_CHECK(temps.IsAvailable(r1));
    VIXL_CHECK(temps.IsAvailable(r2));
    VIXL_CHECK(temps.IsAvailable(r3));

    VIXL_CHECK(temps.IsAvailable(s0));

    VIXL_CHECK(temps.IsAvailable(s1));

    VIXL_CHECK(temps.IsAvailable(d1));
    VIXL_CHECK(temps.IsAvailable(s2));
    VIXL_CHECK(temps.IsAvailable(s3));

    VIXL_CHECK(temps.IsAvailable(q1));
    VIXL_CHECK(temps.IsAvailable(d2));
    VIXL_CHECK(temps.IsAvailable(d3));
    VIXL_CHECK(temps.IsAvailable(s4));
    VIXL_CHECK(temps.IsAvailable(s5));
    VIXL_CHECK(temps.IsAvailable(s6));
    VIXL_CHECK(temps.IsAvailable(s7));

    // Test local exclusion.
    {
      UseScratchRegisterScope local_temps(&masm);
      local_temps.Exclude(r1, r2);
      local_temps.Exclude(s1, q1);

      VIXL_CHECK(temps.IsAvailable(r0));
      VIXL_CHECK(!temps.IsAvailable(r1));
      VIXL_CHECK(!temps.IsAvailable(r2));
      VIXL_CHECK(temps.IsAvailable(r3));

      VIXL_CHECK(temps.IsAvailable(s0));

      VIXL_CHECK(!temps.IsAvailable(s1));

      VIXL_CHECK(temps.IsAvailable(d1));
      VIXL_CHECK(temps.IsAvailable(s2));
      VIXL_CHECK(temps.IsAvailable(s3));

      VIXL_CHECK(!temps.IsAvailable(q1));
      VIXL_CHECK(!temps.IsAvailable(d2));
      VIXL_CHECK(!temps.IsAvailable(d3));
      VIXL_CHECK(!temps.IsAvailable(s4));
      VIXL_CHECK(!temps.IsAvailable(s5));
      VIXL_CHECK(!temps.IsAvailable(s6));
      VIXL_CHECK(!temps.IsAvailable(s7));
    }

    // This time, exclude part of included registers, making sure the entire
    // register gets excluded.
    {
      UseScratchRegisterScope local_temps(&masm);
      local_temps.Exclude(s2, d3);

      VIXL_CHECK(temps.IsAvailable(r0));
      VIXL_CHECK(temps.IsAvailable(r1));
      VIXL_CHECK(temps.IsAvailable(r2));
      VIXL_CHECK(temps.IsAvailable(r3));

      VIXL_CHECK(temps.IsAvailable(s0));

      VIXL_CHECK(temps.IsAvailable(s1));

      // Excluding s2 should exclude d1 but not s3.
      VIXL_CHECK(!temps.IsAvailable(d1));
      VIXL_CHECK(!temps.IsAvailable(s2));
      VIXL_CHECK(temps.IsAvailable(s3));

      // Excluding d3 should exclude q1, s7 and s6 but not d2, s5, s4.
      VIXL_CHECK(!temps.IsAvailable(q1));
      VIXL_CHECK(temps.IsAvailable(d2));
      VIXL_CHECK(!temps.IsAvailable(d3));
      VIXL_CHECK(temps.IsAvailable(s4));
      VIXL_CHECK(temps.IsAvailable(s5));
      VIXL_CHECK(!temps.IsAvailable(s6));
      VIXL_CHECK(!temps.IsAvailable(s7));
    }

    // Make sure the initial state was restored.

    VIXL_CHECK(temps.IsAvailable(r0));
    VIXL_CHECK(temps.IsAvailable(r1));
    VIXL_CHECK(temps.IsAvailable(r2));
    VIXL_CHECK(temps.IsAvailable(r3));

    VIXL_CHECK(temps.IsAvailable(s0));

    VIXL_CHECK(temps.IsAvailable(s1));

    VIXL_CHECK(temps.IsAvailable(d1));
    VIXL_CHECK(temps.IsAvailable(s2));
    VIXL_CHECK(temps.IsAvailable(s3));

    VIXL_CHECK(temps.IsAvailable(q1));
    VIXL_CHECK(temps.IsAvailable(d2));
    VIXL_CHECK(temps.IsAvailable(d3));
    VIXL_CHECK(temps.IsAvailable(s4));
    VIXL_CHECK(temps.IsAvailable(s5));
    VIXL_CHECK(temps.IsAvailable(s6));
    VIXL_CHECK(temps.IsAvailable(s7));
  }
}
#endif  // VIXL_INCLUDE_TARGET_AARCH32

}  // namespace vixl
