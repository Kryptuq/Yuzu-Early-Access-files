/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstddef>

namespace Dynarmic::Backend::X64 {

struct JitStateInfo {
    template<typename JitStateType>
    JitStateInfo(const JitStateType&)
            : offsetof_guest_MXCSR(offsetof(JitStateType, guest_MXCSR))
            , offsetof_asimd_MXCSR(offsetof(JitStateType, asimd_MXCSR))
            , offsetof_rsb_ptr(offsetof(JitStateType, rsb_ptr))
            , rsb_ptr_mask(JitStateType::RSBPtrMask)
            , offsetof_rsb_location_descriptors(offsetof(JitStateType, rsb_location_descriptors))
            , offsetof_rsb_codeptrs(offsetof(JitStateType, rsb_codeptrs))
            , offsetof_cpsr_nzcv(offsetof(JitStateType, cpsr_nzcv))
            , offsetof_fpsr_exc(offsetof(JitStateType, fpsr_exc))
            , offsetof_fpsr_qc(offsetof(JitStateType, fpsr_qc)) {}

    const size_t offsetof_guest_MXCSR;
    const size_t offsetof_asimd_MXCSR;
    const size_t offsetof_rsb_ptr;
    const size_t rsb_ptr_mask;
    const size_t offsetof_rsb_location_descriptors;
    const size_t offsetof_rsb_codeptrs;
    const size_t offsetof_cpsr_nzcv;
    const size_t offsetof_fpsr_exc;
    const size_t offsetof_fpsr_qc;
};

}  // namespace Dynarmic::Backend::X64
