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

#include "test-runner.h"

// TODO: Add unit tests for operands on AArch64
// TODO: It would be nice to have negative tests here too.

#ifdef VIXL_INCLUDE_TARGET_AARCH32
#include "aarch32/operands-aarch32.h"
#endif

#define TEST_AARCH32(name) TEST_(AARCH32_OPERANDS_##name)

namespace vixl {

#ifdef VIXL_INCLUDE_TARGET_AARCH32
TEST_AARCH32(operand_from) {
  // Test signed and unsigned values.
  {
    aarch32::Operand op = aarch32::Operand::From(42);
    VIXL_CHECK(op.IsImmediate());
    VIXL_CHECK(op.GetImmediate() == 42);
    VIXL_CHECK(op.GetSignedImmediate() == 42);
  }
  {
    aarch32::Operand op = aarch32::Operand::From(-42);
    VIXL_CHECK(op.IsImmediate());
    VIXL_CHECK(op.GetImmediate() == (~UINT32_C(42) + 1));
    VIXL_CHECK(op.GetSignedImmediate() == -42);
  }

  // Test with 32-bit integer limits.
  {
    aarch32::Operand op = aarch32::Operand::From(-1);
    VIXL_CHECK(op.IsImmediate());
    VIXL_CHECK(op.GetImmediate() == UINT32_MAX);
    VIXL_CHECK(op.GetSignedImmediate() == -1);
  }
  {
    aarch32::Operand op = aarch32::Operand::From(UINT32_MAX);
    VIXL_CHECK(op.IsImmediate());
    VIXL_CHECK(op.GetImmediate() == UINT32_MAX);
    VIXL_CHECK(op.GetSignedImmediate() == -1);
  }
  {
    aarch32::Operand op = aarch32::Operand::From(INT32_MAX);
    VIXL_CHECK(op.IsImmediate());
    VIXL_CHECK(op.GetImmediate() == 0x7fffffffu);
    VIXL_CHECK(op.GetSignedImmediate() == INT32_MAX);
  }
  {
    aarch32::Operand op = aarch32::Operand::From(INT32_MIN);
    VIXL_CHECK(op.IsImmediate());
    VIXL_CHECK(op.GetImmediate() == 0x80000000u);
    VIXL_CHECK(op.GetSignedImmediate() == INT32_MIN);
  }

  // Test with 64-bit signed numbers encodable in 32 bits.
  {
    aarch32::Operand op = aarch32::Operand::From(INT64_C(-1));
    VIXL_CHECK(op.IsImmediate());
    VIXL_CHECK(op.GetImmediate() == UINT32_MAX);
    VIXL_CHECK(op.GetSignedImmediate() == -1);
  }
  {
    aarch32::Operand op = aarch32::Operand::From(INT64_C(-42));
    VIXL_CHECK(op.IsImmediate());
    VIXL_CHECK(op.GetImmediate() == (~UINT32_C(42) + 1));
    VIXL_CHECK(op.GetSignedImmediate() == -42);
  }

#if defined(VIXL_HOST_POINTER_32)
  // Test with a pointer that is known to fit into 32 bits.
  {
    int data = 42;
    aarch32::Operand op = aarch32::Operand::From(&data);
    VIXL_CHECK(op.IsImmediate());
    VIXL_CHECK(op.GetImmediate() == reinterpret_cast<uintptr_t>(&data));
    VIXL_CHECK(op.GetSignedImmediate() == reinterpret_cast<intptr_t>(&data));
  }
  {
    int* data = new int;
    *data = 42;
    aarch32::Operand op = aarch32::Operand::From(data);
    VIXL_CHECK(op.IsImmediate());
    VIXL_CHECK(op.GetImmediate() == reinterpret_cast<uintptr_t>(data));
    VIXL_CHECK(op.GetSignedImmediate() == reinterpret_cast<intptr_t>(data));
    delete data;
  }
#endif
}
#endif  // VIXL_INCLUDE_TARGET_AARCH32

}  // namespace vixl
