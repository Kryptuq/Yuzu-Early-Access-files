/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/x64/block_of_code.h"

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#else
#    include <sys/mman.h>
#endif

#include <array>
#include <cstring>

#include <xbyak/xbyak.h>

#include "dynarmic/backend/x64/a32_jitstate.h"
#include "dynarmic/backend/x64/abi.h"
#include "dynarmic/backend/x64/hostloc.h"
#include "dynarmic/backend/x64/perf_map.h"
#include "dynarmic/backend/x64/stack_layout.h"
#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"

namespace Dynarmic::Backend::X64 {

#ifdef _WIN32
const Xbyak::Reg64 BlockOfCode::ABI_RETURN = HostLocToReg64(Dynarmic::Backend::X64::ABI_RETURN);
const Xbyak::Reg64 BlockOfCode::ABI_PARAM1 = HostLocToReg64(Dynarmic::Backend::X64::ABI_PARAM1);
const Xbyak::Reg64 BlockOfCode::ABI_PARAM2 = HostLocToReg64(Dynarmic::Backend::X64::ABI_PARAM2);
const Xbyak::Reg64 BlockOfCode::ABI_PARAM3 = HostLocToReg64(Dynarmic::Backend::X64::ABI_PARAM3);
const Xbyak::Reg64 BlockOfCode::ABI_PARAM4 = HostLocToReg64(Dynarmic::Backend::X64::ABI_PARAM4);
const std::array<Xbyak::Reg64, ABI_PARAM_COUNT> BlockOfCode::ABI_PARAMS = {BlockOfCode::ABI_PARAM1, BlockOfCode::ABI_PARAM2, BlockOfCode::ABI_PARAM3, BlockOfCode::ABI_PARAM4};
#else
const Xbyak::Reg64 BlockOfCode::ABI_RETURN = HostLocToReg64(Dynarmic::Backend::X64::ABI_RETURN);
const Xbyak::Reg64 BlockOfCode::ABI_RETURN2 = HostLocToReg64(Dynarmic::Backend::X64::ABI_RETURN2);
const Xbyak::Reg64 BlockOfCode::ABI_PARAM1 = HostLocToReg64(Dynarmic::Backend::X64::ABI_PARAM1);
const Xbyak::Reg64 BlockOfCode::ABI_PARAM2 = HostLocToReg64(Dynarmic::Backend::X64::ABI_PARAM2);
const Xbyak::Reg64 BlockOfCode::ABI_PARAM3 = HostLocToReg64(Dynarmic::Backend::X64::ABI_PARAM3);
const Xbyak::Reg64 BlockOfCode::ABI_PARAM4 = HostLocToReg64(Dynarmic::Backend::X64::ABI_PARAM4);
const Xbyak::Reg64 BlockOfCode::ABI_PARAM5 = HostLocToReg64(Dynarmic::Backend::X64::ABI_PARAM5);
const Xbyak::Reg64 BlockOfCode::ABI_PARAM6 = HostLocToReg64(Dynarmic::Backend::X64::ABI_PARAM6);
const std::array<Xbyak::Reg64, ABI_PARAM_COUNT> BlockOfCode::ABI_PARAMS = {BlockOfCode::ABI_PARAM1, BlockOfCode::ABI_PARAM2, BlockOfCode::ABI_PARAM3, BlockOfCode::ABI_PARAM4, BlockOfCode::ABI_PARAM5, BlockOfCode::ABI_PARAM6};
#endif

namespace {

constexpr size_t CONSTANT_POOL_SIZE = 2 * 1024 * 1024;

class CustomXbyakAllocator : public Xbyak::Allocator {
public:
#ifdef DYNARMIC_ENABLE_NO_EXECUTE_SUPPORT
    bool useProtect() const override { return false; }
#endif
};

// This is threadsafe as Xbyak::Allocator does not contain any state; it is a pure interface.
CustomXbyakAllocator s_allocator;

#ifdef DYNARMIC_ENABLE_NO_EXECUTE_SUPPORT
void ProtectMemory(const void* base, size_t size, bool is_executable) {
#    ifdef _WIN32
    DWORD oldProtect = 0;
    VirtualProtect(const_cast<void*>(base), size, is_executable ? PAGE_EXECUTE_READ : PAGE_READWRITE, &oldProtect);
#    else
    static const size_t pageSize = sysconf(_SC_PAGESIZE);
    const size_t iaddr = reinterpret_cast<size_t>(base);
    const size_t roundAddr = iaddr & ~(pageSize - static_cast<size_t>(1));
    const int mode = is_executable ? (PROT_READ | PROT_EXEC) : (PROT_READ | PROT_WRITE);
    mprotect(reinterpret_cast<void*>(roundAddr), size + (iaddr - roundAddr), mode);
#    endif
}
#endif

HostFeature GetHostFeatures() {
    HostFeature features = {};

#ifdef DYNARMIC_ENABLE_CPU_FEATURE_DETECTION
    using Cpu = Xbyak::util::Cpu;
    Xbyak::util::Cpu cpu_info;

    if (cpu_info.has(Cpu::tSSSE3))
        features |= HostFeature::SSSE3;
    if (cpu_info.has(Cpu::tSSE41))
        features |= HostFeature::SSE41;
    if (cpu_info.has(Cpu::tSSE42))
        features |= HostFeature::SSE42;
    if (cpu_info.has(Cpu::tAVX))
        features |= HostFeature::AVX;
    if (cpu_info.has(Cpu::tAVX2))
        features |= HostFeature::AVX2;
    if (cpu_info.has(Cpu::tAVX512F))
        features |= HostFeature::AVX512F;
    if (cpu_info.has(Cpu::tAVX512CD))
        features |= HostFeature::AVX512CD;
    if (cpu_info.has(Cpu::tAVX512VL))
        features |= HostFeature::AVX512VL;
    if (cpu_info.has(Cpu::tAVX512BW))
        features |= HostFeature::AVX512BW;
    if (cpu_info.has(Cpu::tAVX512DQ))
        features |= HostFeature::AVX512DQ;
    if (cpu_info.has(Cpu::tAVX512_BITALG))
        features |= HostFeature::AVX512BITALG;
    if (cpu_info.has(Cpu::tAVX512VBMI))
        features |= HostFeature::AVX512VBMI;
    if (cpu_info.has(Cpu::tPCLMULQDQ))
        features |= HostFeature::PCLMULQDQ;
    if (cpu_info.has(Cpu::tF16C))
        features |= HostFeature::F16C;
    if (cpu_info.has(Cpu::tFMA))
        features |= HostFeature::FMA;
    if (cpu_info.has(Cpu::tAESNI))
        features |= HostFeature::AES;
    if (cpu_info.has(Cpu::tSHA))
        features |= HostFeature::SHA;
    if (cpu_info.has(Cpu::tPOPCNT))
        features |= HostFeature::POPCNT;
    if (cpu_info.has(Cpu::tBMI1))
        features |= HostFeature::BMI1;
    if (cpu_info.has(Cpu::tBMI2))
        features |= HostFeature::BMI2;
    if (cpu_info.has(Cpu::tLZCNT))
        features |= HostFeature::LZCNT;
    if (cpu_info.has(Cpu::tGFNI))
        features |= HostFeature::GFNI;

    if (cpu_info.has(Cpu::tBMI2)) {
        // BMI2 instructions such as pdep and pext have been very slow up until Zen 3.
        // Check for Zen 3 or newer by its family (0x19).
        // See also: https://en.wikichip.org/wiki/amd/cpuid
        if (cpu_info.has(Cpu::tAMD)) {
            std::array<u32, 4> data{};
            cpu_info.getCpuid(1, data.data());
            const u32 family_base = Common::Bits<8, 11>(data[0]);
            const u32 family_extended = Common::Bits<20, 27>(data[0]);
            const u32 family = family_base + family_extended;
            if (family >= 0x19)
                features |= HostFeature::FastBMI2;
        } else {
            features |= HostFeature::FastBMI2;
        }
    }
#endif

    return features;
}

}  // anonymous namespace

BlockOfCode::BlockOfCode(RunCodeCallbacks cb, JitStateInfo jsi, size_t total_code_size, size_t far_code_offset, std::function<void(BlockOfCode&)> rcp)
        : Xbyak::CodeGenerator(total_code_size, nullptr, &s_allocator)
        , cb(std::move(cb))
        , jsi(jsi)
        , far_code_offset(far_code_offset)
        , constant_pool(*this, CONSTANT_POOL_SIZE)
        , host_features(GetHostFeatures()) {
    ASSERT(total_code_size > far_code_offset);
    EnableWriting();
    GenRunCode(rcp);
}

void BlockOfCode::PreludeComplete() {
    prelude_complete = true;
    near_code_begin = getCurr();
    far_code_begin = getCurr() + far_code_offset;
    ClearCache();
    DisableWriting();
}

void BlockOfCode::EnableWriting() {
#ifdef DYNARMIC_ENABLE_NO_EXECUTE_SUPPORT
    ProtectMemory(getCode(), maxSize_, false);
#endif
}

void BlockOfCode::DisableWriting() {
#ifdef DYNARMIC_ENABLE_NO_EXECUTE_SUPPORT
    ProtectMemory(getCode(), maxSize_, true);
#endif
}

void BlockOfCode::ClearCache() {
    ASSERT(prelude_complete);
    in_far_code = false;
    near_code_ptr = near_code_begin;
    far_code_ptr = far_code_begin;
    SetCodePtr(near_code_begin);
}

size_t BlockOfCode::SpaceRemaining() const {
    ASSERT(prelude_complete);
    const u8* current_near_ptr = in_far_code ? reinterpret_cast<const u8*>(near_code_ptr) : getCurr<const u8*>();
    const u8* current_far_ptr = in_far_code ? getCurr<const u8*>() : reinterpret_cast<const u8*>(far_code_ptr);
    if (current_near_ptr >= far_code_begin)
        return 0;
    if (current_far_ptr >= &top_[maxSize_])
        return 0;
    return std::min(reinterpret_cast<const u8*>(far_code_begin) - current_near_ptr, &top_[maxSize_] - current_far_ptr);
}

void BlockOfCode::RunCode(void* jit_state, CodePtr code_ptr) const {
    run_code(jit_state, code_ptr);
}

void BlockOfCode::StepCode(void* jit_state, CodePtr code_ptr) const {
    step_code(jit_state, code_ptr);
}

void BlockOfCode::ReturnFromRunCode(bool mxcsr_already_exited) {
    size_t index = 0;
    if (mxcsr_already_exited)
        index |= MXCSR_ALREADY_EXITED;
    jmp(return_from_run_code[index]);
}

void BlockOfCode::ForceReturnFromRunCode(bool mxcsr_already_exited) {
    size_t index = FORCE_RETURN;
    if (mxcsr_already_exited)
        index |= MXCSR_ALREADY_EXITED;
    jmp(return_from_run_code[index]);
}

void BlockOfCode::GenRunCode(std::function<void(BlockOfCode&)> rcp) {
    align();
    run_code = getCurr<RunCodeFuncType>();

    // This serves two purposes:
    // 1. It saves all the registers we as a callee need to save.
    // 2. It aligns the stack so that the code the JIT emits can assume
    //    that the stack is appropriately aligned for CALLs.
    ABI_PushCalleeSaveRegistersAndAdjustStack(*this, sizeof(StackLayout));

    mov(r15, ABI_PARAM1);
    mov(rbx, ABI_PARAM2);  // save temporarily in non-volatile register

    cb.GetTicksRemaining->EmitCall(*this);
    mov(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_to_run)], ABI_RETURN);
    mov(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)], ABI_RETURN);

    rcp(*this);

    SwitchMxcsrOnEntry();
    jmp(rbx);

    align();
    step_code = getCurr<RunCodeFuncType>();

    ABI_PushCalleeSaveRegistersAndAdjustStack(*this, sizeof(StackLayout));

    mov(r15, ABI_PARAM1);

    mov(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_to_run)], 1);
    mov(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)], 1);

    rcp(*this);

    SwitchMxcsrOnEntry();
    jmp(ABI_PARAM2);

    // Dispatcher loop

    Xbyak::Label return_to_caller, return_to_caller_mxcsr_already_exited;

    align();
    return_from_run_code[0] = getCurr<const void*>();

    cmp(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)], 0);
    jng(return_to_caller);
    cb.LookupBlock->EmitCall(*this);
    jmp(ABI_RETURN);

    align();
    return_from_run_code[MXCSR_ALREADY_EXITED] = getCurr<const void*>();

    cmp(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)], 0);
    jng(return_to_caller_mxcsr_already_exited);
    SwitchMxcsrOnEntry();
    cb.LookupBlock->EmitCall(*this);
    jmp(ABI_RETURN);

    align();
    return_from_run_code[FORCE_RETURN] = getCurr<const void*>();
    L(return_to_caller);

    SwitchMxcsrOnExit();
    // fallthrough

    return_from_run_code[MXCSR_ALREADY_EXITED | FORCE_RETURN] = getCurr<const void*>();
    L(return_to_caller_mxcsr_already_exited);

    cb.AddTicks->EmitCall(*this, [this](RegList param) {
        mov(param[0], qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_to_run)]);
        sub(param[0], qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)]);
    });

    ABI_PopCalleeSaveRegistersAndAdjustStack(*this, sizeof(StackLayout));
    ret();

    PerfMapRegister(run_code, getCurr(), "dynarmic_dispatcher");
}

void BlockOfCode::SwitchMxcsrOnEntry() {
    stmxcsr(dword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, save_host_MXCSR)]);
    ldmxcsr(dword[r15 + jsi.offsetof_guest_MXCSR]);
}

void BlockOfCode::SwitchMxcsrOnExit() {
    stmxcsr(dword[r15 + jsi.offsetof_guest_MXCSR]);
    ldmxcsr(dword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, save_host_MXCSR)]);
}

void BlockOfCode::EnterStandardASIMD() {
    stmxcsr(dword[r15 + jsi.offsetof_guest_MXCSR]);
    ldmxcsr(dword[r15 + jsi.offsetof_asimd_MXCSR]);
}

void BlockOfCode::LeaveStandardASIMD() {
    stmxcsr(dword[r15 + jsi.offsetof_asimd_MXCSR]);
    ldmxcsr(dword[r15 + jsi.offsetof_guest_MXCSR]);
}

void BlockOfCode::UpdateTicks() {
    cb.AddTicks->EmitCall(*this, [this](RegList param) {
        mov(param[0], qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_to_run)]);
        sub(param[0], qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)]);
    });

    cb.GetTicksRemaining->EmitCall(*this);
    mov(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_to_run)], ABI_RETURN);
    mov(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)], ABI_RETURN);
}

void BlockOfCode::LookupBlock() {
    cb.LookupBlock->EmitCall(*this);
}

Xbyak::Address BlockOfCode::MConst(const Xbyak::AddressFrame& frame, u64 lower, u64 upper) {
    return constant_pool.GetConstant(frame, lower, upper);
}

void BlockOfCode::SwitchToFarCode() {
    ASSERT(prelude_complete);
    ASSERT(!in_far_code);
    in_far_code = true;
    near_code_ptr = getCurr();
    SetCodePtr(far_code_ptr);

    ASSERT_MSG(near_code_ptr < far_code_begin, "Near code has overwritten far code!");
}

void BlockOfCode::SwitchToNearCode() {
    ASSERT(prelude_complete);
    ASSERT(in_far_code);
    in_far_code = false;
    far_code_ptr = getCurr();
    SetCodePtr(near_code_ptr);
}

CodePtr BlockOfCode::GetCodeBegin() const {
    return near_code_begin;
}

size_t BlockOfCode::GetTotalCodeSize() const {
    return maxSize_;
}

void* BlockOfCode::AllocateFromCodeSpace(size_t alloc_size) {
    if (size_ + alloc_size >= maxSize_) {
        throw Xbyak::Error(Xbyak::ERR_CODE_IS_TOO_BIG);
    }

    void* ret = getCurr<void*>();
    size_ += alloc_size;
    memset(ret, 0, alloc_size);
    return ret;
}

void BlockOfCode::SetCodePtr(CodePtr code_ptr) {
    // The "size" defines where top_, the insertion point, is.
    size_t required_size = reinterpret_cast<const u8*>(code_ptr) - getCode();
    setSize(required_size);
}

void BlockOfCode::EnsurePatchLocationSize(CodePtr begin, size_t size) {
    size_t current_size = getCurr<const u8*>() - reinterpret_cast<const u8*>(begin);
    ASSERT(current_size <= size);
    nop(size - current_size);
}

}  // namespace Dynarmic::Backend::X64
