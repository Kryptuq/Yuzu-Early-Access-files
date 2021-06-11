// Copyright 2019, VIXL authors
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

#include <vector>

#include "globals-vixl.h"
#include "aarch64/macro-assembler-aarch64.h"

#include "bench-utils.h"

using namespace vixl;
using namespace vixl::aarch64;

#define __ masm_->

const Register BenchCodeGenerator::scratch = x28;

Register BenchCodeGenerator::PickR(unsigned size_in_bits) {
  // Only select caller-saved registers [x0, x15].
  return Register(static_cast<unsigned>(GetRandomBits(4)), size_in_bits);
}

VRegister BenchCodeGenerator::PickV(unsigned size_in_bits) {
  // Only select caller-saved registers [v0, v7] or [v16, v31].
  // The resulting distribution is not uniform.
  unsigned code = static_cast<unsigned>(GetRandomBits(5));
  if (code < 16) code &= 0x7;  // [v8, v15] -> [v0, v7]
  return VRegister(code, size_in_bits);
}

uint64_t BenchCodeGenerator::GetRandomBits(int bits) {
  VIXL_ASSERT((bits >= 0) && (bits <= 64));
  uint64_t result = 0;

  while (bits >= 32) {
    // For big chunks, call jrand48 directly.
    result = (result << 32) | jrand48(rand_state_);  // [-2^31, 2^31]
    bits -= 32;
  }
  if (bits == 0) return result;

  // We often only want a few bits at a time, so use stored entropy to avoid
  // frequent calls to jrand48.

  if (bits > rnd_bits_) {
    // We want more bits than we have.
    result = (result << rnd_bits_) | rnd_;
    bits -= rnd_bits_;

    rnd_ = static_cast<uint32_t>(jrand48(rand_state_));  // [-2^31, 2^31]
    rnd_bits_ = 32;
  }

  VIXL_ASSERT(bits <= rnd_bits_);
  result = (result << bits) | (rnd_ % (UINT32_C(1) << bits));
  rnd_ >>= bits;
  rnd_bits_ -= bits;
  return result;
}

unsigned BenchCodeGenerator::PickRSize() {
  return PickBool() ? kWRegSize : kXRegSize;
}

unsigned BenchCodeGenerator::PickFPSize() {
  uint64_t entropy = GetRandomBits(4);
  // Doubles and floats are common in most languages, so use half-precision
  // types only rarely.
  if (entropy == 0) return kHRegSize;
  return ((entropy & 1) == 0) ? kSRegSize : kDRegSize;
}

void BenchCodeGenerator::Generate(size_t min_size_in_bytes) {
  Label start;
  __ Bind(&start);

  call_depth_++;
  GeneratePrologue();

  while (masm_->GetSizeOfCodeGeneratedSince(&start) < min_size_in_bytes) {
    GenerateArbitrarySequence();
  }

  GenerateEpilogue();
  call_depth_--;

  // Make sure that any labels (created by GenerateBranchSequence) are bound
  // before we exit.
  if (call_depth_ == 0) BindAllPendingLabels();
}

void BenchCodeGenerator::GeneratePrologue() {
  // Construct a normal frame.
  VIXL_ASSERT(masm_->StackPointer().Is(sp));
  __ Push(lr, x29);  // x29 is the frame pointer (fp).
  __ Mov(x29, sp);
  VIXL_ASSERT(call_depth_ > 0);
  if (call_depth_ == 1) {
    __ Push(scratch, xzr);
    // Claim space to use for load and stores.
    // - We need at least 4 * kQRegSize bytes for Ld4/St4.
    // - The architecture requires that we allocate a multiple of 16 bytes.
    // - There is no hard upper limit, but the Simulator has a limited stack
    //   space.
    __ Claim((4 * kQRegSize) + (16 * GetRandomBits(3)));
    __ Mov(scratch, sp);
  }
}

void BenchCodeGenerator::GenerateEpilogue() {
  VIXL_ASSERT(call_depth_ > 0);
  if (call_depth_ == 1) {
    __ Sub(sp, x29, 2 * kXRegSizeInBytes);  // Drop the scratch space.
    __ Pop(xzr, scratch);
  }
  __ Pop(x29, lr);
  __ Ret();
}

void BenchCodeGenerator::GenerateArbitrarySequence() {
  // Bind pending labels, and remove them from the list.
  // Recently-linked labels are much more likely to be bound than old ones. This
  // should produce a mix of long- (veneered) and short-range branches.
  uint32_t bind_mask = static_cast<uint32_t>(
      GetRandomBits(8) | (GetRandomBits(7) << 1) | (GetRandomBits(6) << 2));
  BindPendingLabels(bind_mask);

  // If we are at the top call level (call_depth_ == 1), generate nested calls
  // 1/4 of the time, and halve the chance for each call level below that.
  VIXL_ASSERT(call_depth_ > 0);
  if (GetRandomBits(call_depth_ + 1) == 0) {
    GenerateCallReturnSequence();
    return;
  }

  // These weightings should be roughly representative of real functions.
  switch (GetRandomBits(4)) {
    case 0x0:
    case 0x1:
      GenerateTrivialSequence();
      return;
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
      GenerateOperandSequence();
      return;
    case 0x6:
    case 0x7:
    case 0x8:
      GenerateMemOperandSequence();
      return;
    case 0xb:
    case 0x9:
    case 0xa:
      GenerateImmediateSequence();
      return;
    case 0xc:
    case 0xd:
      GenerateBranchSequence();
      return;
    case 0xe:
      GenerateFPSequence();
      return;
    case 0xf:
      GenerateNEONSequence();
      return;
  }
}

void BenchCodeGenerator::GenerateTrivialSequence() {
  unsigned size = PickRSize();
  __ Asr(PickR(size), PickR(size), 4);
  __ Bfi(PickR(size), PickR(size), 5, 14);
  __ Bfc(PickR(size), 5, 14);
  __ Cinc(PickR(size), PickR(size), ge);
  __ Cinv(PickR(size), PickR(size), ne);
  __ Cls(PickR(size), PickR(size));
  __ Cneg(PickR(size), PickR(size), lt);
  __ Mrs(PickX(), NZCV);
  __ Nop();
  __ Mul(PickR(size), PickR(size), PickR(size));
  __ Rbit(PickR(size), PickR(size));
  __ Rev(PickR(size), PickR(size));
  __ Sdiv(PickR(size), PickR(size), PickR(size));
  if (!labels_.empty()) {
    __ Adr(PickX(), labels_.begin()->target);
  }
}

void BenchCodeGenerator::GenerateOperandSequence() {
  unsigned size = PickRSize();
  // The cast to Operand is normally implicit for simple registers, but we
  // explicitly specify it in every case here to ensure that the benchmark does
  // what we expect.
  __ And(PickR(size), PickR(size), Operand(PickR(size)));
  __ Bics(PickR(size), PickR(size), Operand(PickR(size)));
  __ Orr(PickR(size), PickR(size), Operand(PickR(size)));
  __ Eor(PickR(size), PickR(size), Operand(PickR(size)));
  __ Tst(PickR(size), Operand(PickR(size)));
  __ Eon(PickR(size), PickR(size), Operand(PickR(size)));
  __ Cmp(PickR(size), Operand(PickR(size)));
  __ Negs(PickR(size), Operand(PickR(size)));
  __ Mvn(PickR(size), Operand(PickR(size)));
  __ Ccmp(PickR(size), Operand(PickR(size)), NoFlag, eq);
  __ Ccmn(PickR(size), Operand(PickR(size)), NoFlag, eq);
  __ Csel(PickR(size), Operand(PickR(size)), Operand(PickR(size)), lt);
  {
    // Ensure that `claim` doesn't alias any PickR().
    UseScratchRegisterScope temps(masm_);
    Register claim = temps.AcquireX();
    // We should only claim a 16-byte-aligned amount, since we're using the
    // system stack pointer.
    __ Mov(claim, GetRandomBits(4) * 16);
    __ Claim(Operand(claim));
    // Also claim a bit more, so we can store at sp+claim.
    __ Claim(Operand(32));
    __ Poke(PickR(size), Operand(claim));
    __ Peek(PickR(size), Operand(8));
    __ Poke(PickR(size), Operand(16));
    __ Peek(PickR(size), Operand(claim.W(), UXTW));
    __ Drop(Operand(32));
    __ Drop(Operand(claim));
  }
}

void BenchCodeGenerator::GenerateMemOperandSequence() {
  unsigned size = PickRSize();
  RegList store_list = GetRandomBits(16);  // Restrict to [x0, x15].
  __ StoreCPURegList(CPURegList(CPURegister::kRegister, size, store_list),
                     MemOperand(scratch));
  RegList load_list = GetRandomBits(16);  // Restrict to [x0, x15].
  __ LoadCPURegList(CPURegList(CPURegister::kRegister, size, load_list),
                    MemOperand(scratch));
  __ Str(PickX(), MemOperand(scratch));
  __ Strb(PickW(), MemOperand(scratch, 42));
  __ Strh(PickW(), MemOperand(scratch, 42, PostIndex));
  __ Ldrsw(PickX(), MemOperand(scratch, -42, PreIndex));
  __ Ldr(PickR(size), MemOperand(scratch, 19));  // Translated to ldur.
  __ Push(PickX(), PickX());
  // Ensure unique registers (in [x0, x15]) for Pop.
  __ Pop(Register(static_cast<int>(GetRandomBits(2)) + 0, kWRegSize),
         Register(static_cast<int>(GetRandomBits(2)) + 4, kWRegSize),
         Register(static_cast<int>(GetRandomBits(2)) + 8, kWRegSize),
         Register(static_cast<int>(GetRandomBits(2)) + 12, kWRegSize));
}

void BenchCodeGenerator::GenerateImmediateSequence() {
  unsigned size = PickRSize();
  __ And(PickR(size), PickR(size), GetRandomBits(size));
  __ Sub(PickR(size), PickR(size), GetRandomBits(size));
  __ Mov(PickR(size), GetRandomBits(size));
  __ Movk(PickX(), GetRandomBits(16), static_cast<int>(GetRandomBits(2)) * 16);
}

void BenchCodeGenerator::BindPendingLabels(uint64_t bind_mask) {
  if (bind_mask == 0) return;
  // The labels we bind here jump back to just after each branch that refers
  // to them. This allows a simple, linear execution path, whilst still
  // benchmarking long-range labels.
  //
  // Ensure that code falling through into this sequence does not jump
  // back to an earlier point in the execution path.
  Label done;
  __ B(&done);

  std::list<LabelPair>::iterator it = labels_.begin();
  while ((it != labels_.end()) && (bind_mask != 0)) {
    if ((bind_mask & 1) != 0) {
      // Bind the label and jump back to its source.
      __ Bind(it->target);
      __ B(it->cont);
      delete it->target;
      delete it->cont;
      it = labels_.erase(it);
    } else {
      ++it;  // Don't bind this one.
    }
    bind_mask >>= 1;
  }
  __ Bind(&done);
}

void BenchCodeGenerator::BindAllPendingLabels() {
  while (!labels_.empty()) {
    // BindPendingLables generates a branch over each block of bound labels.
    // This will be repeated for each call here, but the effect is minimal and
    // (empirically) we rarely accumulate more than 64 pending labels anyway.
    BindPendingLabels(UINT64_MAX);
  }
}

void BenchCodeGenerator::GenerateBranchSequence() {
  {
    LabelPair pair = {new Label(), new Label()};
    __ B(lt, pair.target);
    __ Bind(pair.cont);
    labels_.push_front(pair);
  }

  {
    LabelPair pair = {new Label(), new Label()};
    __ Tbz(PickX(),
           static_cast<int>(GetRandomBits(kXRegSizeLog2)),
           pair.target);
    __ Bind(pair.cont);
    labels_.push_front(pair);
  }

  {
    LabelPair pair = {new Label(), new Label()};
    __ Cbz(PickX(), pair.target);
    __ Bind(pair.cont);
    labels_.push_front(pair);
  }
}

void BenchCodeGenerator::GenerateCallReturnSequence() {
  Label fn, done;

  if (PickBool()) {
    __ Bl(&fn);
  } else {
    Register reg = PickX();
    __ Adr(reg, &fn);
    __ Blr(reg);
  }
  __ B(&done);

  __ Bind(&fn);
  // Recurse with a randomised (but fairly small) minimum size.
  Generate(GetRandomBits(8));

  __ Bind(&done);
}

void BenchCodeGenerator::GenerateFPSequence() {
  unsigned size = PickFPSize();
  unsigned other_size = PickBool() ? size * 2 : size / 2;
  if (other_size < kHRegSize) other_size = kDRegSize;
  if (other_size > kDRegSize) other_size = kHRegSize;

  __ Fadd(PickV(size), PickV(size), PickV(size));
  __ Fmul(PickV(size), PickV(size), PickV(size));
  __ Fcvt(PickV(other_size), PickV(size));
  __ Fjcvtzs(PickW(), PickD());
  __ Fccmp(PickV(size), PickV(size), NCVFlag, pl);
  __ Fdiv(PickV(size), PickV(size), PickV(size));
  __ Fmov(PickV(size), 1.25 * GetRandomBits(2));
  __ Fmsub(PickV(size), PickV(size), PickV(size), PickV(size));
  __ Frintn(PickV(size), PickV(size));
}

void BenchCodeGenerator::GenerateNEONSequence() {
  __ And(PickV().V16B(), PickV().V16B(), PickV().V16B());
  __ Sqrshl(PickV().V8H(), PickV().V8H(), PickV().V8H());
  __ Umull(PickV().V2D(), PickV().V2S(), PickV().V2S());
  __ Sqdmlal2(PickV().V4S(), PickV().V8H(), PickV().V8H());

  // For structured loads and stores, we have to specify sequential (wrapped)
  // registers, so start with [v16, v31] and allow them to wrap in to the
  // [v0, v7] range.
  VRegister vt(16 + static_cast<unsigned>(GetRandomBits(4)), kQRegSize);
  VRegister vt2((vt.GetCode() + 1) % kNumberOfVRegisters, kQRegSize);
  VRegister vt3((vt.GetCode() + 2) % kNumberOfVRegisters, kQRegSize);
  VRegister vt4((vt.GetCode() + 3) % kNumberOfVRegisters, kQRegSize);
  VIXL_ASSERT(!kCalleeSavedV.IncludesAliasOf(vt));
  VIXL_ASSERT(!kCalleeSavedV.IncludesAliasOf(vt2));
  VIXL_ASSERT(!kCalleeSavedV.IncludesAliasOf(vt3));
  VIXL_ASSERT(!kCalleeSavedV.IncludesAliasOf(vt4));
  __ Ld3(vt.V4S(), vt2.V4S(), vt3.V4S(), MemOperand(scratch));
  __ St4(vt.V16B(), vt2.V16B(), vt3.V16B(), vt4.V16B(), MemOperand(scratch));

  __ Fmaxv(PickV().H(), PickV().V8H());
  __ Fminp(PickV().V4S(), PickV().V4S(), PickV().V4S());
}
