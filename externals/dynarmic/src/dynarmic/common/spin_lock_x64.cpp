/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <xbyak/xbyak.h>

#include "dynarmic/backend/x64/abi.h"
#include "dynarmic/backend/x64/hostloc.h"
#include "dynarmic/common/spin_lock.h"

namespace Dynarmic {

void EmitSpinLockLock(Xbyak::CodeGenerator& code, Xbyak::Reg64 ptr, Xbyak::Reg32 tmp) {
    Xbyak::Label start, loop;

    code.jmp(start);
    code.L(loop);
    code.pause();
    code.L(start);
    code.mov(tmp, 1);
    code.lock();
    code.xchg(code.dword[ptr], tmp);
    code.test(tmp, tmp);
    code.jnz(loop);
}

void EmitSpinLockUnlock(Xbyak::CodeGenerator& code, Xbyak::Reg64 ptr, Xbyak::Reg32 tmp) {
    code.xor_(tmp, tmp);
    code.xchg(code.dword[ptr], tmp);
    code.mfence();
}

namespace {

struct SpinLockImpl {
    SpinLockImpl();

    Xbyak::CodeGenerator code;
    void (*lock)(volatile int*);
    void (*unlock)(volatile int*);
};

SpinLockImpl impl;

SpinLockImpl::SpinLockImpl() {
    const Xbyak::Reg64 ABI_PARAM1 = Backend::X64::HostLocToReg64(Backend::X64::ABI_PARAM1);

    code.align();
    lock = code.getCurr<void (*)(volatile int*)>();
    EmitSpinLockLock(code, ABI_PARAM1, code.eax);
    code.ret();

    code.align();
    unlock = code.getCurr<void (*)(volatile int*)>();
    EmitSpinLockUnlock(code, ABI_PARAM1, code.eax);
    code.ret();
}

}  // namespace

void SpinLock::Lock() {
    impl.lock(&storage);
}

void SpinLock::Unlock() {
    impl.unlock(&storage);
}

}  // namespace Dynarmic
