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
#include "test-utils-aarch64.h"

#include "aarch64/abi-aarch64.h"

#if (__cplusplus >= 201103L) && !defined(VIXL_HAS_ABI_SUPPORT)
#error "C++11 should be sufficient to provide ABI support."
#endif  // #if (__cplusplus >= 201103L) && !defined(VIXL_HAS_ABI_SUPPORT)

#ifdef VIXL_HAS_ABI_SUPPORT

#define TEST(name) TEST_(AARCH64_ABI_##name)

namespace vixl {
namespace aarch64 {


TEST(abi) {
  ABI abi;

  VIXL_CHECK(abi.GetStackSpaceRequired() == 0);
  VIXL_CHECK(!abi.GetReturnGenericOperand<void>().IsValid());

  VIXL_CHECK(abi.GetReturnGenericOperand<bool>().Equals(GenericOperand(w0)));
  VIXL_CHECK(abi.GetReturnGenericOperand<char>().Equals(GenericOperand(w0)));
  VIXL_CHECK(abi.GetReturnGenericOperand<int8_t>().Equals(GenericOperand(w0)));
  VIXL_CHECK(abi.GetReturnGenericOperand<uint8_t>().Equals(GenericOperand(w0)));
  VIXL_CHECK(
      abi.GetReturnGenericOperand<short>().Equals(  // NOLINT(runtime/int)
          GenericOperand(w0)));
  VIXL_CHECK(abi.GetReturnGenericOperand<int16_t>().Equals(GenericOperand(w0)));
  VIXL_CHECK(
      abi.GetReturnGenericOperand<uint16_t>().Equals(GenericOperand(w0)));
  VIXL_CHECK(abi.GetReturnGenericOperand<int>().Equals(GenericOperand(w0)));
  VIXL_CHECK(abi.GetReturnGenericOperand<int32_t>().Equals(GenericOperand(w0)));
  VIXL_CHECK(
      abi.GetReturnGenericOperand<uint32_t>().Equals(GenericOperand(w0)));
  VIXL_CHECK(abi.GetReturnGenericOperand<int64_t>().Equals(GenericOperand(x0)));
  VIXL_CHECK(
      abi.GetReturnGenericOperand<uint64_t>().Equals(GenericOperand(x0)));

  VIXL_CHECK(abi.GetReturnGenericOperand<float>().Equals(GenericOperand(s0)));
  VIXL_CHECK(abi.GetReturnGenericOperand<double>().Equals(GenericOperand(d0)));

  GenericOperand found(NoReg);
  GenericOperand expected(NoReg);
#define CHECK_NEXT_PARAMETER_REG(type, reg)           \
  found = abi.GetNextParameterGenericOperand<type>(); \
  expected = GenericOperand(reg);                     \
  VIXL_CHECK(found.Equals(expected))
// Slots on the stack are always 8 bytes.
#define CHECK_NEXT_PARAMETER_MEM(type, mem_op, size)  \
  found = abi.GetNextParameterGenericOperand<type>(); \
  expected = GenericOperand(mem_op, size);            \
  VIXL_CHECK(found.Equals(expected))

  abi.Reset();
  CHECK_NEXT_PARAMETER_REG(int, w0);
  CHECK_NEXT_PARAMETER_REG(char, w1);
  CHECK_NEXT_PARAMETER_REG(bool, w2);
  CHECK_NEXT_PARAMETER_REG(float, s0);
  CHECK_NEXT_PARAMETER_REG(double, d1);
  CHECK_NEXT_PARAMETER_REG(double, d2);
  CHECK_NEXT_PARAMETER_REG(float, s3);
  CHECK_NEXT_PARAMETER_REG(int64_t, x3);
  CHECK_NEXT_PARAMETER_REG(uint64_t, x4);
  CHECK_NEXT_PARAMETER_REG(void*, x5);
  CHECK_NEXT_PARAMETER_REG(uint32_t, w6);
  typedef short my_type;  // NOLINT(runtime/int)
  CHECK_NEXT_PARAMETER_REG(my_type, w7);
  CHECK_NEXT_PARAMETER_MEM(int, MemOperand(sp, 0), kWRegSizeInBytes);
  CHECK_NEXT_PARAMETER_MEM(int, MemOperand(sp, 8), kWRegSizeInBytes);
  CHECK_NEXT_PARAMETER_REG(double, d4);
  CHECK_NEXT_PARAMETER_REG(double, d5);
  CHECK_NEXT_PARAMETER_REG(double, d6);
  CHECK_NEXT_PARAMETER_REG(double, d7);
  CHECK_NEXT_PARAMETER_MEM(double, MemOperand(sp, 16), kDRegSizeInBytes);
  CHECK_NEXT_PARAMETER_MEM(bool, MemOperand(sp, 24), kWRegSizeInBytes);
  CHECK_NEXT_PARAMETER_MEM(short,  // NOLINT(runtime/int)
                           MemOperand(sp, 32),
                           kWRegSizeInBytes);
  CHECK_NEXT_PARAMETER_MEM(float, MemOperand(sp, 40), kSRegSizeInBytes);
  CHECK_NEXT_PARAMETER_MEM(float, MemOperand(sp, 48), kSRegSizeInBytes);
  VIXL_CHECK(abi.GetStackSpaceRequired() == 56);
}
}
}  // namespace vixl::aarch64

#endif  // VIXL_ABI_SUPORT
