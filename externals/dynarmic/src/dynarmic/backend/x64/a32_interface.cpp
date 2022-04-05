/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <functional>
#include <memory>

#include <boost/icl/interval_set.hpp>
#include <fmt/format.h>

#include "dynarmic/backend/x64/a32_emit_x64.h"
#include "dynarmic/backend/x64/a32_jitstate.h"
#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/callback.h"
#include "dynarmic/backend/x64/devirtualize.h"
#include "dynarmic/backend/x64/jitstate_info.h"
#include "dynarmic/common/assert.h"
#include "dynarmic/common/atomic.h"
#include "dynarmic/common/cast_util.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/common/scope_exit.h"
#include "dynarmic/common/x64_disassemble.h"
#include "dynarmic/frontend/A32/translate/a32_translate.h"
#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/interface/A32/context.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/location_descriptor.h"
#include "dynarmic/ir/opt/passes.h"

namespace Dynarmic::A32 {

using namespace Backend::X64;

static RunCodeCallbacks GenRunCodeCallbacks(A32::UserCallbacks* cb, CodePtr (*LookupBlock)(void* lookup_block_arg), void* arg, const A32::UserConfig& conf) {
    return RunCodeCallbacks{
        std::make_unique<ArgCallback>(LookupBlock, reinterpret_cast<u64>(arg)),
        std::make_unique<ArgCallback>(Devirtualize<&A32::UserCallbacks::AddTicks>(cb)),
        std::make_unique<ArgCallback>(Devirtualize<&A32::UserCallbacks::GetTicksRemaining>(cb)),
        conf.enable_cycle_counting,
    };
}

static std::function<void(BlockOfCode&)> GenRCP(const A32::UserConfig& conf) {
    return [conf](BlockOfCode& code) {
        if (conf.page_table) {
            code.mov(code.r14, Common::BitCast<u64>(conf.page_table));
        }
        if (conf.fastmem_pointer) {
            code.mov(code.r13, Common::BitCast<u64>(conf.fastmem_pointer));
        }
    };
}

static Optimization::PolyfillOptions GenPolyfillOptions(const BlockOfCode& code) {
    return Optimization::PolyfillOptions{
        .sha256 = !code.HasHostFeature(HostFeature::SHA),
    };
}

struct Jit::Impl {
    Impl(Jit* jit, A32::UserConfig conf)
            : block_of_code(GenRunCodeCallbacks(conf.callbacks, &GetCurrentBlockThunk, this, conf), JitStateInfo{jit_state}, conf.code_cache_size, conf.far_code_offset, GenRCP(conf))
            , emitter(block_of_code, conf, jit)
            , polyfill_options(GenPolyfillOptions(block_of_code))
            , conf(std::move(conf))
            , jit_interface(jit) {}

    A32JitState jit_state;
    BlockOfCode block_of_code;
    A32EmitX64 emitter;
    Optimization::PolyfillOptions polyfill_options;

    const A32::UserConfig conf;

    // Requests made during execution to invalidate the cache are queued up here.
    size_t invalid_cache_generation = 0;
    boost::icl::interval_set<u32> invalid_cache_ranges;
    bool invalidate_entire_cache = false;

    HaltReason Execute() {
        const CodePtr current_codeptr = [this] {
            // RSB optimization
            const u32 new_rsb_ptr = (jit_state.rsb_ptr - 1) & A32JitState::RSBPtrMask;
            if (jit_state.GetUniqueHash() == jit_state.rsb_location_descriptors[new_rsb_ptr]) {
                jit_state.rsb_ptr = new_rsb_ptr;
                return reinterpret_cast<CodePtr>(jit_state.rsb_codeptrs[new_rsb_ptr]);
            }

            return GetCurrentBlock();
        }();

        return block_of_code.RunCode(&jit_state, current_codeptr);
    }

    HaltReason Step() {
        return block_of_code.StepCode(&jit_state, GetCurrentSingleStep());
    }

    void HaltExecution(HaltReason hr) {
        Atomic::Or(&jit_state.halt_reason, static_cast<u32>(hr));
    }

    void ClearExclusiveState() {
        jit_state.exclusive_state = 0;
    }

    void PerformCacheInvalidation() {
        if (invalidate_entire_cache) {
            jit_state.ResetRSB();
            block_of_code.ClearCache();
            emitter.ClearCache();

            invalid_cache_ranges.clear();
            invalidate_entire_cache = false;
            invalid_cache_generation++;
            return;
        }

        if (invalid_cache_ranges.empty()) {
            return;
        }

        jit_state.ResetRSB();
        emitter.InvalidateCacheRanges(invalid_cache_ranges);
        invalid_cache_ranges.clear();
        invalid_cache_generation++;
    }

    void RequestCacheInvalidation() {
        if (jit_interface->is_executing) {
            HaltExecution(HaltReason::CacheInvalidation);
            return;
        }

        PerformCacheInvalidation();
    }

private:
    Jit* jit_interface;

    static CodePtr GetCurrentBlockThunk(void* this_voidptr) {
        Jit::Impl& this_ = *static_cast<Jit::Impl*>(this_voidptr);
        return this_.GetCurrentBlock();
    }

    IR::LocationDescriptor GetCurrentLocation() const {
        return IR::LocationDescriptor{jit_state.GetUniqueHash()};
    }

    CodePtr GetCurrentBlock() {
        return GetBasicBlock(GetCurrentLocation()).entrypoint;
    }

    CodePtr GetCurrentSingleStep() {
        return GetBasicBlock(A32::LocationDescriptor{GetCurrentLocation()}.SetSingleStepping(true)).entrypoint;
    }

    A32EmitX64::BlockDescriptor GetBasicBlock(IR::LocationDescriptor descriptor) {
        auto block = emitter.GetBasicBlock(descriptor);
        if (block)
            return *block;

        constexpr size_t MINIMUM_REMAINING_CODESIZE = 1 * 1024 * 1024;
        if (block_of_code.SpaceRemaining() < MINIMUM_REMAINING_CODESIZE) {
            invalidate_entire_cache = true;
            PerformCacheInvalidation();
        }

        IR::Block ir_block = A32::Translate(A32::LocationDescriptor{descriptor}, conf.callbacks, {conf.arch_version, conf.define_unpredictable_behaviour, conf.hook_hint_instructions});
        Optimization::PolyfillPass(ir_block, polyfill_options);
        if (conf.HasOptimization(OptimizationFlag::GetSetElimination)) {
            Optimization::A32GetSetElimination(ir_block);
            Optimization::DeadCodeElimination(ir_block);
        }
        if (conf.HasOptimization(OptimizationFlag::ConstProp)) {
            Optimization::A32ConstantMemoryReads(ir_block, conf.callbacks);
            Optimization::ConstantPropagation(ir_block);
            Optimization::DeadCodeElimination(ir_block);
        }
        Optimization::VerificationPass(ir_block);
        return emitter.Emit(ir_block);
    }
};

Jit::Jit(UserConfig conf)
        : impl(std::make_unique<Impl>(this, std::move(conf))) {}

Jit::~Jit() = default;

HaltReason Jit::Run() {
    ASSERT(!is_executing);
    is_executing = true;
    SCOPE_EXIT { this->is_executing = false; };

    const HaltReason hr = impl->Execute();

    impl->PerformCacheInvalidation();

    return hr;
}

HaltReason Jit::Step() {
    ASSERT(!is_executing);
    is_executing = true;
    SCOPE_EXIT { this->is_executing = false; };

    const HaltReason hr = impl->Step();

    impl->PerformCacheInvalidation();

    return hr;
}

void Jit::ClearCache() {
    impl->invalidate_entire_cache = true;
    impl->RequestCacheInvalidation();
}

void Jit::InvalidateCacheRange(std::uint32_t start_address, std::size_t length) {
    impl->invalid_cache_ranges.add(boost::icl::discrete_interval<u32>::closed(start_address, static_cast<u32>(start_address + length - 1)));
    impl->RequestCacheInvalidation();
}

void Jit::Reset() {
    ASSERT(!is_executing);
    impl->jit_state = {};
}

void Jit::HaltExecution(HaltReason hr) {
    impl->HaltExecution(hr);
}

void Jit::ClearExclusiveState() {
    impl->ClearExclusiveState();
}

std::array<u32, 16>& Jit::Regs() {
    return impl->jit_state.Reg;
}
const std::array<u32, 16>& Jit::Regs() const {
    return impl->jit_state.Reg;
}

std::array<u32, 64>& Jit::ExtRegs() {
    return impl->jit_state.ExtReg;
}

const std::array<u32, 64>& Jit::ExtRegs() const {
    return impl->jit_state.ExtReg;
}

u32 Jit::Cpsr() const {
    return impl->jit_state.Cpsr();
}

void Jit::SetCpsr(u32 value) {
    return impl->jit_state.SetCpsr(value);
}

u32 Jit::Fpscr() const {
    return impl->jit_state.Fpscr();
}

void Jit::SetFpscr(u32 value) {
    return impl->jit_state.SetFpscr(value);
}

Context Jit::SaveContext() const {
    Context ctx;
    SaveContext(ctx);
    return ctx;
}

struct Context::Impl {
    A32JitState jit_state;
    size_t invalid_cache_generation;
};

Context::Context()
        : impl(std::make_unique<Context::Impl>()) {
    impl->jit_state.ResetRSB();
}
Context::~Context() = default;
Context::Context(const Context& ctx)
        : impl(std::make_unique<Context::Impl>(*ctx.impl)) {}
Context::Context(Context&& ctx) noexcept
        : impl(std::move(ctx.impl)) {}
Context& Context::operator=(const Context& ctx) {
    *impl = *ctx.impl;
    return *this;
}
Context& Context::operator=(Context&& ctx) noexcept {
    impl = std::move(ctx.impl);
    return *this;
}

std::array<std::uint32_t, 16>& Context::Regs() {
    return impl->jit_state.Reg;
}
const std::array<std::uint32_t, 16>& Context::Regs() const {
    return impl->jit_state.Reg;
}
std::array<std::uint32_t, 64>& Context::ExtRegs() {
    return impl->jit_state.ExtReg;
}
const std::array<std::uint32_t, 64>& Context::ExtRegs() const {
    return impl->jit_state.ExtReg;
}

std::uint32_t Context::Cpsr() const {
    return impl->jit_state.Cpsr();
}
void Context::SetCpsr(std::uint32_t value) {
    impl->jit_state.SetCpsr(value);
}

std::uint32_t Context::Fpscr() const {
    return impl->jit_state.Fpscr();
}
void Context::SetFpscr(std::uint32_t value) {
    return impl->jit_state.SetFpscr(value);
}

void Jit::SaveContext(Context& ctx) const {
    ctx.impl->jit_state.TransferJitState(impl->jit_state, false);
    ctx.impl->invalid_cache_generation = impl->invalid_cache_generation;
}

void Jit::LoadContext(const Context& ctx) {
    bool reset_rsb = ctx.impl->invalid_cache_generation != impl->invalid_cache_generation;
    impl->jit_state.TransferJitState(ctx.impl->jit_state, reset_rsb);
}

void Jit::DumpDisassembly() const {
    const size_t size = reinterpret_cast<const char*>(impl->block_of_code.getCurr()) - reinterpret_cast<const char*>(impl->block_of_code.GetCodeBegin());
    Common::DumpDisassembledX64(impl->block_of_code.GetCodeBegin(), size);
}

std::vector<std::string> Jit::Disassemble() const {
    const size_t size = reinterpret_cast<const char*>(impl->block_of_code.getCurr()) - reinterpret_cast<const char*>(impl->block_of_code.GetCodeBegin());
    return Common::DisassembleX64(impl->block_of_code.GetCodeBegin(), size);
}
}  // namespace Dynarmic::A32
