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

#include <cstdio>
#include <cstring>
#include <string>

#include "test-runner.h"
#include "test-utils.h"
#include "aarch64/test-utils-aarch64.h"

#include "aarch64/macro-assembler-aarch64.h"
#include "aarch64/registers-aarch64.h"
#include "aarch64/simulator-aarch64.h"

#define __ masm.
#define TEST(name) TEST_(AARCH64_API_##name)


namespace vixl {
namespace aarch64 {

// Check compiler intrinsics helpers.

TEST(count_leading_sign_bits) {
  class Helper {
   public:
    static void Check(int64_t value, int non_sign_bits) {
      VIXL_ASSERT((0 <= non_sign_bits) && (non_sign_bits < 64));

      for (int width = 1; width <= 64; width *= 2) {
        // Note that leading_sign_bits does not include the topmost bit.
        int leading_sign_bits = width - non_sign_bits - 1;
        if (leading_sign_bits < 0) continue;

        int64_t result = CountLeadingSignBits(value, width);
        int64_t fallback_result = CountLeadingSignBitsFallBack(value, width);
        VIXL_CHECK(result == leading_sign_bits);
        VIXL_CHECK(fallback_result == leading_sign_bits);
      }
    }
  };

  // Basic positive (and zero) cases. Sign bits are all zeroes.
  Helper::Check(0, 0);  // 0b++++
  Helper::Check(1, 1);  // 0b+++1
  Helper::Check(2, 2);  // 0b++10
  Helper::Check(3, 2);  // 0b++11
  Helper::Check(4, 3);  // 0b+100

  // Basic negative cases. Sign bits are all ones.
  Helper::Check(-1, 0);  // 0b----
  Helper::Check(-2, 1);  // 0b---0
  Helper::Check(-3, 2);  // 0b--01
  Helper::Check(-4, 2);  // 0b--00
  Helper::Check(-5, 3);  // 0b-011

  // Boundary conditions.
  Helper::Check(INT8_MAX, 7);
  Helper::Check(INT8_MIN, 7);
  Helper::Check(static_cast<int64_t>(INT8_MAX) + 1, 8);
  Helper::Check(static_cast<int64_t>(INT8_MIN) - 1, 8);

  Helper::Check(INT16_MAX, 15);
  Helper::Check(INT16_MIN, 15);
  Helper::Check(static_cast<int64_t>(INT16_MAX) + 1, 16);
  Helper::Check(static_cast<int64_t>(INT16_MIN) - 1, 16);

  Helper::Check(INT32_MAX, 31);
  Helper::Check(INT32_MIN, 31);
  Helper::Check(static_cast<int64_t>(INT32_MAX) + 1, 32);
  Helper::Check(static_cast<int64_t>(INT32_MIN) - 1, 32);

  Helper::Check(INT64_MAX, 63);
  Helper::Check(INT64_MIN, 63);

  // Check automatic width detection.
  VIXL_CHECK(CountLeadingSignBits(static_cast<int8_t>(42)) == 1);  // 0b00101010
  VIXL_CHECK(CountLeadingSignBits(static_cast<int16_t>(42)) == 9);
  VIXL_CHECK(CountLeadingSignBits(static_cast<int32_t>(42)) == 25);
  VIXL_CHECK(CountLeadingSignBits(static_cast<int64_t>(42)) == 57);
}

// Check SimFloat16 class mechanics.
TEST(float16_operators) {
  ::vixl::internal::SimFloat16 f1 = kFP16DefaultNaN;
  ::vixl::internal::SimFloat16 f2 = kFP16DefaultNaN;
  ::vixl::internal::SimFloat16 f3 = kFP16PositiveInfinity;
  ::vixl::internal::SimFloat16 f4 = kFP16NegativeInfinity;
  VIXL_CHECK(!(f1 == f2));
  VIXL_CHECK(f1 != f2);
  VIXL_CHECK(!(f3 == f4));
  VIXL_CHECK(f3 != f4);
  VIXL_CHECK(::vixl::internal::SimFloat16(kFP16PositiveZero) ==
             ::vixl::internal::SimFloat16(kFP16NegativeZero));
  VIXL_CHECK(!(::vixl::internal::SimFloat16(kFP16PositiveZero) !=
               ::vixl::internal::SimFloat16(kFP16NegativeZero)));
}

TEST(rawbits_conversions) {
  VIXL_CHECK(RawbitsToInt64(0x0) == 0x0);
  VIXL_CHECK(RawbitsToInt64(0x123) == 0x123);
  VIXL_CHECK(RawbitsToInt64(INT64_MAX) == INT64_MAX);
  VIXL_CHECK(RawbitsToInt64(UINT64_C(0xffffffffffffffff)) == -1);
  VIXL_CHECK(RawbitsToInt64(UINT64_C(0x8000000000000000)) == INT64_MIN);
  VIXL_CHECK(RawbitsToInt64(UINT64_C(0x8000000000000001)) == -INT64_MAX);

  VIXL_CHECK(RawbitsToInt32(0x0) == 0x0);
  VIXL_CHECK(RawbitsToInt32(0x123) == 0x123);
  VIXL_CHECK(RawbitsToInt32(INT32_MAX) == INT32_MAX);
  VIXL_CHECK(RawbitsToInt32(UINT32_C(0xffffffff)) == -1);
  VIXL_CHECK(RawbitsToInt32(UINT32_C(0x80000000)) == INT32_MIN);
  VIXL_CHECK(RawbitsToInt32(UINT32_C(0x80000001)) == -INT32_MAX);
}

// Check moved FP constants are still accessible via the AArch64 namespace.
TEST(float_constants_scope) {
  VIXL_CHECK(vixl::aarch64::kFP64PositiveInfinity ==
             vixl::kFP64PositiveInfinity);
  VIXL_CHECK(vixl::aarch64::kFP64NegativeInfinity ==
             vixl::kFP64NegativeInfinity);
  VIXL_CHECK(vixl::aarch64::kFP32PositiveInfinity ==
             vixl::kFP32PositiveInfinity);
  VIXL_CHECK(vixl::aarch64::kFP32NegativeInfinity ==
             vixl::kFP32NegativeInfinity);
  VIXL_CHECK(Float16ToRawbits(vixl::aarch64::kFP16PositiveInfinity) ==
             Float16ToRawbits(vixl::aarch64::kFP16PositiveInfinity));
  VIXL_CHECK(Float16ToRawbits(vixl::aarch64::kFP16NegativeInfinity) ==
             Float16ToRawbits(vixl::aarch64::kFP16NegativeInfinity));
  VIXL_CHECK(DoubleToRawbits(vixl::aarch64::kFP64DefaultNaN) ==
             DoubleToRawbits(vixl::kFP64DefaultNaN));
  VIXL_CHECK(FloatToRawbits(vixl::aarch64::kFP32DefaultNaN) ==
             FloatToRawbits(vixl::kFP32DefaultNaN));
  VIXL_CHECK(IsNaN(vixl::aarch64::kFP16DefaultNaN) ==
             IsNaN(vixl::kFP16DefaultNaN));
  VIXL_CHECK(vixl::aarch64::kDoubleExponentBits == vixl::kDoubleExponentBits);
  VIXL_CHECK(vixl::aarch64::kDoubleMantissaBits == vixl::kDoubleMantissaBits);
  VIXL_CHECK(vixl::aarch64::kFloatExponentBits == vixl::kFloatExponentBits);
  VIXL_CHECK(vixl::aarch64::kFloatMantissaBits == vixl::kFloatMantissaBits);
  VIXL_CHECK(vixl::aarch64::kFloat16ExponentBits == vixl::kFloat16ExponentBits);
  VIXL_CHECK(vixl::aarch64::kFloat16MantissaBits == vixl::kFloat16MantissaBits);
}


TEST(register_bit) {
  VIXL_CHECK(x0.GetBit() == (UINT64_C(1) << 0));
  VIXL_CHECK(x1.GetBit() == (UINT64_C(1) << 1));
  VIXL_CHECK(x10.GetBit() == (UINT64_C(1) << 10));

  // AAPCS64 definitions.
  VIXL_CHECK(lr.GetBit() == (UINT64_C(1) << kLinkRegCode));

  // Fixed (hardware) definitions.
  VIXL_CHECK(xzr.GetBit() == (UINT64_C(1) << kZeroRegCode));

  // Internal ABI definitions.
  VIXL_CHECK(sp.GetBit() == (UINT64_C(1) << kSPRegInternalCode));
  VIXL_CHECK(sp.GetBit() != xzr.GetBit());

  // xn.GetBit() == wn.GetBit() at all times, for the same n.
  VIXL_CHECK(x0.GetBit() == w0.GetBit());
  VIXL_CHECK(x1.GetBit() == w1.GetBit());
  VIXL_CHECK(x10.GetBit() == w10.GetBit());
  VIXL_CHECK(xzr.GetBit() == wzr.GetBit());
  VIXL_CHECK(sp.GetBit() == wsp.GetBit());
}


TEST(noreg) {
  VIXL_CHECK(NoReg.Is(NoVReg));
  VIXL_CHECK(NoVReg.Is(NoReg));

  VIXL_CHECK(NoVReg.Is(NoReg));
  VIXL_CHECK(NoReg.Is(NoVReg));

  VIXL_CHECK(NoReg.Is(NoCPUReg));
  VIXL_CHECK(NoCPUReg.Is(NoReg));

  VIXL_CHECK(NoVReg.Is(NoCPUReg));
  VIXL_CHECK(NoCPUReg.Is(NoVReg));

  VIXL_CHECK(NoVReg.Is(NoCPUReg));
  VIXL_CHECK(NoCPUReg.Is(NoVReg));

  VIXL_CHECK(NoReg.IsNone());
  VIXL_CHECK(NoVReg.IsNone());
  VIXL_CHECK(NoCPUReg.IsNone());
}


TEST(constructors) {
  // *Register(code)
  VIXL_CHECK(WRegister(0).Is(w0));
  VIXL_CHECK(XRegister(1).Is(x1));

  VIXL_CHECK(BRegister(2).Is(b2));
  VIXL_CHECK(HRegister(3).Is(h3));
  VIXL_CHECK(SRegister(4).Is(s4));
  VIXL_CHECK(DRegister(5).Is(d5));
  VIXL_CHECK(QRegister(6).Is(q6));

  VIXL_CHECK(ZRegister(7).Is(z7));
  VIXL_CHECK(PRegister(8).Is(p8));
}


TEST(constructors_r) {
  // Register(code, size_in_bits)
  VIXL_CHECK(Register(0, kWRegSize).Is(w0));
  VIXL_CHECK(Register(1, kXRegSize).Is(x1));
}


TEST(constructors_v) {
  // VRegister(code)
  VIXL_CHECK(VRegister(0).Is(v0));
  VIXL_CHECK(VRegister(1).Is(v1));
  VIXL_CHECK(VRegister(2).Is(v2));
  VIXL_CHECK(VRegister(3).Is(v3));
  VIXL_CHECK(VRegister(4).Is(v4));

  // VRegister(code, size_in_bits)
  VIXL_CHECK(VRegister(0, kBRegSize).Is(b0));
  VIXL_CHECK(VRegister(1, kHRegSize).Is(h1));
  VIXL_CHECK(VRegister(2, kSRegSize).Is(s2));
  VIXL_CHECK(VRegister(3, kDRegSize).Is(d3));
  VIXL_CHECK(VRegister(4, kQRegSize).Is(q4));

  // VRegister(code, size_in_bits, lanes)
  VIXL_CHECK(VRegister(0, kBRegSize, 1).Is(b0));
  VIXL_CHECK(VRegister(1, kHRegSize, 1).Is(h1));
  VIXL_CHECK(VRegister(2, kSRegSize, 1).Is(s2));
  VIXL_CHECK(VRegister(3, kDRegSize, 1).Is(d3));
  VIXL_CHECK(VRegister(4, kQRegSize, 1).Is(q4));

  VIXL_CHECK(VRegister(0, kSRegSize, 2).Is(v0.V2H()));

  VIXL_CHECK(VRegister(1, kDRegSize, 1).Is(v1.V1D()));
  VIXL_CHECK(VRegister(2, kDRegSize, 2).Is(v2.V2S()));
  VIXL_CHECK(VRegister(3, kDRegSize, 4).Is(v3.V4H()));
  VIXL_CHECK(VRegister(4, kDRegSize, 8).Is(v4.V8B()));

  VIXL_CHECK(VRegister(5, kQRegSize, 2).Is(v5.V2D()));
  VIXL_CHECK(VRegister(6, kQRegSize, 4).Is(v6.V4S()));
  VIXL_CHECK(VRegister(7, kQRegSize, 8).Is(v7.V8H()));
  VIXL_CHECK(VRegister(8, kQRegSize, 16).Is(v8.V16B()));

  // VRegister(code, format)
  VIXL_CHECK(VRegister(0, kFormatB).Is(b0));
  VIXL_CHECK(VRegister(1, kFormatH).Is(h1));
  VIXL_CHECK(VRegister(2, kFormatS).Is(s2));
  VIXL_CHECK(VRegister(3, kFormatD).Is(d3));
  VIXL_CHECK(VRegister(4, kFormat8B).Is(v4.V8B()));
  VIXL_CHECK(VRegister(5, kFormat16B).Is(v5.V16B()));
  VIXL_CHECK(VRegister(6, kFormat2H).Is(v6.V2H()));
  VIXL_CHECK(VRegister(7, kFormat4H).Is(v7.V4H()));
  VIXL_CHECK(VRegister(8, kFormat8H).Is(v8.V8H()));
  VIXL_CHECK(VRegister(9, kFormat2S).Is(v9.V2S()));
  VIXL_CHECK(VRegister(10, kFormat4S).Is(v10.V4S()));
  VIXL_CHECK(VRegister(11, kFormat1D).Is(v11.V1D()));
  VIXL_CHECK(VRegister(12, kFormat2D).Is(v12.V2D()));
}


TEST(constructors_z) {
  // ZRegister(code, lane_size_in_bits)
  VIXL_CHECK(ZRegister(0, kBRegSize).Is(z0.VnB()));
  VIXL_CHECK(ZRegister(1, kHRegSize).Is(z1.VnH()));
  VIXL_CHECK(ZRegister(2, kSRegSize).Is(z2.VnS()));
  VIXL_CHECK(ZRegister(3, kDRegSize).Is(z3.VnD()));

  // ZRegister(code, format)
  VIXL_CHECK(ZRegister(0, kFormatVnB).Is(z0.VnB()));
  VIXL_CHECK(ZRegister(1, kFormatVnH).Is(z1.VnH()));
  VIXL_CHECK(ZRegister(2, kFormatVnS).Is(z2.VnS()));
  VIXL_CHECK(ZRegister(3, kFormatVnD).Is(z3.VnD()));
}


TEST(constructors_p) {
  // ZRegister(code, lane_size_in_bits)
  VIXL_CHECK(PRegisterWithLaneSize(0, kBRegSize).Is(p0.VnB()));
  VIXL_CHECK(PRegisterWithLaneSize(1, kHRegSize).Is(p1.VnH()));
  VIXL_CHECK(PRegisterWithLaneSize(2, kSRegSize).Is(p2.VnS()));
  VIXL_CHECK(PRegisterWithLaneSize(3, kDRegSize).Is(p3.VnD()));

  // ZRegister(code, format)
  VIXL_CHECK(PRegisterWithLaneSize(0, kFormatVnB).Is(p0.VnB()));
  VIXL_CHECK(PRegisterWithLaneSize(1, kFormatVnH).Is(p1.VnH()));
  VIXL_CHECK(PRegisterWithLaneSize(2, kFormatVnS).Is(p2.VnS()));
  VIXL_CHECK(PRegisterWithLaneSize(3, kFormatVnD).Is(p3.VnD()));

  VIXL_CHECK(PRegisterZ(0).Is(p0.Zeroing()));
  VIXL_CHECK(PRegisterM(1).Is(p1.Merging()));
}


TEST(constructors_cpu) {
  // ZRegister(code, size_in_bits, type)
  VIXL_CHECK(CPURegister(0, kWRegSize, CPURegister::kRegister).Is(w0));
  VIXL_CHECK(CPURegister(1, kXRegSize, CPURegister::kRegister).Is(x1));

  VIXL_CHECK(CPURegister(2, kBRegSize, CPURegister::kVRegister).Is(b2));
  VIXL_CHECK(CPURegister(3, kHRegSize, CPURegister::kVRegister).Is(h3));
  VIXL_CHECK(CPURegister(4, kSRegSize, CPURegister::kVRegister).Is(s4));
  VIXL_CHECK(CPURegister(5, kDRegSize, CPURegister::kVRegister).Is(d5));
  VIXL_CHECK(CPURegister(6, kQRegSize, CPURegister::kVRegister).Is(q6));
  VIXL_CHECK(CPURegister(7, kQRegSize, CPURegister::kVRegister).Is(v7));

  VIXL_CHECK(CPURegister(0, CPURegister::kUnknownSize, CPURegister::kVRegister)
                 .Is(z0));
  VIXL_CHECK(CPURegister(1, CPURegister::kUnknownSize, CPURegister::kPRegister)
                 .Is(p1));
}


#ifdef __aarch64__
static void CPURegisterByValueHelper(CPURegister reg) {
  // Test that `reg` can be passed in one register. We'd like to use
  // __attribute__((naked)) for this, but it isn't supported for AArch64, so
  // generate a function using VIXL instead.

  MacroAssembler masm;
  // CPURegister fn(int placeholder, CPURegister reg);
  // Move `reg` to its result register.
  __ Mov(x0, x1);
  // Clobber all other result registers.
  __ Mov(x1, 0xfffffffffffffff1);
  __ Mov(x2, 0xfffffffffffffff2);
  __ Mov(x3, 0xfffffffffffffff3);
  __ Mov(x4, 0xfffffffffffffff4);
  __ Mov(x5, 0xfffffffffffffff5);
  __ Mov(x6, 0xfffffffffffffff6);
  __ Mov(x7, 0xfffffffffffffff7);
  __ Ret();
  masm.FinalizeCode();

  CodeBuffer* buffer = masm.GetBuffer();
  auto fn = buffer->GetStartAddress<CPURegister (*)(int, CPURegister)>();
  buffer->SetExecutable();
  CPURegister out = fn(42, reg);

  VIXL_CHECK(out.Is(reg));
}


TEST(cpureg_by_value) {
  VIXL_STATIC_ASSERT(sizeof(CPURegister) <= sizeof(void*));
  // Check some arbitrary registers to try to exercise each encoding field.
  CPURegisterByValueHelper(x0);
  CPURegisterByValueHelper(v31.V8H());
  CPURegisterByValueHelper(z16.VnD());
  CPURegisterByValueHelper(p15.Merging());
}
#endif  // __aarch64__


TEST(isvalid) {
  VIXL_CHECK(!NoReg.IsValid());
  VIXL_CHECK(!NoVReg.IsValid());
  VIXL_CHECK(!NoCPUReg.IsValid());

  VIXL_CHECK(x0.IsValid());
  VIXL_CHECK(w0.IsValid());
  VIXL_CHECK(x30.IsValid());
  VIXL_CHECK(w30.IsValid());
  VIXL_CHECK(xzr.IsValid());
  VIXL_CHECK(wzr.IsValid());

  VIXL_CHECK(sp.IsValid());
  VIXL_CHECK(wsp.IsValid());

  VIXL_CHECK(d0.IsValid());
  VIXL_CHECK(s0.IsValid());
  VIXL_CHECK(d31.IsValid());
  VIXL_CHECK(s31.IsValid());

  VIXL_CHECK(x0.IsValidRegister());
  VIXL_CHECK(w0.IsValidRegister());
  VIXL_CHECK(xzr.IsValidRegister());
  VIXL_CHECK(wzr.IsValidRegister());
  VIXL_CHECK(sp.IsValidRegister());
  VIXL_CHECK(wsp.IsValidRegister());
  VIXL_CHECK(!x0.IsValidVRegister());
  VIXL_CHECK(!w0.IsValidVRegister());
  VIXL_CHECK(!xzr.IsValidVRegister());
  VIXL_CHECK(!wzr.IsValidVRegister());
  VIXL_CHECK(!sp.IsValidVRegister());
  VIXL_CHECK(!wsp.IsValidVRegister());
  VIXL_CHECK(!x0.IsValidFPRegister());
  VIXL_CHECK(!w0.IsValidFPRegister());
  VIXL_CHECK(!xzr.IsValidFPRegister());
  VIXL_CHECK(!wzr.IsValidFPRegister());
  VIXL_CHECK(!sp.IsValidFPRegister());
  VIXL_CHECK(!wsp.IsValidFPRegister());

  VIXL_CHECK(q0.IsValidVRegister());
  VIXL_CHECK(!q0.IsValidFPRegister());
  VIXL_CHECK(!q0.IsValidRegister());

  VIXL_CHECK(d0.IsValidVRegister());
  VIXL_CHECK(d0.IsValidFPRegister());
  VIXL_CHECK(!d0.IsValidRegister());

  VIXL_CHECK(s0.IsValidVRegister());
  VIXL_CHECK(s0.IsValidFPRegister());
  VIXL_CHECK(!s0.IsValidRegister());

  VIXL_CHECK(h0.IsValidVRegister());
  VIXL_CHECK(h0.IsValidFPRegister());
  VIXL_CHECK(!h0.IsValidRegister());

  VIXL_CHECK(b0.IsValidVRegister());
  VIXL_CHECK(!b0.IsValidFPRegister());
  VIXL_CHECK(!b0.IsValidRegister());

  // IsValidFPRegister() is only true for scalar types.
  VIXL_CHECK(q0.V2D().IsValidVRegister());
  VIXL_CHECK(!q0.V2D().IsValidFPRegister());
  VIXL_CHECK(d0.V2S().IsValidVRegister());
  VIXL_CHECK(!d0.V2S().IsValidFPRegister());
  VIXL_CHECK(s0.V2H().IsValidVRegister());
  VIXL_CHECK(!s0.V2H().IsValidFPRegister());
}


TEST(isvalid_cpu) {
  // As 'isvalid', but using CPURegister types where possible. This shouldn't
  // make any difference.
  VIXL_CHECK(!static_cast<CPURegister>(NoReg).IsValid());
  VIXL_CHECK(!static_cast<CPURegister>(NoVReg).IsValid());
  VIXL_CHECK(!static_cast<CPURegister>(NoCPUReg).IsValid());

  VIXL_CHECK(static_cast<CPURegister>(x0).IsValid());
  VIXL_CHECK(static_cast<CPURegister>(w0).IsValid());
  VIXL_CHECK(static_cast<CPURegister>(x30).IsValid());
  VIXL_CHECK(static_cast<CPURegister>(w30).IsValid());
  VIXL_CHECK(static_cast<CPURegister>(xzr).IsValid());
  VIXL_CHECK(static_cast<CPURegister>(wzr).IsValid());

  VIXL_CHECK(static_cast<CPURegister>(sp).IsValid());
  VIXL_CHECK(static_cast<CPURegister>(wsp).IsValid());

  VIXL_CHECK(static_cast<CPURegister>(d0).IsValid());
  VIXL_CHECK(static_cast<CPURegister>(s0).IsValid());
  VIXL_CHECK(static_cast<CPURegister>(d31).IsValid());
  VIXL_CHECK(static_cast<CPURegister>(s31).IsValid());

  VIXL_CHECK(static_cast<CPURegister>(x0).IsValidRegister());
  VIXL_CHECK(static_cast<CPURegister>(w0).IsValidRegister());
  VIXL_CHECK(static_cast<CPURegister>(xzr).IsValidRegister());
  VIXL_CHECK(static_cast<CPURegister>(wzr).IsValidRegister());
  VIXL_CHECK(static_cast<CPURegister>(sp).IsValidRegister());
  VIXL_CHECK(static_cast<CPURegister>(wsp).IsValidRegister());
  VIXL_CHECK(!static_cast<CPURegister>(x0).IsValidVRegister());
  VIXL_CHECK(!static_cast<CPURegister>(w0).IsValidVRegister());
  VIXL_CHECK(!static_cast<CPURegister>(xzr).IsValidVRegister());
  VIXL_CHECK(!static_cast<CPURegister>(wzr).IsValidVRegister());
  VIXL_CHECK(!static_cast<CPURegister>(sp).IsValidVRegister());
  VIXL_CHECK(!static_cast<CPURegister>(wsp).IsValidVRegister());
  VIXL_CHECK(!static_cast<CPURegister>(x0).IsValidFPRegister());
  VIXL_CHECK(!static_cast<CPURegister>(w0).IsValidFPRegister());
  VIXL_CHECK(!static_cast<CPURegister>(xzr).IsValidFPRegister());
  VIXL_CHECK(!static_cast<CPURegister>(wzr).IsValidFPRegister());
  VIXL_CHECK(!static_cast<CPURegister>(sp).IsValidFPRegister());
  VIXL_CHECK(!static_cast<CPURegister>(wsp).IsValidFPRegister());

  VIXL_CHECK(static_cast<CPURegister>(q0).IsValidVRegister());
  VIXL_CHECK(!static_cast<CPURegister>(q0).IsValidFPRegister());
  VIXL_CHECK(!static_cast<CPURegister>(q0).IsValidRegister());

  VIXL_CHECK(static_cast<CPURegister>(d0).IsValidVRegister());
  VIXL_CHECK(static_cast<CPURegister>(d0).IsValidFPRegister());
  VIXL_CHECK(!static_cast<CPURegister>(d0).IsValidRegister());

  VIXL_CHECK(static_cast<CPURegister>(s0).IsValidVRegister());
  VIXL_CHECK(static_cast<CPURegister>(s0).IsValidFPRegister());
  VIXL_CHECK(!static_cast<CPURegister>(s0).IsValidRegister());

  VIXL_CHECK(static_cast<CPURegister>(h0).IsValidVRegister());
  VIXL_CHECK(static_cast<CPURegister>(h0).IsValidFPRegister());
  VIXL_CHECK(!static_cast<CPURegister>(h0).IsValidRegister());

  VIXL_CHECK(static_cast<CPURegister>(b0).IsValidVRegister());
  VIXL_CHECK(!static_cast<CPURegister>(b0).IsValidFPRegister());
  VIXL_CHECK(!static_cast<CPURegister>(b0).IsValidRegister());
}


TEST(areconsecutive) {
  VIXL_CHECK(AreConsecutive(b0, NoVReg));
  VIXL_CHECK(AreConsecutive(b1, b2));
  VIXL_CHECK(AreConsecutive(b3, b4, b5));
  VIXL_CHECK(AreConsecutive(b6, b7, b8, b9));
  VIXL_CHECK(AreConsecutive(h10, NoVReg));
  VIXL_CHECK(AreConsecutive(h11, h12));
  VIXL_CHECK(AreConsecutive(h13, h14, h15));
  VIXL_CHECK(AreConsecutive(h16, h17, h18, h19));
  VIXL_CHECK(AreConsecutive(s20, NoVReg));
  VIXL_CHECK(AreConsecutive(s21, s22));
  VIXL_CHECK(AreConsecutive(s23, s24, s25));
  VIXL_CHECK(AreConsecutive(s26, s27, s28, s29));
  VIXL_CHECK(AreConsecutive(d30, NoVReg));
  VIXL_CHECK(AreConsecutive(d31, d0));
  VIXL_CHECK(AreConsecutive(d1, d2, d3));
  VIXL_CHECK(AreConsecutive(d4, d5, d6, d7));
  VIXL_CHECK(AreConsecutive(q8, NoVReg));
  VIXL_CHECK(AreConsecutive(q9, q10));
  VIXL_CHECK(AreConsecutive(q11, q12, q13));
  VIXL_CHECK(AreConsecutive(q14, q15, q16, q17));
  VIXL_CHECK(AreConsecutive(v18, NoVReg));
  VIXL_CHECK(AreConsecutive(v19, v20));
  VIXL_CHECK(AreConsecutive(v21, v22, v23));
  VIXL_CHECK(AreConsecutive(v24, v25, v26, v27));
  VIXL_CHECK(AreConsecutive(b29, h30));
  VIXL_CHECK(AreConsecutive(s31, d0, q1));
  VIXL_CHECK(AreConsecutive(v2, b3, h4, s5));

  VIXL_CHECK(!AreConsecutive(b0, b2));
  VIXL_CHECK(!AreConsecutive(h1, h0));
  VIXL_CHECK(!AreConsecutive(s31, s1));
  VIXL_CHECK(!AreConsecutive(d12, d12));
  VIXL_CHECK(!AreConsecutive(q31, q1));

  VIXL_CHECK(!AreConsecutive(b0, b1, b3));
  VIXL_CHECK(!AreConsecutive(h4, h5, h6, h6));
  VIXL_CHECK(!AreConsecutive(d11, d13, NoVReg, d14));
  VIXL_CHECK(!AreConsecutive(d15, d16, d18, NoVReg));
  VIXL_CHECK(!AreConsecutive(b26, b28, NoVReg, b29));
  VIXL_CHECK(!AreConsecutive(s28, s30, NoVReg, NoVReg));

  VIXL_CHECK(AreConsecutive(q19, NoVReg, NoVReg, q22));
  VIXL_CHECK(AreConsecutive(v23, NoVReg, v25, NoVReg));
  VIXL_CHECK(AreConsecutive(b26, b27, NoVReg, NoVReg));
  VIXL_CHECK(AreConsecutive(h28, NoVReg, NoVReg, NoVReg));
  VIXL_CHECK(AreConsecutive(s30, s31, NoVReg, s2));
  VIXL_CHECK(AreConsecutive(d3, NoVReg, d6, d7));
}


TEST(sve_p_registers) {
  enum Qualification { kNone, kZeroing, kMerging, kWithLaneSize };
  class Helper {
   public:
    static Qualification GetQualification(PRegister) { return kNone; }
    static Qualification GetQualification(PRegisterZ) { return kZeroing; }
    static Qualification GetQualification(PRegisterM) { return kMerging; }
    static Qualification GetQualification(PRegisterWithLaneSize) {
      return kWithLaneSize;
    }
  };

  VIXL_CHECK(kNumberOfPRegisters == 16);
  VIXL_CHECK(p0.GetCode() == 0);
  VIXL_CHECK(p15.GetCode() == 15);
  VIXL_CHECK(p14.VnB().GetLaneSizeInBits() == kBRegSize);
  VIXL_CHECK(p14.VnH().GetLaneSizeInBits() == kHRegSize);
  VIXL_CHECK(p14.VnS().GetLaneSizeInBits() == kSRegSize);
  VIXL_CHECK(p14.VnD().GetLaneSizeInBits() == kDRegSize);
  VIXL_CHECK(p14.VnB().GetLaneSizeInBytes() == kBRegSizeInBytes);
  VIXL_CHECK(p14.VnH().GetLaneSizeInBytes() == kHRegSizeInBytes);
  VIXL_CHECK(p14.VnS().GetLaneSizeInBytes() == kSRegSizeInBytes);
  VIXL_CHECK(p14.VnD().GetLaneSizeInBytes() == kDRegSizeInBytes);
  VIXL_CHECK(Helper::GetQualification(p1) == kNone);
  VIXL_CHECK(Helper::GetQualification(p2.Zeroing()) == kZeroing);
  VIXL_CHECK(Helper::GetQualification(p3.Merging()) == kMerging);
  VIXL_CHECK(Helper::GetQualification(p4.VnB()) == kWithLaneSize);
  VIXL_CHECK(Helper::GetQualification(p5.VnH()) == kWithLaneSize);
  VIXL_CHECK(Helper::GetQualification(p6.VnS()) == kWithLaneSize);
  VIXL_CHECK(Helper::GetQualification(p7.VnD()) == kWithLaneSize);
}


TEST(sve_z_registers) {
  VIXL_CHECK(z0.GetCode() == 0);
  VIXL_CHECK(z31.GetCode() == 31);

  VIXL_CHECK(z0.Is(z0));
  VIXL_CHECK(!z0.Is(z1));
  VIXL_CHECK(!z0.Is(v0));
  VIXL_CHECK(!z0.Is(b0));
  VIXL_CHECK(!z0.Is(q0));

  VIXL_CHECK(AreAliased(z5, z5));
  VIXL_CHECK(AreAliased(z5, b5));
  VIXL_CHECK(AreAliased(b5, z5));
  VIXL_CHECK(AreAliased(z5, z5.B()));
  VIXL_CHECK(AreAliased(z5, z5.VnB()));

  VIXL_CHECK(!AreAliased(z6, z7));
  VIXL_CHECK(!AreAliased(b6, z7));
  VIXL_CHECK(!AreAliased(x7, z7));
}


TEST(sve_z_registers_vs_neon) {
  // There are three related register variants to consider in VIXL's API:
  //
  //    "b0": NEON: The least-significant byte of v0.
  //    "v0.B": NEON: v0, with an unspecified number of byte-sized lanes.
  //    "z0.B": SVE: z0, with an unspecified number of byte-sized lanes.
  //
  // The first two cases are indistinguishable in VIXL; both are obtained using
  // something like `v0.B()`. This is fine for NEON because there is no
  // ambiguity in practice; the "v0.B" form is always used with an index that
  // makes the meaning clear.

  VIXL_ASSERT(v6.B().Is(b6));
  VIXL_ASSERT(v7.H().Is(h7));
  VIXL_ASSERT(v8.S().Is(s8));
  VIXL_ASSERT(v9.D().Is(d9));

  VIXL_ASSERT(z6.B().Is(b6));
  VIXL_ASSERT(z7.H().Is(h7));
  VIXL_ASSERT(z8.S().Is(s8));
  VIXL_ASSERT(z9.D().Is(d9));

  // We cannot use the same approach for SVE's "z0.B" because, for example,
  // `Add(VRegister, ...)` and `Add(ZRegister, ...)` generate different
  // instructions.

  // Test that the variants can be distinguished with `Is`.
  VIXL_CHECK(!z6.VnB().Is(b6));
  VIXL_CHECK(!z7.VnH().Is(h7));
  VIXL_CHECK(!z8.VnS().Is(s8));
  VIXL_CHECK(!z9.VnD().Is(d9));

  VIXL_CHECK(!z6.VnB().Is(v6.B()));
  VIXL_CHECK(!z7.VnH().Is(v7.H()));
  VIXL_CHECK(!z8.VnS().Is(v8.S()));
  VIXL_CHECK(!z9.VnD().Is(v9.D()));

  VIXL_CHECK(!z6.VnB().Is(z6.B()));
  VIXL_CHECK(!z7.VnH().Is(z7.H()));
  VIXL_CHECK(!z8.VnS().Is(z8.S()));
  VIXL_CHECK(!z9.VnD().Is(z9.D()));

  // Test that the variants can be distinguished at compile-time using
  // overloading. VIXL's API relies on this.
  enum Variant { kNEON, kSVE, kUnknown };
  class Helper {
   public:
    static Variant GetVariant(ZRegister) { return kSVE; }
    static Variant GetVariant(VRegister) { return kNEON; }
    static Variant GetVariant(CPURegister) { return kUnknown; }
  };
  VIXL_CHECK(Helper::GetVariant(z10.VnB()) == kSVE);
  VIXL_CHECK(Helper::GetVariant(z11.VnH()) == kSVE);
  VIXL_CHECK(Helper::GetVariant(z12.VnS()) == kSVE);
  VIXL_CHECK(Helper::GetVariant(z13.VnD()) == kSVE);

  VIXL_CHECK(Helper::GetVariant(v10.B()) == kNEON);
  VIXL_CHECK(Helper::GetVariant(v11.H()) == kNEON);
  VIXL_CHECK(Helper::GetVariant(v12.S()) == kNEON);
  VIXL_CHECK(Helper::GetVariant(v13.D()) == kNEON);

  VIXL_CHECK(Helper::GetVariant(v10.V16B()) == kNEON);
  VIXL_CHECK(Helper::GetVariant(v11.V8H()) == kNEON);
  VIXL_CHECK(Helper::GetVariant(v12.V4S()) == kNEON);
  VIXL_CHECK(Helper::GetVariant(v13.V2D()) == kNEON);

  VIXL_CHECK(Helper::GetVariant(b10) == kNEON);
  VIXL_CHECK(Helper::GetVariant(h11) == kNEON);
  VIXL_CHECK(Helper::GetVariant(s12) == kNEON);
  VIXL_CHECK(Helper::GetVariant(d13) == kNEON);
}


TEST(move_immediate_helpers) {
  // Using these helpers to query information (without generating code) should
  // not crash.
  MacroAssembler::MoveImmediateHelper(NULL, x0, 0x12345678);
  MacroAssembler::OneInstrMoveImmediateHelper(NULL, x1, 0xabcdef);
}


TEST(generic_operand_helpers) {
  GenericOperand invalid_1;
  GenericOperand invalid_2;
  GenericOperand reg(x3);
  GenericOperand mem(MemOperand(sp, 8), kXRegSizeInBytes);

  VIXL_CHECK(!invalid_1.IsValid());
  VIXL_CHECK(!invalid_2.IsValid());

  VIXL_CHECK(invalid_1.Equals(invalid_1));
  VIXL_CHECK(invalid_2.Equals(invalid_2));
  VIXL_CHECK(reg.Equals(reg));
  VIXL_CHECK(mem.Equals(mem));

  VIXL_CHECK(invalid_1.Equals(invalid_2));
  VIXL_CHECK(invalid_2.Equals(invalid_1));

  VIXL_CHECK(!invalid_1.Equals(reg));
  VIXL_CHECK(!invalid_1.Equals(mem));
  VIXL_CHECK(!reg.Equals(invalid_1));
  VIXL_CHECK(!reg.Equals(invalid_2));
  VIXL_CHECK(!reg.Equals(mem));
  VIXL_CHECK(!mem.Equals(invalid_1));
  VIXL_CHECK(!mem.Equals(reg));
}


TEST(integer_operand_is) {
  VIXL_CHECK(IntegerOperand(0).IsZero());
  VIXL_CHECK(!IntegerOperand(1).IsZero());
  VIXL_CHECK(!IntegerOperand(-1).IsZero());

  VIXL_CHECK(!IntegerOperand(-0x81).IsIntN(8));
  VIXL_CHECK(IntegerOperand(-0x80).IsIntN(8));
  VIXL_CHECK(IntegerOperand(-1).IsIntN(8));
  VIXL_CHECK(IntegerOperand(0).IsIntN(8));
  VIXL_CHECK(IntegerOperand(1).IsIntN(8));
  VIXL_CHECK(IntegerOperand(0x7f).IsIntN(8));
  VIXL_CHECK(!IntegerOperand(0x80).IsIntN(8));

  VIXL_CHECK(!IntegerOperand(-1).IsUintN(8));
  VIXL_CHECK(IntegerOperand(0).IsUintN(8));
  VIXL_CHECK(IntegerOperand(1).IsUintN(8));
  VIXL_CHECK(IntegerOperand(0xff).IsUintN(8));
  VIXL_CHECK(!IntegerOperand(0x100).IsUintN(8));

  VIXL_CHECK(IntegerOperand(INT64_MIN).IsIntN(64));
  VIXL_CHECK(IntegerOperand(0).IsIntN(64));
  VIXL_CHECK(IntegerOperand(INT64_MAX).IsIntN(64));
  VIXL_CHECK(!IntegerOperand(0x8000000000000000).IsIntN(64));

  VIXL_CHECK(!IntegerOperand(-1).IsUintN(64));
  VIXL_CHECK(IntegerOperand(0).IsUintN(64));
  VIXL_CHECK(IntegerOperand(UINT64_MAX).IsUintN(64));

  VIXL_CHECK(!IntegerOperand(-0x801).FitsInBits(12));
  VIXL_CHECK(IntegerOperand(-0x800).FitsInBits(12));
  VIXL_CHECK(IntegerOperand(0).FitsInBits(12));
  VIXL_CHECK(IntegerOperand(0x7ff).FitsInBits(12));
  VIXL_CHECK(IntegerOperand(0x800).FitsInBits(12));
  VIXL_CHECK(IntegerOperand(0xfff).FitsInBits(12));
  VIXL_CHECK(!IntegerOperand(0x1000).FitsInBits(12));

  VIXL_CHECK(!IntegerOperand(-0x8001).FitsInLane(z0.VnH()));
  VIXL_CHECK(IntegerOperand(-0x8000).FitsInLane(z0.VnH()));
  VIXL_CHECK(IntegerOperand(0).FitsInLane(z0.VnH()));
  VIXL_CHECK(IntegerOperand(0x7fff).FitsInLane(z0.VnH()));
  VIXL_CHECK(IntegerOperand(0x8000).FitsInLane(z0.VnH()));
  VIXL_CHECK(IntegerOperand(0xffff).FitsInLane(z0.VnH()));
  VIXL_CHECK(!IntegerOperand(0x10000).FitsInLane(z0.VnH()));
}

TEST(integer_operand_as_uint) {
  // Simple cases.
  VIXL_CHECK(IntegerOperand(1).AsUintN(8) == 1);
  VIXL_CHECK(IntegerOperand(1).AsUintN(16) == 1);
  VIXL_CHECK(IntegerOperand(1).AsUintN(32) == 1);
  VIXL_CHECK(IntegerOperand(1).AsUintN(64) == 1);
  VIXL_CHECK(IntegerOperand(-1).AsUintN(8) == 0xff);
  VIXL_CHECK(IntegerOperand(-1).AsUintN(16) == 0xffff);
  VIXL_CHECK(IntegerOperand(-1).AsUintN(32) == 0xffffffff);
  VIXL_CHECK(IntegerOperand(-1).AsUintN(64) == 0xffffffffffffffff);
  VIXL_CHECK(IntegerOperand(0xf0).AsUintN(8) == 0xf0);
  VIXL_CHECK(IntegerOperand(0xf420).AsUintN(16) == 0xf420);
  VIXL_CHECK(IntegerOperand(0xf4242420).AsUintN(32) == 0xf4242420);
  VIXL_CHECK(IntegerOperand(0xf424242424242420).AsUintN(64) ==
             0xf424242424242420);

  // Boundary conditions for known-size types.
  VIXL_CHECK(IntegerOperand(INT8_MIN).AsUintN(8) == 0x80);
  VIXL_CHECK(IntegerOperand(INT8_MAX).AsUintN(8) == 0x7f);
  VIXL_CHECK(IntegerOperand(UINT8_MAX).AsUintN(8) == 0xff);

  VIXL_CHECK(IntegerOperand(INT16_MIN).AsUintN(16) == 0x8000);
  VIXL_CHECK(IntegerOperand(INT16_MAX).AsUintN(16) == 0x7fff);
  VIXL_CHECK(IntegerOperand(UINT16_MAX).AsUintN(16) == 0xffff);

  VIXL_CHECK(IntegerOperand(INT32_MIN).AsUintN(32) == 0x80000000);
  VIXL_CHECK(IntegerOperand(INT32_MAX).AsUintN(32) == 0x7fffffff);
  VIXL_CHECK(IntegerOperand(UINT32_MAX).AsUintN(32) == 0xffffffff);

  VIXL_CHECK(IntegerOperand(INT64_MIN).AsUintN(64) == 0x8000000000000000);
  VIXL_CHECK(IntegerOperand(INT64_MAX).AsUintN(64) == 0x7fffffffffffffff);
  VIXL_CHECK(IntegerOperand(UINT64_MAX).AsUintN(64) == 0xffffffffffffffff);
}

TEST(integer_operand_as_int) {
  // Simple cases.
  VIXL_CHECK(IntegerOperand(1).AsIntN(8) == 1);
  VIXL_CHECK(IntegerOperand(1).AsIntN(16) == 1);
  VIXL_CHECK(IntegerOperand(1).AsIntN(32) == 1);
  VIXL_CHECK(IntegerOperand(1).AsIntN(64) == 1);
  VIXL_CHECK(IntegerOperand(-1).AsIntN(8) == -1);
  VIXL_CHECK(IntegerOperand(-1).AsIntN(16) == -1);
  VIXL_CHECK(IntegerOperand(-1).AsIntN(32) == -1);
  VIXL_CHECK(IntegerOperand(-1).AsIntN(64) == -1);
  VIXL_CHECK(IntegerOperand(0x70).AsIntN(8) == 0x70);
  VIXL_CHECK(IntegerOperand(0x7420).AsIntN(16) == 0x7420);
  VIXL_CHECK(IntegerOperand(0x74242420).AsIntN(32) == 0x74242420);
  VIXL_CHECK(IntegerOperand(0x7424242424242420).AsIntN(64) ==
             0x7424242424242420);

  // Boundary conditions for known-size types.
  VIXL_CHECK(IntegerOperand(UINT8_MAX).AsIntN(8) == -1);
  VIXL_CHECK(IntegerOperand(UINT16_MAX).AsIntN(16) == -1);
  VIXL_CHECK(IntegerOperand(UINT32_MAX).AsIntN(32) == -1);
  VIXL_CHECK(IntegerOperand(UINT64_MAX).AsIntN(64) == -1);

  VIXL_CHECK(IntegerOperand(INT8_MAX).AsIntN(8) == INT8_MAX);
  VIXL_CHECK(IntegerOperand(INT16_MAX).AsIntN(16) == INT16_MAX);
  VIXL_CHECK(IntegerOperand(INT32_MAX).AsIntN(32) == INT32_MAX);
  VIXL_CHECK(IntegerOperand(INT64_MAX).AsIntN(64) == INT64_MAX);

  VIXL_CHECK(IntegerOperand(0x80).AsIntN(8) == INT8_MIN);
  VIXL_CHECK(IntegerOperand(0x8000).AsIntN(16) == INT16_MIN);
  VIXL_CHECK(IntegerOperand(0x80000000).AsIntN(32) == INT32_MIN);
  VIXL_CHECK(IntegerOperand(0x8000000000000000).AsIntN(64) == INT64_MIN);
}

template <unsigned N>
class IntegerOperandTryEncodeShiftedIntHelper {
 public:
  IntegerOperandTryEncodeShiftedIntHelper() {}

  template <unsigned kShift, typename T>
  void TestEncodable(T value, const ZRegister& zd, int64_t expected_imm) {
    VIXL_CHECK(TestImpl<kShift>(value, zd, expected_imm));
  }

  template <unsigned kShift, typename T>
  void TestUnencodable(T value, const ZRegister& zd) {
    // The `expected_imm` value is ignored, so its value is arbitrary.
    VIXL_CHECK(!TestImpl<kShift>(value, zd, 0));
  }

 private:
  template <unsigned kShift, typename T>
  bool TestImpl(T value, const ZRegister& zd, int64_t expected_imm) {
    IntegerOperand operand(value);
    int64_t imm = 0xdeadbeef42;
    unsigned shift = 0xbeef43;
    bool success =
        operand.TryEncodeAsShiftedIntNForLane<N, kShift>(zd, &imm, &shift);
    if (success) {
      VIXL_CHECK(imm == expected_imm);
      VIXL_CHECK(shift == kShift);
    } else {
      // Check that the outputs were unmodified.
      VIXL_CHECK(imm == 0xdeadbeef42);
      VIXL_CHECK(shift == 0xbeef43);
    }

    // If kShift is 0, also check TryEncodeAsIntNForLane.
    if (kShift == 0) {
      int64_t unshifted_imm = 0xdeadbeef99;
      bool unshifted_success =
          operand.TryEncodeAsIntNForLane<N>(zd, &unshifted_imm);

      VIXL_CHECK(unshifted_success == success);
      if (unshifted_success) {
        VIXL_CHECK(unshifted_imm == expected_imm);
      } else {
        VIXL_CHECK(unshifted_imm == 0xdeadbeef99);
      }
    }

    return success;
  }
};

TEST(integer_operand_encode_as_intn) {
  IntegerOperandTryEncodeShiftedIntHelper<4> int4_helper;
  IntegerOperandTryEncodeShiftedIntHelper<8> int8_helper;
  IntegerOperandTryEncodeShiftedIntHelper<12> int12_helper;

  // Simple cases, where the value is directly encodable.
  int4_helper.TestEncodable<0>(-8, z0.VnH(), -8);
  int4_helper.TestEncodable<0>(-7, z0.VnH(), -7);
  int4_helper.TestEncodable<0>(-1, z0.VnS(), -1);
  int4_helper.TestEncodable<0>(0, z0.VnD(), 0);
  int4_helper.TestEncodable<0>(1, z0.VnB(), 1);
  int4_helper.TestEncodable<0>(7, z0.VnH(), 7);

  int8_helper.TestEncodable<0>(0x7f, z0.VnB(), 0x7f);
  int8_helper.TestEncodable<0>(0x7f, z0.VnH(), 0x7f);
  int12_helper.TestEncodable<0>(0x7ff, z0.VnH(), 0x7ff);

  int8_helper.TestEncodable<0>(-0x80, z0.VnB(), -0x80);
  int8_helper.TestEncodable<0>(-0x80, z0.VnH(), -0x80);
  int12_helper.TestEncodable<0>(-0x800, z0.VnH(), -0x800);

  // Cases that are directly encodable with a shift.
  int8_helper.TestEncodable<4>(-0x800, z0.VnH(), -0x80);
  int8_helper.TestEncodable<4>(-0x7f0, z0.VnH(), -0x7f);
  int8_helper.TestEncodable<4>(-0x010, z0.VnH(), -1);
  int8_helper.TestEncodable<4>(0x000, z0.VnH(), 0);
  int8_helper.TestEncodable<4>(0x010, z0.VnH(), 1);
  int8_helper.TestEncodable<4>(0x7f0, z0.VnH(), 0x7f);

  // Ensure that (positive) bit representations of negative values are treated
  // as negative values, even though their arithmetic values are unencodable.
  int12_helper.TestEncodable<0>(0xffd6, z0.VnH(), -42);
  int12_helper.TestEncodable<0>(0xffffffd6, z0.VnS(), -42);
  int12_helper.TestEncodable<4>(0xfd60, z0.VnH(), -42);
  int12_helper.TestEncodable<8>(0xffffd600, z0.VnS(), -42);

  int8_helper.TestEncodable<0>(UINT8_MAX, z0.VnB(), -1);
  int8_helper.TestEncodable<0>(UINT16_MAX, z0.VnH(), -1);
  int8_helper.TestEncodable<0>(UINT32_MAX, z0.VnS(), -1);
  int8_helper.TestEncodable<0>(UINT64_MAX, z0.VnD(), -1);

  int4_helper.TestEncodable<1>(UINT8_MAX ^ 0x1, z0.VnB(), -1);
  int4_helper.TestEncodable<2>(UINT16_MAX ^ 0x3, z0.VnH(), -1);
  int4_helper.TestEncodable<3>(UINT32_MAX ^ 0x7, z0.VnS(), -1);
  int4_helper.TestEncodable<4>(UINT64_MAX ^ 0xf, z0.VnD(), -1);

  // Unencodable cases.
  int8_helper.TestUnencodable<0>(INT16_MAX, z0.VnH());
  int8_helper.TestUnencodable<0>(INT32_MAX, z0.VnS());
  int8_helper.TestUnencodable<0>(INT64_MAX, z0.VnD());

  int4_helper.TestUnencodable<0>(0x10, z0.VnB());
  int4_helper.TestUnencodable<1>(0x20, z0.VnB());

  int12_helper.TestUnencodable<1>(1, z0.VnD());
  int12_helper.TestUnencodable<12>(1, z0.VnD());
  int12_helper.TestUnencodable<12>(0x800, z0.VnD());
}

TEST(static_register_types) {
  // [WX]Register implicitly casts to Register.
  XRegister x_x0(0);
  WRegister w_w0(0);
  Register r_x0 = x_x0;
  Register r_w0 = w_w0;
  VIXL_CHECK(r_x0.Is(x_x0));
  VIXL_CHECK(x_x0.Is(r_x0));
  VIXL_CHECK(r_w0.Is(w_w0));
  VIXL_CHECK(w_w0.Is(r_w0));

  // Register explicitly casts to [WX]Register.
  Register r_x1(1, kXRegSize);
  Register r_w1(1, kWRegSize);
  XRegister x_x1(r_x1);
  WRegister w_w1(r_w1);
  VIXL_CHECK(r_x1.Is(x_x1));
  VIXL_CHECK(x_x1.Is(r_x1));
  VIXL_CHECK(r_w1.Is(w_w1));
  VIXL_CHECK(w_w1.Is(r_w1));

  // [WX]Register implicitly casts to CPURegister.
  XRegister x_x2(2);
  WRegister w_w2(2);
  CPURegister cpu_x2 = x_x2;
  CPURegister cpu_w2 = w_w2;
  VIXL_CHECK(cpu_x2.Is(x_x2));
  VIXL_CHECK(x_x2.Is(cpu_x2));
  VIXL_CHECK(cpu_w2.Is(w_w2));
  VIXL_CHECK(w_w2.Is(cpu_w2));
}


TEST(operand_is_plain_register) {
  VIXL_CHECK(Operand(x0).IsPlainRegister());
  VIXL_CHECK(Operand(x1, LSL, 0).IsPlainRegister());
  VIXL_CHECK(Operand(x2, LSR, 0).IsPlainRegister());
  VIXL_CHECK(Operand(x3, ASR, 0).IsPlainRegister());
  VIXL_CHECK(Operand(x4, ROR, 0).IsPlainRegister());
  VIXL_CHECK(Operand(x5, UXTX).IsPlainRegister());
  VIXL_CHECK(Operand(x6, SXTX).IsPlainRegister());
  VIXL_CHECK(Operand(w7).IsPlainRegister());
  VIXL_CHECK(Operand(w8, LSL, 0).IsPlainRegister());
  VIXL_CHECK(Operand(w9, LSR, 0).IsPlainRegister());
  VIXL_CHECK(Operand(w10, ASR, 0).IsPlainRegister());
  VIXL_CHECK(Operand(w11, ROR, 0).IsPlainRegister());

  VIXL_CHECK(!Operand(x0, LSL, 1).IsPlainRegister());
  VIXL_CHECK(!Operand(x1, LSR, 2).IsPlainRegister());
  VIXL_CHECK(!Operand(x2, ASR, 3).IsPlainRegister());
  VIXL_CHECK(!Operand(x3, ROR, 4).IsPlainRegister());
  VIXL_CHECK(!Operand(x5, UXTX, 1).IsPlainRegister());
  VIXL_CHECK(!Operand(x6, SXTX, 2).IsPlainRegister());
  VIXL_CHECK(!Operand(w7, LSL, 1).IsPlainRegister());
  VIXL_CHECK(!Operand(w8, LSR, 2).IsPlainRegister());
  VIXL_CHECK(!Operand(w9, ASR, 3).IsPlainRegister());
  VIXL_CHECK(!Operand(w10, ROR, 4).IsPlainRegister());
  VIXL_CHECK(!Operand(w11, UXTB).IsPlainRegister());
  VIXL_CHECK(!Operand(w12, SXTB).IsPlainRegister());
  VIXL_CHECK(!Operand(w13, UXTH).IsPlainRegister());
  VIXL_CHECK(!Operand(w14, SXTH).IsPlainRegister());
  // UXTW and SXTW could be treated as plain registers in 32-bit contexts, but
  // the Operand class doesn't know the context so it has to return false.
  VIXL_CHECK(!Operand(w15, UXTW).IsPlainRegister());
  VIXL_CHECK(!Operand(w16, SXTW).IsPlainRegister());
}


TEST(memoperand_is_plain_register) {
  VIXL_CHECK(MemOperand(x0).IsPlainRegister());
  VIXL_CHECK(MemOperand(sp).IsPlainRegister());
  VIXL_CHECK(MemOperand(x1, 0).IsPlainRegister());

  VIXL_CHECK(!MemOperand(x2, xzr).IsPlainRegister());
  VIXL_CHECK(!MemOperand(x3, xzr, SXTX).IsPlainRegister());
  VIXL_CHECK(!MemOperand(x4, xzr, SXTX, 2).IsPlainRegister());
  VIXL_CHECK(!MemOperand(x5, wzr, UXTW).IsPlainRegister());
  VIXL_CHECK(!MemOperand(x6, wzr, UXTW, 3).IsPlainRegister());

  VIXL_CHECK(!MemOperand(x7, 0, PostIndex).IsPlainRegister());
  VIXL_CHECK(!MemOperand(x8, 0, PreIndex).IsPlainRegister());
  VIXL_CHECK(!MemOperand(x9, xzr, PostIndex).IsPlainRegister());

  VIXL_CHECK(!MemOperand(x20, 1).IsPlainRegister());
  VIXL_CHECK(!MemOperand(x21, x30).IsPlainRegister());
}

TEST(memoperand_is_plain_register_or_equivalent) {
  VIXL_CHECK(MemOperand(x0).IsEquivalentToPlainRegister());
  VIXL_CHECK(MemOperand(sp).IsEquivalentToPlainRegister());
  VIXL_CHECK(MemOperand(x1, 0).IsEquivalentToPlainRegister());

  VIXL_CHECK(MemOperand(x2, xzr).IsEquivalentToPlainRegister());
  VIXL_CHECK(MemOperand(x3, xzr, SXTX).IsEquivalentToPlainRegister());
  VIXL_CHECK(MemOperand(x4, xzr, SXTX, 2).IsEquivalentToPlainRegister());
  VIXL_CHECK(MemOperand(x5, wzr, UXTW).IsEquivalentToPlainRegister());
  VIXL_CHECK(MemOperand(x6, wzr, UXTW, 3).IsEquivalentToPlainRegister());

  VIXL_CHECK(MemOperand(x7, 0, PostIndex).IsEquivalentToPlainRegister());
  VIXL_CHECK(MemOperand(x8, 0, PreIndex).IsEquivalentToPlainRegister());
  VIXL_CHECK(MemOperand(x9, xzr, PostIndex).IsEquivalentToPlainRegister());

  VIXL_CHECK(!MemOperand(x20, 1).IsEquivalentToPlainRegister());
  VIXL_CHECK(!MemOperand(x21, x30).IsEquivalentToPlainRegister());
}

TEST(sve_memoperand_is_plain_scalar) {
  VIXL_CHECK(SVEMemOperand(x0).IsPlainScalar());
  VIXL_CHECK(SVEMemOperand(sp).IsPlainScalar());
  VIXL_CHECK(SVEMemOperand(x1, 0).IsPlainScalar());

  VIXL_CHECK(!SVEMemOperand(x2, xzr).IsPlainScalar());
  VIXL_CHECK(!SVEMemOperand(x4, xzr, LSL, 2).IsPlainScalar());

  VIXL_CHECK(!SVEMemOperand(x20, 1).IsPlainScalar());
  VIXL_CHECK(!SVEMemOperand(x21, x30).IsPlainScalar());

  VIXL_CHECK(!SVEMemOperand(x0, z1.VnD()).IsPlainScalar());
  VIXL_CHECK(!SVEMemOperand(x2, z3.VnS(), UXTW).IsPlainScalar());
  VIXL_CHECK(!SVEMemOperand(z4.VnD(), 0).IsPlainScalar());
}

TEST(sve_memoperand_is_scalar_or_equivalent) {
  VIXL_CHECK(SVEMemOperand(x0).IsEquivalentToScalar());
  VIXL_CHECK(SVEMemOperand(sp).IsEquivalentToScalar());
  VIXL_CHECK(SVEMemOperand(x1, 0).IsEquivalentToScalar());

  VIXL_CHECK(SVEMemOperand(x2, xzr).IsEquivalentToScalar());
  VIXL_CHECK(SVEMemOperand(x4, xzr, LSL, 2).IsEquivalentToScalar());

  VIXL_CHECK(!SVEMemOperand(x20, 1).IsEquivalentToScalar());
  VIXL_CHECK(!SVEMemOperand(x21, x30).IsEquivalentToScalar());

  VIXL_CHECK(!SVEMemOperand(x0, z1.VnD()).IsEquivalentToScalar());
  VIXL_CHECK(!SVEMemOperand(x2, z3.VnD(), SXTW).IsEquivalentToScalar());
  VIXL_CHECK(!SVEMemOperand(z4.VnD(), 0).IsEquivalentToScalar());
}

TEST(sve_memoperand_types) {
  VIXL_CHECK(SVEMemOperand(x0, 42).IsScalarPlusImmediate());
  VIXL_CHECK(SVEMemOperand(x1, 42, SVE_MUL_VL).IsScalarPlusImmediate());
  VIXL_CHECK(SVEMemOperand(x2, -42, SVE_MUL_VL).IsScalarPlusImmediate());

  VIXL_CHECK(SVEMemOperand(sp, x3).IsScalarPlusScalar());
  VIXL_CHECK(SVEMemOperand(x4, xzr).IsScalarPlusScalar());
  VIXL_CHECK(SVEMemOperand(x5, x6, LSL, 1).IsScalarPlusScalar());

  VIXL_CHECK(SVEMemOperand(x7, z0.VnD()).IsScalarPlusVector());
  VIXL_CHECK(SVEMemOperand(x8, z1.VnS(), SXTW).IsScalarPlusVector());
  VIXL_CHECK(SVEMemOperand(x9, z2.VnD(), UXTW).IsScalarPlusVector());
  VIXL_CHECK(SVEMemOperand(x10, z3.VnD(), LSL, 2).IsScalarPlusVector());

  VIXL_CHECK(SVEMemOperand(z4.VnD(), 42).IsVectorPlusImmediate());
  VIXL_CHECK(SVEMemOperand(z5.VnS(), -42).IsVectorPlusImmediate());
}

TEST(sve_memoperand_scatter_gather) {
  // Single-address accesses.
  VIXL_CHECK(!SVEMemOperand(x0, 42).IsScatterGather());
  VIXL_CHECK(!SVEMemOperand(x1, 42, SVE_MUL_VL).IsScatterGather());
  VIXL_CHECK(!SVEMemOperand(x2, -42, SVE_MUL_VL).IsScatterGather());

  VIXL_CHECK(!SVEMemOperand(sp, x3).IsScatterGather());
  VIXL_CHECK(!SVEMemOperand(x4, xzr).IsScatterGather());
  VIXL_CHECK(!SVEMemOperand(x5, x6, LSL, 1).IsScatterGather());

  // Scatter-gather accesses.
  VIXL_CHECK(SVEMemOperand(x7, z0.VnD()).IsScatterGather());
  VIXL_CHECK(SVEMemOperand(x8, z1.VnS(), SXTW).IsScatterGather());
  VIXL_CHECK(SVEMemOperand(x9, z2.VnD(), UXTW).IsScatterGather());
  VIXL_CHECK(SVEMemOperand(x10, z3.VnD(), LSL, 2).IsScatterGather());

  VIXL_CHECK(SVEMemOperand(z4.VnD(), 42).IsScatterGather());
  VIXL_CHECK(SVEMemOperand(z5.VnS(), -42).IsScatterGather());
}

TEST(scratch_scope_basic) {
  MacroAssembler masm;
  // x16 and x17 are available as scratch registers by default.
  {
    UseScratchRegisterScope temps(&masm);
    Register temp1 = temps.AcquireW();
    Register temp2 = temps.AcquireX();
    VIXL_CHECK(temp1.Is(w16));
    VIXL_CHECK(temp2.Is(x17));
  }
  {
    UseScratchRegisterScope temps(&masm);
    Register temp1 = temps.AcquireRegisterOfSize(kXRegSize);
    Register temp2 = temps.AcquireRegisterOfSize(kWRegSize);
    VIXL_CHECK(temp1.Is(x16));
    VIXL_CHECK(temp2.Is(w17));
  }
}

TEST(scratch_scope_basic_v) {
  MacroAssembler masm;
  // v31 is the only V scratch register available by default.
  {
    UseScratchRegisterScope temps(&masm);
    VRegister temp = temps.AcquireH();
    VIXL_CHECK(temp.Is(h31));
  }
  {
    UseScratchRegisterScope temps(&masm);
    VRegister temp = temps.AcquireS();
    VIXL_CHECK(temp.Is(s31));
  }
  {
    UseScratchRegisterScope temps(&masm);
    VRegister temp = temps.AcquireD();
    VIXL_CHECK(temp.Is(d31));
  }
  {
    UseScratchRegisterScope temps(&masm);
    VRegister temp = temps.AcquireVRegisterOfSize(kQRegSize);
    VIXL_CHECK(temp.Is(q31));
  }
  {
    UseScratchRegisterScope temps(&masm);
    VRegister temp = temps.AcquireVRegisterOfSize(kDRegSize);
    VIXL_CHECK(temp.Is(d31));
  }
  {
    UseScratchRegisterScope temps(&masm);
    VRegister temp = temps.AcquireVRegisterOfSize(kSRegSize);
    VIXL_CHECK(temp.Is(s31));
  }
}

TEST(scratch_scope_basic_z) {
  MacroAssembler masm;
  // z31 is the only Z scratch register available by default.
  {
    UseScratchRegisterScope temps(&masm);
    VIXL_CHECK(temps.IsAvailable(v31));
    VIXL_CHECK(temps.IsAvailable(z31));
    ZRegister temp = temps.AcquireZ();
    VIXL_CHECK(temp.Is(z31));
    // Check that allocating a Z register properly reserves the corresponding V
    // register.
    VIXL_CHECK(!temps.IsAvailable(v31));
    VIXL_CHECK(!temps.IsAvailable(z31));
  }
  // Check that the destructor restored the acquired register.
  UseScratchRegisterScope temps(&masm);
  VIXL_CHECK(temps.IsAvailable(v31));
  VIXL_CHECK(temps.IsAvailable(z31));
}

TEST(scratch_scope_basic_p) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    // There are no P scratch registers available by default.
    VIXL_CHECK(masm.GetScratchPRegisterList()->IsEmpty());
    temps.Include(p0, p1);
    VIXL_CHECK(temps.IsAvailable(p0));
    VIXL_CHECK(temps.IsAvailable(p1));
    temps.Include(p7, p8, p15);
    VIXL_CHECK(temps.IsAvailable(p7));
    VIXL_CHECK(temps.IsAvailable(p8));
    VIXL_CHECK(temps.IsAvailable(p15));

    // AcquireGoverningP() can only return p0-p7.
    VIXL_CHECK(temps.AcquireGoverningP().GetCode() <
               kNumberOfGoverningPRegisters);
    VIXL_CHECK(temps.AcquireGoverningP().GetCode() <
               kNumberOfGoverningPRegisters);
    VIXL_CHECK(temps.IsAvailable(p8));
    VIXL_CHECK(temps.IsAvailable(p15));

    // AcquireP() prefers p8-p15, ...
    VIXL_CHECK(temps.AcquireP().GetCode() >= kNumberOfGoverningPRegisters);
    VIXL_CHECK(temps.AcquireP().GetCode() >= kNumberOfGoverningPRegisters);
    // ... but will return p0-p7 if none of p8-p15 are available.
    VIXL_CHECK(temps.AcquireP().GetCode() < kNumberOfGoverningPRegisters);

    VIXL_CHECK(masm.GetScratchPRegisterList()->IsEmpty());

    // Leave some registers available so we can test the destructor.
    temps.Include(p3, p6, p9, p12);
    VIXL_CHECK(!masm.GetScratchPRegisterList()->IsEmpty());
  }
  // Check that the destructor correctly cleared the list.
  VIXL_CHECK(masm.GetScratchPRegisterList()->IsEmpty());
}

TEST(scratch_scope_include_ignored) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    // Start with an empty set of scratch registers.
    temps.ExcludeAll();

    // Including NoReg has no effect.
    temps.Include(NoReg);
    temps.Include(NoCPUReg);
    temps.Include(CPURegList(CPURegister::kNoRegister, 0, 0));

    // Including sp or zr has no effect, since they are never appropriate
    // scratch registers.
    temps.Include(sp);
    temps.Include(xzr, wsp);
    temps.Include(wzr);
    temps.Include(CPURegList(xzr, sp));

    VIXL_CHECK(masm.GetScratchRegisterList()->IsEmpty());
    VIXL_CHECK(masm.GetScratchVRegisterList()->IsEmpty());
  }
}

class ScratchScopeHelper {
 public:
  enum Action { kRelease, kInclude, kExclude };

  ScratchScopeHelper(MacroAssembler* masm,
                     Action action,
                     CPURegister::RegisterType type)
      : masm_(masm),
        action_(action),
        type_(type),
        expected_(GetGuardListFor(CPURegister::kRegister)),
        expected_v_(GetGuardListFor(CPURegister::kVRegister)),
        expected_p_(GetGuardListFor(CPURegister::kPRegister)) {
    *GetExpectedFor(type) = GetInitialList();
    masm->GetScratchRegisterList()->SetList(expected_);
    masm->GetScratchVRegisterList()->SetList(expected_v_);
    masm->GetScratchPRegisterList()->SetList(expected_p_);
  }

  // Notify the helper that the registers in `update` have been passed into
  // DoAction(), and assert that the MacroAssembler's scratch lists are as
  // expected.
  void RecordActionsAndCheck(RegList update) {
    RegList* expected = GetExpectedFor(type_);
    switch (action_) {
      case kRelease:
        // It isn't valid to release a register that is already available.
        VIXL_CHECK((*expected & update) == 0);
        VIXL_FALLTHROUGH();
      case kInclude:
        *expected |= update;
        break;
      case kExclude:
        *expected &= ~update;
        break;
    }
    VIXL_CHECK(masm_->GetScratchRegisterList()->GetList() == expected_);
    VIXL_CHECK(masm_->GetScratchVRegisterList()->GetList() == expected_v_);
    VIXL_CHECK(masm_->GetScratchPRegisterList()->GetList() == expected_p_);
  }

 private:
  RegList GetInitialList() {
    switch (action_) {
      case kRelease:
      case kInclude:
        return 0;
      case kExclude:
        return GetPotentialListFor(type_);
    }
    VIXL_UNREACHABLE();
    return 0;
  }

  // Return some valid, non-zero RegList suitable for use as a guard value.
  static RegList GetGuardListFor(CPURegister::RegisterType type) {
    return (0x1111111111111111 * (type + 1)) & GetPotentialListFor(type);
  }

  static RegList GetPotentialListFor(CPURegister::RegisterType type) {
    RegList list = CPURegList::All(type).GetList();
    // The zr and sp registers cannot be scratch registers.
    if (type == CPURegister::kRegister) list &= ~(xzr.GetBit() | sp.GetBit());
    return list;
  }

  RegList* GetExpectedFor(CPURegister::RegisterType type) {
    switch (type) {
      case CPURegister::kNoRegister:
        VIXL_UNREACHABLE();
        return NULL;
      case CPURegister::kRegister:
        return &expected_;
      case CPURegister::kVRegister:
      case CPURegister::kZRegister:
        return &expected_v_;
      case CPURegister::kPRegister:
        return &expected_p_;
    }
    VIXL_UNREACHABLE();
    return NULL;
  }

  MacroAssembler* masm_;
  Action action_;
  CPURegister::RegisterType type_;

  RegList expected_;
  RegList expected_v_;
  RegList expected_p_;
};

TEST(scratch_scope_include) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kInclude,
                              CPURegister::kRegister);

    // Any suitable register type deriving from CPURegister can be included.
    temps.Include(w0);
    temps.Include(x1);
    temps.Include(WRegister(2));
    temps.Include(XRegister(3));
    temps.Include(Register(w4));
    temps.Include(Register(x5));
    temps.Include(CPURegister(w6));
    temps.Include(CPURegister(x7));
    helper.RecordActionsAndCheck(0xff);
    // Multiple registers can be included at once.
    temps.Include(x8, w9, x10);
    temps.Include(Register(w12), Register(x13), Register(w14));
    temps.Include(XRegister(16), XRegister(17), XRegister(18));
    temps.Include(WRegister(20), WRegister(21), WRegister(22));
    temps.Include(CPURegList(w24, w25, w26));
    helper.RecordActionsAndCheck(0x7777700);
    // Including a register again has no effect.
    temps.Include(Register(w26));
    temps.Include(Register(x25));
    temps.Include(CPURegister(x24));
    temps.Include(CPURegister(x22));
    temps.Include(x21, x20, w18, x17);
    temps.Include(CPURegList(x16, x14, x13, x12));
    helper.RecordActionsAndCheck(0x7777700);
  }
}

TEST(scratch_scope_exclude) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kExclude,
                              CPURegister::kRegister);

    // Any suitable register type deriving from CPURegister can be excluded.
    temps.Exclude(w0);
    temps.Exclude(x1);
    temps.Exclude(WRegister(2));
    temps.Exclude(XRegister(3));
    temps.Exclude(Register(w4));
    temps.Exclude(Register(x5));
    temps.Exclude(CPURegister(w6));
    temps.Exclude(CPURegister(x7));
    helper.RecordActionsAndCheck(0xff);
    // Multiple registers can be excluded at once.
    temps.Exclude(x8, w9, x10);
    temps.Exclude(Register(w12), Register(x13), Register(w14));
    temps.Exclude(XRegister(16), XRegister(17), XRegister(18));
    temps.Exclude(WRegister(20), WRegister(21), WRegister(22));
    temps.Exclude(CPURegList(w24, w25, w26));
    helper.RecordActionsAndCheck(0x7777700);
    // Excluding a register again has no effect.
    temps.Exclude(Register(w26));
    temps.Exclude(Register(x25));
    temps.Exclude(CPURegister(x24));
    temps.Exclude(CPURegister(x22));
    temps.Exclude(x21, x20, w18, x17);
    temps.Exclude(CPURegList(x16, x14, x13, x12));
    helper.RecordActionsAndCheck(0x7777700);
  }
}

TEST(scratch_scope_release) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kRelease,
                              CPURegister::kRegister);

    // Any suitable register type deriving from CPURegister can be released.
    temps.Release(w0);
    temps.Release(x1);
    temps.Release(WRegister(2));
    temps.Release(XRegister(3));
    temps.Release(Register(w4));
    temps.Release(Register(x5));
    temps.Release(CPURegister(w6));
    temps.Release(CPURegister(x7));
    helper.RecordActionsAndCheck(0xff);
    // It is not possible to release more than one register at a time, and it is
    // invalid to release a register that is already available.
  }
}

TEST(scratch_scope_include_v) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kInclude,
                              CPURegister::kVRegister);

    // Any suitable register type deriving from CPURegister can be included.
    temps.Include(b0);
    temps.Include(h1);
    temps.Include(SRegister(2));
    temps.Include(DRegister(3));
    temps.Include(VRegister(q4));
    temps.Include(VRegister(v5.V8B()));
    temps.Include(CPURegister(d6));
    temps.Include(CPURegister(v7.S4B()));
    helper.RecordActionsAndCheck(0xff);
    // Multiple registers can be included at once.
    temps.Include(b8, h9, s10);
    temps.Include(VRegister(d12), VRegister(d13), VRegister(d14));
    temps.Include(QRegister(16), QRegister(17), QRegister(18));
    temps.Include(BRegister(20), BRegister(21), BRegister(22));
    temps.Include(CPURegList(s24, s25, s26));
    helper.RecordActionsAndCheck(0x7777700);
    // Including a register again has no effect.
    temps.Include(VRegister(b26));
    temps.Include(VRegister(h25));
    temps.Include(CPURegister(s24));
    temps.Include(CPURegister(v22.V4H()));
    temps.Include(q21, d20, s18, h17);
    temps.Include(CPURegList(h16, h14, h13, h12));
    helper.RecordActionsAndCheck(0x7777700);
  }
}

TEST(scratch_scope_exclude_v) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kExclude,
                              CPURegister::kVRegister);

    // Any suitable register type deriving from CPURegister can be excluded.
    temps.Exclude(b0);
    temps.Exclude(h1);
    temps.Exclude(SRegister(2));
    temps.Exclude(DRegister(3));
    temps.Exclude(VRegister(q4));
    temps.Exclude(VRegister(v5.V8B()));
    temps.Exclude(CPURegister(d6));
    temps.Exclude(CPURegister(v7.S4B()));
    helper.RecordActionsAndCheck(0xff);
    // Multiple registers can be excluded at once.
    temps.Exclude(b8, h9, s10);
    temps.Exclude(VRegister(d12), VRegister(d13), VRegister(d14));
    temps.Exclude(QRegister(16), QRegister(17), QRegister(18));
    temps.Exclude(BRegister(20), BRegister(21), BRegister(22));
    temps.Exclude(CPURegList(s24, s25, s26));
    helper.RecordActionsAndCheck(0x7777700);
    // Excluding a register again has no effect.
    temps.Exclude(VRegister(b26));
    temps.Exclude(VRegister(h25));
    temps.Exclude(CPURegister(s24));
    temps.Exclude(CPURegister(v22.V4H()));
    temps.Exclude(q21, d20, s18, h17);
    temps.Exclude(CPURegList(h16, h14, h13, h12));
    helper.RecordActionsAndCheck(0x7777700);
  }
}

TEST(scratch_scope_release_v) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kRelease,
                              CPURegister::kVRegister);

    // Any suitable register type deriving from CPURegister can be released.
    temps.Release(b0);
    temps.Release(h1);
    temps.Release(SRegister(2));
    temps.Release(DRegister(3));
    temps.Release(VRegister(q4));
    temps.Release(VRegister(v5.V8B()));
    temps.Release(CPURegister(d6));
    temps.Release(CPURegister(v7.S4B()));
    helper.RecordActionsAndCheck(0xff);
    // It is not possible to release more than one register at a time, and it is
    // invalid to release a register that is already available.
  }
}

TEST(scratch_scope_include_z) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kInclude,
                              CPURegister::kZRegister);

    // Any suitable register type deriving from CPURegister can be included.
    temps.Include(z0);
    temps.Include(z1.VnB());
    temps.Include(ZRegister(2));
    temps.Include(ZRegister(3, kFormatVnD));
    temps.Include(CPURegister(z4));
    temps.Include(CPURegister(z5.VnH()));
    helper.RecordActionsAndCheck(0x3f);
    // Multiple registers can be included at once.
    temps.Include(z8, z9, z10.VnS());
    temps.Include(ZRegister(12), ZRegister(13, kHRegSize), z14);
    temps.Include(CPURegList(z16, z17, z18));
    helper.RecordActionsAndCheck(0x77700);
    // Including a register again has no effect.
    temps.Include(ZRegister(18));
    temps.Include(ZRegister(17, kFormatVnB));
    temps.Include(CPURegister(z16));
    temps.Include(CPURegister(z13.VnD()));
    temps.Include(z12, z10, z9.VnB(), z8);
    temps.Include(CPURegList(z5, z4, z3, z2));
    helper.RecordActionsAndCheck(0x77700);
  }
}

TEST(scratch_scope_exclude_z) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kExclude,
                              CPURegister::kZRegister);

    // Any suitable register type deriving from CPURegister can be excluded.
    temps.Exclude(z0);
    temps.Exclude(z1.VnB());
    temps.Exclude(ZRegister(2));
    temps.Exclude(ZRegister(3, kFormatVnD));
    temps.Exclude(CPURegister(z4));
    temps.Exclude(CPURegister(z5.VnH()));
    helper.RecordActionsAndCheck(0x3f);
    // Multiple registers can be excluded at once.
    temps.Exclude(z8, z9, z10.VnS());
    temps.Exclude(ZRegister(12), ZRegister(13, kHRegSize), z14);
    temps.Exclude(CPURegList(z16, z17, z18));
    helper.RecordActionsAndCheck(0x77700);
    // Exluding a register again has no effect.
    temps.Exclude(ZRegister(18));
    temps.Exclude(ZRegister(17, kFormatVnB));
    temps.Exclude(CPURegister(z16));
    temps.Exclude(CPURegister(z13.VnD()));
    temps.Exclude(z12, z10, z9.VnB(), z8);
    temps.Exclude(CPURegList(z5, z4, z3, z2));
    helper.RecordActionsAndCheck(0x77700);
  }
}

TEST(scratch_scope_release_z) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kRelease,
                              CPURegister::kZRegister);

    // Any suitable register type deriving from CPURegister can be released.
    temps.Release(z0);
    temps.Release(z1.VnB());
    temps.Release(ZRegister(2));
    temps.Release(ZRegister(3, kFormatVnD));
    temps.Release(CPURegister(z4));
    temps.Release(CPURegister(z5.VnH()));
    helper.RecordActionsAndCheck(0x3f);
    // It is not possible to release more than one register at a time, and it is
    // invalid to release a register that is already available.
  }
}

TEST(scratch_scope_include_p) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kInclude,
                              CPURegister::kPRegister);

    // Any suitable register type deriving from CPURegister can be included.
    temps.Include(p0);
    temps.Include(PRegister(1));
    temps.Include(PRegisterWithLaneSize(2, kFormatVnD));
    temps.Include(PRegisterM(3));
    temps.Include(CPURegister(PRegister(4)));
    temps.Include(CPURegister(PRegisterZ(5)));
    helper.RecordActionsAndCheck(0x3f);
    // Multiple registers can be included at once.
    temps.Include(p7, p8.Merging(), p9.VnS());
    temps.Include(PRegister(11), PRegisterWithLaneSize(12, kHRegSize));
    temps.Include(CPURegList(p15));
    helper.RecordActionsAndCheck(0x9b80);
    // Including a register again has no effect.
    temps.Include(PRegister(15));
    temps.Include(PRegisterWithLaneSize(12, kFormatVnB));
    temps.Include(CPURegister(p11));
    temps.Include(CPURegister(p9.VnD()));
    temps.Include(p8.Merging(), p7.Zeroing(), p5.VnB(), p4);
    temps.Include(CPURegList(p3, p2, p1, p0));
    helper.RecordActionsAndCheck(0x9b80);
  }
}

TEST(scratch_scope_exclude_p) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kExclude,
                              CPURegister::kPRegister);

    // Any suitable register type deriving from CPURegister can be excluded.
    temps.Exclude(p0);
    temps.Exclude(PRegister(1));
    temps.Exclude(PRegisterWithLaneSize(2, kFormatVnD));
    temps.Exclude(PRegisterM(3));
    temps.Exclude(CPURegister(PRegister(4)));
    temps.Exclude(CPURegister(PRegisterZ(5)));
    helper.RecordActionsAndCheck(0x3f);
    // Multiple registers can be excluded at once.
    temps.Exclude(p7, p8.Merging(), p9.VnS());
    temps.Exclude(PRegister(11), PRegisterWithLaneSize(12, kHRegSize));
    temps.Exclude(CPURegList(p15));
    helper.RecordActionsAndCheck(0x9b80);
    // Excluding a register again has no effect.
    temps.Exclude(PRegister(15));
    temps.Exclude(PRegisterWithLaneSize(12, kFormatVnB));
    temps.Exclude(CPURegister(p11));
    temps.Exclude(CPURegister(p9.VnD()));
    temps.Exclude(p8.Merging(), p7.Zeroing(), p5.VnB(), p4);
    temps.Exclude(CPURegList(p3, p2, p1, p0));
    helper.RecordActionsAndCheck(0x9b80);
  }
}

TEST(scratch_scope_release_p) {
  MacroAssembler masm;
  {
    UseScratchRegisterScope temps(&masm);
    ScratchScopeHelper helper(&masm,
                              ScratchScopeHelper::kRelease,
                              CPURegister::kPRegister);

    // Any suitable register type deriving from CPURegister can be excluded.
    temps.Release(p0);
    temps.Release(PRegister(1));
    temps.Release(PRegisterWithLaneSize(2, kFormatVnD));
    temps.Release(PRegisterM(3));
    temps.Release(CPURegister(PRegister(4)));
    temps.Release(CPURegister(PRegisterZ(5)));
    helper.RecordActionsAndCheck(0x3f);
    // It is not possible to release more than one register at a time, and it is
    // invalid to release a register that is already available.
  }
}

#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
TEST(sim_stack_default) {
  SimStack::Allocated s = SimStack().Allocate();

  // The default stack is at least 16-byte aligned.
  VIXL_CHECK(IsAligned<16>(s.GetBase()));
  VIXL_CHECK(IsAligned<16>(s.GetLimit() + 1));

  VIXL_CHECK(s.GetBase() > s.GetLimit());

  // The default guard regions are sufficient to detect at least off-by-one
  // errors.
  VIXL_CHECK(s.IsAccessInGuardRegion(s.GetBase(), 1));
  VIXL_CHECK(!s.IsAccessInGuardRegion(s.GetBase() - 1, 1));
  // The limit is one below the lowest address on the stack.
  VIXL_CHECK(s.IsAccessInGuardRegion(s.GetLimit(), 1));
  VIXL_CHECK(!s.IsAccessInGuardRegion(s.GetLimit() + 1, 1));

  // We need to be able to access 16-byte granules at both extremes.
  VIXL_CHECK(!s.IsAccessInGuardRegion(s.GetBase() - 16, 16));
  VIXL_CHECK(!s.IsAccessInGuardRegion(s.GetLimit() + 1, 16));
}

TEST(sim_stack) {
  SimStack builder;
  builder.AlignToBytesLog2(WhichPowerOf2(1024));
  builder.SetBaseGuardSize(42);
  builder.SetLimitGuardSize(2049);
  builder.SetUsableSize(2048);
  SimStack::Allocated s = builder.Allocate();

  VIXL_CHECK(IsAligned<1024>(s.GetBase()));
  VIXL_CHECK(IsAligned<1024>(s.GetLimit() + 1));

  // The stack is accessible for (limit, base), both exclusive.
  // This is checked precisely, using the base and limit modified to respect
  // alignment, so we can test the exact boundary condition.
  VIXL_CHECK(s.IsAccessInGuardRegion(s.GetBase(), 1));
  VIXL_CHECK(!s.IsAccessInGuardRegion(s.GetBase() - 1, 1));
  VIXL_CHECK(s.IsAccessInGuardRegion(s.GetLimit(), 1));
  VIXL_CHECK(!s.IsAccessInGuardRegion(s.GetLimit() + 1, 1));
  VIXL_CHECK((s.GetBase() - s.GetLimit() - 1) == 2048);

  // We can access the whole range (limit, base), both exclusive.
  VIXL_CHECK(!s.IsAccessInGuardRegion(s.GetLimit() + 1, 2048));
  // Off-by-one.
  VIXL_CHECK(s.IsAccessInGuardRegion(s.GetLimit(), 2048));
  VIXL_CHECK(s.IsAccessInGuardRegion(s.GetLimit() + 1, 2049));
  // Accesses spanning whole guard regions.
  VIXL_CHECK(s.IsAccessInGuardRegion(s.GetBase() - 42, 4096));
  VIXL_CHECK(s.IsAccessInGuardRegion(s.GetLimit() - 1280, 2048));
  VIXL_CHECK(s.IsAccessInGuardRegion(s.GetLimit() - 1280, 10000));
}
#endif

}  // namespace aarch64
}  // namespace vixl
