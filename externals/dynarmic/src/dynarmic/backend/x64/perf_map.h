/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <string_view>

#include <mcl/bit_cast.hpp>

namespace Dynarmic::Backend::X64 {

namespace detail {
void PerfMapRegister(const void* start, const void* end, std::string_view friendly_name);
}  // namespace detail

template<typename T>
void PerfMapRegister(T start, const void* end, std::string_view friendly_name) {
    detail::PerfMapRegister(mcl::bit_cast<const void*>(start), end, friendly_name);
}

void PerfMapClear();

}  // namespace Dynarmic::Backend::X64
