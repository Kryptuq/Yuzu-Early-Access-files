# Return Stack Buffer Optimization (x64 Backend)

One of the optimizations that dynarmic does is block-linking. Block-linking is done when
the destination address of a jump is available at JIT-time. Instead of returning to the
dispatcher at the end of a block we can perform block-linking: just jump directly to the
next block. This is beneficial because returning to the dispatcher can often be quite
expensive.

What should we do in cases when we can't predict the destination address? The eponymous
example is when executing a return statement at the end of a function; the return address
is not statically known at compile time.

We deal with this by using a return stack buffer: When we execute a call instruction,
we push our prediction onto the RSB. When we execute a return instruction, we pop a
prediction off the RSB. If the prediction is a hit, we immediately jump to the relevant
compiled block. Otherwise, we return to the dispatcher.

This is the essential idea behind this optimization.

## `UniqueHash`

One complication dynarmic has is that a compiled block is not uniquely identifiable by
the PC alone, but bits in the FPSCR and CPSR are also relevant. We resolve this by
computing a 64-bit `UniqueHash` that is guaranteed to uniquely identify a block.

    u64 LocationDescriptor::UniqueHash() const {
        // This value MUST BE UNIQUE.
        // This calculation has to match up with EmitX64::EmitTerminalPopRSBHint
        u64 pc_u64 = u64(arm_pc) << 32;
        u64 fpscr_u64 = u64(fpscr.Value());
        u64 t_u64 = cpsr.T() ? 1 : 0;
        u64 e_u64 = cpsr.E() ? 2 : 0;
        return pc_u64 | fpscr_u64 | t_u64 | e_u64;
    }

## Our implementation isn't actually a stack

Dynarmic's RSB isn't actually a stack. It was implemented as a ring buffer because
that showed better performance in tests.

### RSB Structure

The RSB is implemented as a ring buffer. `rsb_ptr` is the index of the insertion
point. Each element in `rsb_location_descriptors` is a `UniqueHash` and they
each correspond to an element in `rsb_codeptrs`. `rsb_codeptrs` contains the
host addresses for the corresponding the compiled blocks.

`RSBSize` was chosen by performance testing. Note that this is bigger than the
size of the real RSB in hardware (which has 3 entries). Larger RSBs than 8
showed degraded performance.

    struct JitState {
        // ...

        static constexpr size_t RSBSize = 8; // MUST be a power of 2.
        u32 rsb_ptr = 0;
        std::array<u64, RSBSize> rsb_location_descriptors;
        std::array<u64, RSBSize> rsb_codeptrs;
        void ResetRSB();

        // ...
    };

### RSB Push

We insert our prediction at the insertion point iff the RSB doesn't already
contain a prediction with the same `UniqueHash`.

    void EmitX64::EmitPushRSB(IR::Block&, IR::Inst* inst) {
        using namespace Xbyak::util;

        ASSERT(inst->GetArg(0).IsImmediate());
        u64 imm64 = inst->GetArg(0).GetU64();

        Xbyak::Reg64 code_ptr_reg = reg_alloc.ScratchGpr({HostLoc::RCX});
        Xbyak::Reg64 loc_desc_reg = reg_alloc.ScratchGpr();
        Xbyak::Reg32 index_reg = reg_alloc.ScratchGpr().cvt32();
        u64 code_ptr = unique_hash_to_code_ptr.find(imm64) != unique_hash_to_code_ptr.end()
                        ? u64(unique_hash_to_code_ptr[imm64])
                        : u64(code->GetReturnFromRunCodeAddress());

        code->mov(index_reg, dword[r15 + offsetof(JitState, rsb_ptr)]);
        code->add(index_reg, 1);
        code->and_(index_reg, u32(JitState::RSBSize - 1));

        code->mov(loc_desc_reg, u64(imm64));
        CodePtr patch_location = code->getCurr<CodePtr>();
        patch_unique_hash_locations[imm64].emplace_back(patch_location);
        code->mov(code_ptr_reg, u64(code_ptr)); // This line has to match up with EmitX64::Patch.
        code->EnsurePatchLocationSize(patch_location, 10);

        Xbyak::Label label;
        for (size_t i = 0; i < JitState::RSBSize; ++i) {
            code->cmp(loc_desc_reg, qword[r15 + offsetof(JitState, rsb_location_descriptors) + i * sizeof(u64)]);
            code->je(label, code->T_SHORT);
        }

        code->mov(dword[r15 + offsetof(JitState, rsb_ptr)], index_reg);
        code->mov(qword[r15 + index_reg.cvt64() * 8 + offsetof(JitState, rsb_location_descriptors)], loc_desc_reg);
        code->mov(qword[r15 + index_reg.cvt64() * 8 + offsetof(JitState, rsb_codeptrs)], code_ptr_reg);
        code->L(label);
    }

In pseudocode:

      for (i := 0 .. RSBSize-1)
          if (rsb_location_descriptors[i] == imm64)
            goto label;
      rsb_ptr++;
      rsb_ptr %= RSBSize;
      rsb_location_desciptors[rsb_ptr] = imm64; //< The UniqueHash
      rsb_codeptr[rsb_ptr] = /* codeptr corresponding to the UniqueHash */;
    label:

## RSB Pop

To check if a predicition is in the RSB, we linearly scan the RSB.

    void EmitX64::EmitTerminalPopRSBHint(IR::Term::PopRSBHint, IR::LocationDescriptor initial_location) {
        using namespace Xbyak::util;

        // This calculation has to match up with IREmitter::PushRSB
        code->mov(ecx, MJitStateReg(Arm::Reg::PC));
        code->shl(rcx, 32);
        code->mov(ebx, dword[r15 + offsetof(JitState, FPSCR_mode)]);
        code->or_(ebx, dword[r15 + offsetof(JitState, CPSR_et)]);
        code->or_(rbx, rcx);

        code->mov(rax, u64(code->GetReturnFromRunCodeAddress()));
        for (size_t i = 0; i < JitState::RSBSize; ++i) {
            code->cmp(rbx, qword[r15 + offsetof(JitState, rsb_location_descriptors) + i * sizeof(u64)]);
            code->cmove(rax, qword[r15 + offsetof(JitState, rsb_codeptrs) + i * sizeof(u64)]);
        }

        code->jmp(rax);
    }

In pseudocode:

    rbx := ComputeUniqueHash()
    rax := ReturnToDispatch
    for (i := 0 .. RSBSize-1)
       if (rbx == rsb_location_descriptors[i])
          rax = rsb_codeptrs[i]
    goto rax