// Copyright 2014, VIXL authors
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

#include <cmath>
#include <queue>

#include "test-runner.h"
#include "test-utils-aarch64.h"

#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/macro-assembler-aarch64.h"
#include "aarch64/simulator-aarch64.h"

#define __ masm->

namespace vixl {
namespace aarch64 {


// This value is a signalling NaN as FP64, and also as FP32 or FP16 (taking the
// least-significant bits).
const double kFP64SignallingNaN = RawbitsToDouble(UINT64_C(0x7ff000007f807c01));
const float kFP32SignallingNaN = RawbitsToFloat(0x7f807c01);
const Float16 kFP16SignallingNaN = RawbitsToFloat16(0x7c01);

// A similar value, but as a quiet NaN.
const double kFP64QuietNaN = RawbitsToDouble(UINT64_C(0x7ff800007fc07e01));
const float kFP32QuietNaN = RawbitsToFloat(0x7fc07e01);
const Float16 kFP16QuietNaN = RawbitsToFloat16(0x7e01);


bool Equal32(uint32_t expected, const RegisterDump*, uint32_t result) {
  if (result != expected) {
    printf("Expected 0x%08" PRIx32 "\t Found 0x%08" PRIx32 "\n",
           expected,
           result);
  }

  return expected == result;
}


bool Equal64(uint64_t reference,
             const RegisterDump*,
             uint64_t result,
             ExpectedResult option) {
  switch (option) {
    case kExpectEqual:
      if (result != reference) {
        printf("Expected 0x%016" PRIx64 "\t Found 0x%016" PRIx64 "\n",
               reference,
               result);
      }
      break;
    case kExpectNotEqual:
      if (result == reference) {
        printf("Expected a result not equal to 0x%016" PRIx64 "\n", reference);
      }
      break;
  }

  return reference == result;
}


bool Equal128(QRegisterValue expected,
              const RegisterDump*,
              QRegisterValue result) {
  if (!expected.Equals(result)) {
    printf("Expected 0x%016" PRIx64 "%016" PRIx64
           "\t "
           "Found 0x%016" PRIx64 "%016" PRIx64 "\n",
           expected.GetLane<uint64_t>(1),
           expected.GetLane<uint64_t>(0),
           result.GetLane<uint64_t>(1),
           result.GetLane<uint64_t>(0));
  }

  return expected.Equals(result);
}


bool EqualFP16(Float16 expected, const RegisterDump*, Float16 result) {
  uint16_t e_rawbits = Float16ToRawbits(expected);
  uint16_t r_rawbits = Float16ToRawbits(result);
  if (e_rawbits == r_rawbits) {
    return true;
  } else {
    if (IsNaN(expected) || IsZero(expected)) {
      printf("Expected 0x%04" PRIx16 "\t Found 0x%04" PRIx16 "\n",
             e_rawbits,
             r_rawbits);
    } else {
      printf("Expected %.6f (16 bit): (0x%04" PRIx16
             ")\t "
             "Found %.6f (0x%04" PRIx16 ")\n",
             FPToFloat(expected, kIgnoreDefaultNaN),
             e_rawbits,
             FPToFloat(result, kIgnoreDefaultNaN),
             r_rawbits);
    }
    return false;
  }
}


bool EqualFP32(float expected, const RegisterDump*, float result) {
  if (FloatToRawbits(expected) == FloatToRawbits(result)) {
    return true;
  } else {
    if (IsNaN(expected) || (expected == 0.0)) {
      printf("Expected 0x%08" PRIx32 "\t Found 0x%08" PRIx32 "\n",
             FloatToRawbits(expected),
             FloatToRawbits(result));
    } else {
      printf("Expected %.9f (0x%08" PRIx32
             ")\t "
             "Found %.9f (0x%08" PRIx32 ")\n",
             expected,
             FloatToRawbits(expected),
             result,
             FloatToRawbits(result));
    }
    return false;
  }
}


bool EqualFP64(double expected, const RegisterDump*, double result) {
  if (DoubleToRawbits(expected) == DoubleToRawbits(result)) {
    return true;
  }

  if (IsNaN(expected) || (expected == 0.0)) {
    printf("Expected 0x%016" PRIx64 "\t Found 0x%016" PRIx64 "\n",
           DoubleToRawbits(expected),
           DoubleToRawbits(result));
  } else {
    printf("Expected %.17f (0x%016" PRIx64
           ")\t "
           "Found %.17f (0x%016" PRIx64 ")\n",
           expected,
           DoubleToRawbits(expected),
           result,
           DoubleToRawbits(result));
  }
  return false;
}


bool Equal32(uint32_t expected, const RegisterDump* core, const Register& reg) {
  VIXL_ASSERT(reg.Is32Bits());
  // Retrieve the corresponding X register so we can check that the upper part
  // was properly cleared.
  int64_t result_x = core->xreg(reg.GetCode());
  if ((result_x & 0xffffffff00000000) != 0) {
    printf("Expected 0x%08" PRIx32 "\t Found 0x%016" PRIx64 "\n",
           expected,
           result_x);
    return false;
  }
  uint32_t result_w = core->wreg(reg.GetCode());
  return Equal32(expected, core, result_w);
}


bool Equal64(uint64_t reference,
             const RegisterDump* core,
             const Register& reg,
             ExpectedResult option) {
  VIXL_ASSERT(reg.Is64Bits());
  uint64_t result = core->xreg(reg.GetCode());
  return Equal64(reference, core, result, option);
}


bool NotEqual64(uint64_t reference,
                const RegisterDump* core,
                const Register& reg) {
  VIXL_ASSERT(reg.Is64Bits());
  uint64_t result = core->xreg(reg.GetCode());
  return NotEqual64(reference, core, result);
}


bool Equal128(uint64_t expected_h,
              uint64_t expected_l,
              const RegisterDump* core,
              const VRegister& vreg) {
  VIXL_ASSERT(vreg.Is128Bits());
  QRegisterValue expected;
  expected.SetLane(0, expected_l);
  expected.SetLane(1, expected_h);
  QRegisterValue result = core->qreg(vreg.GetCode());
  return Equal128(expected, core, result);
}


bool EqualFP16(Float16 expected,
               const RegisterDump* core,
               const VRegister& fpreg) {
  VIXL_ASSERT(fpreg.Is16Bits());
  // Retrieve the corresponding D register so we can check that the upper part
  // was properly cleared.
  uint64_t result_64 = core->dreg_bits(fpreg.GetCode());
  if ((result_64 & 0xfffffffffff0000) != 0) {
    printf("Expected 0x%04" PRIx16 " (%f)\t Found 0x%016" PRIx64 "\n",
           Float16ToRawbits(expected),
           FPToFloat(expected, kIgnoreDefaultNaN),
           result_64);
    return false;
  }
  return EqualFP16(expected, core, core->hreg(fpreg.GetCode()));
}


bool EqualFP32(float expected,
               const RegisterDump* core,
               const VRegister& fpreg) {
  VIXL_ASSERT(fpreg.Is32Bits());
  // Retrieve the corresponding D register so we can check that the upper part
  // was properly cleared.
  uint64_t result_64 = core->dreg_bits(fpreg.GetCode());
  if ((result_64 & 0xffffffff00000000) != 0) {
    printf("Expected 0x%08" PRIx32 " (%f)\t Found 0x%016" PRIx64 "\n",
           FloatToRawbits(expected),
           expected,
           result_64);
    return false;
  }

  return EqualFP32(expected, core, core->sreg(fpreg.GetCode()));
}


bool EqualFP64(double expected,
               const RegisterDump* core,
               const VRegister& fpreg) {
  VIXL_ASSERT(fpreg.Is64Bits());
  return EqualFP64(expected, core, core->dreg(fpreg.GetCode()));
}


bool Equal64(const Register& reg0,
             const RegisterDump* core,
             const Register& reg1,
             ExpectedResult option) {
  VIXL_ASSERT(reg0.Is64Bits() && reg1.Is64Bits());
  int64_t reference = core->xreg(reg0.GetCode());
  int64_t result = core->xreg(reg1.GetCode());
  return Equal64(reference, core, result, option);
}


bool NotEqual64(const Register& reg0,
                const RegisterDump* core,
                const Register& reg1) {
  VIXL_ASSERT(reg0.Is64Bits() && reg1.Is64Bits());
  int64_t expected = core->xreg(reg0.GetCode());
  int64_t result = core->xreg(reg1.GetCode());
  return NotEqual64(expected, core, result);
}


bool Equal64(uint64_t expected,
             const RegisterDump* core,
             const VRegister& vreg) {
  VIXL_ASSERT(vreg.Is64Bits());
  uint64_t result = core->dreg_bits(vreg.GetCode());
  return Equal64(expected, core, result);
}


static char FlagN(uint32_t flags) { return (flags & NFlag) ? 'N' : 'n'; }


static char FlagZ(uint32_t flags) { return (flags & ZFlag) ? 'Z' : 'z'; }


static char FlagC(uint32_t flags) { return (flags & CFlag) ? 'C' : 'c'; }


static char FlagV(uint32_t flags) { return (flags & VFlag) ? 'V' : 'v'; }


bool EqualNzcv(uint32_t expected, uint32_t result) {
  VIXL_ASSERT((expected & ~NZCVFlag) == 0);
  VIXL_ASSERT((result & ~NZCVFlag) == 0);
  if (result != expected) {
    printf("Expected: %c%c%c%c\t Found: %c%c%c%c\n",
           FlagN(expected),
           FlagZ(expected),
           FlagC(expected),
           FlagV(expected),
           FlagN(result),
           FlagZ(result),
           FlagC(result),
           FlagV(result));
    return false;
  }

  return true;
}


bool EqualRegisters(const RegisterDump* a, const RegisterDump* b) {
  for (unsigned i = 0; i < kNumberOfRegisters; i++) {
    if (a->xreg(i) != b->xreg(i)) {
      printf("x%d\t Expected 0x%016" PRIx64 "\t Found 0x%016" PRIx64 "\n",
             i,
             a->xreg(i),
             b->xreg(i));
      return false;
    }
  }

  for (unsigned i = 0; i < kNumberOfVRegisters; i++) {
    uint64_t a_bits = a->dreg_bits(i);
    uint64_t b_bits = b->dreg_bits(i);
    if (a_bits != b_bits) {
      printf("d%d\t Expected 0x%016" PRIx64 "\t Found 0x%016" PRIx64 "\n",
             i,
             a_bits,
             b_bits);
      return false;
    }
  }

  return true;
}

bool EqualSVELane(uint64_t expected,
                  const RegisterDump* core,
                  const ZRegister& reg,
                  int lane) {
  unsigned lane_size = reg.GetLaneSizeInBits();
  // For convenience in the tests, we allow negative values to be passed into
  // `expected`, but truncate them to an appropriately-sized unsigned value for
  // the check. For example, in `EqualSVELane(-1, core, z0.VnB())`, the expected
  // value is truncated from 0xffffffffffffffff to 0xff before the comparison.
  VIXL_ASSERT(IsUintN(lane_size, expected) ||
              IsIntN(lane_size, RawbitsToInt64(expected)));
  expected &= GetUintMask(lane_size);

  uint64_t result = core->zreg_lane(reg.GetCode(), lane_size, lane);
  if (expected != result) {
    unsigned lane_size_in_hex_chars = lane_size / 4;
    std::string reg_name = reg.GetArchitecturalName();
    printf("%s[%d]\t Expected 0x%0*" PRIx64 "\t Found 0x%0*" PRIx64 "\n",
           reg_name.c_str(),
           lane,
           lane_size_in_hex_chars,
           expected,
           lane_size_in_hex_chars,
           result);
    return false;
  }
  return true;
}

bool EqualSVELane(uint64_t expected,
                  const RegisterDump* core,
                  const PRegister& reg,
                  int lane) {
  VIXL_ASSERT(reg.HasLaneSize());
  VIXL_ASSERT((reg.GetLaneSizeInBits() % kZRegBitsPerPRegBit) == 0);
  unsigned p_bits_per_lane = reg.GetLaneSizeInBits() / kZRegBitsPerPRegBit;
  VIXL_ASSERT(IsUintN(p_bits_per_lane, expected));
  expected &= GetUintMask(p_bits_per_lane);

  uint64_t result = core->preg_lane(reg.GetCode(), p_bits_per_lane, lane);
  if (expected != result) {
    unsigned lane_size_in_hex_chars = (p_bits_per_lane + 3) / 4;
    std::string reg_name = reg.GetArchitecturalName();
    printf("%s[%d]\t Expected 0x%0*" PRIx64 "\t Found 0x%0*" PRIx64 "\n",
           reg_name.c_str(),
           lane,
           lane_size_in_hex_chars,
           expected,
           lane_size_in_hex_chars,
           result);
    return false;
  }
  return true;
}

struct EqualMemoryChunk {
  typedef uint64_t RawChunk;

  uintptr_t address;
  RawChunk expected;
  RawChunk result;

  bool IsEqual() const { return expected == result; }
};

bool EqualMemory(const void* expected,
                 const void* result,
                 size_t size_in_bytes,
                 size_t zero_offset) {
  if (memcmp(expected, result, size_in_bytes) == 0) return true;

  // Read 64-bit chunks, and print them side-by-side if they don't match.

  // Remember the last few chunks, even if they matched, so we can print some
  // context. We don't want to print the whole buffer, because it could be huge.
  static const size_t kContextLines = 1;
  std::queue<EqualMemoryChunk> context;
  static const size_t kChunkSize = sizeof(EqualMemoryChunk::RawChunk);

  // This assumption keeps the logic simple, and is acceptable for our tests.
  VIXL_ASSERT((size_in_bytes % kChunkSize) == 0);

  const char* expected_it = reinterpret_cast<const char*>(expected);
  const char* result_it = reinterpret_cast<const char*>(result);

  // This is the first error, so print a header row.
  printf("  Address (of result)                  Expected           Result\n");

  // Always print some context at the start of the buffer.
  uintptr_t print_context_to =
      reinterpret_cast<uintptr_t>(result) + (kContextLines + 1) * kChunkSize;
  for (size_t i = 0; i < size_in_bytes; i += kChunkSize) {
    EqualMemoryChunk chunk;
    chunk.address = reinterpret_cast<uintptr_t>(result_it);
    memcpy(&chunk.expected, expected_it, kChunkSize);
    memcpy(&chunk.result, result_it, kChunkSize);

    while (context.size() > kContextLines) context.pop();
    context.push(chunk);

    // Print context after an error, and at the end of the buffer.
    if (!chunk.IsEqual() || ((i + kChunkSize) >= size_in_bytes)) {
      if (chunk.address > print_context_to) {
        // We aren't currently printing context, so separate this context from
        // the previous block.
        printf("...\n");
      }
      print_context_to = chunk.address + (kContextLines + 1) * kChunkSize;
    }

    // Print context (including the current line).
    while (!context.empty() && (context.front().address < print_context_to)) {
      uintptr_t address = context.front().address;
      uint64_t offset = address - reinterpret_cast<uintptr_t>(result);
      bool is_negative = (offset < zero_offset);
      printf("0x%016" PRIxPTR " (result %c %5" PRIu64 "): 0x%016" PRIx64
             " 0x%016" PRIx64 "\n",
             address,
             (is_negative ? '-' : '+'),
             (is_negative ? (zero_offset - offset) : (offset - zero_offset)),
             context.front().expected,
             context.front().result);
      context.pop();
    }

    expected_it += kChunkSize;
    result_it += kChunkSize;
  }

  return false;
}
RegList PopulateRegisterArray(Register* w,
                              Register* x,
                              Register* r,
                              int reg_size,
                              int reg_count,
                              RegList allowed) {
  RegList list = 0;
  int i = 0;
  for (unsigned n = 0; (n < kNumberOfRegisters) && (i < reg_count); n++) {
    if (((UINT64_C(1) << n) & allowed) != 0) {
      // Only assign allowed registers.
      if (r) {
        r[i] = Register(n, reg_size);
      }
      if (x) {
        x[i] = Register(n, kXRegSize);
      }
      if (w) {
        w[i] = Register(n, kWRegSize);
      }
      list |= (UINT64_C(1) << n);
      i++;
    }
  }
  // Check that we got enough registers.
  VIXL_ASSERT(CountSetBits(list, kNumberOfRegisters) == reg_count);

  return list;
}


RegList PopulateVRegisterArray(VRegister* s,
                               VRegister* d,
                               VRegister* v,
                               int reg_size,
                               int reg_count,
                               RegList allowed) {
  RegList list = 0;
  int i = 0;
  for (unsigned n = 0; (n < kNumberOfVRegisters) && (i < reg_count); n++) {
    if (((UINT64_C(1) << n) & allowed) != 0) {
      // Only assigned allowed registers.
      if (v) {
        v[i] = VRegister(n, reg_size);
      }
      if (d) {
        d[i] = VRegister(n, kDRegSize);
      }
      if (s) {
        s[i] = VRegister(n, kSRegSize);
      }
      list |= (UINT64_C(1) << n);
      i++;
    }
  }
  // Check that we got enough registers.
  VIXL_ASSERT(CountSetBits(list, kNumberOfVRegisters) == reg_count);

  return list;
}


void Clobber(MacroAssembler* masm, RegList reg_list, uint64_t const value) {
  Register first = NoReg;
  for (unsigned i = 0; i < kNumberOfRegisters; i++) {
    if (reg_list & (UINT64_C(1) << i)) {
      Register xn(i, kXRegSize);
      // We should never write into sp here.
      VIXL_ASSERT(!xn.Is(sp));
      if (!xn.IsZero()) {
        if (!first.IsValid()) {
          // This is the first register we've hit, so construct the literal.
          __ Mov(xn, value);
          first = xn;
        } else {
          // We've already loaded the literal, so re-use the value already
          // loaded into the first register we hit.
          __ Mov(xn, first);
        }
      }
    }
  }
}


void ClobberFP(MacroAssembler* masm, RegList reg_list, double const value) {
  VRegister first = NoVReg;
  for (unsigned i = 0; i < kNumberOfVRegisters; i++) {
    if (reg_list & (UINT64_C(1) << i)) {
      VRegister dn(i, kDRegSize);
      if (!first.IsValid()) {
        // This is the first register we've hit, so construct the literal.
        __ Fmov(dn, value);
        first = dn;
      } else {
        // We've already loaded the literal, so re-use the value already loaded
        // into the first register we hit.
        __ Fmov(dn, first);
      }
    }
  }
}


void Clobber(MacroAssembler* masm, CPURegList reg_list) {
  if (reg_list.GetType() == CPURegister::kRegister) {
    // This will always clobber X registers.
    Clobber(masm, reg_list.GetList());
  } else if (reg_list.GetType() == CPURegister::kVRegister) {
    // This will always clobber D registers.
    ClobberFP(masm, reg_list.GetList());
  } else {
    VIXL_UNIMPLEMENTED();
  }
}

// TODO: Once registers have sufficiently compatible interfaces, merge the two
// DumpRegisters templates.
template <typename T>
static void DumpRegisters(MacroAssembler* masm,
                          Register dump_base,
                          int offset) {
  UseScratchRegisterScope temps(masm);
  Register dump = temps.AcquireX();
  __ Add(dump, dump_base, offset);
  for (unsigned i = 0; i <= T::GetMaxCode(); i++) {
    T reg(i);
    __ Str(reg, SVEMemOperand(dump));
    __ Add(dump, dump, reg.GetMaxSizeInBytes());
  }
}

template <typename T>
static void DumpRegisters(MacroAssembler* masm,
                          Register dump_base,
                          int offset,
                          int reg_size_in_bytes) {
  UseScratchRegisterScope temps(masm);
  Register dump = temps.AcquireX();
  __ Add(dump, dump_base, offset);
  for (unsigned i = 0; i <= T::GetMaxCode(); i++) {
    T reg(i, reg_size_in_bytes * kBitsPerByte);
    __ Str(reg, MemOperand(dump));
    __ Add(dump, dump, reg_size_in_bytes);
  }
}

void RegisterDump::Dump(MacroAssembler* masm) {
  VIXL_ASSERT(__ StackPointer().Is(sp));

  dump_cpu_features_ = *masm->GetCPUFeatures();

  // We need some scratch registers, but we also need to dump them, so we have
  // to control exactly which registers are used, and dump them separately.
  CPURegList scratch_registers(x0, x1, x2, x3);

  UseScratchRegisterScope temps(masm);
  temps.ExcludeAll();
  __ PushCPURegList(scratch_registers);
  temps.Include(scratch_registers);

  Register dump_base = temps.AcquireX();
  Register tmp = temps.AcquireX();

  // Offsets into the dump_ structure.
  const int x_offset = offsetof(dump_t, x_);
  const int w_offset = offsetof(dump_t, w_);
  const int d_offset = offsetof(dump_t, d_);
  const int s_offset = offsetof(dump_t, s_);
  const int h_offset = offsetof(dump_t, h_);
  const int q_offset = offsetof(dump_t, q_);
  const int z_offset = offsetof(dump_t, z_);
  const int p_offset = offsetof(dump_t, p_);
  const int sp_offset = offsetof(dump_t, sp_);
  const int wsp_offset = offsetof(dump_t, wsp_);
  const int flags_offset = offsetof(dump_t, flags_);
  const int vl_offset = offsetof(dump_t, vl_);

  // Load the address where we will dump the state.
  __ Mov(dump_base, reinterpret_cast<uintptr_t>(&dump_));

  // Dump the stack pointer (sp and wsp).
  // The stack pointer cannot be stored directly; it needs to be moved into
  // another register first. Also, we pushed four X registers, so we need to
  // compensate here.
  __ Add(tmp, sp, 4 * kXRegSizeInBytes);
  __ Str(tmp, MemOperand(dump_base, sp_offset));
  __ Add(tmp.W(), wsp, 4 * kXRegSizeInBytes);
  __ Str(tmp.W(), MemOperand(dump_base, wsp_offset));

  // Dump core registers.
  DumpRegisters<Register>(masm, dump_base, x_offset, kXRegSizeInBytes);
  DumpRegisters<Register>(masm, dump_base, w_offset, kWRegSizeInBytes);

  // Dump NEON and FP registers.
  DumpRegisters<VRegister>(masm, dump_base, q_offset, kQRegSizeInBytes);
  DumpRegisters<VRegister>(masm, dump_base, d_offset, kDRegSizeInBytes);
  DumpRegisters<VRegister>(masm, dump_base, s_offset, kSRegSizeInBytes);
  DumpRegisters<VRegister>(masm, dump_base, h_offset, kHRegSizeInBytes);

  // Dump SVE registers.
  if (CPUHas(CPUFeatures::kSVE)) {
    DumpRegisters<ZRegister>(masm, dump_base, z_offset);
    DumpRegisters<PRegister>(masm, dump_base, p_offset);

    // Record the vector length.
    __ Rdvl(tmp, kBitsPerByte);
    __ Str(tmp, MemOperand(dump_base, vl_offset));
  }

  // Dump the flags.
  __ Mrs(tmp, NZCV);
  __ Str(tmp, MemOperand(dump_base, flags_offset));

  // To dump the values we used as scratch registers, we need a new scratch
  // register. We can use any of the already dumped registers since we can
  // easily restore them.
  Register dump2_base = x10;
  VIXL_ASSERT(!scratch_registers.IncludesAliasOf(dump2_base));

  VIXL_ASSERT(scratch_registers.IncludesAliasOf(dump_base));

  // Ensure that we don't try to use the scratch registers again.
  temps.ExcludeAll();

  // Don't lose the dump_ address.
  __ Mov(dump2_base, dump_base);

  __ PopCPURegList(scratch_registers);

  while (!scratch_registers.IsEmpty()) {
    CPURegister reg = scratch_registers.PopLowestIndex();
    Register x = reg.X();
    Register w = reg.W();
    unsigned code = reg.GetCode();
    __ Str(x, MemOperand(dump2_base, x_offset + (code * kXRegSizeInBytes)));
    __ Str(w, MemOperand(dump2_base, w_offset + (code * kWRegSizeInBytes)));
  }

  // Finally, restore dump2_base.
  __ Ldr(dump2_base,
         MemOperand(dump2_base,
                    x_offset + (dump2_base.GetCode() * kXRegSizeInBytes)));

  completed_ = true;
}

uint64_t GetSignallingNan(int size_in_bits) {
  switch (size_in_bits) {
    case kHRegSize:
      return Float16ToRawbits(kFP16SignallingNaN);
    case kSRegSize:
      return FloatToRawbits(kFP32SignallingNaN);
    case kDRegSize:
      return DoubleToRawbits(kFP64SignallingNaN);
    default:
      VIXL_UNIMPLEMENTED();
      return 0;
  }
}

bool CanRun(const CPUFeatures& required, bool* queried_can_run) {
  bool log_if_missing = true;
  if (queried_can_run != NULL) {
    log_if_missing = !*queried_can_run;
    *queried_can_run = true;
  }

#ifdef VIXL_INCLUDE_SIMULATOR_AARCH64
  // The Simulator can run any test that VIXL can assemble.
  USE(required);
  USE(log_if_missing);
  return true;
#else
  CPUFeatures cpu = CPUFeatures::InferFromOS();
  // If InferFromOS fails, assume that basic features are present.
  if (cpu.HasNoFeatures()) cpu = CPUFeatures::AArch64LegacyBaseline();
  VIXL_ASSERT(cpu.Has(kInfrastructureCPUFeatures));

  if (cpu.Has(required)) return true;

  if (log_if_missing) {
    CPUFeatures missing = required.Without(cpu);
    // Note: This message needs to match REGEXP_MISSING_FEATURES from
    // tools/threaded_test.py.
    std::cout << "SKIPPED: Missing features: { " << missing << " }\n";
    std::cout << "This test requires the following features to run its "
                 "generated code on this CPU: "
              << required << "\n";
  }
  return false;
#endif
}

}  // namespace aarch64
}  // namespace vixl
