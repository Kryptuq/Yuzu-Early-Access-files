// Copyright 2018, VIXL authors
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

#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64

#include "test-runner.h"
#include "aarch64/simulator-aarch64.h"

#define TEST(name) TEST_(AARCH64_POINTER_AUTH_##name)

namespace vixl {
namespace aarch64 {


TEST(compute_pac) {
  Decoder decoder;
  Simulator sim(&decoder);

  uint64_t data1 = 0xfb623599da6e8127;
  uint64_t data2 = 0x27979fadf7d53cb7;
  uint64_t context = 0x477d469dec0b8762;
  Simulator::PACKey key = {0x84be85ce9804e94b, 0xec2802d4e0a488e9, -1};

  uint64_t pac1 = sim.ComputePAC(data1, context, key);
  uint64_t pac2 = sim.ComputePAC(data2, context, key);

  // NOTE: If the PAC implementation is changed, this may fail due to a hash
  // collision.
  VIXL_CHECK(pac1 != pac2);
}

TEST(add_and_auth_pac) {
  Decoder decoder;
  Simulator sim(&decoder);

  uint64_t ptr = 0x0000000012345678;
  uint64_t context = 0x477d469dec0b8762;
  Simulator::PACKey key_a = {0x84be85ce9804e94b, 0xec2802d4e0a488e9, 0};
  Simulator::PACKey key_b = {0xec1119e288704d13, 0xd7f6b76e1cea585e, 1};

  uint64_t ptr_a =
      sim.AddPAC(ptr, context, key_a, Simulator::kInstructionPointer);

  // Attempt to authenticate the pointer with PAC using different keys.
  uint64_t success =
      sim.AuthPAC(ptr_a, context, key_a, Simulator::kInstructionPointer);
  uint64_t fail =
      sim.AuthPAC(ptr_a, context, key_b, Simulator::kInstructionPointer);

  uint64_t pac_mask =
      sim.CalculatePACMask(ptr, Simulator::kInstructionPointer, 0);

  // NOTE: If the PAC implementation is changed, this may fail due to a hash
  // collision.
  VIXL_CHECK((ptr_a & pac_mask) != 0x0);
  VIXL_CHECK(success == ptr);
  VIXL_CHECK(fail != ptr);
}

TEST(add_and_strip_pac) {
  Decoder decoder;
  Simulator sim(&decoder);

  uint64_t ptr = 0xff00000012345678;
  uint64_t pac_mask =
      sim.CalculatePACMask(ptr, Simulator::kInstructionPointer, 0);
  uint64_t ptr_a = ptr | pac_mask;


  VIXL_CHECK(sim.StripPAC(ptr_a, Simulator::kInstructionPointer) == ptr);
}
}
}  // namespace vixl::aarch64

#endif  // VIXL_INCLUDE_SIMULATOR_AARCH64
