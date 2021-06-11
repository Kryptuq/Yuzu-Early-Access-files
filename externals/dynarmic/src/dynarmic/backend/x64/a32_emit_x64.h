/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <optional>
#include <set>
#include <tuple>

#include <tsl/robin_map.h>

#include "dynarmic/backend/x64/a32_jitstate.h"
#include "dynarmic/backend/x64/block_range_information.h"
#include "dynarmic/backend/x64/emit_x64.h"
#include "dynarmic/frontend/A32/location_descriptor.h"
#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/interface/A32/config.h"
#include "dynarmic/ir/terminal.h"

namespace Dynarmic::Backend::X64 {

class RegAlloc;

struct A32EmitContext final : public EmitContext {
    A32EmitContext(const A32::UserConfig& conf, RegAlloc& reg_alloc, IR::Block& block);

    A32::LocationDescriptor Location() const;
    A32::LocationDescriptor EndLocation() const;
    bool IsSingleStep() const;
    FP::FPCR FPCR(bool fpcr_controlled = true) const override;

    bool HasOptimization(OptimizationFlag flag) const override {
        return conf.HasOptimization(flag);
    }

    const A32::UserConfig& conf;
};

class A32EmitX64 final : public EmitX64 {
public:
    A32EmitX64(BlockOfCode& code, A32::UserConfig conf, A32::Jit* jit_interface);
    ~A32EmitX64() override;

    /**
     * Emit host machine code for a basic block with intermediate representation `block`.
     * @note block is modified.
     */
    BlockDescriptor Emit(IR::Block& block);

    void ClearCache() override;

    void InvalidateCacheRanges(const boost::icl::interval_set<u32>& ranges);

protected:
    const A32::UserConfig conf;
    A32::Jit* jit_interface;
    BlockRangeInformation<u32> block_ranges;

    void EmitCondPrelude(const A32EmitContext& ctx);

    struct FastDispatchEntry {
        u64 location_descriptor = 0xFFFF'FFFF'FFFF'FFFFull;
        const void* code_ptr = nullptr;
    };
    static_assert(sizeof(FastDispatchEntry) == 0x10);
    static constexpr u64 fast_dispatch_table_mask = 0xFFFF0;
    static constexpr size_t fast_dispatch_table_size = 0x10000;
    std::array<FastDispatchEntry, fast_dispatch_table_size> fast_dispatch_table;
    void ClearFastDispatchTable();

    std::map<std::tuple<size_t, int, int>, void (*)()> read_fallbacks;
    std::map<std::tuple<size_t, int, int>, void (*)()> write_fallbacks;
    void GenFastmemFallbacks();

    const void* terminal_handler_pop_rsb_hint;
    const void* terminal_handler_fast_dispatch_hint = nullptr;
    FastDispatchEntry& (*fast_dispatch_table_lookup)(u64) = nullptr;
    void GenTerminalHandlers();

    // Microinstruction emitters
#define OPCODE(...)
#define A32OPC(name, type, ...) void EmitA32##name(A32EmitContext& ctx, IR::Inst* inst);
#define A64OPC(...)
#include "dynarmic/ir/opcodes.inc"
#undef OPCODE
#undef A32OPC
#undef A64OPC

    // Helpers
    std::string LocationDescriptorToFriendlyName(const IR::LocationDescriptor&) const override;

    // Fastmem information
    using DoNotFastmemMarker = std::tuple<IR::LocationDescriptor, std::ptrdiff_t>;
    struct FastmemPatchInfo {
        u64 resume_rip;
        u64 callback;
        DoNotFastmemMarker marker;
    };
    tsl::robin_map<u64, FastmemPatchInfo> fastmem_patch_info;
    std::set<DoNotFastmemMarker> do_not_fastmem;
    std::optional<DoNotFastmemMarker> ShouldFastmem(A32EmitContext& ctx, IR::Inst* inst) const;
    FakeCall FastmemCallback(u64 rip);

    // Memory access helpers
    template<std::size_t bitsize, auto callback>
    void EmitMemoryRead(A32EmitContext& ctx, IR::Inst* inst);
    template<std::size_t bitsize, auto callback>
    void EmitMemoryWrite(A32EmitContext& ctx, IR::Inst* inst);
    template<std::size_t bitsize, auto callback>
    void ExclusiveReadMemory(A32EmitContext& ctx, IR::Inst* inst);
    template<std::size_t bitsize, auto callback>
    void ExclusiveWriteMemory(A32EmitContext& ctx, IR::Inst* inst);

    // Terminal instruction emitters
    void EmitSetUpperLocationDescriptor(IR::LocationDescriptor new_location, IR::LocationDescriptor old_location);
    void EmitTerminalImpl(IR::Term::Interpret terminal, IR::LocationDescriptor initial_location, bool is_single_step) override;
    void EmitTerminalImpl(IR::Term::ReturnToDispatch terminal, IR::LocationDescriptor initial_location, bool is_single_step) override;
    void EmitTerminalImpl(IR::Term::LinkBlock terminal, IR::LocationDescriptor initial_location, bool is_single_step) override;
    void EmitTerminalImpl(IR::Term::LinkBlockFast terminal, IR::LocationDescriptor initial_location, bool is_single_step) override;
    void EmitTerminalImpl(IR::Term::PopRSBHint terminal, IR::LocationDescriptor initial_location, bool is_single_step) override;
    void EmitTerminalImpl(IR::Term::FastDispatchHint terminal, IR::LocationDescriptor initial_location, bool is_single_step) override;
    void EmitTerminalImpl(IR::Term::If terminal, IR::LocationDescriptor initial_location, bool is_single_step) override;
    void EmitTerminalImpl(IR::Term::CheckBit terminal, IR::LocationDescriptor initial_location, bool is_single_step) override;
    void EmitTerminalImpl(IR::Term::CheckHalt terminal, IR::LocationDescriptor initial_location, bool is_single_step) override;

    // Patching
    void Unpatch(const IR::LocationDescriptor& target_desc) override;
    void EmitPatchJg(const IR::LocationDescriptor& target_desc, CodePtr target_code_ptr = nullptr) override;
    void EmitPatchJmp(const IR::LocationDescriptor& target_desc, CodePtr target_code_ptr = nullptr) override;
    void EmitPatchMovRcx(CodePtr target_code_ptr = nullptr) override;
};

}  // namespace Dynarmic::Backend::X64
