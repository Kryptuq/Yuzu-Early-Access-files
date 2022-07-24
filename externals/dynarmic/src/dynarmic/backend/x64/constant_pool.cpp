/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/x64/constant_pool.h"

#include <cstring>

#include <mcl/assert.hpp>

#include "dynarmic/backend/x64/block_of_code.h"

namespace Dynarmic::Backend::X64 {

ConstantPool::ConstantPool(BlockOfCode& code, size_t size)
        : code(code), insertion_point(0) {
    code.int3();
    code.align(align_size);
    pool = std::span<ConstantT>(
        reinterpret_cast<ConstantT*>(code.AllocateFromCodeSpace(size)), size / align_size);
}

Xbyak::Address ConstantPool::GetConstant(const Xbyak::AddressFrame& frame, u64 lower, u64 upper) {
    const auto constant = ConstantT(lower, upper);
    auto iter = constant_info.find(constant);
    if (iter == constant_info.end()) {
        ASSERT(insertion_point < pool.size());
        ConstantT& target_constant = pool[insertion_point];
        target_constant = constant;
        iter = constant_info.emplace(constant, &target_constant).first;
        ++insertion_point;
    }
    return frame[code.rip + iter->second];
}

}  // namespace Dynarmic::Backend::X64
