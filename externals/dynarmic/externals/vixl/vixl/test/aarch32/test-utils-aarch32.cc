// Copyright 2015, VIXL authors
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

#include "aarch32/test-utils-aarch32.h"

#define __ masm->

namespace vixl {
namespace aarch32 {

#define VIXL_OFFSET(type, member) offsetof(type, member)

void RegisterDump::Dump(MacroAssembler* masm) {
  UseScratchRegisterScope scratch(masm);
  scratch.ExcludeAll();

  // Preserve some temporary registers.
  Register dump_base = r0;
  Register tmp = r1;

  // Check that the the dump registers can be used
  VIXL_STATIC_ASSERT(sizeof(dump_.r_[0]) == kRegSizeInBytes);
  VIXL_STATIC_ASSERT(sizeof(dump_.d_[0]) == kDRegSizeInBytes);

  // Offsets into the dump_ structure.
  const int r_offset = static_cast<int>(VIXL_OFFSET(dump_t, r_));
  const int d_offset = static_cast<int>(VIXL_OFFSET(dump_t, d_));
  const int flags_offset = static_cast<int>(VIXL_OFFSET(dump_t, flags_));

  __ Push(dump_base);
  __ Push(tmp);

  // Load the address of the dump_ structure.
  __ Mov(dump_base, Operand::From(&dump_));

  // Dump all core registers.  Note that the stack pointer and temporary
  // registers will be stored again later after they are restored.
  for (unsigned i = 0; i < kNumberOfRegisters; i++) {
    Register rt(i);
    // In T32 mode, "str pc, [base, #offset]" is not allowed so we cannot store
    // the program counter this way. We could however compute the relative
    // offset from the start of the buffer and store it but it's not very
    // useful. So for now, testing the value of the PC is not supported.
    if (!rt.IsPC()) {
      __ Str(rt, MemOperand(dump_base, r_offset + (i * kRegSizeInBytes)));
    }
  }

  for (unsigned i = 0; i < kMaxNumberOfDRegisters; i++) {
    DRegister rt(i);
    __ Vstr(Untyped64,
            rt,
            MemOperand(dump_base, d_offset + (i * kDRegSizeInBytes)));
  }

  // Dump the flags.
  __ Mrs(tmp, APSR);
  __ Str(tmp, MemOperand(dump_base, flags_offset));

  // We need a new pointer to dump_ in order to dump the temporary registers and
  // the stack pointer.
  Register dump2_base = r2;
  // TODO: Assert that dump_base, dump2_base and tmp are not aliases.
  __ Mov(dump2_base, dump_base);

  __ Pop(tmp);
  __ Pop(dump_base);

  // Dump tmp, dump_base and the stack pointer.
  __ Str(tmp,
         MemOperand(dump2_base, r_offset + (tmp.GetCode() * kRegSizeInBytes)));
  __ Str(dump_base,
         MemOperand(dump2_base,
                    r_offset + (dump_base.GetCode() * kRegSizeInBytes)));
  __ Str(sp, MemOperand(dump2_base, r_offset + (kSPRegNum * kRegSizeInBytes)));

  completed_ = true;
}


bool Equal32(uint32_t expected, const RegisterDump*, uint32_t result) {
  if (result != expected) {
    printf("Expected 0x%08" PRIx32 "\t Found 0x%08" PRIx32 "\n",
           expected,
           result);
  }

  return expected == result;
}


bool Equal32(uint32_t expected, const RegisterDump* core, const Register& reg) {
  if (reg.IsPC()) {
    printf("Testing the value of the program counter is not supported.");
    return false;
  } else {
    return Equal32(expected, core, core->reg(reg.GetCode()));
  }
}


bool Equal32(uint32_t expected,
             const RegisterDump* core,
             const SRegister& sreg) {
  return Equal32(expected, core, core->GetSRegisterBits(sreg.GetCode()));
}


bool Equal64(uint64_t expected, const RegisterDump*, uint64_t result) {
  if (result != expected) {
    printf("Expected 0x%016" PRIx64 "\t Found 0x%016" PRIx64 "\n",
           expected,
           result);
  }

  return expected == result;
}


bool Equal64(uint64_t expected,
             const RegisterDump* core,
             const DRegister& dreg) {
  return Equal64(expected, core, core->GetDRegisterBits(dreg.GetCode()));
}


bool Equal128(vec128_t expected, const RegisterDump*, vec128_t result) {
  if ((result.h != expected.h) || (result.l != expected.l)) {
    printf("Expected 0x%016" PRIx64 "%016" PRIx64
           "\t "
           "Found 0x%016" PRIx64 "%016" PRIx64 "\n",
           expected.h,
           expected.l,
           result.h,
           result.l);
  }

  return ((expected.h == result.h) && (expected.l == result.l));
}


bool Equal128(uint64_t expected_h,
              uint64_t expected_l,
              const RegisterDump* core,
              const QRegister& qreg) {
  vec128_t expected = {expected_l, expected_h};
  vec128_t result = core->GetQRegisterBits(qreg.GetCode());
  return Equal128(expected, core, result);
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


bool EqualFP32(float expected,
               const RegisterDump* core,
               const SRegister& sreg) {
  // Retrieve the corresponding S register
  uint32_t result = core->GetSRegisterBits(sreg.GetCode());

  if (FloatToRawbits(expected) == result) {
    return true;
  } else {
    if (IsNaN(expected) || (expected == 0.0)) {
      printf("Expected 0x%08" PRIx32 "\t Found 0x%08" PRIx32 "\n",
             FloatToRawbits(expected),
             result);
    } else {
      printf("Expected %.9f (0x%08" PRIx32
             ")\t "
             "Found %.9f (0x%08" PRIx32 ")\n",
             expected,
             FloatToRawbits(expected),
             RawbitsToFloat(result),
             result);
    }
    return false;
  }
}


bool EqualFP64(double expected,
               const RegisterDump* core,
               const DRegister& dreg) {
  // Retrieve the corresponding D register
  uint64_t result = core->GetDRegisterBits(dreg.GetCode());

  if (DoubleToRawbits(expected) == result) {
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
           RawbitsToDouble(result),
           result);
  }
  return false;
}


}  // namespace aarch32
}  // namespace vixl
